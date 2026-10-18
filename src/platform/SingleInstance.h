#pragma once
#include <QObject>
#include <QLocalServer>
#include <QLockFile>
#include <memory>
class SingleInstance final:public QObject {
    Q_OBJECT
public:
    explicit SingleInstance(QObject *parent=nullptr):QObject(parent){}
    // 0 primary/isolated, 1 forwarded, -1 unavailable (do not start a duplicate).
    int start(const QStringList &paths);
signals:
    void requested(const QStringList &paths);
private:
    QLocalServer m_server;
    std::unique_ptr<QLockFile> m_lock;
};
