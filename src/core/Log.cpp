#include "Log.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QThread>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
namespace {
std::mutex mutex;
std::condition_variable changed;
std::deque<QByteArray> messages;
bool stopping=false;
std::thread worker;
void handler(QtMsgType, const QMessageLogContext &, const QString &text) { Log::write(text); }
}
void Log::start() {
    const QString directory=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/FlowPlayer/logs";
    worker=std::thread([directory] {
        QDir().mkpath(directory);
        const auto path=directory+"/flowplayer.log";
        auto rotate=[&] {
            QFile::remove(path+".4");
            for(int i=3;i>=1;--i) QFile::rename(path+"."+QString::number(i),path+"."+QString::number(i+1));
            QFile::rename(path,path+".1");
        };
        if(QFile::exists(path)) rotate();
        QFile file(path); if(!file.open(QIODevice::WriteOnly|QIODevice::Append))return;
        for(;;) {
            std::deque<QByteArray> batch;
            { std::unique_lock lock(mutex); changed.wait(lock,[]{return stopping||!messages.empty();}); batch.swap(messages); if(batch.empty()&&stopping) break; }
            for(const auto &line:batch) file.write(line);
            file.flush();
            if(file.size()>8*1024*1024) {file.close();rotate();if(!file.open(QIODevice::WriteOnly))return;}
        }
    });
    qInstallMessageHandler(handler);
}
void Log::write(const QString &message) {
    const auto line=(QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs)+" ["+QString::number(quintptr(QThread::currentThreadId()))+"] "+message+"\n").toUtf8();
    {std::lock_guard lock(mutex); messages.push_back(line);} changed.notify_one();
}
void Log::stop() {
    qInstallMessageHandler(nullptr);
    {std::lock_guard lock(mutex);stopping=true;} changed.notify_one();
    if(worker.joinable()) worker.join();
}
