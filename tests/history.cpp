#include "HistoryDb.h"
#include "FileIdentity.h"
#include "RecentHistoryModel.h"
#include "../src/playlist/FolderScanner.h"
#include <windows.h>
#include <sqlite3.h>
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QEventLoop>
#include <QTimer>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
#include <cstdlib>
static void require(bool v,const char *m){if(!v){std::cerr<<m<<'\n';std::exit(1);}}
int main(int argc,char **argv) {
    QCoreApplication app(argc,argv);QTemporaryDir temp;require(temp.isValid(),"Temporary directory");
    {RecentHistoryModel model;QVariantList rows{QVariantMap{{"file_key","a"},{"thumbnail",""}}};model.replace(rows);QPersistentModelIndex selected(model.index(0));rows[0]=QVariantMap{{"file_key","a"},{"thumbnail","ready.jpg"}};model.replace(rows);require(selected.isValid()&&selected.data(RecentHistoryModel::RecordRole).toMap().value("thumbnail")=="ready.jpg","Thumbnail update preserves selected row identity");}
    const auto a=temp.path()+"/A.mkv",b=temp.path()+"/B.mkv";
    {QFile f(a);require(f.open(QIODevice::WriteOnly),"Create A");f.write(QByteArray(180000,'a'));}
    {QFile f(b);require(f.open(QIODevice::WriteOnly),"Create B");f.write(QByteArray(180000,'b'));}
    const auto id=FileIdentity::inspect(a),bid=FileIdentity::inspect(b);
    require(id.value("file_key")!=bid.value("file_key"),"Distinct content");
    {QFile f(temp.path()+"/tiny");require(f.open(QIODevice::WriteOnly),"Create tiny file");f.write("tiny");}
    require(FileIdentity::inspect(temp.path()+"/tiny").contains("file_key"),"Small whole-file hash");
    QVariantMap first=id;first["duration"]=1000;first["position"]=600;first["last_played"]=100;first["audio_track"]=2;first["sub_track"]=-1;first["sub_delay"]=.3;first["volume"]=61;first["speed"]=1.25;
    require(FileIdentity::resumePosition(first)==598,"Two second preroll");
    auto finished=first;finished["position"]=960;require(FileIdentity::resumePosition(finished)==0,"96 percent starts at zero");finished["position"]=10;finished["finished"]=true;require(FileIdentity::resumePosition(finished)==0,"Finished starts at zero");
    const auto dbPath=temp.path()+"/flow.db",thumbs=temp.path()+"/thumbs";
    double maxGap=0,enqueueMs=0,drainMs=0,recentMs=0;int rows=0;
    {
        HistoryDb db(dbPath,thumbs);QEventLoop loop;QString error;
        QObject::connect(&db,&HistoryDb::failed,&loop,[&](const QString &e){error=e;loop.quit();});
        QObject::connect(&db,&HistoryDb::drained,&loop,&QEventLoop::quit);
        db.save(first);db.barrier();QTimer::singleShot(15000,&loop,&QEventLoop::quit);loop.exec();require(error.isEmpty(),"Database initial save");
        {QFile thumb(flowThumbnailPath(thumbs,id.value("file_key").toString()));require(thumb.open(QIODevice::WriteOnly),"Create prune cache fixture");thumb.write("cached thumbnail");}
        HANDLE locked=CreateFileW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(a).utf16()),GENERIC_READ,0,nullptr,OPEN_EXISTING,0,nullptr);require(locked!=INVALID_HANDLE_VALUE,"Deny-read lock fixture");
        QObject::connect(&db,&HistoryDb::resolved,&loop,[&](quint64,const QVariantMap &r){require(r.value("known").toBool()&&r.value("position").toDouble()==600,"Locked file path fallback");loop.quit();});db.lookup(a,0);loop.exec();CloseHandle(locked);QObject::disconnect(&db,&HistoryDb::resolved,&loop,nullptr);
        require(QFile::rename(a,temp.path()+"/renamed.mkv"),"Rename");
        QObject::connect(&db,&HistoryDb::resolved,&loop,[&](quint64,const QVariantMap &r){require(r.value("known").toBool(),"Renamed identity known");require(r.value("position").toDouble()==600,"A independent resume");require(r.value("audio_track").toInt()==2&&r.value("sub_track").toInt()==-1&&r.value("volume").toInt()==61&&r.value("speed").toDouble()==1.25,"Per-file controls");loop.quit();});
        auto second=bid;second["position"]=123;second["last_played"]=101;db.save(second);db.lookup(temp.path()+"/renamed.mkv",1);loop.exec();
        QObject::disconnect(&db,&HistoryDb::resolved,&loop,nullptr);
        QElapsedTimer total,gap;QTimer heartbeat;heartbeat.setTimerType(Qt::PreciseTimer);heartbeat.setInterval(1);gap.start();
        QObject::connect(&heartbeat,&QTimer::timeout,&loop,[&]{maxGap=qMax(maxGap,gap.nsecsElapsed()/1e6);gap.restart();});heartbeat.start();total.start();
        for(int i=0;i<1000;++i){auto r=first;r["file_key"]=QString("stress-%1").arg(i);r["path"]=temp.path()+QString("/missing-%1.mkv").arg(i);r["last_played"]=1000+i;db.save(r);}enqueueMs=total.nsecsElapsed()/1e6;db.barrier();loop.exec();drainMs=total.nsecsElapsed()/1e6;
        QElapsedTimer recent;recent.start();
        QObject::connect(&db,&HistoryDb::recentReady,&loop,[&](const QVariantList &list){if(!list.isEmpty()&&list.first().toMap().value("metadataPending").toBool())return;rows=list.size();require(rows==10,"Exactly ten recent rows");require(list.first().toMap().value("last_played").toInt()==1999,"Newest first");require(!list.first().toMap().value("available").toBool(),"Missing dim state");recentMs=recent.nsecsElapsed()/1e6;loop.quit();});db.recent();loop.exec();
        require(error.isEmpty(),"No worker SQL failures");
        require(!QFile::exists(flowThumbnailPath(thumbs,id.value("file_key").toString())),"Pruned record thumbnail removed");
        QObject::disconnect(&db,&HistoryDb::recentReady,&loop,nullptr);
        const auto removedKey=QString("stress-999"),removedThumb=flowThumbnailPath(thumbs,removedKey);
        {QFile f(removedThumb);require(f.open(QIODevice::WriteOnly),"Remove cache fixture");f.write("cache");}
        db.remove(removedKey);db.barrier();loop.exec();require(!QFile::exists(removedThumb),"History removal prunes thumbnail");
    }
    sqlite3 *sql=nullptr;require(sqlite3_open(dbPath.toUtf8().constData(),&sql)==SQLITE_OK,"Reopen saved database");sqlite3_stmt *s=nullptr;
    sqlite3_prepare_v2(sql,"SELECT count(*) FROM history",-1,&s,nullptr);require(sqlite3_step(s)==SQLITE_ROW&&sqlite3_column_int(s,0)==199,"Prune to 200 then remove one");sqlite3_finalize(s);
    sqlite3_prepare_v2(sql,"PRAGMA journal_mode",-1,&s,nullptr);sqlite3_step(s);require(QByteArray(reinterpret_cast<const char*>(sqlite3_column_text(s,0)))=="wal","WAL persisted");sqlite3_finalize(s);
    sqlite3_prepare_v2(sql,"PRAGMA user_version",-1,&s,nullptr);sqlite3_step(s);require(sqlite3_column_int(s,0)==1,"Schema version");sqlite3_finalize(s);sqlite3_close(sql);
    QDir().mkpath(temp.path()+"/series");
    for(const auto &name:{"Episode 1.mkv","Episode 2.mkv","Episode 10.mkv","notes.txt"}){QFile f(temp.path()+"/series/"+name);require(f.open(QIODevice::WriteOnly),"Neighbor fixture");f.write(name);}
    require(FolderScanner::neighbor(temp.path()+"/series/Episode 2.mkv",1).endsWith("Episode 10.mkv"),"Next file natural order");
    require(FolderScanner::neighbor(temp.path()+"/series/Episode 2.mkv",-1).endsWith("Episode 1.mkv"),"Previous file natural order");
    require(FolderScanner::neighbor(temp.path()+"/series/Episode 10.mkv",1).isEmpty(),"No wrap after last file");
    {
        const auto corrupt=temp.path()+"/corrupt.db";{QFile f(corrupt);require(f.open(QIODevice::WriteOnly),"Corrupt database fixture");f.write("not a database");}
        HistoryDb bad(corrupt,thumbs);QEventLoop wait;bool failure=false,resolved=false;
        QObject::connect(&bad,&HistoryDb::failed,&wait,[&](const QString &){failure=true;});
        QObject::connect(&bad,&HistoryDb::resolved,&wait,[&](quint64,const QVariantMap &r){resolved=r.value("available").toBool()&&!r.value("known").toBool();wait.quit();});
        bad.lookup(b,1);QTimer::singleShot(5000,&wait,&QEventLoop::quit);wait.exec();require(failure&&resolved,"Corrupt DB reports error without trapping file open");
    }
    std::cout<<QJsonDocument(QJsonObject{{"queuedSaves",1000},{"retained",200},{"homeRows",rows},{"enqueueMs",enqueueMs},{"drainMs",drainMs},{"homeLoadMs",recentMs},{"maxEventLoopGapMs",maxGap}}).toJson(QJsonDocument::Compact).constData()<<'\n';
}
