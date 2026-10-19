#include "FramelessWindow.h"
#include "CompactResizePolicy.h"
#include "../core/MpvController.h"
#include "../core/Log.h"
#include "../core/AppSettings.h"
#include "../data/ResumeStore.h"
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
#include <QClipboard>
#include <QFileInfo>
#include <windows.h>
#include <dwmapi.h>
#include <shobjidl.h>
#include <algorithm>
FramelessWindow::FramelessWindow(MpvController *c):m_controller(c) {
    setTitle("FlowPlayer");setColor(QColor("#29292b"));
    setFlags(Qt::Window|Qt::FramelessWindowHint|Qt::WindowMinimizeButtonHint|Qt::WindowMaximizeButtonHint|Qt::WindowCloseButtonHint);
    if(qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND")) {
        setFlag(Qt::WindowDoesNotAcceptFocus);
        setFlag(Qt::WindowStaysOnBottomHint);
        setFlags((flags()&~Qt::WindowType_Mask)|Qt::Tool);
    }
    // Keep the render API attached while hidden/minimized; destruction still
    // releases the renderer on Qt's render thread before controller teardown.
    setPersistentGraphics(true);setPersistentSceneGraph(true);
    setMinimumSize(QSize(360,CompactResizePolicy::height));
    QSettings settings;m_lockAspect=settings.value("window/lockAspectRatio",true).toBool();
    const auto work=screen()->availableGeometry();
    auto geometry=settings.value("window/geometry",QRect(work.center()-QPoint(480,270),QSize(960,540))).toRect();
    if(!work.intersects(geometry))geometry.moveCenter(work.center());
    setGeometry(geometry);m_lastNormalGeometry=geometry;
    m_modes=WindowStateMachine(geometry);
    m_tinyPreview=settings.value("window/tinyPreview",false).toBool();
    if(c->settings()) {
        m_lockAspect=c->settings()->values().value("lockAspectRatio",true).toBool();
        m_tinyPreview=c->settings()->values().value("tinyPreview",false).toBool();
        connect(c->settings(),&AppSettings::valueChanged,this,[this](const QString &key,const QVariant &value){
            if(key=="lockAspectRatio")setLockAspectRatio(value.toBool());
            if(key=="tinyPreview")setTinyPreview(value.toBool());
        });
    }
    installEventFilter(this);
    m_cursorTimer.setSingleShot(true);m_cursorTimer.setInterval(2500);
    connect(&m_cursorTimer,&QTimer::timeout,this,[this]{
        if(!mini()&&!m_controller->pause()&&!m_uiHeld&&!m_nativeHover) {m_controlsVisible=false;setCursor(Qt::BlankCursor);emit controlsVisibilityChanged();}
    });
    connect(this,&QWindow::screenChanged,this,[this]{
        if(fullscreen()&&!m_applyingMode) {rememberModeGeometry();applyMode();}
    });
    connect(c,&MpvController::videoSizeChanged,this,&FramelessWindow::fitVideo);
    connect(c,&MpvController::playbackIdle,this,[this]{
        if(m_closeRequested) {m_closeReady=true;QTimer::singleShot(0,this,&QWindow::close);}
    });
    connect(c,&MpvController::stateChanged,this,[this]{
        updatePlaybackTiming();
        if(m_cursorPaused!=m_controller->pause()) {m_cursorPaused=m_controller->pause();m_controllerPaused=m_cursorPaused;pointerActivity();}
        if(m_minimizePending&&m_controller->pause()&&m_minimizeSwaps==0) {m_minimizeSwaps=2;emit m_controller->renderUpdateRequested();update();}
    });
    connect(this,&QWindow::visibilityChanged,this,[this]{updatePlaybackTiming();});
    connect(this,&QQuickWindow::frameSwapped,this,[this]{
        if(m_minimizePending&&m_minimizeSwaps>0) {
            if(--m_minimizeSwaps==0)commitMinimize();
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
    context->setContextProperty("preferences",m_controller->settings());
    context->setContextProperty("history",m_controller->resumeStore());
    QQmlComponent component(engine,QUrl("qrc:/ui/Main.qml"));
    auto *object=component.create();
    auto *root=qobject_cast<QQuickItem*>(object);
    if(!root) {Log::write("QML load failed: "+component.errorString());delete object;return false;}
    root->setParent(contentItem());
    root->setParentItem(contentItem());
    root->setSize(size());
    // Repeater delegates belong to the visual tree; their QObject ownership
    // does not have to descend from the title row.
    auto findButton=[](auto &&self,QQuickItem *item)->QQuickItem*{if(item->objectName()=="maximizeButton")return item;for(auto *child:item->childItems())if(auto *found=self(self,child))return found;return nullptr;};
    m_maximizeButton=findButton(findButton,root);
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
    if(m_closeReady||!m_controller->ready()||m_controller->duration()==0||m_controller->idleActive()) {
        QQuickWindow::closeEvent(event);return;
    }
    event->ignore();
    if(m_closeRequested)return;
    m_closeRequested=true;
    Log::write("CLOSE_REQUEST stop playback asynchronously before releasing the scene graph");
    m_controller->stop();
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
    const auto role=visit(visit,contentItem());
    // Home fills the scene behind the title overlay. Its fallback input role
    // must not turn the native caption drag area into a client control.
    return role=="home"&&point.y()<38?QString():role;
}
bool FramelessWindow::seekAt(const QPointF &point) const {return inputRoleAt(point)=="seek";}
bool FramelessWindow::maximizeButtonAt(const QPointF &point) const {
    return m_maximizeButton&&m_maximizeButton->isVisible()&&m_maximizeButton->isEnabled()&&m_maximizeButton->opacity()>0&&m_maximizeButton->contains(m_maximizeButton->mapFromScene(point));
}
QRectF FramelessWindow::maximizeButtonRect() const {return m_maximizeButton?m_maximizeButton->mapRectToScene(m_maximizeButton->boundingRect()):QRectF();}
bool FramelessWindow::titleAt(const QPointF &point) const {return controlsVisible()&&point.y()<38&&!controlAt(point);}
void FramelessWindow::rememberModeGeometry() {
    if(m_applyingMode||m_interactiveSizing)return;
    if(!mini()&&!fullscreen()&&m_nativeId)
        m_modes.setBaseMode(IsZoomed(reinterpret_cast<HWND>(m_nativeId))?WindowStateMachine::Mode::Maximized:WindowStateMachine::Mode::Windowed);
    m_modes.observe(geometry(),screen()->name(),devicePixelRatio());
    if(m_modes.current().mode==WindowStateMachine::Mode::Windowed)m_lastNormalGeometry=geometry();
}
void FramelessWindow::beginInteractiveResize(){rememberModeGeometry();m_resizeOrigin=m_modes.current();m_resizeOrigin.geometry=geometry();m_resizeEdge=0;m_interactiveSizing=true;}
void FramelessWindow::endInteractiveResize(){
    const auto resized=geometry();m_interactiveSizing=false;
    const bool vertical=m_resizeEdge==WMSZ_TOP||m_resizeEdge==WMSZ_BOTTOM||m_resizeEdge==WMSZ_TOPLEFT||m_resizeEdge==WMSZ_TOPRIGHT||m_resizeEdge==WMSZ_BOTTOMLEFT||m_resizeEdge==WMSZ_BOTTOMRIGHT;
    if(!vertical||fullscreen()){rememberModeGeometry();return;}
    if(CompactResizePolicy::collapse(mini(),resized.height())&&m_resizeOrigin.mode==WindowStateMachine::Mode::Windowed){
        m_modes.observe(m_resizeOrigin.geometry,m_resizeOrigin.screen,m_resizeOrigin.dpr);m_lastNormalGeometry=m_resizeOrigin.geometry;
        auto compact=resized;compact.setHeight(MiniHeight);compact.setWidth(std::max(520,compact.width()));
        m_modes.toggle(WindowStateMachine::Mode::Mini,screen()->geometry(),compact);m_modes.observe(compact,screen()->name(),devicePixelRatio());applyMode();
    }else if(CompactResizePolicy::expand(mini(),resized.height())){
        auto expanded=resized;
        if(m_lockAspect){const int h=std::max(expanded.height(),qRound(expanded.width()/m_aspect));expanded.setSize(QSize(qRound(h*m_aspect),h));if(m_resizeEdge==WMSZ_TOP||m_resizeEdge==WMSZ_TOPLEFT||m_resizeEdge==WMSZ_TOPRIGHT)expanded.moveBottom(resized.bottom());if(m_resizeEdge==WMSZ_TOPLEFT||m_resizeEdge==WMSZ_BOTTOMLEFT)expanded.moveRight(resized.right());}
        m_modes.resizeToWindowed(expanded);m_lastNormalGeometry=expanded;applyMode();
    }
    else if(mini()){auto compact=resized;compact.setHeight(MiniHeight);m_modes.observe(compact,screen()->name(),devicePixelRatio());applyMode();}
    else rememberModeGeometry();
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
    const QRect normal=m_modes.windowedGeometry();
    // Native move/resize notifications are synchronous during ShowWindow.
    // Never let the intermediate rectangle replace the saved restore bounds.
    m_applyingMode=true;
    m_modes.setBaseMode(maximize?WindowStateMachine::Mode::Maximized:WindowStateMachine::Mode::Windowed);
    if(maximize)showMaximized();else {showNormal();setGeometry(normal);m_lastNormalGeometry=normal;}
    m_applyingMode=false;
    m_modes.observe(geometry(),screen()->name(),devicePixelRatio());
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
    setMaximumHeight(16777215);setMinimumSize(mini()?QSize(520,MiniHeight):QSize(360,CompactResizePolicy::height));
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
    applyPin();m_applyingMode=false;updateControlTimer();
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
    setCursor(Qt::ArrowCursor);
    if(!m_controlsVisible){m_controlsVisible=true;emit controlsVisibilityChanged();}
    updateControlTimer();
}
void FramelessWindow::updateControlTimer(){m_cursorTimer.stop();if(m_controlsVisible&&!mini()&&!m_controller->pause()&&!m_uiHeld&&!m_nativeHover&&isVisible())m_cursorTimer.start();}
void FramelessWindow::setUiHeld(bool held) {if(m_uiHeld==held)return;m_uiHeld=held;updateControlTimer();}
void FramelessWindow::requestUrl() {emit urlRequested(QGuiApplication::clipboard()->text());}
bool FramelessWindow::eventFilter(QObject *watched,QEvent *e) {
    if(watched!=this)return false;
    if(e->type()==QEvent::MouseMove)pointerActivity();
    if(e->type()==QEvent::Leave){setCursor(Qt::ArrowCursor);m_cursorTimer.stop();}
    if(e->type()==QEvent::MouseButtonPress||e->type()==QEvent::MouseButtonRelease||e->type()==QEvent::MouseButtonDblClick) {
        auto *mouse=static_cast<QMouseEvent*>(e);
        if(mouse->button()==Qt::RightButton&&e->type()==QEvent::MouseButtonPress) {if(inputRoleAt(mouse->position())=="home")return false;emit contextMenuRequested(mouse->position());return true;}
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
        if(m_popupOpen)return false;
        auto *wheel=static_cast<QWheelEvent*>(e);
        const double steps=wheel->angleDelta().y()/120.0;
        if(steps!=0) {if(seekAt(wheel->position()))m_controller->seek(steps*m_controller->settings()->values().value("seekStep",5).toDouble());else m_controller->adjustVolume(steps*5);}
        return true;
    }
    if(e->type()==QEvent::KeyPress) {
        auto *key=static_cast<QKeyEvent*>(e);const auto mods=key->modifiers();
        const bool ctrl=mods.testFlag(Qt::ControlModifier),shift=mods.testFlag(Qt::ShiftModifier);
        // Escape is global, including menu and text-entry focus. Keep it ahead
        // of every focus guard; it must pause and minimize in one action.
        if(key->key()==Qt::Key_Escape){if(!key->isAutoRepeat()){emit dismissPopup();pauseAndMinimize();}return true;}
        if(activeFocusItem()&&activeFocusItem()->property("textEntry").toBool())return false;
        if(m_controller->resumeStore()&&m_controller->resumeStore()->homeVisible()&&!mini()&&
           (key->key()==Qt::Key_Return||key->key()==Qt::Key_Enter||key->key()==Qt::Key_Space||key->key()==Qt::Key_Up||key->key()==Qt::Key_Down||key->key()==Qt::Key_Left||key->key()==Qt::Key_Right))return false;
        if(m_popupOpen&&(key->key()==Qt::Key_Tab||key->key()==Qt::Key_Return||key->key()==Qt::Key_Enter||key->key()==Qt::Key_Up||key->key()==Qt::Key_Down||key->key()==Qt::Key_Left||key->key()==Qt::Key_Right))return false;
        if((key->nativeScanCode()&0xff)==0x29&&!ctrl&&!mods.testFlag(Qt::AltModifier)&&!mods.testFlag(Qt::MetaModifier)){if(!key->isAutoRepeat())toggleMini();return true;}
        switch(key->key()) {
        case Qt::Key_Space:if(!key->isAutoRepeat())m_controller->togglePause();break;
        case Qt::Key_L:if(!key->isAutoRepeat())emit playlistRequested();break;
        case Qt::Key_N:case Qt::Key_PageDown:case Qt::Key_MediaNext:if(!key->isAutoRepeat())m_controller->resumeStore()->navigate(1);break;
        case Qt::Key_P:case Qt::Key_PageUp:case Qt::Key_MediaPrevious:if(!key->isAutoRepeat())m_controller->resumeStore()->navigate(-1);break;
        case Qt::Key_MediaPlay:case Qt::Key_MediaTogglePlayPause:if(!key->isAutoRepeat())m_controller->togglePause();break;
        case Qt::Key_MediaPause:m_controller->setPause(true);break;
        case Qt::Key_MediaStop:m_controller->resumeStore()->showHome();break;
        case Qt::Key_F:if(!key->isAutoRepeat())toggleFullscreen();break;
        case Qt::Key_QuoteLeft:case Qt::Key_AsciiTilde:if(!key->isAutoRepeat())toggleMini();break;
        case Qt::Key_M:if(!key->isAutoRepeat()){if(ctrl)toggleMini();else m_controller->toggleMute();}break;
        case Qt::Key_T:if(!ctrl)return false;if(!key->isAutoRepeat())togglePinned();break;
        case Qt::Key_O:if(!ctrl)return false;openDialog();break;
        case Qt::Key_U:if(!ctrl)return false;requestUrl();break;
        case Qt::Key_S:if(!key->isAutoRepeat())m_controller->cycleTrack("sub");break;
        case Qt::Key_A:if(!key->isAutoRepeat())m_controller->cycleTrack("audio");break;
        case Qt::Key_Z:case Qt::Key_X:m_controller->setPlaybackOption("sub-delay",m_controller->playbackOptions().value("sub-delay",0).toDouble()+(key->key()==Qt::Key_Z?-.1:.1));break;
        case Qt::Key_Left:case Qt::Key_Right: {
            const auto values=m_controller->settings()->values();
            m_controller->seek((key->key()==Qt::Key_Left?-1:1)*(ctrl?values.value("largeSeekStep",30).toDouble():shift?values.value("exactSeekStep",1).toDouble():5.0),true);break;
        }
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
void FramelessWindow::requestMinimize(){minimize(false);}
void FramelessWindow::pauseAndMinimize(){minimize(true);}
void FramelessWindow::commitMinimize(){
    m_minimizePending=false;m_minimizeSwaps=0;
    // Background validation records the request without changing the desktop.
    if(qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND")){Log::write(QString("BACKGROUND_MINIMIZE_SUPPRESSED resumeOnRestore=%1").arg(m_resumeAfterMinimize));return;}
    m_minimizePrepared=true;setWindowStates(windowStates()|Qt::WindowMinimized);setVisible(true);
}
void FramelessWindow::minimize(bool stayPaused) {
    if(stayPaused){m_resumeAfterMinimize=false;m_controller->setPause(true);}
    if(visibility()==QWindow::Minimized||m_minimizePending)return;
    if(!stayPaused)m_resumeAfterMinimize=!m_controller->pause();
    if(!m_controller->ready()||m_controller->idleActive()||!isExposed()){commitMinimize();return;}
    m_minimizePending=true;
    if(m_controller->pause()) {m_minimizeSwaps=2;emit m_controller->renderUpdateRequested();update();}
    else m_controller->setPause(true);
}
void FramelessWindow::fitVideo(int w,int h) {
    if(w<=0||h<=0)return;
    const double previous=m_aspect;
    m_aspect=double(w)/h;
    if(m_hasFittedVideo&&std::abs(previous-m_aspect)<.01)return;
    m_hasFittedVideo=true;
    if(qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND"))return;
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
    openNativeDialog(false);
}
void FramelessWindow::openSubtitleDialog() {openNativeDialog(true);}
void FramelessWindow::openNativeDialog(bool subtitle) {
    setUiHeld(true);
    IFileOpenDialog *dialog=nullptr;
    if(SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dialog)))) {
        const COMDLG_FILTERSPEC filters[]{{L"Media files",L"*.mkv;*.mp4;*.webm;*.avi;*.mov;*.m4v;*.ts;*.m2ts;*.mp3;*.flac"},{L"All files",L"*.*"}};
        const COMDLG_FILTERSPEC subtitles[]{{L"Subtitle files",L"*.srt;*.ass;*.ssa;*.vtt;*.sup;*.sub;*.idx"},{L"All files",L"*.*"}};
        dialog->SetFileTypes(2,subtitle?subtitles:filters);
        if(!subtitle){DWORD options=0;dialog->GetOptions(&options);dialog->SetOptions(options|FOS_ALLOWMULTISELECT);}
        if(SUCCEEDED(dialog->Show(reinterpret_cast<HWND>(winId())))) {
            if(!subtitle){IShellItemArray *items=nullptr;if(SUCCEEDED(dialog->GetResults(&items))){DWORD count=0;items->GetCount(&count);QStringList paths;for(DWORD i=0;i<count;++i){IShellItem *entry=nullptr;if(SUCCEEDED(items->GetItemAt(i,&entry))){PWSTR name=nullptr;if(SUCCEEDED(entry->GetDisplayName(SIGDN_FILESYSPATH,&name))){paths.append(QString::fromWCharArray(name));CoTaskMemFree(name);}entry->Release();}}items->Release();m_controller->resumeStore()->openMany(paths);}}
            IShellItem *item=nullptr;
            if(subtitle&&SUCCEEDED(dialog->GetResult(&item))) {
                PWSTR name=nullptr;
                if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&name))) {const auto path=QString::fromWCharArray(name);if(subtitle)m_controller->addSubtitle(path);else m_controller->open(path);CoTaskMemFree(name);}
                item->Release();
            }
        }
        dialog->Release();
    }
    setUiHeld(false);
}
bool FramelessWindow::event(QEvent *e) {
    if((e->type()==QEvent::Move||e->type()==QEvent::Resize)&&m_nativeId&&!m_applyingMode)rememberModeGeometry();
    if(e->type()==QEvent::DragEnter||e->type()==QEvent::DragMove) {
        auto *drag=static_cast<QDragMoveEvent*>(e);if(drag->mimeData()->hasUrls()) {drag->acceptProposedAction();return true;}
    }
    if(e->type()==QEvent::Drop) {
        auto *drop=static_cast<QDropEvent*>(e);if(drop->mimeData()->hasUrls()) {
            QStringList media;
            for(const auto &url:drop->mimeData()->urls()) {
                const auto extension=QFileInfo(url.path()).suffix().toLower();
                if(QStringList{"srt","ass","ssa","vtt","sup","sub","idx"}.contains(extension))m_controller->addSubtitle(url.toString());
                else media.append(url.isLocalFile()?url.toLocalFile():url.toString());
            }
            if(!media.isEmpty())m_controller->resumeStore()->openMany(media);
            drop->acceptProposedAction();return true;
        }
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
