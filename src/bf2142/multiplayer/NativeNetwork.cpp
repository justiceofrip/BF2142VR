#include "NativeNetwork.h"
#include "NativeRoster.h"
#include "LoopbackTransport.h"
#include "RemoteArmMath.h"
#include <bcrypt.h>
#include <MinHook.h>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
using namespace net;
Settings settings;Transport transport;BYTE* game=nullptr;BYTE* render=nullptr;LogFunction logger=nullptr;
using RemoteFinalize=void(__thiscall*)(void*,float,unsigned);RemoteFinalize originalRemoteFinalize=nullptr;
void __fastcall RemoteFinalizeHook(void* soldier,void*,float delta,unsigned update){originalRemoteFinalize(soldier,delta,update);TickNetworkClient();ApplyRemoteNetworkPose(soldier);}
bool enabled=false,reportedSend=false;DWORD ownerThread=0;ULONGLONG lastPump=0,lastSend=0;
std::uint64_t session=0;std::uint32_t sequence=0;std::array<Actor,256> actors{};std::array<FreshPose,256> poses{};
struct Binding {void* weak=nullptr;void* skeleton=nullptr;PalmBinding left{},right{};bool valid=false,reported=false,reportedSolveSkip=false;};
BodyBones detailedReference{};PalmBinding referenceLeft{},referenceRight{};bool referenceValid=false;
std::array<Binding,256> bindings{};Packet outgoing{};ULONGLONG outgoingTime=0;unsigned localId=256;
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
  std::memcpy(native->data(),bones,sizeof(*native));return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool WriteRig(void* skeleton,const BodyBones& solved){
 __try {auto bones=Read<Matrix*>(skeleton,0x10);if(!bones)return false;
  std::memcpy(bones+14,solved.data()+14,sizeof(Matrix)*31);
  std::memcpy(bones+64,solved.data()+64,sizeof(Matrix)*8);return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
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
 enabled=true;if(log)log("Experimental multiplayer poses enabled on loopback UDP %u; server extension required.",settings.port);
}
void TickNetworkClient(){
 if(!enabled)return;if(!ownerThread)ownerThread=GetCurrentThreadId();if(ownerThread!=GetCurrentThreadId())return;
 const auto now=GetTickCount64();if(now-lastPump<16)return;lastPump=now;
 if(!Roster()){localId=256;outgoingTime=0;for(auto& p:poses)p.Clear();return;}
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
  if(port!=settings.port||packet.kind==Pose||packet.player==localId||!actors[packet.player].soldier)continue;
  if(packet.kind==Mirror&&!settings.mirror)continue;
  poses[packet.player].Accept(packet,now);
 }
 for(unsigned i=0;i<256;++i)if(bindings[i].weak!=actors[i].weak){bindings[i]={};bindings[i].weak=actors[i].weak;poses[i].Clear();}
 if(outgoingTime&&now-outgoingTime<=150&&now-lastSend>=30&&localId==outgoing.player&&actors[localId].weapon){
  outgoing.sequence=++sequence;outgoing.session=session;outgoing.secret=settings.secret;
  if(Validate(outgoing,settings.secret)&&transport.Send(outgoing,settings.port)){lastSend=now;if(!reportedSend){reportedSend=true;logger("Network VR sending player %u: head, palms and held-weapon pose.",localId);}}
 }
}
void PublishNetworkPose(void* soldier,void* weapon,const net::Matrix& body,const net::Matrix& camera,const net::Matrix& head,const net::Matrix& leftPalm,const net::Matrix& rightPalm,const net::Matrix& weaponLocal,bool leftValid,bool held,const std::array<std::array<float,5>,2>& curls){
 if(!enabled||ownerThread!=GetCurrentThreadId()||localId>255||actors[localId].soldier!=soldier||actors[localId].weapon!=weapon)return;
 Packet p;p.player=localId;p.flags=RightValid|(leftValid?LeftValid:0)|(held?WeaponHeld:0);p.body=body;p.camera=camera;p.head=head;p.left=leftPalm;p.right=rightPalm;p.weapon=held?weaponLocal:rightPalm;p.weaponName=actors[localId].name;
 for(int i=0;i<2;++i)for(int j=0;j<5;++j)p.curls[i*5+j]=curls[i][j];outgoing=p;outgoingTime=GetTickCount64();
}
void ApplyRemoteNetworkPose(void* soldier){
 if(!enabled||GetCurrentThreadId()!=ownerThread||localId>255||actors[localId].soldier==soldier)return;
 const auto now=GetTickCount64();
 for(unsigned id=0;id<256;++id){
  if(actors[id].soldier!=soldier)continue;const auto p=poses[id].Read(now);if(!p)return;
  BodyBones native;void* skeleton=nullptr;if(!Skeleton(soldier,&skeleton,&native))return;auto& b=bindings[id];
  if(b.skeleton!=skeleton){b={};b.weak=actors[id].weak;b.skeleton=skeleton;}
  if(!b.valid){auto l=CaptureBodyPalm(native,true),r=CaptureBodyPalm(native,false);
   if(referenceValid){if(!l)l=referenceLeft;if(!r)r=referenceRight;}
   if(!l||!r)return;b.left=*l;b.right=*r;b.valid=true;}
  const auto solved=SolveRemoteArms(native,*p,b.left,b.right,referenceValid?&detailedReference:nullptr);if(!solved){if(!b.reportedSolveSkip&&logger){b.reportedSolveSkip=true;logger("Network VR remote player %u: rig bound, pose outside arm solver limits; native animation retained.",id);}return;}
  if(WriteRig(skeleton,*solved)&&!b.reported){b.reported=true;if(logger)logger("Network VR third-person arm IK active for player %u%s.",id,p->kind==Mirror?" (explicit local mirror test)":"");}return;
 }
}
}
