#include "VoiceClient.h"
#include "VoiceCodec.h"
#include "WaveAudio.h"
#include <algorithm>
#include <cmath>
#include <memory>
namespace bfvr::bf2142::voice {
namespace {
struct Shared {State state{};Preferences prefs{};Status status{};ULONGLONG stateTime=0,controlTime=0;bool focused=false,radio=false;};
SRWLOCK lock=SRWLOCK_INIT;Shared shared;bool started=false;
struct Start {unsigned port;net::Secret key;bool vr,receiveOnly;LogFunction log;};
struct Stream {
 Decoder decoder;std::array<Packet,8> queue{};unsigned head=0,count=0,player=256;std::uint64_t session=0,last=0,ready=0;
 std::uint32_t sequence=0;unsigned loss=0;stereo::Vec3 position{};float range=20;
 void Reset(){decoder.Reset();head=count=0;player=256;session=last=ready=0;sequence=loss=0;}
 bool Push(const Packet& p,ULONGLONG now){
  if(player!=p.player||session!=p.session||now-last>500){Reset();player=p.player;session=p.session;ready=now+40;}
  if(!net::Newer(p.sequence,sequence))return false;sequence=p.sequence;last=now;position=p.position;range=p.range;loss=0;
  if(count==queue.size()){head=(head+1)%unsigned(queue.size());--count;}
  queue[(head+count)%queue.size()]=p;++count;return true;
 }
 bool Read(std::array<short,FrameSamples>& pcm,ULONGLONG now){
  if(player>255||now<ready||now-last>250)return false;
  if(count){const auto& p=queue[head];const bool ok=decoder.Decode(p.data.data(),p.size,pcm);head=(head+1)%unsigned(queue.size());--count;return ok;}
  if(loss++<2)return decoder.Decode(nullptr,0,pcm);return false;
 }
};
DWORD WINAPI Worker(void* parameter){
 std::unique_ptr<Start> start(static_cast<Start*>(parameter));
 Socket socket;Encoder encoder;WaveAudio audio;Gate gate;std::array<Stream,16> streams;
 Status status;status.running=socket.Open(false,start->port)&&encoder.Open();for(auto& s:streams)status.running=s.decoder.Open()&&status.running;
 if(!status.running){if(start->log)start->log("Proximity voice disabled: codec or loopback socket unavailable.");return 0;}
 if(start->log)start->log("Proximity voice worker ready: Opus 16 kHz/24 kbps, UDP %u, %s.",start->port,start->receiveOnly?"receive-only observer":"voice activation; focus/spawn gates");
 ULONGLONG helloTime=0,nextMix=0,retryInput=0,retryOutput=0;std::uint64_t session=0;unsigned player=256;std::uint32_t sequence=0;
 unsigned inputDevice=~0u,outputDevice=~0u;std::array<std::array<short,FrameSamples>,2> preroll{};unsigned previousFrames=0;bool wasTalking=false;
 for(;;){
  const auto now=GetTickCount64();Shared snapshot;AcquireSRWLockShared(&lock);snapshot=shared;ReleaseSRWLockShared(&lock);
  auto prefs=snapshot.prefs;const bool live=snapshot.state.alive&&snapshot.state.player<256&&snapshot.state.session&&now-snapshot.stateTime<500;
  const bool listening=live&&prefs.enabled;
  DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
  const bool radio=(snapshot.radio&&now-snapshot.controlTime<150) || (GetAsyncKeyState('V')&0x8000) || (GetAsyncKeyState('B')&0x8000);
  const bool capture=listening&&!start->receiveOnly&&!prefs.muted&&!radio&&foreground==GetCurrentProcessId()&&
   (!start->vr||(snapshot.focused&&now-snapshot.controlTime<150));
  if(player!=snapshot.state.player||session!=snapshot.state.session){player=snapshot.state.player;session=snapshot.state.session;sequence=0;helloTime=0;gate.Reset();encoder.Reset();for(auto& s:streams)s.Reset();audio.CloseCapture();audio.ClosePlayback();previousFrames=0;wasTalking=false;}
  if(inputDevice!=prefs.input){inputDevice=prefs.input;audio.CloseCapture();retryInput=0;}
  if(outputDevice!=prefs.output){outputDevice=prefs.output;audio.ClosePlayback();retryOutput=0;}
  if(!capture){audio.CloseCapture();gate.Reset();previousFrames=0;wasTalking=false;status.transmitting=false;status.micDb=-96;}
  else if(!audio.Capturing()&&now>=retryInput){if(!audio.Capture(inputDevice)){retryInput=now+2000;if(start->log&&status.inputError!=audio.InputError())start->log("Proximity microphone open failed: WinMM %u; choose an available recording device.",audio.InputError());}}
  if(!listening){audio.ClosePlayback();for(auto& s:streams)s.Reset();helloTime=0;nextMix=now;}
  else if(now-helloTime>=250||!helloTime){Packet h;h.player=player;h.session=session;h.secret=start->key;if(socket.Send(h,start->port))helloTime=now;}
  std::array<short,FrameSamples> pcm{};
  const auto send=[&](const std::array<short,FrameSamples>& samples){Packet p;p.kind=Audio;p.player=player;p.session=session;p.secret=start->key;if(!++sequence)++sequence;p.sequence=sequence;
   const int n=encoder.Encode(samples,p.data);if(n>0){p.size=std::uint16_t(n);if(socket.Send(p,start->port)){if(!status.sent&&start->log)start->log("Proximity voice: first microphone frame sent for player %u.",player);++status.sent;}}};
  for(unsigned work=0;capture&&work<8&&audio.Read(pcm);++work){
   double sum=0;for(auto v:pcm){const double a=double(v)/32768;sum+=a*a;}status.micDb=float(10*std::log10(std::max(1.e-10,sum/FrameSamples)));
   const bool talking=gate.Update(pcm,prefs.threshold,true);status.transmitting=talking;
   if(talking){if(!wasTalking){encoder.Reset();for(unsigned i=0;i<previousFrames;++i)send(preroll[i]);}send(pcm);previousFrames=0;}
   else {if(previousFrames==2){preroll[0]=preroll[1];previousFrames=1;}preroll[previousFrames++]=pcm;}
   wasTalking=talking;
  }
  Packet packet;unsigned port=0;
  for(unsigned work=0;work<64&&socket.Receive(packet,port,start->key);++work){
   if(!listening||port!=start->port||packet.kind!=Relay||packet.player==player)continue;
   Stream* stream=nullptr;for(auto& s:streams)if(s.player==packet.player){stream=&s;break;}
   if(!stream)for(auto& s:streams)if(s.player>255||now-s.last>500){stream=&s;break;}
   if(stream&&stream->Push(packet,now))++status.received;
  }
  if(listening&&now>=nextMix){
   nextMix=nextMix&&now-nextMix<FrameMs*2?nextMix+FrameMs:now+FrameMs;std::array<float,FrameSamples*2> mix{};bool any=false;
   for(auto& stream:streams)if(stream.Read(pcm,now)){
    const auto gains=SpatialGains(stream.position,snapshot.state.listener,stream.range);const float volume=std::clamp(prefs.volume,0.f,2.f);any=true;
    for(unsigned i=0;i<FrameSamples;++i){mix[i*2]+=pcm[i]*gains.left*volume;mix[i*2+1]+=pcm[i]*gains.right*volume;}
   }
   if(any){std::array<short,FrameSamples*2> stereo{};for(unsigned i=0;i<stereo.size();++i)stereo[i]=short(std::clamp(mix[i],-32767.f,32767.f));
    if(!audio.Playing()&&now>=retryOutput&&!audio.Playback(outputDevice))retryOutput=now+2000;
    if(audio.Write(stereo)){if(!status.played&&start->log)start->log("Proximity voice: first remote decoded frame submitted to stereo playback.");++status.played;}
   }
  }
  status.capturing=audio.Capturing();status.inputError=audio.InputError();status.outputError=audio.OutputError();
  AcquireSRWLockExclusive(&lock);shared.status=status;ReleaseSRWLockExclusive(&lock);
  Sleep(3);
 }
}
}
void StartClient(unsigned port,const net::Secret& key,bool vr,bool receiveOnly,const Preferences& prefs,LogFunction log){
 if(started)return;started=true;shared.prefs=prefs;auto start=std::make_unique<Start>(Start{port,key,vr,receiveOnly,log});
 const HANDLE thread=CreateThread(nullptr,0,Worker,start.get(),0,nullptr);if(thread){start.release();CloseHandle(thread);}else started=false;
}
void PublishState(const State& state){AcquireSRWLockExclusive(&lock);shared.state=state;shared.stateTime=GetTickCount64();ReleaseSRWLockExclusive(&lock);}
void PublishControls(const Preferences& p,bool focused,bool radio){AcquireSRWLockExclusive(&lock);shared.prefs=p;shared.focused=focused;shared.radio=radio;shared.controlTime=GetTickCount64();ReleaseSRWLockExclusive(&lock);}
Status ReadStatus(){AcquireSRWLockShared(&lock);const auto s=shared.status;ReleaseSRWLockShared(&lock);return s;}
}
