#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Media.h>
#include <systemmediatransportcontrolsinterop.h>
#include "Smtc.h"
#include "../core/MpvController.h"
#include "../data/ResumeStore.h"
#include "../core/Log.h"
#include <QPointer>
using namespace winrt::Windows::Media;
struct Smtc::State {SystemMediaTransportControls controls{nullptr};winrt::event_token token{};QString title;int status=-1;};
Smtc::Smtc(quintptr window,MpvController *c,ResumeStore *resume):m_state(std::make_unique<State>()){
    try{
        auto factory=winrt::get_activation_factory<SystemMediaTransportControls,ISystemMediaTransportControlsInterop>();
        winrt::check_hresult(factory->GetForWindow(reinterpret_cast<HWND>(window),winrt::guid_of<SystemMediaTransportControls>(),winrt::put_abi(m_state->controls)));
        auto &controls=m_state->controls;controls.IsEnabled(true);controls.IsPlayEnabled(true);controls.IsPauseEnabled(true);controls.IsStopEnabled(true);controls.IsNextEnabled(true);controls.IsPreviousEnabled(true);
        QPointer<Smtc> guard(this);
        m_state->token=controls.ButtonPressed([guard,c,resume](auto const &,SystemMediaTransportControlsButtonPressedEventArgs const &args){const auto button=args.Button();if(guard)QMetaObject::invokeMethod(guard,[guard,c,resume,button]{if(!guard)return;switch(button){case SystemMediaTransportControlsButton::Play:c->setPause(false);break;case SystemMediaTransportControlsButton::Pause:c->setPause(true);break;case SystemMediaTransportControlsButton::Stop:resume->showHome();break;case SystemMediaTransportControlsButton::Next:resume->navigate(1);break;case SystemMediaTransportControlsButton::Previous:resume->navigate(-1);break;default:break;}},Qt::QueuedConnection);});
        auto update=[this,c]{try{auto &s=*m_state;int status=int(c->idleActive()?MediaPlaybackStatus::Stopped:c->pause()?MediaPlaybackStatus::Paused:MediaPlaybackStatus::Playing);if(status!=s.status){s.controls.PlaybackStatus(MediaPlaybackStatus(status));s.status=status;}if(s.title!=c->title()){s.title=c->title();auto updater=s.controls.DisplayUpdater();updater.Type(c->audioOnly()?MediaPlaybackType::Music:MediaPlaybackType::Video);if(c->audioOnly())updater.MusicProperties().Title(winrt::hstring(s.title.toStdWString()));else updater.VideoProperties().Title(winrt::hstring(s.title.toStdWString()));updater.Update();}}catch(const winrt::hresult_error &e){Log::write("SMTC update: "+QString::fromWCharArray(e.message().c_str()));}};
        connect(c,&MpvController::stateChanged,this,update);connect(c,&MpvController::metadataChanged,this,update);connect(c,&MpvController::playbackIdle,this,update);update();Log::write("SMTC_READY same HWND");
    }catch(const winrt::hresult_error &e){Log::write("SMTC unavailable: "+QString::fromWCharArray(e.message().c_str()));}
}
Smtc::~Smtc(){if(m_state->controls){try{m_state->controls.ButtonPressed(m_state->token);m_state->controls.IsEnabled(false);}catch(...){}}}
