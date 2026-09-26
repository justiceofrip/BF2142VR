#include "NativeNetwork.h"
#include "../NativeComfort.h"
#include "NativeRoster.h"
#include "LoopbackTransport.h"
#include "RemoteArmMath.h"
#include "RemotePresentation.h"
#include "RemoteCollision.h"
#include "FistBump.h"
#include "NativeRemoteWeapon.h"
#include "../voice/VoiceClient.h"
#include <bcrypt.h>
#include <MinHook.h>
#include <cstring>
#include <cmath>
#include <memory>
#include <new>
namespace bfvr::bf2142 {
namespace {
using namespace net;
Settings settings;Transport transport;BYTE* game=nullptr;BYTE* render=nullptr;LogFunction logger=nullptr;
using RemoteFinalize=void(__thiscall*)(void*,float,unsigned);RemoteFinalize originalRemoteFinalize=nullptr;
void RestoreRemoteArms(void* soldier);
void __fastcall RemoteFinalizeHook(void* soldier,void*,float delta,unsigned update){RestoreRemoteArms(soldier);originalRemoteFinalize(soldier,delta,update);TickNetworkClient();ApplyRemoteNetworkPose(soldier);}
bool enabled=false,reportedSend=false,observerOnly=false;DWORD ownerThread=0;ULONGLONG lastPump=0,lastSend=0;
std::uint64_t session=0;std::uint32_t sequence=0;std::array<Actor,256> actors{};std::array<FreshPose,256> poses{};
struct RigOverride {BodyBones native{},applied{};std::array<bool,80> written{};bool valid=false;};
struct Binding {RemotePresentation presentation;RemoteArmContinuity continuity;FistBumpPeer fists;std::unique_ptr<RigOverride> overridePose;void* weak=nullptr;void* skeleton=nullptr;PalmBinding left{},right{};bool valid=false,reported=false,reportedSolveSkip=false,reportedHead=false;};
BodyBones detailedReference{};PalmBinding referenceLeft{},referenceRight{};bool referenceValid=false;
std::array<Binding,256> bindings{};Packet outgoing{};ULONGLONG outgoingTime=0;unsigned localId=256;
struct LocalEvents {void* soldier=nullptr;void* weapon=nullptr;float snap=0;Matrix launch{};stereo::Vec3 velocity{};
 std::uint32_t snapSerial=0,throwSerial=0;ULONGLONG snapTime=0,throwTime=0;} events;
std::uint32_t nextSnap=0,nextThrow=0;JoinProofPolicy proofPolicy;
volatile LONG fistActive=0,pendingFists=0,pendingFistTime=0;
void UpdateFistBumps(ULONGLONG now);
void AttachEvents(Packet& p,ULONGLONG now){
 if(localId>255||actors[localId].soldier!=events.soldier)return;
 if(events.snapTime&&now>=events.snapTime&&now-events.snapTime<300){p.snapSerial=events.snapSerial;p.snapDegrees=events.snap;}
 if(events.throwTime&&now>=events.throwTime&&now-events.throwTime<600&&actors[localId].weapon==events.weapon&&(p.flags&LeftCrateHeld)){
  p.throwSerial=events.throwSerial;p.throwLaunch=events.launch;p.throwVelocity=events.velocity;
 }
}
bool NativeProfile(){
 __try {
  if(!game||!render)return false;
  const BYTE remote[]={0x55,0x8b,0xec,0x81,0xec,0xb0,1,0,0,0x53,0x56,0x8b,0xf1,0x8b,0x86,0xf0,2,0,0};
  if(std::memcmp(game+0x1efc00,remote,sizeof(remote)))return false;
  const BYTE list[]={0x8d,0x41,0x0c,0xc3},local[]={0x8b,0x41,0x6c,0xc3};
  return Read<void*>(game+ClientProfile.managerVt,0x40)==game+0xb7510&&!std::memcmp(game+0xb7510,list,sizeof(list))&&
   Read<void*>(game+ClientProfile.managerVt,0x30)==game+0xb7530&&!std::memcmp(game+0xb7530,local,sizeof(local));
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool Roster(){
 __try {
  void* pm=Read<void*>(render,0x221a58);if(!ReadRoster(pm,game,ClientProfile,&actors))return false;
  const auto local=Read<void*>(pm,0x6c);localId=256;for(unsigned i=0;i<256;++i)if(actors[i].player==local)localId=i;
  return localId<256;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool Skeleton(void* soldier,void** skeleton,BodyBones* native,bool active=true){
 __try {
  if(!soldier||Read<void*>(soldier,0)!=game+ClientProfile.soldierVt||(active&&Read<int>(soldier,0x2f0)!=1))return false;
  *skeleton=Read<void*>(soldier,0x2c8);if(!*skeleton||Read<int>(*skeleton,0x14)!=80)return false;
  auto meta=Read<BYTE*>(*skeleton,8);auto bones=Read<Matrix*>(*skeleton,0x10);if(!meta||!bones)return false;
  const int ids[]={0,11,12,13,14,15,16,17,18,19,20,30,31,32,33,34,35,45,46,47,64,71};
  const short parents[]={-1,0,11,12,13,14,15,16,17,18,19,13,30,31,32,33,34,13,45,46,34,34};
  for(unsigned i=0;i<std::size(ids);++i)if(Read<int>(meta,ids[i]*0x24)!=ids[i]||Read<short>(meta,ids[i]*0x24+6)!=parents[i])return false;
  for(int wrist:{20,35})for(int group=0;group<3;++group)for(int joint=0;joint<3;++joint){
   const int id=wrist+1+group*3+joint,parent=joint?id-1:wrist;
   if(Read<int>(meta,id*0x24)!=id||Read<short>(meta,id*0x24+6)!=parent)return false;
  }
  std::memcpy(native->data(),bones,sizeof(*native));return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void RestoreRemoteArms(void* soldier){
 if(!enabled||ownerThread!=GetCurrentThreadId()||localId>255||actors[localId].soldier==soldier)return;
 for(unsigned id=0;id<256;++id){
  if(actors[id].soldier!=soldier)continue;auto& binding=bindings[id];
  if(!binding.overridePose||!binding.overridePose->valid)return;
  BodyBones current;void* skeleton=nullptr;
  if(binding.weak!=actors[id].weak||!Skeleton(soldier,&skeleton,&current)||skeleton!=binding.skeleton){binding.overridePose.reset();return;}
  __try {
   auto bones=Read<Matrix*>(skeleton,0x10);if(!bones)return;
   // Undo only our exact last writes before native animation runs again. The
   // engine can skip or partially refresh remote animation at reduced LOD;
   // feeding our stretched arms back into that animation causes pose drift.
   // Native changes since the last callback always take precedence.
   for(int i=11;i<=74;++i)if(binding.overridePose->written[i])
    if(!std::memcmp(&current[i],&binding.overridePose->applied[i],sizeof(Matrix)))bones[i]=binding.overridePose->native[i];
  }__except(EXCEPTION_EXECUTE_HANDLER){}
  binding.overridePose->valid=false;return;
 }
}
bool HeadTopology(void* soldier,void* skeleton){
 __try {
  if((Read<DWORD>(soldier,0x14)&0x20)||Read<void*>(soldier,0x34)||Read<void*>(soldier,0x28c))return false;
  const auto meta=Read<BYTE*>(skeleton,8);if(!meta)return false;
  for(int id=47;id<64;++id){
   const short parent=id==47?46:id>=49&&id<=51?48:47;
   if(Read<int>(meta,id*0x24)!=id||Read<short>(meta,id*0x24+6)!=parent)return false;
  }
  return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool TorsoTopology(void* skeleton){
 __try {
  const auto meta=Read<BYTE*>(skeleton,8);if(!meta)return false;
  for(int i=64;i<80;++i){const short parent=i<72?34:i==72?13:i<75?12:0;
   if(Read<int>(meta,i*0x24)!=i||Read<short>(meta,i*0x24+6)!=parent)return false;}
  return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool WriteRig(void* skeleton,const RigOverride& pose){
 __try {auto bones=Read<Matrix*>(skeleton,0x10);if(!bones)return false;
  for(int i=11;i<=74;++i)if(pose.written[i])bones[i]=pose.applied[i];return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool FistActorAvailable(void* soldier){
 __try {return soldier&&Read<void*>(soldier,0)==game+ClientProfile.soldierVt&&Read<int>(soldier,0x2f0)==1&&
  !(Read<DWORD>(soldier,0x14)&0x20)&&!Read<void*>(soldier,0x34)&&!Read<void*>(soldier,0x28c);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void UpdateFistBumps(ULONGLONG now){
 if(!InterlockedCompareExchange(&fistActive,0,0)||observerOnly||localId>255||!FistActorAvailable(actors[localId].soldier)){
  for(auto& b:bindings)b.fists={};return;
 }
 Packet local=outgoing;local.session=session;local.body=actors[localId].body;unsigned mask=0;
 for(unsigned id=0;id<256;++id){
  auto& state=bindings[id].fists;const auto p=poses[id].Read(now,150);
  if(id==localId||!p||p->kind!=Relay||actors[id].ai||!FistActorAvailable(actors[id].soldier)||actors[id].name!=p->weaponName){state={};continue;}
  mask|=state.Update(local,*p,now);
 }
 if(mask){InterlockedExchange(&pendingFistTime,LONG(GetTickCount()));InterlockedOr(&pendingFists,LONG(mask));}
}
}
void InstallNetworkClient(LogFunction log){
 if(enabled)return;logger=log;settings=net::LoadSettings();if(!settings.enabled)return;
 game=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));render=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9_ori.dll"));if(!render)render=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9.dll"));
 if(!NativeProfile()||BCryptGenRandom(nullptr,reinterpret_cast<PUCHAR>(&session),sizeof(session),BCRYPT_USE_SYSTEM_PREFERRED_RNG)<0||!session||!transport.Open(false,settings.port)){
  if(log)log("Network VR disabled: profile, random session or loopback socket unavailable.");return;
 }
 const auto target=game+0x1efc00;
 if(MH_CreateHook(target,RemoteFinalizeHook,reinterpret_cast<void**>(&originalRemoteFinalize))!=MH_OK){transport.Close();if(log)log("Network VR disabled: remote-animation hook unavailable.");return;}
 if(MH_EnableHook(target)!=MH_OK){MH_RemoveHook(target);transport.Close();return;}
 enabled=true;if(!InstallRemoteWeaponVisibility(render,log)&&log)log("Remote weapon visibility unavailable: renderer profile mismatch; native weapons retained.");
 if(settings.voiceEnabled){wchar_t receiveOnly[8]{};GetEnvironmentVariableW(L"BF2142VR_VOICE_RECEIVE_ONLY",receiveOnly,8);
  voice::StartClient(settings.voicePort,settings.secret,!observerOnly,receiveOnly[0]==L'1',voice::VoicePreferences(LoadVrSettings(L"")),log);}
 if(log)log("Experimental multiplayer poses enabled on loopback UDP %u; server extension required.",settings.port);
}
void InstallNetworkObserver(LogFunction log){
 if(enabled)return;observerOnly=true;InstallNetworkClient(log);
 if(enabled&&log)log("Flat pose observer connected; authenticated receive subscription only.");
}
bool NetworkClientActive(){return enabled&&!observerOnly;}
unsigned TakeNetworkFistBumps(bool active){
 InterlockedExchange(&fistActive,active?1:0);const auto mask=InterlockedExchange(&pendingFists,0);
 return active&&DWORD(GetTickCount()-DWORD(InterlockedCompareExchange(&pendingFistTime,0,0)))<=100?unsigned(mask):0;
}
void TickNetworkClient(){
 if(!enabled)return;if(!ownerThread)ownerThread=GetCurrentThreadId();if(ownerThread!=GetCurrentThreadId())return;
 const auto now=GetTickCount64();if(now-lastPump<16)return;lastPump=now;
 if(!Roster()){voice::PublishState({});localId=256;outgoingTime=0;for(auto& p:poses)p.Clear();return;}
 if(settings.voiceEnabled){voice::State state;state.player=localId;state.session=session;
  state.alive=actors[localId].soldier&&InverseRigid(actors[localId].body).has_value();
  if(state.alive){state.listener=actors[localId].body;state.listener.values[3][1]+=1.6f;
   if(!observerOnly&&outgoingTime&&now-outgoingTime<=150&&outgoing.player==localId)state.listener=Multiply(outgoing.head,actors[localId].body);}
  voice::PublishState(state);
 }
 // Remote LODs collapse finger bones onto wrists. Calibrate the same 80-bone
 // rig from the local avatar while it still has anatomical finger landmarks.
 if(!referenceValid&&actors[localId].soldier){BodyBones native;void* skeleton=nullptr;
  if(Skeleton(actors[localId].soldier,&skeleton,&native,false)){
   const auto l=CaptureBodyPalm(native,true),r=CaptureBodyPalm(native,false);
   if(l&&r){detailedReference=native;referenceLeft=*l;referenceRight=*r;referenceValid=true;}
  }
 }
 Packet packet;unsigned port=0;
 for(unsigned i=0;i<32&&transport.Receive(&packet,&port,settings.secret);++i){
  if(port!=settings.port||(packet.kind!=Relay&&packet.kind!=Mirror)||packet.player==localId||!actors[packet.player].soldier)continue;
  if(packet.kind==Mirror&&!settings.mirror)continue;
  poses[packet.player].Accept(packet,now);
 }
 JoinChallenge challenge;
 if(transport.TakeJoinChallenge(challenge)&&proofPolicy.Accept(challenge,localId,session,now)){
  if(!SendNativeJoinProof(game,actors[localId].player,challenge.nonce)&&logger)logger("Community join proof unavailable for this native profile.");
 }
 if(localId<256&&events.weapon&&events.weapon!=actors[localId].weapon){events.throwTime=0;events.weapon=nullptr;}
 for(unsigned i=0;i<256;++i)if(bindings[i].weak!=actors[i].weak){bindings[i]={};bindings[i].weak=actors[i].weak;poses[i].Clear();}
 if(observerOnly){
  if(now-lastSend>=250){Packet request;request.kind=Subscribe;request.player=localId;
   request.sequence=++sequence;request.session=session;request.secret=settings.secret;
   if(transport.Send(request,settings.port))lastSend=now;
  }
  return;
 }
 if(outgoingTime&&now-outgoingTime<=150&&now-lastSend>=30&&localId==outgoing.player&&actors[localId].weapon&&actors[localId].name==outgoing.weaponName){
  outgoing.snapSerial=outgoing.throwSerial=0;outgoing.snapDegrees=0;AttachEvents(outgoing,now);
  outgoing.sequence=++sequence;outgoing.session=session;outgoing.secret=settings.secret;
  if(Validate(outgoing,settings.secret)&&transport.Send(outgoing,settings.port)){lastSend=now;if(!reportedSend){reportedSend=true;logger("Network VR sending player %u: head, palms and held-weapon pose.",localId);}}
 }
}
void PublishNetworkPose(void* soldier,void* weapon,const net::Matrix& body,const net::Matrix& camera,const net::Matrix& head,const net::Matrix& leftPalm,const net::Matrix& rightPalm,const net::Matrix& weaponLocal,bool leftValid,bool held,const std::array<std::array<float,5>,2>& curls,bool leftCrate){
 if(!enabled||observerOnly||ownerThread!=GetCurrentThreadId()||localId>255||actors[localId].soldier!=soldier||actors[localId].weapon!=weapon)return;
 Packet p;p.player=localId;p.flags=RightValid|(leftValid?LeftValid:0)|(held?WeaponHeld:0)|(leftCrate?LeftCrateHeld:0);p.body=body;p.camera=camera;p.head=head;p.left=leftPalm;p.right=rightPalm;p.weapon=(held||leftCrate)?weaponLocal:rightPalm;p.weaponName=actors[localId].name;
 if(ReadNativeMovementYaw(soldier,&p.movementYawDegrees))p.flags|=MovementValid;
 for(int i=0;i<2;++i)for(int j=0;j<5;++j)p.curls[i*5+j]=curls[i][j];outgoing=p;outgoingTime=GetTickCount64();UpdateFistBumps(outgoingTime);
}
void PublishNetworkSnap(void* soldier,float degrees){
 if(!enabled||observerOnly||ownerThread!=GetCurrentThreadId()||localId>255||actors[localId].soldier!=soldier||!std::isfinite(degrees)||std::abs(degrees)<15||std::abs(degrees)>90)return;
 if(events.soldier!=soldier){events={};events.soldier=soldier;}
 if(!++nextSnap)++nextSnap;events.snap=degrees;events.snapSerial=nextSnap;events.snapTime=GetTickCount64();lastSend=0;
}
void PublishNetworkCrateThrow(void* soldier,void* weapon,const Matrix& launch,stereo::Vec3 velocity){
 if(!enabled||observerOnly||ownerThread!=GetCurrentThreadId()||localId>255||actors[localId].soldier!=soldier||actors[localId].weapon!=weapon)return;
 if(events.soldier!=soldier){events={};events.soldier=soldier;}
 if(!++nextThrow)++nextThrow;events.weapon=weapon;events.launch=launch;events.velocity=velocity;
 events.throwSerial=nextThrow;events.throwTime=GetTickCount64();lastSend=0;
}
bool HideRemoteWeaponGeometry(void* geometry){
 if(!enabled||ownerThread!=GetCurrentThreadId()||localId>255||!geometry)return false;
 __try {
  if(Read<void*>(geometry,0)!=render+0x1d5400)return false;
  const auto weapon=Read<void*>(geometry,0x290);
  if(!weapon||Read<void*>(weapon,0)!=game+ClientProfile.weaponVt||Read<void*>(weapon,0x44)!=geometry)return false;
  const auto soldier=Read<void*>(weapon,0x34);
  if(!soldier||soldier==actors[localId].soldier||Read<void*>(soldier,0)!=game+ClientProfile.soldierVt||
     (Read<DWORD>(soldier,0x14)&0x20)||Read<void*>(soldier,0x34)||Read<void*>(soldier,0x28c))return false;
  const int index=Read<int>(soldier,0x218);const auto begin=Read<BYTE*>(soldier,0x234),end=Read<BYTE*>(soldier,0x238);
  if(index<=0||index>=32||!begin||end<begin||std::size_t(end-begin)>128||std::size_t(index*4)>=std::size_t(end-begin)||Read<void*>(begin,index*4)!=weapon)return false;
  const auto now=GetTickCount64();
  for(unsigned id=0;id<256;++id){
   const auto& a=actors[id];if(a.soldier!=soldier||a.weapon!=weapon)continue;
   if(!a.player||!a.weak||Read<void*>(a.player,0)!=game+ClientProfile.playerVt||Read<void*>(a.player,0xcc)!=a.weak||Read<void*>(a.weak,4)!=soldier)return false;
   const auto p=poses[id].Read(now);
   if(!p||(p->flags&(WeaponHeld|LeftCrateHeld))||!(p->flags&(LeftValid|RightValid)))return false;
   if(p->kind==Mirror)return settings.mirror&&a.ai;
   std::array<char,48> name{};
   return p->kind==Relay&&!a.ai&&WeaponName(weapon,game,ClientProfile,&name)&&p->weaponName==name;
  }
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
 return false;
}
void ApplyRemoteNetworkPose(void* soldier){
 if(!enabled||GetCurrentThreadId()!=ownerThread||localId>255||actors[localId].soldier==soldier)return;
 const auto now=GetTickCount64();
 for(unsigned id=0;id<256;++id){
  if(actors[id].soldier!=soldier)continue;const auto p=poses[id].Read(now);if(!p){bindings[id].presentation={};bindings[id].continuity={};return;}
  if(p->kind==Relay&&(!actors[id].weapon||actors[id].name!=p->weaponName))return;
  BodyBones native;void* skeleton=nullptr;if(!Skeleton(soldier,&skeleton,&native))return;auto& b=bindings[id];
  if(b.skeleton!=skeleton){b={};b.weak=actors[id].weak;b.skeleton=skeleton;}
  if(!b.valid){auto l=CaptureBodyPalm(native,true),r=CaptureBodyPalm(native,false);
   if(referenceValid){if(!l)l=referenceLeft;if(!r)r=referenceRight;}
   if(l&&r){b.left=*l;b.right=*r;b.valid=true;}}
  const auto previousTime=b.presentation.time;
  auto displayed=b.presentation.Update(*p,now);if(b.presentation.reset)b.continuity={};
  const bool safeHead=HeadTopology(soldier,skeleton);
  auto torso=safeHead&&TorsoTopology(skeleton)?SolveRemoteTorso(native,displayed):std::optional<BodyBones>{};
  const auto& source=torso?*torso:native;
  if(torso)displayed=ConstrainRemoteHands(source,displayed);
  const float elapsed=previousTime&&now>=previousTime?float(now-previousTime)*.001f:0.f;
  auto solved=b.valid?SolveRemoteArms(source,displayed,b.left,b.right,referenceValid?&detailedReference:nullptr,
   displayed.kind==Relay?&b.continuity:nullptr,elapsed):std::optional<BodyBones>{};
  const bool arms=solved.has_value();
  if(!arms&&!b.reportedSolveSkip&&logger){b.reportedSolveSkip=true;logger("Network VR remote player %u: arm solve unavailable; native arms retained.",id);}
  if(!solved&&torso)solved=torso;
  auto head=safeHead?SolveRemoteHead(solved?*solved:native,displayed.head):std::optional<BodyBones>{};
  if(head)solved=*head;
  if(solved){
   if(!b.overridePose)b.overridePose.reset(new(std::nothrow) RigOverride);
   if(!b.overridePose)return;b.overridePose->native=native;b.overridePose->applied=*solved;b.overridePose->valid=false;
   for(int i=0;i<80;++i)b.overridePose->written[i]=i>=11&&i<=74&&std::memcmp(&native[i],&(*solved)[i],sizeof(Matrix))!=0;
  }
  if(solved&&WriteRig(skeleton,*b.overridePose)){
   b.overridePose->valid=true;
   if(arms&&!b.reported){b.reported=true;if(logger)logger("Network VR third-person arm IK active for player %u%s.",id,p->kind==Mirror?" (explicit local mirror test)":"");}
   if(head&&!b.reportedHead){b.reportedHead=true;if(logger)logger("Network VR remote HMD head rotation active for player %u.",id);}
  }return;
 }
}
}
