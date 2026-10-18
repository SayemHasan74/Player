#pragma once
#include <QQuickWindow>
#include <QSettings>
#include <QTimer>
#include "WindowStateMachine.h"
class MpvController;
class FramelessWindow final : public QQuickWindow {
    Q_OBJECT
    Q_PROPERTY(bool lockAspectRatio READ lockAspectRatio WRITE setLockAspectRatio NOTIFY lockAspectRatioChanged)
    Q_PROPERTY(bool maxHovered READ maxHovered NOTIFY maxHoveredChanged)
    Q_PROPERTY(bool mini READ mini NOTIFY modeChanged)
    Q_PROPERTY(bool fullscreen READ fullscreen NOTIFY modeChanged)
    Q_PROPERTY(bool pinned READ pinned NOTIFY modeChanged)
    Q_PROPERTY(bool infoVisible READ infoVisible NOTIFY modeChanged)
    Q_PROPERTY(bool tinyPreview READ tinyPreview WRITE setTinyPreview NOTIFY modeChanged)
    Q_PROPERTY(bool videoHidden READ videoHidden NOTIFY modeChanged)
    Q_PROPERTY(bool controlsVisible READ controlsVisible NOTIFY controlsVisibilityChanged)
public:
    static constexpr int MiniHeight=160;
    explicit FramelessWindow(MpvController *controller);
    ~FramelessWindow() override;
    void prepareNativeFrame();
    bool initializeUi(QObject *diagnostics,bool enableVideo);
    bool lockAspectRatio() const {return m_lockAspect;}
    void setLockAspectRatio(bool enabled);
    bool maxHovered() const {return m_maxHovered;}
    void setMaxHovered(bool value);
    void fitVideo(int width,int height);
    void beginInteractiveResize();
    void noteInteractiveResize(int edge) {m_resizeEdge=edge;}
    void endInteractiveResize();
    Q_INVOKABLE void toggleMaximized();
    Q_INVOKABLE void toggleFullscreen();
    Q_INVOKABLE void exitFullscreen();
    Q_INVOKABLE void toggleMini();
    Q_INVOKABLE void togglePinned();
    Q_INVOKABLE void toggleInfo();
    bool mini() const {return m_modes.current().mode==WindowStateMachine::Mode::Mini;}
    bool fullscreen() const {return m_modes.current().mode==WindowStateMachine::Mode::Fullscreen;}
    bool pinned() const {return m_pinned;}
    bool infoVisible() const {return m_infoVisible;}
    bool tinyPreview() const {return m_tinyPreview;}
    void setTinyPreview(bool enabled);
    bool videoHidden() const {return (mini()&&!m_tinyPreview)||m_diagnosticVideoHidden;}
    void setDiagnosticVideoHidden(bool hidden);
    void pointerActivity();
    bool controlsVisible() const {return m_controlsVisible||mini()||m_controllerPaused;}
    Q_INVOKABLE void setUiHeld(bool held);
    Q_INVOKABLE void setPopupOpen(bool open) {m_popupOpen=open;}
    bool popupOpen() const {return m_popupOpen;}
    void setNativeHover(bool hovered) {m_nativeHover=hovered;pointerActivity();}
    Q_INVOKABLE void openSubtitleDialog();
    Q_INVOKABLE void requestUrl();
    bool seekAt(const QPointF &point) const;
    bool titleAt(const QPointF &point) const;
    Q_INVOKABLE void requestMinimize();
    Q_INVOKABLE void openDialog();
    double videoAspect() const {return m_aspect;}
    quintptr nativeId() const {return m_nativeId;}
    bool controlAt(const QPointF &point) const;
signals:
    void lockAspectRatioChanged();
    void maxHoveredChanged();
    void modeChanged();
    void controlsVisibilityChanged();
    void contextMenuRequested(const QPointF &position);
    void urlRequested(const QString &clipboard);
    void dismissPopup();
protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched,QEvent *event) override;
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
    WindowStateMachine m_modes;
    bool m_applyingMode=false,m_pinned=false,m_infoVisible=false,m_tinyPreview=false;
    bool m_cursorPaused=true,m_dragCandidate=false;
    bool m_diagnosticVideoHidden=false;
    bool m_controlsVisible=true,m_uiHeld=false,m_controllerPaused=true;
    bool m_popupOpen=false,m_nativeHover=false;
    QPointF m_pressPosition;
    bool m_interactiveSizing=false;
    int m_resizeEdge=0;
    WindowStateMachine::Snapshot m_resizeOrigin;
    QTimer m_cursorTimer;
    void rememberModeGeometry();
    void applyMode();
    void toggleMode(WindowStateMachine::Mode target);
    void applyPin();
    QString inputRoleAt(const QPointF &point) const;
    void openNativeDialog(bool subtitle);
};
