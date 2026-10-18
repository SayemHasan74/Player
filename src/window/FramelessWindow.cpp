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
#include <QMouseEvent>
#include <QWheelEvent>
#include <QGuiApplication>
#include <QStyleHints>
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
    m_modes=WindowStateMachine(geometry);
    m_tinyPreview=settings.value("window/tinyPreview",false).toBool();
    installEventFilter(this);
    m_cursorTimer.setSingleShot(true);m_cursorTimer.setInterval(2500);
    connect(&m_cursorTimer,&QTimer::timeout,this,[this]{if(!mini()&&!m_controller->pause())setCursor(Qt::BlankCursor);});
    connect(this,&QWindow::screenChanged,this,[this]{
        if(fullscreen()&&!m_applyingMode) {rememberModeGeometry();applyMode();}
    });
    connect(c,&MpvController::videoSizeChanged,this,&FramelessWindow::fitVideo);
    connect(c,&MpvController::playbackIdle,this,[this]{
        if(m_closeRequested) {m_closeReady=true;QTimer::singleShot(0,this,&QWindow::close);}
    });
    connect(c,&MpvController::stateChanged,this,[this]{
        updatePlaybackTiming();
        if(m_cursorPaused!=m_controller->pause()) {m_cursorPaused=m_controller->pause();pointerActivity();}
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
    s.setValue("window/tinyPreview",m_tinyPreview);
    if(m_modes.miniGeometry().isValid())s.setValue("window/miniGeometry",m_modes.miniGeometry());
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
    return !inputRoleAt(p).isEmpty();
}
QString FramelessWindow::inputRoleAt(const QPointF &point) const {
    auto visit=[&](auto &&self,QQuickItem *item)->QString {
        if(!item->isVisible()||!item->isEnabled()||item->opacity()==0)return {};
        const QPointF local=item->mapFromScene(point);
        if(!item->contains(local))return {};
        const auto children=item->childItems();
        for(auto i=children.crbegin();i!=children.crend();++i) {auto role=self(self,*i);if(!role.isEmpty())return role;}
        return item->property("inputRole").toString();
    };
    return visit(visit,contentItem());
}
bool FramelessWindow::seekAt(const QPointF &point) const {return inputRoleAt(point)=="seek";}
bool FramelessWindow::titleAt(const QPointF &point) const {return point.y()<38&&!controlAt(point);}
void FramelessWindow::rememberModeGeometry() {
    if(m_applyingMode)return;
    if(!mini()&&!fullscreen()&&m_nativeId)
        m_modes.setBaseMode(IsZoomed(reinterpret_cast<HWND>(m_nativeId))?WindowStateMachine::Mode::Maximized:WindowStateMachine::Mode::Windowed);
    m_modes.observe(geometry(),screen()->name(),devicePixelRatio());
    if(m_modes.current().mode==WindowStateMachine::Mode::Windowed)m_lastNormalGeometry=geometry();
}
void FramelessWindow::toggleMode(WindowStateMachine::Mode target) {
    rememberModeGeometry();
    const auto work=screen()->availableGeometry();
    QRect firstMini(work.right()-560-20,work.bottom()-MiniHeight-20,560,MiniHeight);
    if(!m_modes.miniGeometry().isValid()) {
        QSettings settings;const auto saved=settings.value("window/miniGeometry").toRect();
        if(saved.isValid())firstMini=saved;
    }
    m_modes.toggle(target,screen()->geometry(),firstMini);applyMode();
}
void FramelessWindow::toggleFullscreen() {toggleMode(WindowStateMachine::Mode::Fullscreen);}
void FramelessWindow::exitFullscreen() {if(fullscreen()){m_modes.exitFullscreen();applyMode();}}
void FramelessWindow::toggleMini() {toggleMode(WindowStateMachine::Mode::Mini);}
void FramelessWindow::toggleMaximized() {
    if(fullscreen()){exitFullscreen();return;}
    if(mini()){toggleMini();return;}
    rememberModeGeometry();
    const bool maximize=m_modes.current().mode!=WindowStateMachine::Mode::Maximized;
    m_modes.setBaseMode(maximize?WindowStateMachine::Mode::Maximized:WindowStateMachine::Mode::Windowed);
    if(maximize)showMaximized();else {showNormal();setGeometry(m_modes.windowedGeometry());}
    emit modeChanged();
}
void FramelessWindow::applyPin() {
    if(!m_nativeId)return;
    const bool background=qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND");
    const HWND order=background?HWND_BOTTOM:((mini()||m_pinned)?HWND_TOPMOST:HWND_NOTOPMOST);
    SetWindowPos(reinterpret_cast<HWND>(m_nativeId),order,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);
}
void FramelessWindow::applyMode() {
    m_applyingMode=true;
    const auto oldId=winId();
    setMaximumHeight(16777215);setMinimumSize(mini()?QSize(520,MiniHeight):QSize(320,180));
    if(mini())setMaximumHeight(MiniHeight);
    emit modeChanged();
    if(fullscreen()) {
        for(auto *candidate:QGuiApplication::screens()) {
            if(candidate->name()==m_modes.current().screen&&candidate!=screen()) {setGeometry(candidate->geometry());break;}
        }
        showFullScreen();
        // Qt maintains the same HWND and scene; use its current monitor's full
        // physical rectangle, including taskbar, without changing display mode.
        MONITORINFO monitor{sizeof(monitor)};
        if(GetMonitorInfo(MonitorFromWindow(reinterpret_cast<HWND>(winId()),MONITOR_DEFAULTTONEAREST),&monitor))
            SetWindowPos(reinterpret_cast<HWND>(winId()),qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND")?HWND_BOTTOM:HWND_TOP,
                         monitor.rcMonitor.left,monitor.rcMonitor.top,monitor.rcMonitor.right-monitor.rcMonitor.left,
                         monitor.rcMonitor.bottom-monitor.rcMonitor.top,SWP_NOACTIVATE|SWP_NOOWNERZORDER);
    } else {
        QRect desired=m_modes.current().geometry;
        QScreen *destination=nullptr;
        for(auto *candidate:QGuiApplication::screens())if(candidate->name()==m_modes.current().screen)destination=candidate;
        if(!destination)destination=screen();
        const auto work=destination->availableGeometry();
        if(mini()) {desired.setHeight(MiniHeight);desired.setWidth(std::max(520,desired.width()));}
        if(!work.contains(desired)) {
            desired.setSize(desired.size().boundedTo(work.size()));
            desired.moveLeft(std::clamp(desired.left(),work.left(),work.right()-desired.width()+1));
            desired.moveTop(std::clamp(desired.top(),work.top(),work.bottom()-desired.height()+1));
        }
        showNormal();
        if(m_modes.current().mode==WindowStateMachine::Mode::Maximized) {setGeometry(m_modes.windowedGeometry());showMaximized();}
        else setGeometry(desired);
    }
    applyPin();m_applyingMode=false;pointerActivity();
    emit m_controller->renderUpdateRequested();update();
    Log::write(QString("WINDOW_MODE mode=%1 previous=%2 geometry=%3,%4,%5,%6 hwndUnchanged=%7 depth=%8")
        .arg(int(m_modes.current().mode)).arg(m_modes.previous()?int(m_modes.previous()->mode):-1)
        .arg(x()).arg(y()).arg(width()).arg(height()).arg(oldId==winId()).arg(m_modes.depth()));
}
void FramelessWindow::togglePinned() {m_pinned=!m_pinned;applyPin();emit modeChanged();}
void FramelessWindow::toggleInfo() {m_infoVisible=!m_infoVisible;emit modeChanged();}
void FramelessWindow::setTinyPreview(bool enabled) {
    if(m_tinyPreview==enabled)return;m_tinyPreview=enabled;
    emit modeChanged();
}
void FramelessWindow::setDiagnosticVideoHidden(bool hidden) {
    if(!qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND"))return;
    m_diagnosticVideoHidden=hidden;emit modeChanged();
    Log::write(QString("MINI_VIDEO_COVER hidden=%1 track=%2 position=%3").arg(hidden).arg(m_controller->videoTrack()).arg(m_controller->position()));
}
void FramelessWindow::pointerActivity() {
    setCursor(Qt::ArrowCursor);m_cursorTimer.stop();
    if(!mini()&&!m_controller->pause()&&isVisible())m_cursorTimer.start();
}
bool FramelessWindow::eventFilter(QObject *watched,QEvent *e) {
    if(watched!=this)return false;
    if(e->type()==QEvent::MouseMove||e->type()==QEvent::Enter)pointerActivity();
    if(e->type()==QEvent::Leave){setCursor(Qt::ArrowCursor);m_cursorTimer.stop();}
    if(e->type()==QEvent::MouseButtonPress||e->type()==QEvent::MouseButtonRelease||e->type()==QEvent::MouseButtonDblClick) {
        auto *mouse=static_cast<QMouseEvent*>(e);pointerActivity();
        if(mouse->button()==Qt::MiddleButton) {
            if(e->type()!=QEvent::MouseButtonRelease)toggleFullscreen();
            return true;
        }
        if(mouse->button()==Qt::LeftButton) {
            if(e->type()==QEvent::MouseButtonDblClick) {
                m_dragCandidate=false;
                if(!controlAt(mouse->position())) {if(titleAt(mouse->position()))toggleMaximized();else m_controller->togglePause();return true;}
            }
            if(e->type()==QEvent::MouseButtonPress) {m_dragCandidate=!controlAt(mouse->position())&&!fullscreen();m_pressPosition=mouse->position();}
            if(e->type()==QEvent::MouseButtonRelease)m_dragCandidate=false;
        }
    }
    if(e->type()==QEvent::MouseMove&&m_dragCandidate) {
        auto *mouse=static_cast<QMouseEvent*>(e);
        if((mouse->position()-m_pressPosition).manhattanLength()>=QGuiApplication::styleHints()->startDragDistance()) {m_dragCandidate=false;startSystemMove();return true;}
    }
    if(e->type()==QEvent::Wheel) {
        auto *wheel=static_cast<QWheelEvent*>(e);pointerActivity();
        const double steps=wheel->angleDelta().y()/120.0;
        if(steps!=0) {if(seekAt(wheel->position()))m_controller->seek(steps*5);else m_controller->adjustVolume(steps*5);}
        return true;
    }
    if(e->type()==QEvent::KeyPress) {
        auto *key=static_cast<QKeyEvent*>(e);const auto mods=key->modifiers();
        const bool ctrl=mods.testFlag(Qt::ControlModifier),shift=mods.testFlag(Qt::ShiftModifier);
        pointerActivity();
        switch(key->key()) {
        case Qt::Key_Space:if(!key->isAutoRepeat())m_controller->togglePause();break;
        case Qt::Key_F:if(!key->isAutoRepeat())toggleFullscreen();break;
        case Qt::Key_Escape:exitFullscreen();break;
        case Qt::Key_M:if(!key->isAutoRepeat()){if(ctrl)toggleMini();else m_controller->toggleMute();}break;
        case Qt::Key_T:if(!ctrl)return false;if(!key->isAutoRepeat())togglePinned();break;
        case Qt::Key_O:if(!ctrl)return false;openDialog();break;
        case Qt::Key_Left:case Qt::Key_Right:m_controller->seek((key->key()==Qt::Key_Left?-1:1)*(ctrl?30:shift?1:5),shift);break;
        case Qt::Key_Up:case Qt::Key_Down:m_controller->adjustVolume(key->key()==Qt::Key_Up?5:-5);break;
        case Qt::Key_BracketLeft:case Qt::Key_BracketRight:m_controller->adjustSpeed(key->key()==Qt::Key_BracketLeft?-.1:.1);break;
        case Qt::Key_Backspace:m_controller->resetSpeed();break;
        case Qt::Key_Comma:case Qt::Key_Period:m_controller->frameStep(key->key()==Qt::Key_Comma);break;
        case Qt::Key_I:if(!key->isAutoRepeat())toggleInfo();break;
        default:return false;
        }
        return true;
    }
    return false;
}
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
    if(mini()||m_modes.current().mode==WindowStateMachine::Mode::Maximized||fullscreen())return;
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
    if((e->type()==QEvent::Move||e->type()==QEvent::Resize)&&m_nativeId&&!m_applyingMode)rememberModeGeometry();
    if(e->type()==QEvent::DragEnter||e->type()==QEvent::DragMove) {
        auto *drag=static_cast<QDragMoveEvent*>(e);if(drag->mimeData()->hasUrls()) {drag->acceptProposedAction();return true;}
    }
    if(e->type()==QEvent::Drop) {
        auto *drop=static_cast<QDropEvent*>(e);if(drop->mimeData()->hasUrls()) {m_controller->open(drop->mimeData()->urls().first().toString());drop->acceptProposedAction();return true;}
    }
    if(e->type()==QEvent::WindowStateChange) {
        if(!m_applyingMode&&!fullscreen()&&!mini()&&visibility()!=QWindow::Minimized) {
            m_modes.setBaseMode(IsZoomed(reinterpret_cast<HWND>(m_nativeId))?WindowStateMachine::Mode::Maximized:WindowStateMachine::Mode::Windowed);emit modeChanged();
        }
        if(visibility()==QWindow::Minimized) {
            m_wasMinimized=true;
            if(!m_minimizePrepared) {m_resumeAfterMinimize=!m_controller->pause();m_controller->setPause(true);}
            m_minimizePrepared=false;
        }
    }
    return QQuickWindow::event(e);
}
