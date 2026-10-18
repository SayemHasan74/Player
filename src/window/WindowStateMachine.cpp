#include "WindowStateMachine.h"
void WindowStateMachine::observe(QRect geometry,QString screen,qreal dpr) {
    m_current.geometry=geometry;m_current.screen=std::move(screen);m_current.dpr=dpr;
    if(m_current.mode==Mode::Windowed)m_windowed=geometry;
    if(m_current.mode==Mode::Mini)m_mini=geometry;
}
void WindowStateMachine::setBaseMode(Mode mode) {
    if((mode==Mode::Windowed||mode==Mode::Maximized)&&
       (m_current.mode==Mode::Windowed||m_current.mode==Mode::Maximized))m_current.mode=mode;
}
WindowStateMachine::Snapshot WindowStateMachine::toggle(Mode target,QRect fullscreen,QRect firstMini) {
    if(target!=Mode::Fullscreen&&target!=Mode::Mini)return m_current;
    if(m_current.mode==target) {
        if(!m_history.empty()) {m_current=m_history.back();m_history.pop_back();}
        return m_current;
    }
    m_history.push_back(m_current);
    m_current.mode=target;
    m_current.geometry=target==Mode::Fullscreen?fullscreen:(m_mini.isValid()?m_mini:firstMini);
    return m_current;
}
WindowStateMachine::Snapshot WindowStateMachine::exitFullscreen() {
    if(m_current.mode==Mode::Fullscreen&&!m_history.empty()) {m_current=m_history.back();m_history.pop_back();}
    return m_current;
}
