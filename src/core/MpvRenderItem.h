#pragma once
#include <QQuickFramebufferObject>
#include "MpvController.h"
class MpvRenderItem : public QQuickFramebufferObject {
    Q_OBJECT
    Q_PROPERTY(MpvController* controller READ controller WRITE setController NOTIFY controllerChanged)
public:
    inline static std::atomic<quint64> renderedFrames{0},updateCallbacks{0};
    inline static std::atomic<qint64> callbackAtUs{0},callbackDelayMaxUs{0},renderCallMaxUs{0};
    inline static bool diagnosticsEnabled=false;
    explicit MpvRenderItem(QQuickItem *parent=nullptr);
    MpvController *controller() const {return m_controller;}
    void setController(MpvController *controller);
    Renderer *createRenderer() const override;
    Q_INVOKABLE void scheduleFrame();
signals:
    void controllerChanged();
    void firstFrame();
private:
    MpvController *m_controller=nullptr;
};
