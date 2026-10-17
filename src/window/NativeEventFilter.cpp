#include "NativeEventFilter.h"
#include "FramelessWindow.h"
#include <windows.h>
#include <windowsx.h>
#include <cmath>
#include <algorithm>
bool NativeEventFilter::nativeEventFilter(const QByteArray &type,void *opaque,qintptr *result) {
    // Dispatcher notifications may have a null result pointer and are seen
    // again when Windows actually dispatches them to the target window.
    if(type!="windows_generic_MSG"||!result)return false;
    auto *msg=static_cast<MSG*>(opaque);
    if(!m_window->nativeId()||msg->hwnd!=reinterpret_cast<HWND>(m_window->nativeId()))return false;
    switch(msg->message) {
    case WM_SYSCOMMAND:if((msg->wParam&0xFFF0)==SC_MINIMIZE){m_window->requestMinimize();*result=0;return true;}break;
    case WM_NCCALCSIZE:*result=0;return true;
    case WM_ERASEBKGND:*result=1;return true;
    case WM_GETMINMAXINFO: {
        auto *info=reinterpret_cast<MINMAXINFO*>(msg->lParam);
        MONITORINFO monitor{sizeof(monitor)};GetMonitorInfo(MonitorFromWindow(msg->hwnd,MONITOR_DEFAULTTONEAREST),&monitor);
        info->ptMaxPosition={monitor.rcWork.left-monitor.rcMonitor.left,monitor.rcWork.top-monitor.rcMonitor.top};
        info->ptMaxSize={monitor.rcWork.right-monitor.rcWork.left,monitor.rcWork.bottom-monitor.rcWork.top};
        const auto dpr=m_window->devicePixelRatio();
        double minWidth=m_window->minimumWidth(),minHeight=m_window->minimumHeight();
        if(m_window->lockAspectRatio()) {
            minWidth=std::max(minWidth,minHeight*m_window->videoAspect());
            minHeight=std::max(minHeight,minWidth/m_window->videoAspect());
        }
        info->ptMinTrackSize={qRound(minWidth*dpr),qRound(minHeight*dpr)};
        *result=0;return true;
    }
    case WM_NCHITTEST: {
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
        if(logical.y()<38&&logical.x()>=m_window->width()-92&&logical.x()<m_window->width()-46) {*result=HTMAXBUTTON;return true;}
        *result=m_window->controlAt(logical)?HTCLIENT:HTCAPTION;return true;
    }
    case WM_NCMOUSEMOVE: {
        m_window->setMaxHovered(msg->wParam==HTMAXBUTTON);
        TRACKMOUSEEVENT track{sizeof(track),TME_LEAVE|TME_NONCLIENT,msg->hwnd,0};TrackMouseEvent(&track);break;
    }
    case WM_NCMOUSELEAVE:m_window->setMaxHovered(false);break;
    case WM_NCLBUTTONDOWN:if(msg->wParam==HTMAXBUTTON){*result=0;return true;}break;
    case WM_NCLBUTTONUP:if(msg->wParam==HTMAXBUTTON){m_window->toggleMaximized();*result=0;return true;}break;
    case WM_SIZING: {
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
