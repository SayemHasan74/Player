#include "AppSettings.h"
#include "Log.h"
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <algorithm>
#include <cmath>
AppSettings::AppSettings(QObject *parent):QObject(parent) {
    m_path=qEnvironmentVariable("FLOW_SETTINGS_FILE");
    if(m_path.isEmpty())m_path=QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/FlowPlayer/settings.ini";
    m_values={{"audioLanguage","en,eng"},{"subtitleLanguage","en,eng"},{"autoResume",false},
        {"autoplayNext",true},{"lockAspectRatio",true},{"highQuality",false},{"hardwareDecoding",true},
        {"seekStep",5.0},{"exactSeekStep",1.0},{"largeSeekStep",30.0},{"tinyPreview",false},
        {"defaultSpeed",1.0},{"volume",100.0}};
    QSettings file(m_path,QSettings::IniFormat);
    for(auto i=m_values.begin();i!=m_values.end();++i) {
        const auto saved=file.value(i.key(),i.value());
        if(i.value().metaType().id()==QMetaType::Bool)i.value()=saved.toBool();
        else if(i.value().metaType().id()==QMetaType::Double)i.value()=saved.toDouble();
        else i.value()=saved.toString();
    }
    m_worker=std::thread(&AppSettings::writeLoop,this);
}
AppSettings::~AppSettings() {
    {std::lock_guard lock(m_mutex);m_stop=true;}
    m_wake.notify_one();m_worker.join();
}
void AppSettings::setValue(const QString &key,const QVariant &input) {
    if(!m_values.contains(key))return;
    QVariant value=input;
    const auto current=m_values.value(key);
    if((key=="volume"||key=="defaultSpeed"||key.endsWith("Step"))&&!std::isfinite(input.toDouble()))return;
    if(QStringList{"autoResume","autoplayNext","lockAspectRatio","highQuality","hardwareDecoding","tinyPreview"}.contains(key))value=input.toBool();
    else if(key=="volume")value=std::clamp(input.toDouble(),0.0,150.0);
    else if(key=="defaultSpeed")value=std::clamp(input.toDouble(),0.1,16.0);
    else if(key.endsWith("Step"))value=std::clamp(input.toDouble(),0.1,600.0);
    else if(key.endsWith("Language"))value=input.toString().trimmed().left(128);
    if(current==value)return;
    m_values[key]=value;
    {std::lock_guard lock(m_mutex);m_pending=m_values;m_dirty=true;}
    m_wake.notify_one();emit changed();emit valueChanged(key,value);
}
void AppSettings::writeLoop() {
    for(;;) {
        QVariantMap values;bool stop;
        {std::unique_lock lock(m_mutex);m_wake.wait(lock,[this]{return m_dirty||m_stop;});
         stop=m_stop;if(!m_dirty&&stop)return;values=m_pending;m_dirty=false;}
        QDir().mkpath(QFileInfo(m_path).absolutePath());
        QSettings file(m_path,QSettings::IniFormat);
        for(auto i=values.cbegin();i!=values.cend();++i)file.setValue(i.key(),i.value());
        file.sync();
        if(file.status()!=QSettings::NoError)Log::write("Settings write failed: "+m_path);
        if(stop)return;
    }
}
