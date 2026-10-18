#include "FolderScanner.h"
#include <QFileInfo>
#include <QDir>
#include "NaturalSort.h"
#include <QRegularExpression>
#include <algorithm>
QStringList FolderScanner::scan(const QString &path,const QStringList &ignore) {
    if(path.contains("://"))return {};
    const QFileInfo current(path);
    static const QStringList extensions=QString("mkv mp4 m4v avi mov wmv webm mpg mpeg ts m2ts mts flv vob ogv mxf m2v 3gp mka mp3 flac wav ogg opus aac m4a aiff aif ape wv wma").split(' ');
    auto entries=current.dir().entryInfoList(QDir::Files|QDir::Readable,QDir::NoSort);
    entries.erase(std::remove_if(entries.begin(),entries.end(),[](const QFileInfo &entry){return !extensions.contains(entry.suffix().toLower());}),entries.end());
    std::sort(entries.begin(),entries.end(),[](const QFileInfo &a,const QFileInfo &b){return NaturalSort::less(a.fileName(),b.fileName());});
    QStringList result;
    for(const auto &entry:entries){bool extra=false;
        if(entry.absoluteFilePath().compare(current.absoluteFilePath(),Qt::CaseInsensitive)!=0)
            for(const auto &word:ignore){if(word.trimmed().isEmpty())continue;QRegularExpression pattern("(^|[\\s._\\-\\[\\]()])"+QRegularExpression::escape(word.trimmed())+"($|[\\s._\\-\\[\\]()])",QRegularExpression::CaseInsensitiveOption);if(pattern.match(entry.completeBaseName()).hasMatch()){extra=true;break;}}
        if(!extra)result.append(entry.absoluteFilePath());
    }
    if(!result.contains(current.absoluteFilePath(),Qt::CaseInsensitive))result.append(current.absoluteFilePath());
    std::sort(result.begin(),result.end(),[](const QString &a,const QString &b){return NaturalSort::less(QFileInfo(a).fileName(),QFileInfo(b).fileName());});
    return result;
}
QString FolderScanner::neighbor(const QString &path,int direction) {
    const auto entries=scan(path);
    for(int i=0;i<entries.size();++i)if(entries[i].compare(path,Qt::CaseInsensitive)==0){const int index=i+(direction<0?-1:1);return index>=0&&index<entries.size()?entries[index]:QString();}
    return {};
}
