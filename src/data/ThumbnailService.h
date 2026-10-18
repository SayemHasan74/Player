#pragma once
#include <QObject>
#include <QVariantMap>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <QSet>
class ThumbnailService final:public QObject {
    Q_OBJECT
public:
    explicit ThumbnailService(QString directory,QObject *parent=nullptr);
    ~ThumbnailService() override;
    void request(const QVariantMap &record);
    void setAllowed(bool allowed);
    void forget(const QString &key);
signals:
    void available(const QString &key);
private:
    void run();
    QString m_directory;
    std::thread m_worker;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<QVariantMap> m_queue;
    QSet<QString> m_pending,m_cancelled;
    std::atomic_bool m_stop{false},m_allowed{false};
};
