#include "FileIdentity.h"
#include <QCryptographicHash>
#include <QFileInfo>
#include <QDir>
#include <QUrl>
#include <QDateTime>
#include <windows.h>
QVariantMap FileIdentity::inspect(const QString &input) {
    QUrl url(input);QString path=url.isLocalFile()?url.toLocalFile():input;
    if(!url.scheme().isEmpty()&&url.scheme().size()>1&&!url.isLocalFile())
        return {{"path",path},{"file_key","url:"+QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(),QCryptographicHash::Sha256).toHex())},{"size",0},{"mtime",0},{"available",true}};
    path=QDir::cleanPath(QFileInfo(path).absoluteFilePath());QFileInfo info(path);
    QVariantMap result{{"path",path},{"size",info.size()},{"mtime",info.lastModified().toMSecsSinceEpoch()},{"available",info.exists()}};
    HANDLE file=CreateFileW(reinterpret_cast<LPCWSTR>(QDir::toNativeSeparators(path).utf16()),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_RANDOM_ACCESS,nullptr);
    if(file==INVALID_HANDLE_VALUE) {result["identityError"]=int(GetLastError());return result;}
    LARGE_INTEGER size{};if(!GetFileSizeEx(file,&size)){result["identityError"]=int(GetLastError());CloseHandle(file);return result;}result["size"]=size.QuadPart;
    QCryptographicHash hash(QCryptographicHash::Sha256);bool valid=true;
    auto read=[&](qint64 offset,DWORD bytes) {
        LARGE_INTEGER pos{};pos.QuadPart=offset;DWORD received=0;QByteArray buffer(bytes,Qt::Uninitialized);
        if(!SetFilePointerEx(file,pos,nullptr,FILE_BEGIN)||!ReadFile(file,buffer.data(),bytes,&received,nullptr)||received!=bytes)valid=false;
        else hash.addData(QByteArrayView(buffer.constData(),received));
    };
    if(size.QuadPart<=131072)read(0,DWORD(size.QuadPart));else {read(0,65536);read(size.QuadPart-65536,65536);}
    CloseHandle(file);
    if(valid)result["file_key"]=QString::number(size.QuadPart)+":"+QString::fromLatin1(hash.result().toHex());
    return result;
}
double FileIdentity::resumePosition(const QVariantMap &r) {
    const double duration=r.value("duration").toDouble(),pos=r.value("position").toDouble();
    return r.value("finished").toBool()||(duration>0&&pos/duration>.95)?0:qMax(0.0,pos-2);
}
