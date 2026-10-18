#include "SingleInstance.h"
#include "../core/Log.h"
#include <QLocalSocket>
#include <QStandardPaths>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
int SingleInstance::start(const QStringList &paths){
    if(qEnvironmentVariableIsSet("FLOW_TEST_BACKGROUND")&&!qEnvironmentVariableIsSet("FLOW_INSTANCE_NAME"))return 0;
    const QString name=qEnvironmentVariable("FLOW_INSTANCE_NAME","FlowPlayer-v1-"+qEnvironmentVariable("USERNAME"));
    m_lock=std::make_unique<QLockFile>(QDir::tempPath()+"/"+name+".lock");
    if(!m_lock->tryLock(0)){
        QLocalSocket client;client.connectToServer(name);
        if(!client.waitForConnected(1500)){Log::write("IPC connect failed: "+client.errorString());return -1;}
        client.write(QJsonDocument(QJsonArray::fromStringList(paths)).toJson(QJsonDocument::Compact)+'\n');
        if(!client.waitForBytesWritten(1000)||!client.waitForReadyRead(1500)||client.readAll()!="OK"){Log::write("IPC delivery failed");return -1;}
        return 1;
    }
    QLocalServer::removeServer(name);m_server.setSocketOptions(QLocalServer::UserAccessOption);
    if(!m_server.listen(name)){Log::write("IPC listen failed: "+m_server.errorString());return -1;}
    connect(&m_server,&QLocalServer::newConnection,this,[this]{while(auto *socket=m_server.nextPendingConnection()){
        auto buffer=std::make_shared<QByteArray>();
        connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
        QTimer::singleShot(3000,socket,[socket]{socket->disconnectFromServer();});
        connect(socket,&QLocalSocket::readyRead,this,[this,socket,buffer]{buffer->append(socket->readAll());if(buffer->size()>1024*1024){socket->abort();return;}if(!buffer->endsWith('\n'))return;QJsonParseError error;auto doc=QJsonDocument::fromJson(*buffer,&error);if(error.error!=QJsonParseError::NoError||!doc.isArray()){socket->abort();return;}QStringList files;for(const auto &p:doc.array())if(p.isString())files.append(p.toString());emit requested(files);socket->write("OK");socket->flush();socket->disconnectFromServer();});
    }});return 0;
}
