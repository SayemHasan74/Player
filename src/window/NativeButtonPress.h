#pragma once
// Capture pairs a non-client press with either a client or non-client release.
// Cancel/duplicate releases must never activate a window command.
class NativeButtonPress {
public:
    void press(){m_pressed=true;}
    bool pressed() const{return m_pressed;}
    bool release(bool inside){const bool activate=m_pressed&&inside;m_pressed=false;return activate;}
    void cancel(){m_pressed=false;}
private:
    bool m_pressed=false;
};
