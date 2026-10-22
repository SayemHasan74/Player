#include "NativeEventFilter.h"
#include "FramelessWindow.h"
#include "CompactResizePolicy.h"
#include <windows.h>
#include <windowsx.h>
#include <cmath>
#include <algorithm>
#include <QCoreApplication>
#include <QMouseEvent>
void NativeEventFilter::forwardMaximizeMouse(unsigned int message,const QPoint &position){
    // Let Qt own the same press/grab/release path as every other QML button.
    // The former independent native capture could lose the click before QML
    // ever received a press. Do not also invoke toggleMaximized here.
    const bool down=message==WM_LBUTTONDOWN||message==WM_LBUTTONDBLCLK;
    if(qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND")){
        // Isolated diagnostics exercise QML delivery without taking OS capture
        // or sending input to the desktop while the user works.
        const QPointF local=QPointF(position)/m_window->devicePixelRatio();
        QMouseEvent event(message==WM_MOUSEMOVE?QEvent::MouseMove:message==WM_LBUTTONUP?QEvent::MouseButtonRelease:message==WM_LBUTTONDBLCLK?QEvent::MouseButtonDblClick:QEvent::MouseButtonPress,local,m_window->mapToGlobal(local),message==WM_MOUSEMOVE?Qt::NoButton:Qt::LeftButton,down?Qt::LeftButton:Qt::NoButton,Qt::NoModifier);
        QCoreApplication::sendEvent(m_window,&event);
    }else{
        const WPARAM keys=(down?MK_LBUTTON:0)|((GetKeyState(VK_SHIFT)&0x8000)?MK_SHIFT:0)|((GetKeyState(VK_CONTROL)&0x8000)?MK_CONTROL:0);
        PostMessageW(reinterpret_cast<HWND>(m_window->nativeId()),message,keys,MAKELPARAM(position.x(),position.y()));
    }
}
bool NativeEventFilter::nativeEventFilter(const QByteArray &type,void *opaque,qintptr *result) {
    // Dispatcher notifications may have a null result pointer and are seen
    // again when Windows actually dispatches them to the target window.
    if(type!="windows_generic_MSG"||!result)return false;
    auto *msg=static_cast<MSG*>(opaque);
    if(!m_window->nativeId()||msg->hwnd!=reinterpret_cast<HWND>(m_window->nativeId()))return false;
    switch(msg->message) {
    case WM_ENTERSIZEMOVE:m_window->beginInteractiveResize();break;
    case WM_EXITSIZEMOVE:m_window->endInteractiveResize();break;
    case WM_NCRBUTTONDOWN:case WM_CONTEXTMENU: {
        POINT point{GET_X_LPARAM(msg->lParam),GET_Y_LPARAM(msg->lParam)};
        if(msg->lParam==-1) {point.x=m_window->width()/2;point.y=m_window->height()/2;}
        else {ScreenToClient(msg->hwnd,&point);point.x=qRound(point.x/m_window->devicePixelRatio());point.y=qRound(point.y/m_window->devicePixelRatio());}
        m_window->contextMenuRequested(QPointF(point.x,point.y));*result=0;return true;
    }
    case WM_NCRBUTTONUP:*result=0;return true;
    case WM_NCMBUTTONDOWN:case WM_NCMBUTTONDBLCLK:m_window->toggleFullscreen();*result=0;return true;
    case WM_NCMBUTTONUP:*result=0;return true;
    case WM_NCLBUTTONDBLCLK:
        if(msg->wParam==HTCAPTION){m_window->toggleMaximized();*result=0;return true;}
        if(msg->wParam==HTMAXBUTTON){POINT p{GET_X_LPARAM(msg->lParam),GET_Y_LPARAM(msg->lParam)};ScreenToClient(msg->hwnd,&p);forwardMaximizeMouse(WM_LBUTTONDBLCLK,QPoint(p.x,p.y));*result=0;return true;}break;
    case WM_SYSCOMMAND:if((msg->wParam&0xFFF0)==SC_MINIMIZE){m_window->requestMinimize();*result=0;return true;}break;
    case WM_NCCALCSIZE:*result=0;return true;
    case WM_ERASEBKGND:*result=1;return true;
    case WM_GETMINMAXINFO: {
        if(m_window->fullscreen())break;
        auto *info=reinterpret_cast<MINMAXINFO*>(msg->lParam);
        MONITORINFO monitor{sizeof(monitor)};GetMonitorInfo(MonitorFromWindow(msg->hwnd,MONITOR_DEFAULTTONEAREST),&monitor);
        info->ptMaxPosition={monitor.rcWork.left-monitor.rcMonitor.left,monitor.rcWork.top-monitor.rcMonitor.top};
        info->ptMaxSize={monitor.rcWork.right-monitor.rcWork.left,monitor.rcWork.bottom-monitor.rcWork.top};
        const auto dpr=m_window->devicePixelRatio();
        double minWidth=m_window->minimumWidth(),minHeight=m_window->minimumHeight();
        if(m_window->lockAspectRatio()&&!m_window->mini()) {
            minWidth=std::max(minWidth,minHeight*m_window->videoAspect());
            // Vertical edges may collapse into compact mode; do not enforce
            // an aspect-derived minimum height that prevents the threshold.
        }
        info->ptMinTrackSize={qRound(minWidth*dpr),qRound(minHeight*dpr)};
        *result=0;return true;
    }
    case WM_NCHITTEST: {
        if(m_window->fullscreen()||m_window->popupOpen()){*result=HTCLIENT;return true;}
        POINT p{GET_X_LPARAM(msg->lParam),GET_Y_LPARAM(msg->lParam)};ScreenToClient(msg->hwnd,&p);
        RECT rect;GetClientRect(msg->hwnd,&rect);
        const auto dpr=m_window->devicePixelRatio();const int edge=qRound(6*dpr);
        if(!IsZoomed(msg->hwnd)) {
            bool l=p.x<edge,r=p.x>=rect.right-edge,t=p.y<edge,b=p.y>=rect.bottom-edge;
            if(t||b||l||r) {
                *result=t?(l?HTTOPLEFT:r?HTTOPRIGHT:HTTOP):b?(l?HTBOTTOMLEFT:r?HTBOTTOMRIGHT:HTBOTTOM):l?HTLEFT:HTRIGHT;
                return true;
            }
        }
        const QPointF logical(p.x/dpr,p.y/dpr);
        if(m_window->controlsVisible()&&!m_window->mini()&&m_window->maximizeButtonAt(logical)) {*result=HTMAXBUTTON;return true;}
        *result=m_window->titleAt(logical)?HTCAPTION:HTCLIENT;return true;
    }
    case WM_NCMOUSEMOVE: {
        m_window->setNativeHover(true);
        m_window->setMaxHovered(msg->wParam==HTMAXBUTTON);
        if(msg->wParam==HTMAXBUTTON){POINT p{GET_X_LPARAM(msg->lParam),GET_Y_LPARAM(msg->lParam)};ScreenToClient(msg->hwnd,&p);forwardMaximizeMouse(WM_MOUSEMOVE,QPoint(p.x,p.y));}
        TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE|TME_NONCLIENT,msg->hwnd,0};TrackMouseEvent(&track);break;
    }
    case WM_NCMOUSELEAVE:m_window->setNativeHover(false);m_window->setMaxHovered(false);break;
    case WM_NCLBUTTONDOWN:case WM_NCLBUTTONUP:
        if(msg->wParam==HTMAXBUTTON){POINT p{GET_X_LPARAM(msg->lParam),GET_Y_LPARAM(msg->lParam)};ScreenToClient(msg->hwnd,&p);forwardMaximizeMouse(msg->message==WM_NCLBUTTONDOWN?WM_LBUTTONDOWN:WM_LBUTTONUP,QPoint(p.x,p.y));*result=0;return true;}break;
    case WM_SIZING: {
        m_window->noteInteractiveResize(int(msg->wParam));
        if(m_window->mini())break;
        if(m_window->fullscreen())break;
        auto *proposed=reinterpret_cast<RECT*>(msg->lParam);
        if((msg->wParam==WMSZ_TOP||msg->wParam==WMSZ_BOTTOM)&&CompactResizePolicy::collapse(false,qRound((proposed->bottom-proposed->top)/m_window->devicePixelRatio())))break;
        if(!m_window->lockAspectRatio())break;
        auto *r=reinterpret_cast<RECT*>(msg->lParam);const double a=m_window->videoAspect();
        const int w=r->right-r->left,h=r->bottom-r->top;
        if(msg->wParam==WMSZ_TOP||msg->wParam==WMSZ_BOTTOM)r->right=r->left+LONG(std::lround(h*a));
        else if(msg->wParam==WMSZ_TOPLEFT||msg->wParam==WMSZ_TOPRIGHT)r->top=r->bottom-LONG(std::lround(w/a));
        else r->bottom=r->top+LONG(std::lround(w/a));
        *result=TRUE;return true;
    }
    }
    return false;
}
