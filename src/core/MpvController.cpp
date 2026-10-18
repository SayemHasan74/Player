#include "MpvController.h"
#include "Log.h"
#include "AppSettings.h"
#include <QUrl>
#include <QSettings>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <cmath>
namespace {
QVariant nodeValue(const mpv_node &n) {
    switch(n.format) {
    case MPV_FORMAT_STRING:return QString::fromUtf8(n.u.string);
    case MPV_FORMAT_FLAG:return bool(n.u.flag);
    case MPV_FORMAT_INT64:return QVariant::fromValue<qint64>(n.u.int64);
    case MPV_FORMAT_DOUBLE:return n.u.double_;
    case MPV_FORMAT_NODE_ARRAY: {QVariantList a; for(int i=0;i<n.u.list->num;++i)a.push_back(nodeValue(n.u.list->values[i]));return a;}
    case MPV_FORMAT_NODE_MAP: {QVariantMap m;for(int i=0;i<n.u.list->num;++i)m[QString::fromUtf8(n.u.list->keys[i])]=nodeValue(n.u.list->values[i]);return m;}
    default:return {};
    }
}
}
MpvController::MpvController(QObject *p):QObject(p) {}
void MpvController::configure(AppSettings *settings) {
    m_settings=settings;m_initialSettings=settings->values();
    m_highQuality=m_initialSettings.value("highQuality").toBool();
    connect(settings,&AppSettings::valueChanged,this,&MpvController::applySetting);
    connect(this,&MpvController::readyChanged,this,[this]{
        const auto values=m_settings->values();
        for(auto i=values.cbegin();i!=values.cend();++i)applySetting(i.key(),i.value());
    });
}
void MpvController::applySetting(const QString &key,const QVariant &value) {
    if(key=="highQuality") {if(m_ready)setHighQuality(value.toBool());return;}
    QByteArray option;
    if(key=="hardwareDecoding") {command({"set","hwdec",value.toBool()?"nvdec,d3d11va-copy":"no"});return;}
    if(key=="volume")option="volume";
    if(key=="defaultSpeed")option="speed";
    if(key=="audioLanguage")option="alang";
    if(key=="subtitleLanguage")option="slang";
    if(!option.isEmpty())command({"set",option,value.toString().toUtf8()});
}
void MpvController::start() {m_thread=std::thread(&MpvController::run,this);}
MpvController::~MpvController() {
    // Change the condition under the wait mutex, including the atomic flag:
    // otherwise notify can race between the predicate check and wait, leaving
    // shutdown blocked on a worker which missed its final notification.
    {std::lock_guard lock(m_mutex);m_stop=true;}
    m_wakeup.notify_one();
    if(m_thread.joinable()) m_thread.join();
}
void MpvController::wakeup(void *p) {
    auto *self=static_cast<MpvController*>(p);
    {std::lock_guard lock(self->m_mutex);self->m_pending=true;} self->m_wakeup.notify_one();
}
void MpvController::run() {
    auto *mpv=mpv_create();
    if(!mpv) {QMetaObject::invokeMethod(this,[this]{emit error("mpv_create failed");});return;}
    const std::pair<const char*,const char*> options[]{
        {"config","no"},{"vo","libmpv"},{"idle","yes"},{"keep-open","yes"},{"terminal","no"},
        {"load-scripts","no"},{"ytdl","no"},{"osc","no"},{"osd-level","0"},{"osd-font-provider","none"},
        {"input-default-bindings","no"},{"input-vo-keyboard","no"},{"input-cursor","no"},
        {"cursor-autohide","no"},{"resume-playback","no"},{"save-position-on-quit","no"},
        {"hwdec","nvdec,d3d11va-copy"},{"hwdec-codecs","all"},{"hwdec-threads","1"},{"gpu-hwdec-interop","cuda"},{"vd-lavc-dr","yes"},
        {"scale","bilinear"},{"cscale","bilinear"},{"dscale","bilinear"},{"correct-downscaling","no"},
        {"deband","no"},{"interpolation","no"},{"video-sync","audio"},
        {"tone-mapping","bt.2390"},{"hdr-compute-peak","no"},
        {"cache","yes"},{"cache-secs","20"},{"demuxer-donate-buffer","no"},{"demuxer-max-bytes","150MiB"},{"demuxer-max-back-bytes","50MiB"},{"demuxer-readahead-secs","20"},
        {"ao","wasapi"},{"audio-exclusive","no"},{"audio-channels","auto-safe"},{"volume-max","150"},
        {"sub-auto","fuzzy"},{"sub-file-paths","Subs;Subtitles;Sub"},{"sub-ass","yes"},{"blend-subtitles","no"},
        {"audio-display","embedded-first"},
        {"msg-level","vd=v,vo=v"}
    };
    bool valid=true;
    for(const auto &[key,value]:options) {
        const int rc=mpv_set_option_string(mpv,key,value);
        Log::write(QString("option %1=%2: %3").arg(key,value,mpv_error_string(rc)));
        if(rc<0)valid=false;
    }
    QSettings settings;
    // Diagnostic baseline only; never used by the acceptance suite or normal
    // playback. Measures resident overhead without the packet-cache allocator.
    if(qEnvironmentVariableIsSet("FLOW_DIAGNOSTIC_NO_CACHE")) {
        mpv_set_option_string(mpv,"cache","no");
        mpv_set_option_string(mpv,"demuxer-readahead-secs","0");
        mpv_set_option_string(mpv,"demuxer-max-back-bytes","0");
        Log::write("DIAGNOSTIC_ONLY packet cache disabled; not an acceptance configuration");
    }
    if(qEnvironmentVariableIsSet("FLOW_TEST_MUTE"))mpv_set_option_string(mpv,"mute","yes");
    for(const auto *name:{"slang","alang"}) {
        auto language=m_initialSettings.value(QString(name)=="slang"?"subtitleLanguage":"audioLanguage","en,eng").toString().toUtf8();
        if(mpv_set_option_string(mpv,name,language)<0)valid=false;
    }
    for(const auto &[key,value]:std::vector<std::pair<QByteArray,QByteArray>>{
        {"volume",m_initialSettings.value("volume",100).toString().toUtf8()},
        {"speed",m_initialSettings.value("defaultSpeed",1).toString().toUtf8()},
        {"hwdec",m_initialSettings.value("hardwareDecoding",true).toBool()?"nvdec,d3d11va-copy":"no"}})
        if(mpv_set_option_string(mpv,key.constData(),value.constData())<0)valid=false;
    const int init=valid?mpv_initialize(mpv):MPV_ERROR_OPTION_ERROR;
    if(init<0) {
        Log::write(QString("mpv init failed: %1").arg(mpv_error_string(init)));
        mpv_terminate_destroy(mpv);
        QMetaObject::invokeMethod(this,[this]{emit error("libmpv initialization/options failed; see log");});return;
    }
    mpv_request_log_messages(mpv,"v");
    const std::pair<const char*,mpv_format> properties[]{
        {"time-pos",MPV_FORMAT_DOUBLE},{"duration",MPV_FORMAT_DOUBLE},{"pause",MPV_FORMAT_FLAG},{"avsync",MPV_FORMAT_DOUBLE},
        {"volume",MPV_FORMAT_DOUBLE},{"mute",MPV_FORMAT_FLAG},{"speed",MPV_FORMAT_DOUBLE},
        {"media-title",MPV_FORMAT_STRING},{"track-list",MPV_FORMAT_NODE},{"hwdec-current",MPV_FORMAT_STRING},
        {"frame-drop-count",MPV_FORMAT_INT64},{"video-out-params",MPV_FORMAT_NODE},
        {"idle-active",MPV_FORMAT_FLAG},
        {"demuxer-cache-state",MPV_FORMAT_NODE}
        ,{"vid",MPV_FORMAT_STRING},{"video-codec",MPV_FORMAT_STRING},
        {"container-fps",MPV_FORMAT_DOUBLE},{"video-bitrate",MPV_FORMAT_DOUBLE},
        {"chapter-list",MPV_FORMAT_NODE},{"sid",MPV_FORMAT_INT64},{"aid",MPV_FORMAT_INT64},
        {"sub-delay",MPV_FORMAT_DOUBLE},{"audio-delay",MPV_FORMAT_DOUBLE},
        {"sub-scale",MPV_FORMAT_DOUBLE},{"sub-pos",MPV_FORMAT_DOUBLE}
    };
    for(const auto &[name,format]:properties) mpv_observe_property(mpv,0,name,format);
    mpv_set_wakeup_callback(mpv,wakeup,this);
    m_handle=mpv;
    QMetaObject::invokeMethod(this,[this]{
        m_ready=true;
        emit readyChanged();
    });
    while(!m_stop) {
        {std::unique_lock lock(m_mutex);m_wakeup.wait(lock,[this]{return m_pending||m_stop;});m_pending=false;}
        if(m_stop)break;
        for(;;) {
            auto *event=mpv_wait_event(mpv,0);
            if(event->event_id==MPV_EVENT_NONE)break;
            if(event->event_id==MPV_EVENT_LOG_MESSAGE) {
                const auto *entry=static_cast<mpv_event_log_message*>(event->data);
                Log::write(QString("mpv [%1/%2] %3").arg(entry->prefix,entry->level,QString::fromUtf8(entry->text).trimmed()));
            } else if(event->event_id==MPV_EVENT_PROPERTY_CHANGE) {
                auto *p=static_cast<mpv_event_property*>(event->data);
                if(!p->data) {
                    const auto name=QString::fromUtf8(p->name);
                    if(name=="sid"||name=="aid")QMetaObject::invokeMethod(this,[this,name]{apply(name,-1);});
                    continue;
                }
                QVariant v;
                switch(p->format) {
                case MPV_FORMAT_DOUBLE:v=*static_cast<double*>(p->data);break;
                case MPV_FORMAT_INT64:v=QVariant::fromValue<qint64>(*static_cast<int64_t*>(p->data));break;
                case MPV_FORMAT_FLAG:v=bool(*static_cast<int*>(p->data));break;
                case MPV_FORMAT_STRING:v=QString::fromUtf8(*static_cast<char**>(p->data));break;
                case MPV_FORMAT_NODE:v=nodeValue(*static_cast<mpv_node*>(p->data));break;
                default:continue;
                }
                const auto name=QString::fromUtf8(p->name);
                QMetaObject::invokeMethod(this,[this,name,v]{apply(name,v);});
            } else if(event->event_id==MPV_EVENT_PLAYBACK_RESTART) {
                QMetaObject::invokeMethod(this,[this]{
                    if(m_exactSeekTarget>=0&&handle())mpv_get_property_async(handle(),4242,"time-pos",MPV_FORMAT_DOUBLE);
                });
            } else if(event->event_id==MPV_EVENT_GET_PROPERTY_REPLY&&event->reply_userdata==4242&&event->error>=0) {
                auto *property=static_cast<mpv_event_property*>(event->data);
                if(property->format==MPV_FORMAT_DOUBLE&&property->data) {
                    const double actual=*static_cast<double*>(property->data);
                    QMetaObject::invokeMethod(this,[this,actual]{
                        if(m_exactSeekTarget>=0) {emit seekCompleted(m_exactSeekTarget,actual);Log::write(QString("EXACT_SEEK target=%1 actual=%2 error=%3").arg(m_exactSeekTarget).arg(actual).arg(std::abs(actual-m_exactSeekTarget)));m_exactSeekTarget=-1;}
                    });
                }
            } else if(event->event_id==MPV_EVENT_COMMAND_REPLY&&event->error<0) {
                const auto message=QString("mpv command failed: %1").arg(mpv_error_string(event->error));
                Log::write(message); QMetaObject::invokeMethod(this,[this,message]{emit error(message);});
            }
        }
    }
    mpv_set_wakeup_callback(mpv,nullptr,nullptr);m_handle=nullptr;
    Log::write("mpv_terminate_destroy on event worker (render context must already be freed)");
    mpv_terminate_destroy(mpv);
}
void MpvController::apply(const QString &n,const QVariant &v) {
    if(n=="chapter-list") {m_chapters=v.toList();emit metadataChanged();return;}
    if(n=="sid"||n=="aid"||n=="sub-delay"||n=="audio-delay"||n=="sub-scale"||n=="sub-pos") {m_options[n]=v;emit metadataChanged();return;}
    if(n=="vid") {m_vid=v.toString();Log::write("VIDEO_TRACK="+m_vid);emit stateChanged();return;}
    if(n=="video-codec"||n=="container-fps"||n=="video-bitrate") {m_videoInfo[n]=v;emit stateChanged();return;}
    if(n=="idle-active") {if(v.toBool())emit playbackIdle();return;}
    if(n=="demuxer-cache-state") {m_cacheBytes=v.toMap().value("total-bytes",-1).toLongLong();m_buffered=v.toMap().value("seekable-ranges").toList();emit cacheChanged();return;}
    if(n=="avsync") {m_avSync=v.toDouble();return;}
    if(n=="video-out-params") {
        const auto parameters=v.toMap();const int w=parameters.value("dw").toInt(),h=parameters.value("dh").toInt();
        m_videoInfo["width"]=w;m_videoInfo["height"]=h;emit stateChanged();
        if(w>0&&h>0&&(w!=m_width||h!=m_height)) {m_width=w;m_height=h;emit videoSizeChanged(w,h);}
        return;
    }
    if(n=="time-pos")m_position=v.toDouble();
    else if(n=="duration")m_duration=v.toDouble();
    else if(n=="pause")m_pause=v.toBool();
    else if(n=="volume") {m_volume=v.toDouble();if(m_settings)m_settings->setValue("volume",m_volume);}
    else if(n=="mute") {const bool old=m_mute;m_mute=v.toBool();if(old!=m_mute)emit toast(m_mute?"Muted":"Unmuted");}
    else if(n=="speed")m_speed=v.toDouble();
    else if(n=="media-title")m_title=v.toString();
    else if(n=="track-list") {
        m_tracks=v.toList();bool audio=false,video=false;
        for(const auto &entry:m_tracks) {const auto track=entry.toMap();audio|=track.value("type")=="audio";video|=track.value("type")=="video"&&!track.value("albumart").toBool();}
        m_audioOnly=audio&&!video;emit metadataChanged();
    }
    else if(n=="hwdec-current") {m_hwdec=v.toString();Log::write("ACTIVE_HWDEC="+m_hwdec);}
    else if(n=="frame-drop-count")m_dropped=v.toLongLong();
    emit stateChanged();
}
void MpvController::command(const QList<QByteArray> &args) {
    if(auto *mpv=handle()) {
        std::vector<const char*> pointers;for(const auto &arg:args)pointers.push_back(arg.constData());pointers.push_back(nullptr);
        const int rc=mpv_command_async(mpv,0,pointers.data());
        if(rc<0)emit error(QString::fromUtf8(mpv_error_string(rc)));
    }
}
void MpvController::open(const QString &path) {
    if(!m_ready||!m_renderReady) {m_pendingFile=path;return;}
    const QUrl url(path);const auto file=url.isLocalFile()?url.toLocalFile():path;
    m_network=!url.scheme().isEmpty()&&!url.isLocalFile()&&url.scheme().size()>1;emit metadataChanged();
    command({"set","demuxer-max-bytes",url.scheme().startsWith("http")?"300MiB":"150MiB"});
    Log::write("OPEN_FILE "+file);
    command({"loadfile",file.toUtf8()});
}
void MpvController::setRenderReady(bool ready) {
    m_renderReady=ready;
    if(ready&&!m_pendingFile.isEmpty()) {auto path=m_pendingFile;m_pendingFile.clear();open(path);}
}
void MpvController::togglePause() {command({"cycle","pause"});}
void MpvController::setPause(bool paused) {command({"set","pause",paused?"yes":"no"});}
void MpvController::setHighQuality(bool on) {
    m_highQuality=on;
    command({"set","scale",on?"ewa_lanczossharp":"bilinear"});
    command({"set","cscale",on?"spline36":"bilinear"});
    command({"set","video-sync",on?"display-resample":"audio"});
    command({"set","interpolation",on?"yes":"no"});
    emit stateChanged();
}
void MpvController::seek(double seconds,bool exact) {
    command({"seek",QByteArray::number(seconds),exact?"relative+exact":"relative"});
    emit toast(QString("Seek %1%2 s").arg(seconds>=0?"+":"").arg(seconds));
}
void MpvController::seekTo(double seconds,bool exact) {
    seconds=std::clamp(seconds,0.0,m_duration);if(exact)m_exactSeekTarget=seconds;
    command({"seek",QByteArray::number(seconds),exact?"absolute+exact":"absolute+keyframes"});
}
void MpvController::selectTrack(const QString &type,int id) {
    if(type!="audio"&&type!="sub")return;
    command({"set",type=="audio"?"aid":"sid",id<0?QByteArray("no"):QByteArray::number(id)});
    emit toast(QString("%1: %2").arg(type=="audio"?"Audio":"Subtitles").arg(id<0?"Off":QString::number(id)));
}
void MpvController::cycleTrack(const QString &type) {
    QList<int> ids;if(type=="sub")ids.append(-1);int selected=-1;
    for(const auto &entry:m_tracks) {const auto t=entry.toMap();if(t.value("type")==type) {ids.append(t.value("id").toInt());if(t.value("selected").toBool())selected=t.value("id").toInt();}}
    if(ids.isEmpty()){emit toast("No tracks");return;}selectTrack(type,ids[(ids.indexOf(selected)+1)%ids.size()]);
}
void MpvController::addSubtitle(const QString &path) {
    const QUrl url(path);command({"sub-add",(url.isLocalFile()?url.toLocalFile():path).toUtf8(),"select"});emit toast("Subtitle loaded");
}
void MpvController::setPlaybackOption(const QString &key,const QVariant &value) {
    if(key!="sub-delay"&&key!="audio-delay"&&key!="sub-scale"&&key!="sub-pos"&&key!="speed")return;
    double number=value.toDouble();if(!std::isfinite(number))return;
    if(key=="sub-scale")number=std::clamp(number,0.2,5.0);
    if(key=="sub-pos")number=std::clamp(number,0.0,100.0);
    if(key=="speed")number=std::clamp(number,0.1,16.0);
    command({"set",key.toUtf8(),QByteArray::number(number)});emit toast(QString("%1: %2").arg(key).arg(number));
}
void MpvController::setAspect(const QString &aspect) {
    const QStringList choices{"auto","16:9","4:3","21:9","fill"};if(!choices.contains(aspect))return;
    command({"set","video-aspect-override",(aspect=="auto"||aspect=="fill")?QByteArray("-1"):aspect.toUtf8()});
    command({"set","panscan",aspect=="fill"?"1":"0"});emit toast("Aspect: "+aspect);
}
void MpvController::adjustVolume(double delta) {setVolume(m_volume+delta);}
void MpvController::setVolume(double value) {
    value=std::clamp(value,0.0,150.0);command({"set","volume",QByteArray::number(value)});emit toast(QString("Volume %1%").arg(qRound(value)));
}
void MpvController::toggleMute() {command({"cycle","mute"});}
void MpvController::adjustSpeed(double delta) {
    const double value=std::clamp(std::round((m_speed+delta)*10)/10,0.1,16.0);
    command({"set","speed",QByteArray::number(value)});emit toast(QString("Speed %1×").arg(value,0,'f',1));
}
void MpvController::resetSpeed() {command({"set","speed","1"});emit toast("Speed 1.0×");}
void MpvController::frameStep(bool backward) {if(m_pause)command({backward?"frame-back-step":"frame-step"});}
