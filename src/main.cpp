#include "core/MpvController.h"
#include "core/MpvRenderItem.h"
#include "core/Log.h"
#include "core/AppSettings.h"
#include "data/HistoryDb.h"
#include "data/ResumeStore.h"
#include "window/FramelessWindow.h"
#include "window/NativeEventFilter.h"
#include "platform/SingleInstance.h"
#include "platform/SleepInhibitor.h"
#include "platform/Smtc.h"
#include <QFileInfo>
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
#include <QKeyEvent>
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
    Q_INVOKABLE void homeReady(int rows) {Log::write(QString("HOME_SCENE_READY_MS=%1 rows=%2").arg(clock.elapsed()).arg(rows));}
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
    QStringList launchPaths;for(const auto &arg:app.arguments().mid(1))launchPaths.append(arg.contains("://")?arg:QFileInfo(arg).absoluteFilePath());
    SingleInstance instance;const int instanceResult=instance.start(launchPaths);if(instanceResult!=0){CoUninitialize();return instanceResult>0?0:4;}
    Log::start();
    Log::write(QString("START Qt=%1 PID=%2").arg(qVersion()).arg(QCoreApplication::applicationPid()));
    Log::write("QT_APP_READY_MS="+QString::number(diagnostics.clock.elapsed()));
    qmlRegisterType<MpvRenderItem>("FlowPlayer",1,0,"MpvRenderItem");
    MpvRenderItem::diagnosticsEnabled=qEnvironmentVariableIsSet("FLOW_METRICS");
    int exitCode=0;
    {
        AppSettings settings;
        HistoryDb history;
        MpvController controller;controller.configure(&settings);
        ResumeStore resume(&controller,&settings,&history);
        {
            FramelessWindow window(&controller);
            SleepInhibitor sleep(&controller);
            QObject::connect(&window,&QQuickWindow::frameSwapped,&window,[&]{++diagnostics.swaps;},Qt::DirectConnection);
            Log::write("WINDOW_CREATED_MS="+QString::number(diagnostics.clock.elapsed()));
            NativeEventFilter native(&window);app.installNativeEventFilter(&native);
            const auto profile=qEnvironmentVariable("FLOW_PROFILE");
            {
                window.prepareNativeFrame();window.show();
                std::unique_ptr<Smtc> smtc;
                QObject::connect(&controller,&MpvController::fileLoaded,&window,[&](const QString &){if(!smtc)smtc=std::make_unique<Smtc>(window.nativeId(),&controller,&resume);});
                QStringList pendingLaunch=launchPaths;
                QObject::connect(&instance,&SingleInstance::requested,&window,[&](const QStringList &paths){
                    if(!qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND")){if(window.visibility()==QWindow::Minimized){if(window.fullscreen())window.showFullScreen();else window.showNormal();}window.raise();window.requestActivate();}
                    if(paths.isEmpty())return;if(controller.ready())resume.openMany(paths);else pendingLaunch=paths;
                    Log::write(QString("IPC_RECEIVED paths=%1").arg(paths.size()));
                });
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
                    resume.launch(profile.isEmpty()&&app.arguments().size()==1);
                    if(profile.isEmpty()&&!pendingLaunch.isEmpty())resume.openMany(pendingLaunch);
                });
                // Explicit diagnostics only; never used for rendering or normal idle work.
                bool ok=false;const int quitMs=qEnvironmentVariableIntValue("FLOW_QUIT_MS",&ok);
                if(ok&&quitMs>0)QTimer::singleShot(quitMs,Qt::PreciseTimer,&window,&QWindow::close);
                const auto scenario=qEnvironmentVariable("FLOW_SCENARIO");
                if(scenario.startsWith("part5")&&scenario!="part5-perf"){
                    QObject::connect(&resume,&ResumeStore::playlistChanged,&window,[&]{QJsonArray paths;for(const auto &item:resume.playlist())paths.append(item.toMap().value("path").toString());Log::write("PART5_LIST "+QString::fromUtf8(QJsonDocument(QJsonObject{{"paths",paths},{"current",resume.currentIndex()}}).toJson(QJsonDocument::Compact)));});
                    auto stage=std::make_shared<int>(0);
                    QObject::connect(&controller,&MpvController::fileLoaded,&window,[&,stage,scenario](const QString &path){
                        const int index=(*stage)++;Log::write("PART5_LOADED "+path);
                        if((scenario=="part5-auto"||scenario=="part5-end")&&index==0)QTimer::singleShot(700,&window,[&]{controller.seekTo(controller.duration()-.3);});
                        else controller.setPause(true);
                        QTimer::singleShot(400,&window,[&]{auto *root=window.contentItem()->findChild<QObject*>("sceneRoot",Qt::FindDirectChildrenOnly);QVariant result;if(root)QMetaObject::invokeMethod(root,"diagnosticPlaylistChecks",Q_RETURN_ARG(QVariant,result));Log::write("PART5_PANEL "+result.toString());});
                        if(scenario=="part5-nav")QTimer::singleShot(850,&window,[&,index]{if(index==0)resume.navigate(1);if(index==1)resume.navigate(-1);if(index==2)resume.playIndex(resume.playlist().size()-1);if(index==3){resume.navigate(1);resume.playIndex(0);}if(index==4)resume.navigate(-1);});
                        if(scenario=="part5-custom"&&index==0)QTimer::singleShot(700,&window,[&]{resume.reorder(0,1);resume.navigate(-1);});
                        if(scenario=="part5-previous"&&index==0){QTimer::singleShot(600,&window,[&]{controller.seekTo(8);});QTimer::singleShot(1200,&window,[&]{resume.navigate(-1);});}
                    });
                    QObject::connect(&controller,&MpvController::positionSettled,&window,[&]{Log::write("PART5_SETTLED "+QString::number(controller.position(),'f',3));});
                    QTimer::singleShot(2800,&window,[&]{Log::write(QString("PART5_STATE home=%1 current=%2 pause=%3 position=%4").arg(resume.homeVisible()).arg(resume.currentIndex()).arg(controller.pause()).arg(controller.position()));});
                }
                if(scenario=="part5-perf"){
                    auto armed=std::make_shared<bool>(false);
                    QObject::connect(&controller,&MpvController::videoFramePresented,&window,[&,armed]{if(*armed)return;*armed=true;
                        auto mark=[&](const QString &phase){Log::write("PART5_PHASE "+QString::fromUtf8(QJsonDocument(QJsonObject{{"phase",phase},{"elapsedMs",double(diagnostics.clock.elapsed())},{"renders",double(MpvRenderItem::renderedFrames.load())},{"swaps",double(diagnostics.swaps.load())},{"dropped",double(controller.droppedFrames())},{"position",controller.position()}}).toJson(QJsonDocument::Compact)));};
                        auto panel=[&](bool open){auto *root=window.contentItem()->findChild<QObject*>("sceneRoot",Qt::FindDirectChildrenOnly);if(!root)return;QVariant result;if(open)QMetaObject::invokeMethod(root,"diagnosticPlaylistChecks",Q_RETURN_ARG(QVariant,result));else QMetaObject::invokeMethod(root,"closePopups");};
                        QTimer::singleShot(6000,&window,[mark]{mark("hidden_begin");});
                        QTimer::singleShot(10000,&window,[mark,panel]{mark("hidden_end");panel(true);});
                        QTimer::singleShot(12000,&window,[mark]{mark("playlist_begin");});
                        QTimer::singleShot(16000,&window,[&,mark,panel]{mark("playlist_end");panel(false);controller.setPause(true);});
                        QTimer::singleShot(18000,&window,[mark]{mark("paused_begin");});
                        QTimer::singleShot(23000,&window,[&,mark]{mark("paused_end");controller.setPause(false);});
                        QTimer::singleShot(27000,&window,[mark]{mark("resumed_begin");});
                        QTimer::singleShot(31000,&window,[&,mark]{mark("resumed_end");window.setDiagnosticVideoHidden(true);});
                        QTimer::singleShot(33000,&window,[mark]{mark("covered_begin");});
                        QTimer::singleShot(37000,&window,[mark]{mark("covered_end");});
                    });
                }
                QTimer uiHeartbeat;
                QElapsedTimer heartbeatClock;
                double maximumUiGapMs=0;
                struct SceneTimes {std::atomic<qint64> begin{0},maximum{0};};
                auto sceneTimes=std::make_shared<SceneTimes>();
                if(scenario.startsWith("part4")) {
                    auto checkHome=[&]{if(resume.homeVisible())Log::write(QString("PART4_HOME_INPUT caption=%1 client=%2 focusHome=%3").arg(window.titleAt(QPointF(100,20))).arg(window.controlAt(QPointF(100,100))).arg(window.activeFocusItem()&&window.activeFocusItem()->property("inputRole")=="home"));};
                    QTimer::singleShot(1500,&window,checkHome);QTimer::singleShot(4500,&window,checkHome);
                    heartbeatClock.start();uiHeartbeat.setTimerType(Qt::PreciseTimer);uiHeartbeat.setInterval(16);
                    QObject::connect(&uiHeartbeat,&QTimer::timeout,&window,[&]{maximumUiGapMs=qMax(maximumUiGapMs,heartbeatClock.nsecsElapsed()/1e6);heartbeatClock.restart();});uiHeartbeat.start();
                    QObject::connect(&window,&QQuickWindow::beforeSynchronizing,&window,[&diagnostics,sceneTimes]{sceneTimes->begin=diagnostics.clock.nsecsElapsed()/1000;},Qt::DirectConnection);
                    QObject::connect(&window,&QQuickWindow::afterRendering,&window,[&diagnostics,sceneTimes]{auto elapsed=diagnostics.clock.nsecsElapsed()/1000-sceneTimes->begin.load();auto old=sceneTimes->maximum.load();while(old<elapsed&&!sceneTimes->maximum.compare_exchange_weak(old,elapsed)){}},Qt::DirectConnection);
                    QObject::connect(&window,&QQuickWindow::closing,&window,[&,sceneTimes]{Log::write(QString("PART4_UI maxEventLoopGapMs=%1 maxSceneSyncRenderMs=%2").arg(maximumUiGapMs).arg(sceneTimes->maximum.load()/1000.0));});
                }
                if(scenario=="part4-playing") {
                    QObject::connect(&controller,&MpvController::videoFramePresented,&window,[&]{window.setUiHeld(true);});
                }
                if(scenario=="part4-pause") {
                    QObject::connect(&controller,&MpvController::fileLoaded,&window,[&](const QString &){controller.setPause(true);});
                }
                if(scenario=="part4-audit-cancel") {
                    QObject::connect(&controller,&MpvController::readyChanged,&window,[&]{QTimer::singleShot(0,&window,[&]{resume.showHome();});QTimer::singleShot(1000,&window,[&]{controller.open(app.arguments()[1]);QTimer::singleShot(0,&window,[&]{resume.showHome();});});});
                }
                if(scenario=="part4-audit-keys") {
                    QObject::connect(&controller,&MpvController::fileLoaded,&window,[&](const QString &){
                        controller.setPause(true);settings.setValue("seekStep",17);
                        QTimer::singleShot(600,&window,[&]{controller.seekTo(12.123);});
                        QTimer::singleShot(1300,&window,[&]{QKeyEvent event(QEvent::KeyPress,Qt::Key_Left,Qt::NoModifier);QCoreApplication::sendEvent(&window,&event);});
                        QTimer::singleShot(2000,&window,[&]{QKeyEvent event(QEvent::KeyPress,Qt::Key_Right,Qt::NoModifier);QCoreApplication::sendEvent(&window,&event);});
                    });
                }
                if(scenario=="part4-audit-nav") {
                    auto stage=std::make_shared<int>(0);
                    QObject::connect(&controller,&MpvController::fileLoaded,&window,[&,stage](const QString &path){
                        const int index=(*stage)++;controller.setPause(true);Log::write("AUDIT_NAV "+path);
                        if(index<3)QTimer::singleShot(500,&window,[&,index]{resume.navigate(index==0?-1:1);});
                    });
                }
                if(scenario=="part4-audit-ui") {
                    QObject::connect(&controller,&MpvController::videoFramePresented,&window,[&]{
                        auto *root=window.contentItem()->findChild<QObject*>("sceneRoot",Qt::FindDirectChildrenOnly);QVariant result;
                        if(root)QMetaObject::invokeMethod(root,"diagnosticLayoutChecks",Q_RETURN_ARG(QVariant,result));Log::write("AUDIT_LAYOUT "+result.toString());
                        auto mark=[&](const QString &phase){Log::write("AUDIT_FRAME "+QString::fromUtf8(QJsonDocument(QJsonObject{{"phase",phase},{"renders",double(MpvRenderItem::renderedFrames.load())},{"swaps",double(diagnostics.swaps.load())},{"dropped",double(controller.droppedFrames())},{"position",controller.position()},{"hidden",!window.controlsVisible()}}).toJson(QJsonDocument::Compact)));};
                        QTimer::singleShot(6000,&window,[mark]{mark("hidden_begin");});
                        QTimer::singleShot(10000,&window,[&,mark]{mark("hidden_end");controller.setPause(true);});
                        QTimer::singleShot(12000,&window,[mark]{mark("paused_begin");});
                        QTimer::singleShot(16000,&window,[&,mark]{mark("paused_end");controller.setPause(false);});
                        QTimer::singleShot(18000,&window,[mark]{mark("resume_begin");});
                        QTimer::singleShot(22000,&window,[mark]{mark("resume_end");});
                    });
                }
                if(scenario=="part4-switch") {
                    auto stage=std::make_shared<int>(0);
                    QObject::connect(&controller,&MpvController::fileLoaded,&window,[&,stage](const QString &){
                        const int current=(*stage)++;
                        if(current==0)QTimer::singleShot(700,&window,[&]{controller.setPause(true);controller.seekTo(12.123);controller.setVolume(61);controller.setPlaybackOption("speed",1.25);controller.selectTrack("audio",2);controller.selectTrack("sub",2);controller.setPlaybackOption("sub-delay",.3);});
                        if(current==0)QTimer::singleShot(1700,&window,[&]{controller.open(qEnvironmentVariable("FLOW_PART4_OTHER"));});
                        if(current==1)QTimer::singleShot(700,&window,[&]{controller.setPause(true);controller.seekTo(7.123);});
                        if(current==1)QTimer::singleShot(1700,&window,[&]{controller.open(app.arguments()[1]);});
                        if(current==2)QTimer::singleShot(400,&window,[&]{controller.setPause(true);});
                    });
                }
                if(scenario.startsWith("part4")) {
                    QObject::connect(&controller,&MpvController::positionSettled,&window,[&]{
                        Log::write("PART4_SETTLED "+QString::fromUtf8(QJsonDocument(QJsonObject{{"position",controller.position()},{"duration",controller.duration()},{"volume",controller.volume()},{"speed",controller.speed()},{"options",QJsonObject::fromVariantMap(controller.playbackOptions())}}).toJson(QJsonDocument::Compact)));
                    });
                    QObject::connect(&history,&HistoryDb::committed,&window,[](const QVariantMap &r){Log::write("PART4_COMMIT "+QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantMap(r)).toJson(QJsonDocument::Compact)));});
                }
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
