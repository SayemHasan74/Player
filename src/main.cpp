#include "core/MpvController.h"
#include "core/MpvRenderItem.h"
#include "core/Log.h"
#include "core/AppSettings.h"
#include "window/FramelessWindow.h"
#include "window/NativeEventFilter.h"
#include <QGuiApplication>
#include <QQmlContext>
#include <QSurfaceFormat>
#include <QElapsedTimer>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QFont>
#include <QQuickItem>
#include <windows.h>
#include <objbase.h>
#include <clocale>
#include <memory>

class Diagnostics final : public QObject {
    Q_OBJECT
public:
    QElapsedTimer clock;
    bool hadFrame=false;
    std::atomic<quint64> swaps{0};
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
        AppSettings settings;
        MpvController controller;controller.configure(&settings);
        {
            FramelessWindow window(&controller);
            QObject::connect(&window,&QQuickWindow::frameSwapped,&window,[&]{++diagnostics.swaps;},Qt::DirectConnection);
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
                if(scenario=="part3-controls"||scenario=="part3-tracks") {
                    auto armed=std::make_shared<bool>(false);
                    QObject::connect(&controller,&MpvController::videoFramePresented,&window,[&,armed,scenario]{
                        if(*armed)return;*armed=true;
                        auto mark=[&](const QString &phase) {
                            QJsonObject state{{"phase",phase},{"elapsedMs",double(diagnostics.clock.elapsed())},
                                {"position",controller.position()},{"dropped",double(controller.droppedFrames())},
                                {"renders",double(MpvRenderItem::renderedFrames.load())},{"swaps",double(diagnostics.swaps.load())},
                                {"controlsVisible",window.controlsVisible()},{"pause",controller.pause()},
                                {"highQuality",controller.highQuality()},{"volume",controller.volume()},
                                {"audioOnly",controller.audioOnly()},{"tracks",QJsonArray::fromVariantList(controller.tracks())},
                                {"chapters",QJsonArray::fromVariantList(controller.chapters())},
                                {"options",QJsonObject::fromVariantMap(controller.playbackOptions())}};
                            Log::write("PART3_STAGE "+QString::fromUtf8(QJsonDocument(state).toJson(QJsonDocument::Compact)));
                        };
                        if(scenario=="part3-controls") {
                            auto *root=window.contentItem()->findChild<QObject*>("sceneRoot",Qt::FindDirectChildrenOnly);
                            QVariant result;
                            const bool invoked=root&&QMetaObject::invokeMethod(root,"diagnosticPopupChecks",Q_RETURN_ARG(QVariant,result));
                            Log::write(QString("PART3_POPUPS invoked=%1 result=%2").arg(invoked).arg(result.toString()));
                            window.setUiHeld(true);mark("visible_begin");
                            QTimer::singleShot(6000,Qt::PreciseTimer,&window,[&,mark]{mark("visible_end");window.setUiHeld(false);});
                            QTimer::singleShot(8800,Qt::PreciseTimer,&window,[mark]{mark("hidden_begin");});
                            QTimer::singleShot(15000,Qt::PreciseTimer,&window,[&,mark]{mark("hidden_end");controller.setPause(true);});
                            QTimer::singleShot(17000,Qt::PreciseTimer,&window,[mark]{mark("paused_begin");});
                            QTimer::singleShot(22000,Qt::PreciseTimer,&window,[&,mark]{mark("paused_end");controller.setPause(false);});
                            QTimer::singleShot(24000,Qt::PreciseTimer,&window,[&,mark]{mark("before_quality");window.setUiHeld(true);controller.setHighQuality(true);});
                            QTimer::singleShot(25500,Qt::PreciseTimer,&window,[mark]{mark("quality_begin");});
                            QTimer::singleShot(31500,Qt::PreciseTimer,&window,[&,mark]{mark("quality_end");controller.setHighQuality(false);window.setUiHeld(false);});
                            QTimer::singleShot(33000,Qt::PreciseTimer,&window,[&]{controller.setPause(true);});
                            QTimer::singleShot(34000,Qt::PreciseTimer,&window,[&]{controller.seekTo(10.123,true);});
                            QTimer::singleShot(36500,Qt::PreciseTimer,&window,[mark]{mark("seek_done");});
                        } else {
                            mark("tracks_loaded");
                            QTimer::singleShot(1000,Qt::PreciseTimer,&window,[&]{controller.cycleTrack("audio");controller.selectTrack("sub",2);});
                            QTimer::singleShot(2500,Qt::PreciseTimer,&window,[&]{controller.setPlaybackOption("sub-delay",.1);controller.setPlaybackOption("audio-delay",.1);controller.setPlaybackOption("sub-scale",1.2);controller.setPlaybackOption("sub-pos",90);});
                            QTimer::singleShot(4000,Qt::PreciseTimer,&window,[&]{controller.addSubtitle("tests/fixtures/part3/sample.vtt");controller.setVolume(73);});
                            QTimer::singleShot(6000,Qt::PreciseTimer,&window,[&,mark]{mark("tracks_switched");controller.setPause(true);});
                            QTimer::singleShot(7000,Qt::PreciseTimer,&window,[&]{controller.seekTo(4.123,true);});
                            QTimer::singleShot(9000,Qt::PreciseTimer,&window,[&,mark]{mark("subtitle_seek");controller.setPause(false);controller.open("tests/media/part3-cover.mp3");});
                            QTimer::singleShot(13000,Qt::PreciseTimer,&window,[mark]{mark("cover_loaded");});
                        }
                    });
                }
                if(scenario=="part2-warm-mini") {
                    // Presentation-only smoke: never fullscreen, raise, minimize,
                    // move or resize the background window on the user's desktop.
                    auto armed=std::make_shared<bool>(false);
                    QObject::connect(&controller,&MpvController::videoFramePresented,&window,[&,armed]{
                        if(*armed)return;*armed=true;Log::write("PART2_WARM_MINI_STARTED_AFTER_FIRST_FRAME");
                        QTimer::singleShot(6000,Qt::PreciseTimer,&window,[&]{window.setDiagnosticVideoHidden(true);});
                        QTimer::singleShot(12000,Qt::PreciseTimer,&window,[&]{window.setDiagnosticVideoHidden(false);});
                    });
                }
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
                            {"videoTrack",controller.videoTrack()},
                            {"videoHidden",window.videoHidden()},
                            {"controlsVisible",window.controlsVisible()},{"sceneSwaps",double(diagnostics.swaps.load())},
                            {"volume",controller.volume()},{"highQuality",controller.highQuality()},
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
