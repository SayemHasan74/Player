#pragma once
#include <QObject>
#include <QVariant>
#include <QVariantList>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <mpv/client.h>
class AppSettings;
class ResumeStore;

class MpvController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool ready READ ready NOTIFY readyChanged)
    Q_PROPERTY(bool surfaceRequested READ surfaceRequested NOTIFY surfaceRequestedChanged)
    Q_PROPERTY(double position READ position NOTIFY stateChanged)
    Q_PROPERTY(double duration READ duration NOTIFY stateChanged)
    Q_PROPERTY(bool pause READ pause NOTIFY stateChanged)
    Q_PROPERTY(double volume READ volume NOTIFY stateChanged)
    Q_PROPERTY(bool mute READ mute NOTIFY stateChanged)
    Q_PROPERTY(double speed READ speed NOTIFY stateChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(QVariantList tracks READ tracks NOTIFY metadataChanged)
    Q_PROPERTY(QString hwdecCurrent READ hwdecCurrent NOTIFY stateChanged)
    Q_PROPERTY(qint64 droppedFrames READ droppedFrames NOTIFY stateChanged)
    Q_PROPERTY(bool highQuality READ highQuality WRITE setHighQuality NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap videoInfo READ videoInfo NOTIFY stateChanged)
    Q_PROPERTY(QString videoTrack READ videoTrack NOTIFY stateChanged)
    Q_PROPERTY(QVariantList chapters READ chapters NOTIFY metadataChanged)
    Q_PROPERTY(QVariantList bufferedRanges READ bufferedRanges NOTIFY cacheChanged)
    Q_PROPERTY(qint64 cacheBytes READ cacheBytes NOTIFY cacheChanged)
    Q_PROPERTY(QVariantMap playbackOptions READ playbackOptions NOTIFY metadataChanged)
    Q_PROPERTY(bool networkSource READ networkSource NOTIFY metadataChanged)
    Q_PROPERTY(bool audioOnly READ audioOnly NOTIFY metadataChanged)
public:
    explicit MpvController(QObject *parent=nullptr);
    ~MpvController() override;
    void start();
    void configure(AppSettings *settings);
    AppSettings *settings() const {return m_settings;}
    ResumeStore *resumeStore() const {return m_resumeStore;}
    void setResumeStore(ResumeStore *store) {m_resumeStore=store;}
    void loadResolved(const QString &path,const QVariantMap &options);
    Q_INVOKABLE void stop();
    mpv_handle *handle() const { return m_handle.load(); }
    bool ready() const {return m_ready;}
    bool surfaceRequested() const {return m_surfaceRequested;}
    bool idleActive() const {return m_idle;}
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
    QVariantList chapters() const {return m_chapters;}
    QVariantList bufferedRanges() const {return m_buffered;}
    QVariantMap playbackOptions() const {return m_options;}
    bool networkSource() const {return m_network;}
    bool audioOnly() const {return m_audioOnly;}
    qint64 cacheBytes() const {return m_cacheBytes;}
    double avSync() const {return m_avSync;}
    Q_INVOKABLE void open(const QString &path);
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void setPause(bool paused);
    Q_INVOKABLE void setHighQuality(bool enabled);
    Q_INVOKABLE void seek(double seconds,bool exact=false);
    Q_INVOKABLE void seekTo(double seconds,bool exact=true);
    Q_INVOKABLE void selectTrack(const QString &type,int id);
    Q_INVOKABLE void cycleTrack(const QString &type);
    Q_INVOKABLE void addSubtitle(const QString &path);
    Q_INVOKABLE void setPlaybackOption(const QString &key,const QVariant &value);
    Q_INVOKABLE void setAspect(const QString &aspect);
    Q_INVOKABLE void notify(const QString &message) {emit toast(message);}
    Q_INVOKABLE void adjustVolume(double delta);
    Q_INVOKABLE void setVolume(double value);
    Q_INVOKABLE void toggleMute();
    Q_INVOKABLE void adjustSpeed(double delta);
    Q_INVOKABLE void resetSpeed();
    Q_INVOKABLE void frameStep(bool backward);
    void setRenderReady(bool ready);
    void command(const QList<QByteArray> &arguments);
signals:
    void surfaceRequestedChanged();
    void openRequested(const QString &path);
    void fileLoaded(const QString &path);
    void positionSettled();
    void playbackFinished();
    void beforeStop();
    void readyChanged();
    void stateChanged();
    void videoSizeChanged(int width,int height);
    void error(const QString &message);
    void renderUpdateRequested();
    void videoFramePresented();
    void playbackIdle();
    void toast(const QString &message);
    void metadataChanged();
    void cacheChanged();
    void seekCompleted(double requested,double actual);
private:
    void run();
    void apply(const QString &name,const QVariant &value);
    void applySetting(const QString &key,const QVariant &value);
    static void wakeup(void *opaque);
    std::atomic<mpv_handle*> m_handle{nullptr};
    std::atomic_bool m_stop{false};
    std::mutex m_mutex;
    std::condition_variable m_wakeup;
    bool m_pending=false;
    std::thread m_thread;
    bool m_ready=false,m_pause=true,m_mute=false,m_highQuality=false,m_idle=true;
    double m_position=0,m_duration=0,m_volume=100,m_speed=1;
    double m_avSync=0;
    qint64 m_dropped=0;
    qint64 m_cacheBytes=-1;
    int m_width=0,m_height=0;
    QString m_title="FlowPlayer",m_hwdec="none";
    QVariantList m_tracks;
    QVariantMap m_videoInfo;
    QString m_vid="auto";
    bool m_renderReady=false;
    bool m_surfaceRequested=false;
    QString m_pendingFile;
    QVariantMap m_pendingLoadOptions;
    ResumeStore *m_resumeStore=nullptr;
    AppSettings *m_settings=nullptr;
    QVariantMap m_initialSettings,m_options;
    QVariantList m_chapters,m_buffered;
    bool m_network=false,m_audioOnly=false;
    double m_exactSeekTarget=-1;
};
