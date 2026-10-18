#include "ResumeStore.h"
#include "HistoryDb.h"
#include "FileIdentity.h"
#include "../core/MpvController.h"
#include "../core/AppSettings.h"
#include "../core/Log.h"
#include <QDateTime>
#include <QDesktopServices>
#include <QUrl>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFileInfo>
ResumeStore::ResumeStore(MpvController *c,AppSettings *s,HistoryDb *db):m_controller(c),m_settings(s),m_db(db),m_thumbnails(db->thumbnailDirectory()) {
    m_home=QCoreApplication::arguments().size()==1;
    c->setResumeStore(this);
    m_saveTimer.setInterval(5000);m_saveTimer.setTimerType(Qt::PreciseTimer);
    connect(&m_saveTimer,&QTimer::timeout,this,&ResumeStore::save);
    connect(c,&MpvController::openRequested,this,[this](const QString &p){open(p);});
    connect(c,&MpvController::fileLoaded,this,[this](const QString &path){
        if(!m_loadExpected)return;
        if(QDir::fromNativeSeparators(path).compare(QDir::fromNativeSeparators(m_active.value("path").toString()),Qt::CaseInsensitive)!=0)return;
        m_loadExpected=false;
        m_loaded=true;m_home=false;emit homeChanged();updateActivity();
        refreshPlaylist();
        if(m_resumeStart>0){int seconds=int(m_resumeStart);emit m_controller->toast(QString("Resumed from %1:%2").arg(seconds/60).arg(seconds%60,2,10,QChar('0')));}
        // Wait for the first observed time-pos before committing a loaded resume.
    });
    connect(c,&MpvController::stateChanged,this,[this]{
        if(m_loaded&&m_controller->pause()!=m_lastPause&&m_controller->pause())save();
        m_lastPause=m_controller->pause();updateActivity();
    });
    connect(c,&MpvController::positionSettled,this,[this]{if(m_loaded){m_positionReady=true;save();}});
    connect(c,&MpvController::beforeStop,this,[this]{save();m_loaded=false;m_loadExpected=false;m_openPending=false;m_autoLaunch=false;m_waitAdvance=false;++m_generation;++m_navigationToken;updateActivity();});
    connect(c,&MpvController::playbackFinished,this,&ResumeStore::finish);
    connect(c,&MpvController::fileFailed,this,[this](const QString &message){Log::write("PLAYLIST_ERROR "+message);m_loadExpected=false;m_openPending=false;m_loaded=false;m_failedPaths.insert(m_active.value("path").toString());refreshPlaylist();advance(true);});
    connect(QCoreApplication::instance(),&QCoreApplication::aboutToQuit,this,[this]{m_controller->stop();});
    connect(db,&HistoryDb::resolved,this,[this](quint64 token,const QVariantMap &r){
        if(token!=m_generation)return;m_openPending=false;
        if(!r.value("available").toBool()){emit m_controller->toast("File unavailable");m_failedPaths.insert(r.value("path").toString());m_active=r;m_loaded=false;refreshPlaylist();advance(true);return;}
        save();m_loaded=false;m_positionReady=false;m_saveTimer.stop();m_active=r;
        m_resumeStart=m_fromStart?0:FileIdentity::resumePosition(r);
        QVariantMap opts{{"start",QString::number(m_resumeStart,'f',3)},{"pause","no"},{"aid","auto"},{"sid","auto"},{"sub-delay","0"},{"speed",QString::number(m_settings->values().value("defaultSpeed",1).toDouble())},{"volume",QString::number(m_settings->values().value("volume",100).toDouble())}};
        if(r.value("known").toBool()) {
            for(const auto &[column,option]:{std::pair{"audio_track","aid"},{"sub_track","sid"},{"sub_delay","sub-delay"},{"volume","volume"},{"speed","speed"}}){
                auto value=r.value(column);opts[option]=(QString(option)=="aid"||QString(option)=="sid")?(value.toInt()<0?QString("no"):value.toString()):value.toString();
            }
        }
        Log::write("RESUME_LOAD "+QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(opts)).toJson(QJsonDocument::Compact)));
        m_loadExpected=true;m_controller->loadResolved(r.value("path").toString(),opts);
    });
    connect(db,&HistoryDb::recentReady,this,[this](const QVariantList &list){
        m_haveRecent=list.isEmpty()||!list.first().toMap().value("metadataPending").toBool();m_items=list;m_recentModel.replace(list);emit itemsChanged();
        for(const auto &r:list)m_thumbnails.request(r.toMap());
        if(m_launchReady&&m_autoLaunch&&m_haveRecent){m_autoLaunch=false;int i=continueIndex();if(i>=0)open(m_items[i].toMap().value("path").toString());}
    });
    connect(db,&HistoryDb::playlistReady,this,[this](quint64 token,const QVariantList &rows){if(token!=m_playlistToken)return;m_playlistPending=false;m_playlist=rows;publishPlaylist();if(m_waitAdvance){m_waitAdvance=false;advance(!m_failedPaths.isEmpty());}});
    connect(db,&HistoryDb::committed,this,[this](const QVariantMap &r){for(auto &item:m_playlist){auto row=item.toMap();if(row.value("path")==r.value("path")){for(auto it=r.cbegin();it!=r.cend();++it)row[it.key()]=it.value();item=row;publishPlaylist();break;}}});
    connect(s,&AppSettings::valueChanged,this,[this](const QString &key,const QVariant &){if(key=="playlistIgnore"&&!m_active.isEmpty()){m_playlist.clear();m_playlistFolder.clear();refreshPlaylist();}});
    connect(&m_thumbnails,&ThumbnailService::available,db,[db](const QString &){db->recent();});
    connect(db,&HistoryDb::failed,this,[this](const QString &message){emit m_controller->toast("History: "+message);Log::write("HISTORY_ERROR "+message);});
    connect(db,&HistoryDb::folderReady,this,[](const QString &p){QDesktopServices::openUrl(QUrl::fromLocalFile(p));});
    connect(db,&HistoryDb::neighborReady,this,[this](quint64 token,const QString &origin,const QString &path){
        if(token!=m_navigationToken||m_openPending||origin!=m_active.value("path").toString())return;
        if(path.isEmpty())emit m_controller->toast("No adjacent media file");else open(path);
    });
    m_db->recent();
}
ResumeStore::~ResumeStore(){save();m_thumbnails.setAllowed(false);}
int ResumeStore::continueIndex() const {
    for(int i=0;i<m_items.size();++i){auto r=m_items[i].toMap();if(r.value("available").toBool()&&!r.value("finished").toBool()&&r.value("position").toDouble()>0)return i;}return -1;
}
void ResumeStore::launch(bool noFile) {
    m_launchReady=true;m_autoLaunch=noFile&&m_settings->values().value("autoResume",false).toBool();
    if(m_autoLaunch&&m_haveRecent){m_autoLaunch=false;int i=continueIndex();if(i>=0)open(m_items[i].toMap().value("path").toString());}
    updateActivity();
}
void ResumeStore::open(const QString &path,bool fromStart) {
    const QString decoded=path.startsWith("file:")?QUrl(path).toLocalFile():path;
    const QString local=decoded.contains("://")?decoded:QDir::fromNativeSeparators(decoded);
    if(!m_customPaths.isEmpty()&&!m_customPaths.contains(local,Qt::CaseInsensitive)){m_customPaths.clear();m_playlist.clear();m_playlistFolder.clear();}
    if(!m_waitAdvance)m_failedPaths.remove(local);
    m_autoLaunch=false;save();m_openPending=true;m_fromStart=fromStart;m_home=false;emit homeChanged();m_thumbnails.setAllowed(false);m_db->lookup(local,++m_generation);
}
void ResumeStore::save() {
    if(!m_loaded||!m_positionReady||m_active.isEmpty()||m_controller->duration()<=0)return;
    auto r=m_active;r["duration"]=m_controller->duration();r["position"]=m_controller->position();
    // Keep the completed marker at the initial replay frame, then start a
    // fresh unfinished session once the replay makes meaningful progress.
    if(m_controller->position()>2&&m_controller->position()/m_controller->duration()<=.95)m_active["finished"]=false;
    r["finished"]=m_active.value("finished").toBool()||m_controller->position()/m_controller->duration()>.95;
    r["last_played"]=QDateTime::currentMSecsSinceEpoch();r["volume"]=m_controller->volume();r["speed"]=m_controller->speed();
    const auto options=m_controller->playbackOptions();r["audio_track"]=options.value("aid",-1);r["sub_track"]=options.value("sid",-1);r["sub_delay"]=options.value("sub-delay",0);
    m_db->save(r);
}
void ResumeStore::updateActivity() {
    const bool playing=m_loaded&&!m_controller->pause();
    if(playing&&!m_saveTimer.isActive())m_saveTimer.start();else if(!playing)m_saveTimer.stop();
    m_thumbnails.setAllowed(m_launchReady&&!m_openPending&&(m_home||(m_loaded&&m_controller->pause())));
}
void ResumeStore::finish() {if(!m_loaded||m_openPending)return;m_active["finished"]=true;save();m_loaded=false;updateActivity();m_failedPaths.clear();if(m_settings->values().value("autoplayNext",true).toBool())advance();else if(m_settings->values().value("stayOnLastFrame",false).toBool()){m_loaded=true;m_controller->setPause(true);}else showHome();}
void ResumeStore::showHome(){save();m_loaded=false;m_openPending=false;++m_generation;m_controller->stop();m_home=true;emit homeChanged();updateActivity();m_db->recent();}
void ResumeStore::remove(const QString &key){m_thumbnails.forget(key);m_db->remove(key);}
void ResumeStore::openFolder(const QString &path){m_db->locate(path);}
int ResumeStore::currentIndex() const {const auto path=m_active.value("path").toString();for(int i=0;i<m_playlist.size();++i)if(m_playlist[i].toMap().value("path").toString().compare(path,Qt::CaseInsensitive)==0)return i;return -1;}
void ResumeStore::refreshPlaylist(){const auto path=m_active.value("path").toString();const auto folder=path.contains("://")?path:QFileInfo(path).absolutePath();if(!m_playlist.isEmpty()&&folder==m_playlistFolder){publishPlaylist();return;}m_playlistFolder=folder;m_playlistPending=true;m_db->playlist(path,m_customPaths,m_settings->values().value("playlistIgnore","sample,trailer").toString().split(','),++m_playlistToken);}
void ResumeStore::openMany(const QStringList &paths){if(paths.isEmpty())return;m_failedPaths.clear();m_customPaths.clear();if(paths.size()>1)for(const auto &path:paths){auto p=path.startsWith("file:")?QUrl(path).toLocalFile():path;if(!p.contains("://"))p=QDir::fromNativeSeparators(p);if(!m_customPaths.contains(p,Qt::CaseInsensitive))m_customPaths.append(p);}m_playlist.clear();m_playlistFolder.clear();open(paths.first());}
void ResumeStore::playIndex(int index){if(index<0||index>=m_playlist.size()||m_openPending)return;m_failedPaths.clear();open(m_playlist[index].toMap().value("path").toString());}
void ResumeStore::reorder(int from,int to){if(from<0||to<0||from>=m_playlist.size()||to>=m_playlist.size()||from==to)return;m_playlist.move(from,to);m_customPaths.clear();for(const auto &r:m_playlist)m_customPaths.append(r.toMap().value("path").toString());publishPlaylist();}
void ResumeStore::navigate(int direction){if(m_openPending||m_playlistPending)return;if(direction<0&&m_loaded&&m_controller->position()>5&&m_settings->values().value("previousRestarts",true).toBool()){m_controller->seekTo(0);return;}const int index=currentIndex()+(direction<0?-1:1);if(currentIndex()<0||index<0||index>=m_playlist.size()){emit m_controller->toast("No adjacent media file");return;}playIndex(index);}
void ResumeStore::advance(bool failure){if(m_playlistPending){m_waitAdvance=true;return;}int next=currentIndex()+1;while(next<m_playlist.size()&&m_failedPaths.contains(m_playlist[next].toMap().value("path").toString()))++next;if(currentIndex()>=0&&next<m_playlist.size()){open(m_playlist[next].toMap().value("path").toString());return;}if(!failure&&m_settings->values().value("stayOnLastFrame",false).toBool()){m_loaded=true;m_controller->setPause(true);}else showHome();}
