#include "core/MpvController.h"
#include "core/MpvRenderItem.h"
#include "core/Log.h"
#include "window/FramelessWindow.h"
#include "window/NativeEventFilter.h"
#include <QGuiApplication>
#include <QQmlContext>
#include <QSurfaceFormat>
#include <QElapsedTimer>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QFont>
#include <windows.h>
#include <objbase.h>
#include <clocale>

class Diagnostics final : public QObject {
    Q_OBJECT
public:
    QElapsedTimer clock;
    bool hadFrame=false;
    Diagnostics() {clock.start();}
    Q_INVOKABLE void firstFrame() {if(!hadFrame) {hadFrame=true;Log::write("FIRST_VIDEO_SWAP_MS="+QString::number(clock.elapsed()));}}
    Q_INVOKABLE void overlayClick() {Log::write("OVERLAY_CLICK");}
};
int main(int argc,char **argv) {
    Diagnostics diagnostics;
    qputenv("QSG_RENDER_LOOP","threaded");
    qputenv("QSG_INFO","1");
    QCoreApplication::setAttribute(Qt::AA_UseDesktopOpenGL);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
    QSurfaceFormat surface;surface.setSwapInterval(1);QSurfaceFormat::setDefaultFormat(surface);
    QGuiApplication app(argc,argv);
    app.setOrganizationName("FlowPlayer");app.setApplicationName("FlowPlayer");
    app.setFont(QFont("Segoe UI",9));
    std::setlocale(LC_NUMERIC,"C");
    CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    Log::start();
    Log::write(QString("START Qt=%1 PID=%2").arg(qVersion()).arg(QCoreApplication::applicationPid()));
    Log::write("QT_APP_READY_MS="+QString::number(diagnostics.clock.elapsed()));
    qmlRegisterType<MpvRenderItem>("FlowPlayer",1,0,"MpvRenderItem");
    MpvRenderItem::diagnosticsEnabled=qEnvironmentVariableIsSet("FLOW_METRICS");
    int exitCode=0;
    {
        MpvController controller;
        {
            FramelessWindow window(&controller);
            Log::write("WINDOW_CREATED_MS="+QString::number(diagnostics.clock.elapsed()));
            NativeEventFilter native(&window);app.installNativeEventFilter(&native);
            const auto profile=qEnvironmentVariable("FLOW_PROFILE");
            {
                window.prepareNativeFrame();window.show();
                bool firstSwap=true;
                QObject::connect(&window,&QQuickWindow::frameSwapped,&app,[&]{
                    if(!firstSwap)return;
                    firstSwap=false;
                    Log::write("FIRST_SWAP_MS="+QString::number(diagnostics.clock.elapsed()));
                    // Present this same dark window first. QML creation and
                    // engine loading cannot delay the first visible frame.
                    if(!window.initializeUi(&diagnostics,profile!="engine")) {app.exit(3);return;}
                    Log::write("QML_LOADED_MS="+QString::number(diagnostics.clock.elapsed()));
                    if(profile!="qt")controller.start();
                },Qt::QueuedConnection);
                Log::write("SHOW_CALLED_MS="+QString::number(diagnostics.clock.elapsed()));
                QObject::connect(&window,&QWindow::activeChanged,&app,[&]{
                    Log::write(QString("WINDOW_ACTIVE=%1 elapsedMs=%2").arg(window.isActive()).arg(diagnostics.clock.elapsed()));
                });
                QObject::connect(&controller,&MpvController::readyChanged,&app,[&]{
                    if(profile.isEmpty()&&app.arguments().size()>1)controller.open(app.arguments()[1]);
                });
                // Explicit diagnostics only; never used for rendering or normal idle work.
                bool ok=false;const int quitMs=qEnvironmentVariableIntValue("FLOW_QUIT_MS",&ok);
                if(ok&&quitMs>0)QTimer::singleShot(quitMs,Qt::PreciseTimer,&window,&QWindow::close);
                const auto scenario=qEnvironmentVariable("FLOW_SCENARIO");
                if(scenario=="pause") {
                    QTimer::singleShot(10000,Qt::PreciseTimer,&window,[&]{controller.setPause(true);Log::write("TEST_PAUSE_BEGIN");});
                    QTimer::singleShot(12000,Qt::PreciseTimer,&window,[]{Log::write("TEST_PAUSE_STEADY_BEGIN");});
                    QTimer::singleShot(72000,Qt::PreciseTimer,&window,[]{Log::write("TEST_PAUSE_STEADY_END");});
                }
                // Never minimize or restore a background test window.
                if(!qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND")&&(scenario=="idle"||scenario=="lifecycle")) {
                    const auto at=[&](int ms){return scenario=="lifecycle"?ms/10:ms;};
                    auto mark=[&](const QString &label) {
                        QJsonObject value{{"label",label},{"elapsedMs",double(diagnostics.clock.elapsed())},
                            {"renders",double(MpvRenderItem::renderedFrames.load())},{"callbacks",double(MpvRenderItem::updateCallbacks.load())}};
                        Log::write("IDLE_MARK "+QString::fromUtf8(QJsonDocument(value).toJson(QJsonDocument::Compact)));
                    };
                    QTimer::singleShot(at(10000),Qt::PreciseTimer,&window,[&]{controller.setPause(true);Log::write("TEST_PAUSE_BEGIN");});
                    QTimer::singleShot(at(12000),Qt::PreciseTimer,&window,[mark]{mark("pause-start");});
                    QTimer::singleShot(at(72000),Qt::PreciseTimer,&window,[&,mark]{mark("pause-end");controller.setPause(false);});
                    QTimer::singleShot(at(80000),Qt::PreciseTimer,&window,[&]{window.requestMinimize();Log::write("TEST_MINIMIZE_BEGIN");});
                    QTimer::singleShot(at(82000),Qt::PreciseTimer,&window,[mark]{mark("minimize-start");});
                    QTimer::singleShot(at(142000),Qt::PreciseTimer,&window,[&,mark]{mark("minimize-end");window.showNormal();});
                }
                QTimer metrics;
                if(qEnvironmentVariableIsSet("FLOW_METRICS")) {
                    auto captureMetrics=[&]{
                        QJsonObject sample{{"elapsedMs",double(diagnostics.clock.elapsed())},{"position",controller.position()},
                            {"hwdec",controller.hwdecCurrent()},{"dropped",double(controller.droppedFrames())},
                            {"mute",controller.mute()},{"windowActive",window.isActive()},
                            {"callbackToRenderMaxUs",double(MpvRenderItem::callbackDelayMaxUs.exchange(0))},
                            {"mpvRenderCallMaxUs",double(MpvRenderItem::renderCallMaxUs.exchange(0))},
                            {"renders",double(MpvRenderItem::renderedFrames.load())},{"callbacks",double(MpvRenderItem::updateCallbacks.load())},
                            {"pause",controller.pause()},{"cacheBytes",double(controller.cacheBytes())},{"avSyncSeconds",controller.avSync()}};
                        Log::write("METRIC "+QString::fromUtf8(QJsonDocument(sample).toJson(QJsonDocument::Compact)));
                    };
                    QObject::connect(&metrics,&QTimer::timeout,&app,captureMetrics);
                    QObject::connect(&window,&QQuickWindow::closing,&app,captureMetrics);
                    metrics.start(10000);
                }
                exitCode=app.exec();
                window.hide();window.releaseResources();
            }
            app.removeNativeEventFilter(&native);
        } // Qt destroys the scene graph and render context before the controller.
    } // Event worker terminates mpv and joins after the window is gone.
    Log::write("CLEAN_EXIT");Log::stop();CoUninitialize();return exitCode;
}
#include "main.moc"
