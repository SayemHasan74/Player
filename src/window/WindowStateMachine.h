#pragma once
#include <QRect>
#include <QString>
#include <vector>

// Pure transition model: no native windows, rendering, engine or timers.
class WindowStateMachine {
public:
    enum class Mode { Windowed, Maximized, Fullscreen, Mini };
    struct Snapshot {
        Mode mode=Mode::Windowed;
        QRect geometry;
        QString screen;
        qreal dpr=1;
        bool operator==(const Snapshot &) const = default;
    };
    explicit WindowStateMachine(QRect windowed={}):m_current{Mode::Windowed,windowed,{},1},m_windowed(windowed) {}
    const Snapshot &current() const {return m_current;}
    const Snapshot *previous() const {return m_history.empty()?nullptr:&m_history.back();}
    const QRect &windowedGeometry() const {return m_windowed;}
    const QRect &miniGeometry() const {return m_mini;}
    size_t depth() const {return m_history.size();}
    void observe(QRect geometry,QString screen={},qreal dpr=1);
    void setBaseMode(Mode mode);
    Snapshot toggle(Mode target,QRect fullscreen,QRect firstMini);
    Snapshot exitFullscreen();
    void resizeToWindowed(QRect geometry);
private:
    Snapshot m_current;
    QRect m_windowed,m_mini;
    std::vector<Snapshot> m_history;
};
