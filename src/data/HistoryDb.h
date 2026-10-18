#pragma once
#include <QObject>
#include <QVariantMap>
#include <QVariantList>
#include <QHash>
#include <functional>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
struct sqlite3;
QString flowThumbnailPath(const QString &directory,const QString &key);
class HistoryDb final:public QObject {
    Q_OBJECT
public:
    explicit HistoryDb(QString path={},QString thumbs={},QObject *parent=nullptr);
    ~HistoryDb() override;
    void lookup(QString path,quint64 token);
    void neighbor(QString path,int direction,quint64 token);
    void save(QVariantMap record);
    void recent();
    void remove(QString key);
    void locate(QString path);
    void barrier();
    QString thumbnailDirectory() const {return m_thumbs;}
signals:
    void resolved(quint64 token,const QVariantMap &record);
    void recentReady(const QVariantList &records);
    void failed(const QString &message);
    void committed(const QVariantMap &record);
    void drained();
    void folderReady(const QString &path);
    void neighborReady(quint64 token,const QString &origin,const QString &path);
private:
    void enqueue(std::function<void()> work);
    void run();
    void runIdentity();
    void runMetadata();
    void resolveIdentity(QVariantMap identity,quint64 token,qint64 elapsedMs);
    void publishRecent();
    void prune();
    void execute(const char *sql);
    QVariantMap row(void *statement);
    QString m_path,m_thumbs;
    sqlite3 *m_db=nullptr;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::condition_variable m_identityCv;
    std::condition_variable m_metadataCv;
    std::deque<std::function<void()>> m_queue;
    std::deque<std::function<void()>> m_identityQueue;
    std::deque<std::function<void()>> m_metadataQueue;
    bool m_stop=false;
    bool m_identityStop=false;
    bool m_metadataStop=false;
    std::thread m_worker;
    std::thread m_identityWorker;
    std::thread m_metadataWorker;
    quint64 m_recentGeneration=0;
    QHash<QString,QVariantMap> m_recentMetadata;
};
