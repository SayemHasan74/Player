#pragma once
#include <QQuickWindow>
#include <QSettings>
class MpvController;
class FramelessWindow final : public QQuickWindow {
    Q_OBJECT
    Q_PROPERTY(bool lockAspectRatio READ lockAspectRatio WRITE setLockAspectRatio NOTIFY lockAspectRatioChanged)
    Q_PROPERTY(bool maxHovered READ maxHovered NOTIFY maxHoveredChanged)
public:
    explicit FramelessWindow(MpvController *controller);
    ~FramelessWindow() override;
    void prepareNativeFrame();
    bool initializeUi(QObject *diagnostics,bool enableVideo);
    bool lockAspectRatio() const {return m_lockAspect;}
    void setLockAspectRatio(bool enabled);
    bool maxHovered() const {return m_maxHovered;}
    void setMaxHovered(bool value);
    void fitVideo(int width,int height);
    Q_INVOKABLE void toggleMaximized();
    Q_INVOKABLE void requestMinimize();
    Q_INVOKABLE void openDialog();
    double videoAspect() const {return m_aspect;}
    quintptr nativeId() const {return m_nativeId;}
    bool controlAt(const QPointF &point) const;
signals:
    void lockAspectRatioChanged();
    void maxHoveredChanged();
protected:
    bool event(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private:
    MpvController *m_controller;
    bool m_lockAspect=true,m_maxHovered=false,m_resumeAfterMinimize=false;
    bool m_minimizePending=false,m_minimizePrepared=false,m_wasMinimized=false;
    int m_minimizeSwaps=0;
    bool m_closeRequested=false,m_closeReady=false;
    bool m_precisePlaybackTiming=false;
    void updatePlaybackTiming();
    double m_aspect=16.0/9.0;
    QRect m_lastNormalGeometry;
    quintptr m_nativeId=0;
};
