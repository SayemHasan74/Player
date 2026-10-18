#include "HistoryDb.h"
#include "FileIdentity.h"
#include "../core/Log.h"
#include <sqlite3.h>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QDateTime>
#include <QUrl>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <stdexcept>
#include <windows.h>
QString flowThumbnailPath(const QString &dir,const QString &key) {return dir+"/"+QString::fromLatin1(QCryptographicHash::hash(key.toUtf8(),QCryptographicHash::Sha256).toHex())+".jpg";}
namespace {
const char *columns="file_key,path,size,mtime,duration,position,finished,last_played,audio_track,sub_track,sub_delay,volume,speed";
struct Statement {
    sqlite3_stmt *s=nullptr;
    Statement(sqlite3 *db,const QByteArray &sql) {if(sqlite3_prepare_v2(db,sql.constData(),-1,&s,nullptr)!=SQLITE_OK)throw std::runtime_error(sqlite3_errmsg(db));}
    ~Statement(){sqlite3_finalize(s);}
    void text(int i,const QString &v){auto b=v.toUtf8();sqlite3_bind_text(s,i,b.constData(),b.size(),SQLITE_TRANSIENT);}
    int step(){return sqlite3_step(s);}
};
}
HistoryDb::HistoryDb(QString path,QString thumbs,QObject *parent):QObject(parent) {
    m_path=path.isEmpty()?qEnvironmentVariable("FLOW_HISTORY_DB",qEnvironmentVariable("APPDATA")+"/FlowPlayer/flow.db"):path;
    m_thumbs=thumbs.isEmpty()?qEnvironmentVariable("FLOW_THUMBS_DIR",qEnvironmentVariable("LOCALAPPDATA")+"/FlowPlayer/thumbs"):thumbs;
    m_worker=std::thread(&HistoryDb::run,this);
    m_identityWorker=std::thread(&HistoryDb::runIdentity,this);
}
HistoryDb::~HistoryDb() {
    {std::lock_guard l(m_mutex);m_identityStop=true;m_identityQueue.clear();}m_identityCv.notify_one();
    // Cancel only identity reads; never cancel queued database transactions.
    CancelSynchronousIo(m_identityWorker.native_handle());m_identityWorker.join();
    {std::lock_guard l(m_mutex);m_stop=true;}m_cv.notify_one();m_worker.join();
}
void HistoryDb::enqueue(std::function<void()> w) {{std::lock_guard l(m_mutex);m_queue.push_back(std::move(w));}m_cv.notify_one();}
void HistoryDb::execute(const char *sql) {if(sqlite3_exec(m_db,sql,nullptr,nullptr,nullptr)!=SQLITE_OK)throw std::runtime_error(sqlite3_errmsg(m_db));}
void HistoryDb::run() {
    try {
        QDir().mkpath(QFileInfo(m_path).absolutePath());QDir().mkpath(m_thumbs);
        if(sqlite3_open_v2(m_path.toUtf8().constData(),&m_db,SQLITE_OPEN_READWRITE|SQLITE_OPEN_CREATE|SQLITE_OPEN_NOMUTEX,nullptr)!=SQLITE_OK)throw std::runtime_error("Cannot open history database");
        sqlite3_busy_timeout(m_db,2000);
        execute("PRAGMA journal_mode=WAL; PRAGMA synchronous=FULL; PRAGMA wal_autocheckpoint=100;");
        {Statement version(m_db,"PRAGMA user_version");version.step();if(sqlite3_column_int(version.s,0)>1)throw std::runtime_error("History schema is newer than this application");}
        execute("BEGIN IMMEDIATE; CREATE TABLE IF NOT EXISTS history(file_key TEXT PRIMARY KEY,path TEXT NOT NULL,size INTEGER,mtime INTEGER,duration REAL,position REAL,finished INTEGER,last_played INTEGER,audio_track INTEGER,sub_track INTEGER,sub_delay REAL,volume REAL,speed REAL); CREATE INDEX IF NOT EXISTS history_recent ON history(last_played DESC); CREATE INDEX IF NOT EXISTS history_path ON history(path COLLATE NOCASE); PRAGMA user_version=1; COMMIT;");
    } catch(const std::exception &e) {const QString msg=e.what();QMetaObject::invokeMethod(this,[this,msg]{emit failed(msg);});if(m_db){sqlite3_close(m_db);m_db=nullptr;}}
    for(;;) {
        std::function<void()> w;
        {std::unique_lock l(m_mutex);m_cv.wait(l,[this]{return m_stop||!m_queue.empty();});if(m_queue.empty()&&m_stop)break;w=std::move(m_queue.front());m_queue.pop_front();}
        try {w();}catch(const std::exception &e){if(m_db)sqlite3_exec(m_db,"ROLLBACK",nullptr,nullptr,nullptr);const QString msg=e.what();QMetaObject::invokeMethod(this,[this,msg]{emit failed(msg);});}
    }
    if(m_db){sqlite3_wal_checkpoint_v2(m_db,nullptr,SQLITE_CHECKPOINT_PASSIVE,nullptr,nullptr);sqlite3_close(m_db);}
}
QVariantMap HistoryDb::row(void *p) {
    auto *s=static_cast<sqlite3_stmt*>(p);QVariantMap r;const auto names=QString::fromLatin1(columns).split(',');
    for(int i=0;i<names.size();++i) {
        if(i<2)r[names[i]]=QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(s,i)));
        else if(i==4||i==5||i>=10)r[names[i]]=sqlite3_column_double(s,i);
        else r[names[i]]=QVariant::fromValue<qint64>(sqlite3_column_int64(s,i));
    }
    return r;
}
void HistoryDb::lookup(QString path,quint64 token) {{std::lock_guard l(m_mutex);m_identityQueue.emplace_back(path,token);}m_identityCv.notify_one();}
void HistoryDb::runIdentity() {
    for(;;) {
        std::pair<QString,quint64> request;
        {std::unique_lock l(m_mutex);m_identityCv.wait(l,[this]{return m_identityStop||!m_identityQueue.empty();});if(m_identityStop)break;request=std::move(m_identityQueue.front());m_identityQueue.pop_front();}
        QElapsedTimer timer;timer.start();auto identity=FileIdentity::inspect(request.first);const auto elapsed=timer.elapsed();
        enqueue([this,identity,token=request.second,elapsed]{resolveIdentity(identity,token,elapsed);});
    }
}
void HistoryDb::resolveIdentity(QVariantMap identity,quint64 token,qint64 elapsedMs) {
    QVariantMap found;
    if(m_db) {
        Statement s(m_db,QByteArray("SELECT ")+columns+" FROM history WHERE file_key=? OR path=? COLLATE NOCASE ORDER BY (file_key=?) DESC LIMIT 1");
        s.text(1,identity.value("file_key").toString());s.text(2,identity.value("path").toString());s.text(3,identity.value("file_key").toString());
        if(s.step()==SQLITE_ROW)found=row(s.s);
        if(!found.isEmpty()&&identity.contains("file_key")&&found.value("file_key")!=identity.value("file_key"))found.clear();
    }
    if(!identity.contains("file_key")&&!found.isEmpty())identity["file_key"]=found.value("file_key");
    if(!identity.contains("file_key"))identity["file_key"]="path:"+identity.value("path").toString().toCaseFolded();
    found.insert("known",!found.isEmpty());
    for(auto i=identity.cbegin();i!=identity.cend();++i)found[i.key()]=i.value();
    if(found.value("duration").toDouble()>0&&found.value("position").toDouble()/found.value("duration").toDouble()>.95)found["finished"]=true;
    Log::write(QString("HISTORY_LOOKUP token=%1 workerMs=%2 start=%3 known=%4").arg(token).arg(elapsedMs).arg(FileIdentity::resumePosition(found),0,'f',3).arg(found.value("known").toBool()));
    QMetaObject::invokeMethod(this,[this,token,found]{emit resolved(token,found);});
}
void HistoryDb::save(QVariantMap r) {enqueue([this,r]{
    if(!m_db)return;execute("BEGIN IMMEDIATE");
    Statement s(m_db,"INSERT INTO history VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?) ON CONFLICT(file_key) DO UPDATE SET path=excluded.path,size=excluded.size,mtime=excluded.mtime,duration=excluded.duration,position=excluded.position,finished=excluded.finished,last_played=excluded.last_played,audio_track=excluded.audio_track,sub_track=excluded.sub_track,sub_delay=excluded.sub_delay,volume=excluded.volume,speed=excluded.speed");
    s.text(1,r.value("file_key").toString());s.text(2,r.value("path").toString());const auto names=QString::fromLatin1(columns).split(',');
    for(int i=2;i<names.size();++i) {if(i==4||i==5||i>=10)sqlite3_bind_double(s.s,i+1,r.value(names[i]).toDouble());else sqlite3_bind_int64(s.s,i+1,r.value(names[i]).toLongLong());}
    if(s.step()!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(m_db));prune();execute("COMMIT");
    QMetaObject::invokeMethod(this,[this,r]{emit committed(r);});
});}
void HistoryDb::publishRecent() {
    if(!m_db)return;QElapsedTimer timer;timer.start();QVariantList list;
    Statement s(m_db,QByteArray("SELECT ")+columns+" FROM history ORDER BY last_played DESC LIMIT 10");
    while(s.step()==SQLITE_ROW) {
        auto r=row(s.s);QFileInfo f(r.value("path").toString());r["title"]=f.fileName();r["folder"]=f.dir().dirName();r["available"]=r.value("path").toString().contains("://")||f.exists();
        auto thumb=flowThumbnailPath(m_thumbs,r.value("file_key").toString());r["thumbnail"]=QFileInfo::exists(thumb)?QUrl::fromLocalFile(thumb).toString():QString();list.append(r);
    }
    Log::write(QString("HISTORY_RECENT rows=%1 workerMs=%2").arg(list.size()).arg(timer.elapsed()));
    QMetaObject::invokeMethod(this,[this,list]{emit recentReady(list);});
}
void HistoryDb::recent(){enqueue([this]{publishRecent();});}
void HistoryDb::prune() {
    Statement s(m_db,"SELECT file_key FROM history ORDER BY last_played DESC LIMIT -1 OFFSET 200");QStringList keys;
    while(s.step()==SQLITE_ROW)keys.append(QString::fromUtf8(reinterpret_cast<const char*>(sqlite3_column_text(s.s,0))));
    for(const auto &key:keys){QFile::remove(flowThumbnailPath(m_thumbs,key));Statement d(m_db,"DELETE FROM history WHERE file_key=?");d.text(1,key);d.step();}
}
void HistoryDb::remove(QString key){enqueue([this,key]{if(!m_db)return;Statement d(m_db,"DELETE FROM history WHERE file_key=?");d.text(1,key);if(d.step()!=SQLITE_DONE)throw std::runtime_error(sqlite3_errmsg(m_db));QFile::remove(flowThumbnailPath(m_thumbs,key));publishRecent();});}
void HistoryDb::locate(QString path){enqueue([this,path]{auto folder=QFileInfo(path).absolutePath();QMetaObject::invokeMethod(this,[this,folder]{emit folderReady(folder);});});}
void HistoryDb::barrier(){enqueue([this]{QMetaObject::invokeMethod(this,[this]{emit drained();});});}
