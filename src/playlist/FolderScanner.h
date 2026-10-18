#pragma once
#include <QString>
#include <QStringList>
namespace FolderScanner {
QStringList scan(const QString &path,const QStringList &ignore={"sample","trailer"});
QString neighbor(const QString &path,int direction);
}
