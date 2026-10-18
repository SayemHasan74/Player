#pragma once
#include <QObject>
#include <QVariantMap>
#include <condition_variable>
#include <mutex>
#include <thread>

class AppSettings final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap values READ values NOTIFY changed)
    Q_PROPERTY(QString filePath READ filePath CONSTANT)
public:
    explicit AppSettings(QObject *parent=nullptr);
    ~AppSettings() override;
    QVariantMap values() const {return m_values;}
    QString filePath() const {return m_path;}
    Q_INVOKABLE void setValue(const QString &key,const QVariant &value);
signals:
    void changed();
    void valueChanged(const QString &key,const QVariant &value);
private:
    QVariantMap m_values,m_pending;
    QString m_path;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    bool m_dirty=false,m_stop=false;
    std::thread m_worker;
    void writeLoop();
};
