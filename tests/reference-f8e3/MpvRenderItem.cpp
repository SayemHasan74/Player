#include "MpvRenderItem.h"
#include "Log.h"
#include <QOpenGLContext>
#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>
#include <QQuickWindow>
#include <mpv/render_gl.h>
#include <chrono>

namespace {
qint64 nowUs() {return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void keepMaximum(std::atomic<qint64> &value,qint64 candidate) {
    auto previous=value.load();
    while(previous<candidate&&!value.compare_exchange_weak(previous,candidate)) {}
}
class MpvRenderer final : public QQuickFramebufferObject::Renderer {
    MpvController *m_controller;
    QQuickWindow *m_window;
    mpv_render_context *m_context=nullptr;
    QMetaObject::Connection m_swap;
    bool m_resize=true,m_first=true,m_firstPending=false;
    static void *getProc(void *,const char *name) {
        return reinterpret_cast<void*>(QOpenGLContext::currentContext()->getProcAddress(name));
    }
public:
    explicit MpvRenderer(MpvRenderItem *item):m_controller(item->controller()),m_window(item->window()) {}
    ~MpvRenderer() override {
        QObject::disconnect(m_swap);
        if(m_context) {
            mpv_render_context_set_update_callback(m_context,nullptr,nullptr);
            Log::write("mpv_render_context_free on render thread; GL current="+QString::number(QOpenGLContext::currentContext()!=nullptr));
            mpv_render_context_free(m_context);
            auto *controller=m_controller;QMetaObject::invokeMethod(controller,[controller]{controller->setRenderReady(false);});
        }
    }
    QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
        // Qt supplies a physical-pixel size here, already multiplied by DPR.
        if(!m_context) {
            mpv_opengl_init_params gl{getProc,nullptr};
            mpv_render_param params[]{
                {MPV_RENDER_PARAM_API_TYPE,(void*)MPV_RENDER_API_TYPE_OPENGL},
                {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS,&gl},
                {MPV_RENDER_PARAM_INVALID,nullptr}
            };
            int rc=mpv_render_context_create(&m_context,m_controller->handle(),params);
            if(rc<0) {
                const auto message=QString("Render context failed: %1").arg(mpv_error_string(rc));Log::write(message);
                auto *controller=m_controller;QMetaObject::invokeMethod(controller,[controller,message]{emit controller->error(message);});
                return new QOpenGLFramebufferObject(size);
            }
            mpv_render_context_set_update_callback(m_context,[](void *p){
                ++MpvRenderItem::updateCallbacks;
                if(MpvRenderItem::diagnosticsEnabled) {
                    qint64 empty=0;MpvRenderItem::callbackAtUs.compare_exchange_strong(empty,nowUs());
                }
                auto *controller=static_cast<MpvController*>(p);
                QMetaObject::invokeMethod(controller,[controller]{emit controller->renderUpdateRequested();},Qt::QueuedConnection);
            },m_controller);
            m_swap=QObject::connect(m_window,&QQuickWindow::frameSwapped,m_window,[this]{
                if(m_context)mpv_render_context_report_swap(m_context);
                if(m_firstPending) {m_firstPending=false;auto *controller=m_controller;QMetaObject::invokeMethod(controller,[controller]{emit controller->videoFramePresented();});}
            },Qt::DirectConnection);
            Log::write("Render context created: OpenGL, threaded Qt Quick, same scene");
            auto *controller=m_controller;QMetaObject::invokeMethod(controller,[controller]{controller->setRenderReady(true);});
        }
        m_resize=true;
        return new QOpenGLFramebufferObject(size);
    }
    void render() override {
        if(!m_context)return;
        if(MpvRenderItem::diagnosticsEnabled) {
            const auto callback=MpvRenderItem::callbackAtUs.exchange(0);
            if(callback)keepMaximum(MpvRenderItem::callbackDelayMaxUs,nowUs()-callback);
        }
        const auto flags=mpv_render_context_update(m_context);
        // A replaced framebuffer must also be populated while paused.
        if(!(flags&MPV_RENDER_UPDATE_FRAME)&&!m_resize)return;
        mpv_render_frame_info info{};
        mpv_render_context_get_info(m_context,{MPV_RENDER_PARAM_NEXT_FRAME_INFO,&info});
        QQuickOpenGLUtils::resetOpenGLState();
        auto *f=framebufferObject();mpv_opengl_fbo target{int(f->handle()),f->width(),f->height(),0};
        int flip=0;
        mpv_render_param params[]{
            {MPV_RENDER_PARAM_OPENGL_FBO,&target},{MPV_RENDER_PARAM_FLIP_Y,&flip},
            {MPV_RENDER_PARAM_INVALID,nullptr}
        };
        const auto started=MpvRenderItem::diagnosticsEnabled?nowUs():0;
        mpv_render_context_render(m_context,params);m_resize=false;++MpvRenderItem::renderedFrames;
        if(started)keepMaximum(MpvRenderItem::renderCallMaxUs,nowUs()-started);
        QQuickOpenGLUtils::resetOpenGLState();
        if(m_first&&(info.flags&MPV_RENDER_FRAME_INFO_PRESENT)&&!(info.flags&MPV_RENDER_FRAME_INFO_REDRAW)) {
            m_first=false;m_firstPending=true;
        }
    }
};
}
MpvRenderItem::MpvRenderItem(QQuickItem *parent):QQuickFramebufferObject(parent) {}
void MpvRenderItem::setController(MpvController *c) {
    if(m_controller==c)return;
    if(m_controller)disconnect(m_controller,nullptr,this,nullptr);
    m_controller=c;
    if(c) {
        connect(c,&MpvController::renderUpdateRequested,this,&MpvRenderItem::scheduleFrame);
        connect(c,&MpvController::videoFramePresented,this,&MpvRenderItem::firstFrame);
    }
    emit controllerChanged();update();
}
QQuickFramebufferObject::Renderer *MpvRenderItem::createRenderer() const {return new MpvRenderer(const_cast<MpvRenderItem*>(this));}
void MpvRenderItem::scheduleFrame() {if(window()&&window()->isVisible()&&window()->visibility()!=QWindow::Minimized)update();}
