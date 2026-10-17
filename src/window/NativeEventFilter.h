#pragma once
#include <QAbstractNativeEventFilter>
class FramelessWindow;
class NativeEventFilter final : public QAbstractNativeEventFilter {
public:
    explicit NativeEventFilter(FramelessWindow *window):m_window(window) {}
    bool nativeEventFilter(const QByteArray &,void *,qintptr *) override;
private:
    FramelessWindow *m_window;
};
