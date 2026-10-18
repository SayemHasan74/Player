#pragma once
#include <QObject>
class MpvController;
class SleepInhibitor final:public QObject {
public:
    explicit SleepInhibitor(MpvController *controller);
    ~SleepInhibitor() override;
private: bool m_active=false;
};
