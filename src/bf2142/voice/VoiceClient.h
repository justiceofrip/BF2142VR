#pragma once
#include "VoiceProtocol.h"
#include "../StereoSession.h"
#include "../VrSettings.h"
namespace bfvr::bf2142::voice {
struct Preferences {bool enabled=true,muted=false;float threshold=-40,volume=1;unsigned input=~0u,output=~0u;};
struct State {unsigned player=256;std::uint64_t session=0;bool alive=false;stereo::Matrix4 listener{};};
struct Status {bool running=false,capturing=false,transmitting=false;unsigned inputError=0,outputError=0;std::uint64_t sent=0,received=0,played=0;float micDb=-96;};
inline Preferences VoicePreferences(const VrSettings& s){return {s.proximityVoice,s.proximityMuted,s.voiceThreshold,s.voiceVolume,s.voiceInput,s.voiceOutput};}
void StartClient(unsigned port,const net::Secret&,bool vr,bool receiveOnly,const Preferences&,LogFunction);
void PublishState(const State&);
void PublishControls(const Preferences&,bool trackingFocused,bool radioPressed);
Status ReadStatus();
}
