#include "FolderScanner.h"
#include <QFileInfo>
#include <QDir>
#include <QCollator>
#include <algorithm>
QString FolderScanner::neighbor(const QString &path,int direction) {
    if(path.contains("://"))return {};
    const QFileInfo current(path);
    static const QStringList extensions=QString("mkv mp4 m4v avi mov wmv webm mpg mpeg ts m2ts mts flv vob ogv mxf m2v 3gp mka mp3 flac wav ogg opus aac m4a aiff aif ape wv wma").split(' ');
    auto entries=current.dir().entryInfoList(QDir::Files|QDir::Readable,QDir::NoSort);
    entries.erase(std::remove_if(entries.begin(),entries.end(),[](const QFileInfo &entry){return !extensions.contains(entry.suffix().toLower());}),entries.end());
    QCollator collator;collator.setNumericMode(true);collator.setCaseSensitivity(Qt::CaseInsensitive);
    std::sort(entries.begin(),entries.end(),[&](const QFileInfo &a,const QFileInfo &b){const int result=collator.compare(a.fileName(),b.fileName());return result==0?a.fileName()<b.fileName():result<0;});
    for(int i=0;i<entries.size();++i)if(entries[i].absoluteFilePath().compare(current.absoluteFilePath(),Qt::CaseInsensitive)==0){const int index=i+(direction<0?-1:1);return index>=0&&index<entries.size()?entries[index].absoluteFilePath():QString();}
    return {};
}
