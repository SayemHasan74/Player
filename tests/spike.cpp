// Render-API spike, following mpv-examples/libmpv/qt_opengl and libmpv/qml.
#include <QGuiApplication>
#include <QQuickView>
#include <QQuickFramebufferObject>
#include <QOpenGLContext>
#include <QtOpenGL/QOpenGLFramebufferObject>
#include <QQuickOpenGLUtils>
#include <QSurfaceFormat>
#include <QTimer>
#include <mpv/client.h>
#include <mpv/render_gl.h>
#include <thread>
#include <cstdio>
#include <clocale>

class Video final : public QQuickFramebufferObject {
    Q_OBJECT
public:
    mpv_handle *mpv = nullptr;
    explicit Video(QQuickItem *parent=nullptr) : QQuickFramebufferObject(parent) {}
    class Render final : public Renderer {
        Video *item;
        mpv_render_context *context=nullptr;
    public:
        explicit Render(Video *v): item(v) {}
        ~Render() override { if(context) mpv_render_context_free(context); }
        QOpenGLFramebufferObject *createFramebufferObject(const QSize &size) override {
            if(!context) {
                mpv_opengl_init_params gl{[](void*,const char *name)->void* {
                    return reinterpret_cast<void*>(QOpenGLContext::currentContext()->getProcAddress(name));
                },nullptr};
                mpv_render_param p[]{{MPV_RENDER_PARAM_API_TYPE,(void*)MPV_RENDER_API_TYPE_OPENGL},{MPV_RENDER_PARAM_OPENGL_INIT_PARAMS,&gl},{MPV_RENDER_PARAM_INVALID,nullptr}};
                if(mpv_render_context_create(&context,item->mpv,p)<0) qFatal("render context failed");
                mpv_render_context_set_update_callback(context,[](void *p){ QMetaObject::invokeMethod(static_cast<Video*>(p),[p]{static_cast<Video*>(p)->update();},Qt::QueuedConnection); },item);
                QObject::connect(item->window(),&QQuickWindow::frameSwapped,item->window(),[this]{mpv_render_context_report_swap(context);},Qt::DirectConnection);
            }
            return new QOpenGLFramebufferObject(size);
        }
        void render() override {
            QQuickOpenGLUtils::resetOpenGLState();
            auto *f=framebufferObject(); mpv_opengl_fbo target{int(f->handle()),f->width(),f->height(),0}; int flip=0;
            mpv_render_param p[]{{MPV_RENDER_PARAM_OPENGL_FBO,&target},{MPV_RENDER_PARAM_FLIP_Y,&flip},{MPV_RENDER_PARAM_INVALID,nullptr}};
            mpv_render_context_render(context,p);
            QQuickOpenGLUtils::resetOpenGLState();
        }
    };
    Renderer *createRenderer() const override { return new Render(const_cast<Video*>(this)); }
};
int main(int argc,char **argv) {
    qputenv("QSG_RENDER_LOOP","threaded");
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QSurfaceFormat format; format.setSwapInterval(1); QSurfaceFormat::setDefaultFormat(format);
    QGuiApplication app(argc,argv); std::setlocale(LC_NUMERIC,"C");
    auto *mpv=mpv_create();
    for(auto opt : {std::pair{"config","no"},{"vo","libmpv"},{"hwdec","nvdec,d3d11va-copy"},{"idle","yes"},{"keep-open","yes"},{"terminal","no"},{"msg-level","vd=v,vo=v"}}) mpv_set_option_string(mpv,opt.first,opt.second);
    mpv_set_option_string(mpv,"log-file",qEnvironmentVariable("FLOW_SPIKE_LOG").toUtf8());
    if(mpv_initialize(mpv)<0) return 2;
    {
        QQuickView view; view.setColor(QColor("#15171b")); view.resize(960,540);
        view.setPersistentSceneGraph(false); view.setPersistentGraphics(false);
        auto *video=new Video(view.contentItem()); video->mpv=mpv; video->setSize(view.size());
        QObject::connect(&view,&QQuickWindow::widthChanged,video,[&]{video->setWidth(view.width());});
        QObject::connect(&view,&QQuickWindow::heightChanged,video,[&]{video->setHeight(view.height());});
        view.show();
        QTimer::singleShot(250,&app,[&]{ if(argc>1) { const char *c[]{"loadfile",argv[1],nullptr}; mpv_command_async(mpv,0,c); }});
        QTimer::singleShot(10000,&app,&QCoreApplication::quit);
        app.exec();
        view.hide(); view.releaseResources();
    }
    mpv_terminate_destroy(mpv);
}
#include "spike.moc"
