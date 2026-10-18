#pragma once
#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <mpv/client.h>

class MpvController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(double position READ position NOTIFY stateChanged)
    Q_PROPERTY(double duration READ duration NOTIFY stateChanged)
    Q_PROPERTY(bool pause READ pause NOTIFY stateChanged)
    Q_PROPERTY(double volume READ volume NOTIFY stateChanged)
    Q_PROPERTY(bool mute READ mute NOTIFY stateChanged)
    Q_PROPERTY(double speed READ speed NOTIFY stateChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY stateChanged)
    Q_PROPERTY(QString hwdecCurrent READ hwdecCurrent NOTIFY stateChanged)
    Q_PROPERTY(qint64 droppedFrames READ droppedFrames NOTIFY stateChanged)
    Q_PROPERTY(bool highQuality READ highQuality WRITE setHighQuality NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap videoInfo READ videoInfo NOTIFY stateChanged)
    Q_PROPERTY(QString videoTrack READ videoTrack NOTIFY stateChanged)
public:
    explicit MpvController(QObject *parent=nullptr);
    ~MpvController() override;
    void start();
    mpv_handle *handle() const { return m_handle.load(); }
    bool ready() const {return m_ready;}
    double position() const {return m_position;}
    double duration() const {return m_duration;}
    bool pause() const {return m_pause;}
    double volume() const {return m_volume;}
    bool mute() const {return m_mute;}
    double speed() const {return m_speed;}
    QString title() const {return m_title;}
    QVariantList tracks() const {return m_tracks;}
    QString hwdecCurrent() const {return m_hwdec;}
    qint64 droppedFrames() const {return m_dropped;}
    bool highQuality() const {return m_highQuality;}
    QVariantMap videoInfo() const {return m_videoInfo;}
    QString videoTrack() const {return m_vid;}
    qint64 cacheBytes() const {return m_cacheBytes;}
    double avSync() const {return m_avSync;}
    Q_INVOKABLE void open(const QString &path);
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void setPause(bool paused);
    Q_INVOKABLE void setHighQuality(bool enabled);
    Q_INVOKABLE void seek(double seconds,bool exact=false);
    Q_INVOKABLE void seekTo(double seconds);
    Q_INVOKABLE void adjustVolume(double delta);
    Q_INVOKABLE void setVolume(double value);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void adjustSpeed(double delta);
    Q_INVOKABLE void resetSpeed();
    Q_INVOKABLE void frameStep(bool backward);
    void setMiniVideoDisabled(bool disabled);
    void setRenderReady(bool ready);
    void command(const QList<QByteArray> &arguments);
signals:
    void readyChanged();
    void stateChanged();
    void videoSizeChanged(int width,int height);
    void error(const QString &message);
    void renderUpdateRequested();
    void videoFramePresented();
    void playbackIdle();
    void toast(const QString &message);
private:
    void run();
    void apply(const QString &name,const QVariant &value);
    static void wakeup(void *opaque);
    std::atomic<mpv_handle*> m_handle{nullptr};
    std::atomic_bool m_stop{false};
    std::mutex m_mutex;
    std::condition_variable m_wakeup;
    bool m_pending=false;
    std::thread m_thread;
    bool m_ready=false,m_pause=true,m_mute=false,m_highQuality=false;
    double m_position=0,m_duration=0,m_volume=100,m_speed=1;
    double m_avSync=0;
    qint64 m_dropped=0;
    qint64 m_cacheBytes=-1;
    int m_width=0,m_height=0;
    QString m_title="FlowPlayer",m_hwdec="none";
    QVariantList m_tracks;
    QVariantMap m_videoInfo;
    QString m_vid="auto",m_savedVid="auto";
    bool m_miniVideoDisabled=false,m_miniRequested=false;
    bool m_renderReady=false;
    QString m_pendingFile;
};
