#pragma once
#include <QString>
namespace FolderScanner {
// Worker-only, user-authorized transport neighbors; no playlist panel/advance.
QString neighbor(const QString &path,int direction);
}
