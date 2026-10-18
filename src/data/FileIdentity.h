#pragma once
#include <QString>
#include <QVariantMap>
namespace FileIdentity {
// Worker-only: stat and hashing may block on remote/locked files.
QVariantMap inspect(const QString &path);
double resumePosition(const QVariantMap &record);
}
