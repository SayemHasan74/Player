#include "MpvController.h"
#include "Log.h"
#include <QUrl>
#include <QSettings>
#include <stdexcept>
#include <vector>
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
        {"sub-auto","fuzzy"},{"sub-file-paths","Subs:subs:Subtitles:Sub"},{"sub-ass","yes"},{"blend-subtitles","no"},
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
        auto language=settings.value(name,"en,eng").toString().toUtf8();
        if(mpv_set_option_string(mpv,name,language)<0)valid=false;
    }
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
    };
    for(const auto &[name,format]:properties) mpv_observe_property(mpv,0,name,format);
    mpv_set_wakeup_callback(mpv,wakeup,this);
    m_handle=mpv;
    QMetaObject::invokeMethod(this,[this]{m_ready=true;emit readyChanged();});
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
                auto *p=static_cast<mpv_event_property*>(event->data); if(!p->data)continue;
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
    if(n=="idle-active") {if(v.toBool())emit playbackIdle();return;}
    if(n=="demuxer-cache-state") {m_cacheBytes=v.toMap().value("total-bytes",-1).toLongLong();return;}
    if(n=="avsync") {m_avSync=v.toDouble();return;}
    if(n=="video-out-params") {
        const auto parameters=v.toMap();const int w=parameters.value("dw").toInt(),h=parameters.value("dh").toInt();
        if(w>0&&h>0&&(w!=m_width||h!=m_height)) {m_width=w;m_height=h;emit videoSizeChanged(w,h);}
        return;
    }
    if(n=="time-pos")m_position=v.toDouble();
    else if(n=="duration")m_duration=v.toDouble();
    else if(n=="pause")m_pause=v.toBool();
    else if(n=="volume")m_volume=v.toDouble();
    else if(n=="mute")m_mute=v.toBool();
    else if(n=="speed")m_speed=v.toDouble();
    else if(n=="media-title")m_title=v.toString();
    else if(n=="track-list")m_tracks=v.toList();
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
    command({"set","interpolation",on?"yes":"no"});emit stateChanged();
}
