#include "FramelessWindow.h"
#include "../core/MpvController.h"
#include "../core/Log.h"
#include <QScreen>
#include <QKeyEvent>
#include <QMimeData>
#include <QDropEvent>
#include <QUrl>
#include <QCloseEvent>
#include <QTimer>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>
#include <QQuickItem>
#include <windows.h>
#include <dwmapi.h>
#include <shobjidl.h>
#include <algorithm>
FramelessWindow::FramelessWindow(MpvController *c):m_controller(c) {
    setTitle("FlowPlayer");setColor(QColor("#121417"));
    setFlags(Qt::Window|Qt::FramelessWindowHint|Qt::WindowMinimizeButtonHint|Qt::WindowMaximizeButtonHint|Qt::WindowCloseButtonHint);
    if(qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND")) {
        setFlag(Qt::WindowDoesNotAcceptFocus);
        setFlag(Qt::WindowStaysOnBottomHint);
        setFlags((flags()&~Qt::WindowType_Mask)|Qt::Tool);
    }
    // Keep the render API attached while hidden/minimized; destruction still
    // releases the renderer on Qt's render thread before controller teardown.
    setPersistentGraphics(true);setPersistentSceneGraph(true);
    setMinimumSize(QSize(320,180));
    QSettings settings;m_lockAspect=settings.value("window/lockAspectRatio",true).toBool();
    const auto work=screen()->availableGeometry();
    auto geometry=settings.value("window/geometry",QRect(work.center()-QPoint(480,270),QSize(960,540))).toRect();
    if(!work.intersects(geometry))geometry.moveCenter(work.center());
    setGeometry(geometry);m_lastNormalGeometry=geometry;
    connect(c,&MpvController::videoSizeChanged,this,&FramelessWindow::fitVideo);
    connect(c,&MpvController::playbackIdle,this,[this]{
        if(m_closeRequested) {m_closeReady=true;QTimer::singleShot(0,this,&QWindow::close);}
    });
    connect(c,&MpvController::stateChanged,this,[this]{
        updatePlaybackTiming();
        if(m_minimizePending&&m_controller->pause()&&m_minimizeSwaps==0) {m_minimizeSwaps=2;emit m_controller->renderUpdateRequested();update();}
    });
    connect(this,&QWindow::visibilityChanged,this,[this]{updatePlaybackTiming();});
    connect(this,&QQuickWindow::frameSwapped,this,[this]{
        if(m_minimizePending&&m_minimizeSwaps>0) {
            if(--m_minimizeSwaps==0) {m_minimizePending=false;m_minimizePrepared=true;showMinimized();}
            else update();
        }
        if(m_wasMinimized&&visibility()!=QWindow::Minimized&&isExposed()) {
            m_wasMinimized=false;
            // A render callback received while hidden remains pending in mpv.
            // Consume it on exposure even when no new callback edge arrives.
            emit m_controller->renderUpdateRequested();
            if(m_resumeAfterMinimize) {m_resumeAfterMinimize=false;m_controller->setPause(false);}
        }
    },Qt::QueuedConnection);
}
bool FramelessWindow::initializeUi(QObject *diagnostics,bool enableVideo) {
    // QQuickView constructs its QQmlEngine before it can show a window.
    // Keep the same QQuickWindow, and initialize its scene after first swap.
    auto *engine=new QQmlEngine(this);
    auto *context=engine->rootContext();
    context->setContextProperty("mpv",m_controller);
    context->setContextProperty("playerWindow",this);
    context->setContextProperty("appDiagnostics",diagnostics);
    context->setContextProperty("enableVideoSurface",enableVideo);
    QQmlComponent component(engine,QUrl("qrc:/ui/Main.qml"));
    auto *object=component.create();
    auto *root=qobject_cast<QQuickItem*>(object);
    if(!root) {Log::write("QML load failed: "+component.errorString());delete object;return false;}
    root->setParent(contentItem());
    root->setParentItem(contentItem());
    root->setSize(size());
    connect(this,&QWindow::widthChanged,root,[root](int width){root->setWidth(width);});
    connect(this,&QWindow::heightChanged,root,[root](int height){root->setHeight(height);});
    return true;
}
void FramelessWindow::updatePlaybackTiming() {
    const bool playing=!m_controller->pause()&&m_controller->duration()>0&&isVisible()&&visibility()!=QWindow::Minimized;
    if(playing==m_precisePlaybackTiming)return;
    m_precisePlaybackTiming=playing;
    PROCESS_POWER_THROTTLING_STATE state{};
    state.Version=PROCESS_POWER_THROTTLING_CURRENT_VERSION;
    // Win11 can ignore timer resolution for inaudible, occluded windows.
    // Honor Qt/mpv's existing requests while playing; do not add a timer.
    state.ControlMask=playing?PROCESS_POWER_THROTTLING_IGNORE_TIMER_RESOLUTION:0;
    state.StateMask=0;
    const BOOL success=SetProcessInformation(GetCurrentProcess(),ProcessPowerThrottling,&state,sizeof(state));
    Log::write(QString("PLAYBACK_TIMER_POLICY active=%1 success=%2 error=%3").arg(playing).arg(success).arg(success?0:GetLastError()));
}
FramelessWindow::~FramelessWindow() {
    if(qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND"))return;
    QSettings s;s.setValue("window/geometry",m_lastNormalGeometry);s.setValue("window/lockAspectRatio",m_lockAspect);
}
void FramelessWindow::closeEvent(QCloseEvent *event) {
    if(m_closeReady||!m_controller->ready()||m_controller->duration()==0) {
        QQuickWindow::closeEvent(event);return;
    }
    event->ignore();
    if(m_closeRequested)return;
    m_closeRequested=true;
    Log::write("CLOSE_REQUEST stop playback asynchronously before releasing the scene graph");
    m_controller->command({"stop"});
}
void FramelessWindow::prepareNativeFrame() {
    HWND hwnd=reinterpret_cast<HWND>(winId());
    m_nativeId=reinterpret_cast<quintptr>(hwnd);
    SetWindowLongPtr(hwnd,GWL_STYLE,GetWindowLongPtr(hwnd,GWL_STYLE)|WS_THICKFRAME|WS_CAPTION|WS_MAXIMIZEBOX|WS_MINIMIZEBOX|WS_SYSMENU);
    if(qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND"))
        SetWindowLongPtr(hwnd,GWL_EXSTYLE,GetWindowLongPtr(hwnd,GWL_EXSTYLE)|WS_EX_NOACTIVATE|WS_EX_TOOLWINDOW);
    BOOL dark=TRUE;DwmSetWindowAttribute(hwnd,20,&dark,sizeof(dark));
    const int rounded=2;DwmSetWindowAttribute(hwnd,33,&rounded,sizeof(rounded));
    MARGINS margin{1,1,1,1};DwmExtendFrameIntoClientArea(hwnd,&margin);
    SetWindowPos(hwnd,nullptr,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER|SWP_FRAMECHANGED|SWP_NOACTIVATE);
}
void FramelessWindow::setLockAspectRatio(bool enabled) {if(m_lockAspect==enabled)return;m_lockAspect=enabled;emit lockAspectRatioChanged();}
void FramelessWindow::setMaxHovered(bool v) {if(m_maxHovered==v)return;m_maxHovered=v;emit maxHoveredChanged();}
bool FramelessWindow::controlAt(const QPointF &p) const {
    return (p.y()<38&&p.x()>width()-138)||(p.x()>=16&&p.x()<=154&&p.y()>=height()-68&&p.y()<=height()-16);
}
void FramelessWindow::toggleMaximized() {visibility()==QWindow::Maximized?showNormal():showMaximized();}
void FramelessWindow::requestMinimize() {
    if(visibility()==QWindow::Minimized||m_minimizePending)return;
    m_resumeAfterMinimize=!m_controller->pause();
    m_minimizePending=true;
    if(m_controller->pause()) {m_minimizeSwaps=2;emit m_controller->renderUpdateRequested();update();}
    else m_controller->setPause(true);
}
void FramelessWindow::fitVideo(int w,int h) {
    if(w<=0||h<=0)return;
    m_aspect=double(w)/h;
    if(visibility()==QWindow::Maximized||visibility()==QWindow::FullScreen)return;
    const QRect work=screen()->availableGeometry();
    const double logicalWidth=w/devicePixelRatio(),logicalHeight=h/devicePixelRatio();
    const double fit=std::min({1.0,work.width()*.8/logicalWidth,work.height()*.8/logicalHeight});
    const double minimum=std::max(minimumWidth()/logicalWidth,minimumHeight()/logicalHeight);
    const double factor=std::max(fit,minimum);
    QSize size(qRound(logicalWidth*factor),qRound(logicalHeight*factor));
    setGeometry(QRect(work.center()-QPoint(size.width()/2,size.height()/2),size));
}
void FramelessWindow::openDialog() {
    IFileOpenDialog *dialog=nullptr;
    if(SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog)))) {
        const COMDLG_FILTERSPEC filters[]{{L"Media files",L"*.mkv;*.mp4;*.webm;*.avi;*.mov;*.m4v;*.ts;*.m2ts;*.mp3;*.flac"},{L"All files",L"*.*"}};
        dialog->SetFileTypes(2,filters);
        if(SUCCEEDED(dialog->Show(reinterpret_cast<HWND>(winId())))) {
            IShellItem *item=nullptr;
            if(SUCCEEDED(dialog->GetResult(&item))) {
                PWSTR name=nullptr;
                if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&name))) {m_controller->open(QString::fromWCharArray(name));CoTaskMemFree(name);}
                item->Release();
            }
        }
        dialog->Release();
    }
}
bool FramelessWindow::event(QEvent *e) {
    if((e->type()==QEvent::Move||e->type()==QEvent::Resize)&&visibility()==QWindow::Windowed&&m_nativeId&&!IsZoomed(reinterpret_cast<HWND>(m_nativeId)))m_lastNormalGeometry=geometry();
    if(e->type()==QEvent::KeyPress) {
        auto *key=static_cast<QKeyEvent*>(e);
        if(key->key()==Qt::Key_Space) {m_controller->togglePause();return true;}
        if(key->key()==Qt::Key_O&&key->modifiers().testFlag(Qt::ControlModifier)) {openDialog();return true;}
    }
    if(e->type()==QEvent::DragEnter||e->type()==QEvent::DragMove) {
        auto *drag=static_cast<QDragMoveEvent*>(e);if(drag->mimeData()->hasUrls()) {drag->acceptProposedAction();return true;}
    }
    if(e->type()==QEvent::Drop) {
        auto *drop=static_cast<QDropEvent*>(e);if(drop->mimeData()->hasUrls()) {m_controller->open(drop->mimeData()->urls().first().toString());drop->acceptProposedAction();return true;}
    }
    if(e->type()==QEvent::WindowStateChange) {
        if(visibility()==QWindow::Minimized) {
            m_wasMinimized=true;
            if(!m_minimizePrepared) {m_resumeAfterMinimize=!m_controller->pause();m_controller->setPause(true);}
            m_minimizePrepared=false;
        }
    }
    return QQuickWindow::event(e);
}
