#pragma once
#include <QObject>
#include <memory>
class MpvController;
class ResumeStore;
class Smtc final:public QObject {
public:
    Smtc(quintptr window,MpvController *controller,ResumeStore *resume);
    ~Smtc() override;
private:
    struct State;
    std::unique_ptr<State> m_state;
};
