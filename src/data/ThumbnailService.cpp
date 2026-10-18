#include "ThumbnailService.h"
#include "HistoryDb.h"
#include "../core/Log.h"
#include <QImage>
#include <QPainter>
#include <QFileInfo>
#include <QSaveFile>
#include <QDir>
#include <QElapsedTimer>
#include <mpv/client.h>
#include <windows.h>
ThumbnailService::ThumbnailService(QString dir,QObject *p):QObject(p),m_directory(dir) {m_worker=std::thread(&ThumbnailService::run,this);}
ThumbnailService::~ThumbnailService(){{std::lock_guard l(m_mutex);m_stop=true;}m_cv.notify_one();m_worker.join();}
void ThumbnailService::setAllowed(bool allowed){{std::lock_guard l(m_mutex);if(m_allowed==allowed)return;m_allowed=allowed;}m_cv.notify_one();}
void ThumbnailService::request(const QVariantMap &r) {
    const QString key=r.value("file_key").toString();if(key.isEmpty()||!r.value("available").toBool()||!r.value("thumbnail").toString().isEmpty()||r.value("path").toString().contains("://"))return;
    {std::lock_guard l(m_mutex);if(m_pending.contains(key))return;m_cancelled.remove(key);m_pending.insert(key);m_queue.push_back(r);}m_cv.notify_one();
}
void ThumbnailService::forget(const QString &key){std::lock_guard l(m_mutex);m_cancelled.insert(key);}
void ThumbnailService::run() {
    SetThreadPriority(GetCurrentThread(),THREAD_MODE_BACKGROUND_BEGIN);
    mpv_handle *mpv=nullptr;
    while(!m_stop) {
        QVariantMap record;
        {std::unique_lock l(m_mutex);m_cv.wait(l,[this]{return m_stop||(m_allowed&&!m_queue.empty());});if(m_stop)break;record=m_queue.front();m_queue.pop_front();}
        const auto key=record.value("file_key").toString();const auto path=flowThumbnailPath(m_directory,key);
        bool success=QFileInfo::exists(path);
        if(!success&&m_allowed) {
            if(!mpv) {
                mpv=mpv_create();
                if(mpv) {
                    for(const auto &[name,value]:{std::pair{"config","no"},{"vo","null"},{"ao","null"},{"hwdec","no"},{"terminal","no"},{"load-scripts","no"},{"ytdl","no"},{"audio","no"},{"sub","no"},{"pause","yes"},{"keep-open","yes"},{"idle","yes"},{"cache","no"},{"demuxer-readahead-secs","0"},{"demuxer-max-bytes","8MiB"},{"screenshot-sw","yes"},{"vd-lavc-threads","1"},{"vf","scale=320:-2"}})mpv_set_option_string(mpv,name,value);
                    if(mpv_initialize(mpv)<0){mpv_terminate_destroy(mpv);mpv=nullptr;}
                }
            }
            if(mpv) {
                QElapsedTimer timer;timer.start();
                auto input=record.value("path").toString().toUtf8();auto options=QByteArray("start=")+QByteArray::number(record.value("duration").toDouble()*.1,'f',3);
                const char *load[]{"loadfile",input.constData(),"replace","-1",options.constData(),nullptr};mpv_command_async(mpv,1,load);
                bool frame=false;
                while(!m_stop&&m_allowed&&timer.elapsed()<10000) {
                    auto *e=mpv_wait_event(mpv,.05);
                    if(e->event_id==MPV_EVENT_PLAYBACK_RESTART){frame=true;break;}
                    if(e->event_id==MPV_EVENT_END_FILE)break;
                }
                if(frame&&m_allowed&&!m_stop) {
                    mpv_node node{};const char *args[]{"screenshot-raw","video","bgra",nullptr};
                    if(mpv_command_ret(mpv,args,&node)>=0&&node.format==MPV_FORMAT_NODE_MAP) {
                        int w=0,h=0,stride=0;mpv_byte_array *pixels=nullptr;
                        for(int i=0;i<node.u.list->num;++i){const QByteArray name=node.u.list->keys[i];auto &v=node.u.list->values[i];if(name=="w")w=int(v.u.int64);if(name=="h")h=int(v.u.int64);if(name=="stride")stride=int(v.u.int64);if(name=="data"&&v.format==MPV_FORMAT_BYTE_ARRAY)pixels=v.u.ba;}
                        if(pixels&&w>0&&h>0&&stride>=w*4&&pixels->size>=size_t(stride)*h) {
                            QImage source(static_cast<const uchar*>(pixels->data),w,h,stride,QImage::Format_ARGB32);
                            QImage image(320,180,QImage::Format_RGB32);image.fill(QColor("#20242c"));
                            auto scaled=source.scaled(320,180,Qt::KeepAspectRatio,Qt::SmoothTransformation);{QPainter painter(&image);painter.drawImage((320-scaled.width())/2,(180-scaled.height())/2,scaled);}
                            std::lock_guard l(m_mutex);
                            if(!m_cancelled.contains(key)&&m_allowed){QDir().mkpath(m_directory);QSaveFile file(path);success=file.open(QIODevice::WriteOnly)&&image.save(&file,"JPG",80)&&file.commit();}
                        }
                    }
                    mpv_free_node_contents(&node);
                }
                const char *stop[]{"stop",nullptr};mpv_command_async(mpv,2,stop);
                Log::write(QString("THUMBNAIL workerMs=%1 success=%2 deferredDuringPlayback=%3").arg(timer.elapsed()).arg(success).arg(!m_allowed));
            }
        }
        {std::lock_guard l(m_mutex);m_pending.remove(key);if(!success&&!m_allowed&&!m_stop&&!m_cancelled.contains(key)){m_pending.insert(key);m_queue.push_front(record);}}
        if(success)QMetaObject::invokeMethod(this,[this,key]{emit available(key);});
    }
    if(mpv)mpv_terminate_destroy(mpv);
    SetThreadPriority(GetCurrentThread(),THREAD_MODE_BACKGROUND_END);
}
