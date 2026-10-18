#include "SleepInhibitor.h"
#include "../core/MpvController.h"
#include "../core/Log.h"
#include <windows.h>
SleepInhibitor::SleepInhibitor(MpvController *c){auto update=[this,c]{const bool active=!c->idleActive()&&!c->pause()&&!c->audioOnly();if(active==m_active)return;const auto result=SetThreadExecutionState(ES_CONTINUOUS|(active?ES_DISPLAY_REQUIRED:0));if(result){m_active=active;Log::write(QString("DISPLAY_SLEEP_INHIBIT=%1").arg(active));}else Log::write("SetThreadExecutionState failed");};connect(c,&MpvController::stateChanged,this,update);connect(c,&MpvController::metadataChanged,this,update);connect(c,&MpvController::playbackIdle,this,update);connect(c,&MpvController::fileLoaded,this,update);}
SleepInhibitor::~SleepInhibitor(){SetThreadExecutionState(ES_CONTINUOUS);}
