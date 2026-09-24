#include "NativeRoster.h"
#include "LoopbackTransport.h"
#include "../GrenadeArc.h"
#include <MinHook.h>
#include <cstdio>
#include <cstdarg>
#include <limits>
#include <intrin.h>
#include <cmath>
#include <string_view>
namespace bfvr::bf2142::net {
namespace {
BYTE* game=nullptr;void* manager=nullptr;HANDLE logFile=INVALID_HANDLE_VALUE;Settings settings;Transport transport;
DWORD ownerThread=0;ULONGLONG lastPump=0;std::array<Actor,256> actors{};
struct Peer {FreshPose pose;void* weak=nullptr;unsigned port=0,mirror=256;bool reported=false;
 EventWindow snaps,throws;float pendingSnap=0;ULONGLONG snapTime=0,throwTime=0;
 Packet pendingThrow{};void* throwWeapon=nullptr;
};
std::array<Peer,256> peers{};
using Constructor=void*(__thiscall*)(void*);Constructor originalConstructor=nullptr;
using Lookup=void*(__thiscall*)(void*,int);Lookup originalLookup=nullptr;
using List=void*(__thiscall*)(void*);List originalList=nullptr;
using Launch=const Matrix*(__thiscall*)(void*);Launch originalLaunch=nullptr,originalCrateLaunch=nullptr;
using LookDelta=float(__thiscall*)(void*);LookDelta originalLook=nullptr;
using Fire=void*(__thiscall*)(void*,const Matrix*,const Matrix*,const stereo::Vec3*);Fire originalFire=nullptr;
thread_local Matrix mappedLaunch{},mappedGun{};thread_local void* mappedReceiver=nullptr;thread_local unsigned mappedId=256,mappedSequence=0;
unsigned shots=0,snaps=0,crateShots=0;
thread_local stereo::Vec3 mappedVelocity{};thread_local std::uint32_t mappedThrowSerial=0;
bool Crate(const Actor& a){return !std::strcmp(a.name.data(),"unl_hub_medic")||!std::strcmp(a.name.data(),"unl_hub_ammo");}
bool EventProfile(){
 const BYTE look[]={0x8b,0x81,0x18,2,0,0,0x83,0xf8,0xff,0x74,0x3f,0x8b,0x91,0x88,1,0,0};
 const BYTE tail[]={0xff,0xa0,0x20,1,0,0};
 const BYTE caller[]={0x8b,0xcb,0xe8,0xc2,0xbc,0xff,0xff,0xd8,0x45,8,0x8b,0xcb,0xd9,0x5d,8};
 const BYTE crate[]={0x55,0x8b,0xec,0x81,0xec,0x90,0,0,0,0x53,0x56,0x57,0x8b,0xf9,0x8b,0x77,0xfc};
 return !memcmp(game+0x129160,look,sizeof(look))&&!memcmp(game+0x1291a3,tail,sizeof(tail))&&
  !memcmp(game+0x12d497,caller,sizeof(caller))&&Read<void*>(game+ServerProfile.soldierVt,63*4)==game+0x12ce70&&
  !memcmp(game+0x18c720,crate,sizeof(crate))&&Read<void*>(game+0x3dbdf0,0xd4)==game+0x18c720&&Read<void*>(game+0x3dbf90,0xd4)==game+0x18c720;
}
void Log(const char* format,...){
 if(logFile==INVALID_HANDLE_VALUE)return;char line[1024];va_list args;va_start(args,format);int n=vsnprintf_s(line,sizeof(line)-2,_TRUNCATE,format,args);va_end(args);if(n<0)n=int(strlen(line));line[n++]='\r';line[n++]='\n';DWORD written=0;WriteFile(logFile,line,n,&written,nullptr);
}
bool ProfileValid(){
 __try {
  const auto dos=reinterpret_cast<IMAGE_DOS_HEADER*>(game);if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>4096)return false;
  const auto nt=reinterpret_cast<IMAGE_NT_HEADERS*>(game+dos->e_lfanew);if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||nt->OptionalHeader.SizeOfImage<0x3e4000)return false;
  const BYTE constructor[]={0x55,0x8b,0xec,0x51,0x53,0x56,0x8b,0xf1,0x57,0xc7,0x06};
  const BYTE lookup[]={0x55,0x8b,0xec,0x51,0x56,0x8b,0xf1,0x8d,0x45,0x08,0x50,0x8d,0x4d,0xfc,0x51,0x8d,0x4e,0x54};
  const BYTE list[]={0x8d,0x41,0x0c,0xc3};
  const BYTE launch[]={0x55,0x8b,0xec,0x83,0xec,0x10,0x53,0x56,0x8b,0xf1,0x8b,0x4e,0xfc,0x57};
  const BYTE fire[]={0x55,0x8b,0xec,0x83,0xec,0x28,0x53,0x56,0x8b,0xf1,0x8b,0x4e,0x0c,0x8b,0x81,0xa0,1,0,0,0x57};
  return EventProfile()&&!memcmp(game+0x3f540,constructor,sizeof(constructor))&&Read<void*>(game,0x3f54b)==game+ServerProfile.managerVt&&
   !memcmp(game+0x3e6f0,lookup,sizeof(lookup))&&!memcmp(game+0x3dd50,list,sizeof(list))&&Read<void*>(game+ServerProfile.managerVt,0x20)==game+0x3e6f0&&Read<void*>(game+ServerProfile.managerVt,0x40)==game+0x3dd50&&!memcmp(game+0x1999d0,launch,sizeof(launch))&&!memcmp(game+0x19a8b0,fire,sizeof(fire))&&
   Read<void*>(game+ServerProfile.managerVt,0x30)==game+0x3dd70&&Read<void*>(game+ServerProfile.fireVt,0xd4)==game+0x1999d0;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void Pump(){
 if(!manager||GetCurrentThreadId()!=ownerThread)return;const auto now=GetTickCount64();if(now-lastPump<15)return;lastPump=now;
 if(!ReadRoster(manager,game,ServerProfile,&actors)){for(auto& p:peers)p={};return;}
 for(unsigned i=0;i<256;++i)if(peers[i].weak!=actors[i].weak||!actors[i].soldier){peers[i]={};peers[i].weak=actors[i].weak;}
 Packet packet;unsigned port=0;
 for(unsigned work=0;work<32&&transport.Receive(&packet,&port,settings.secret);++work){
  if(packet.kind!=Pose)continue;const auto id=packet.player;auto& actor=actors[id];auto& peer=peers[id];
  if(!actor.soldier||!actor.weapon||actor.ai||packet.weaponName!=actor.name||!InverseRigid(actor.body)||Distance(packet.body,actor.body)>3.f)continue;
  if(peer.port&&peer.port!=port&&peer.pose.Read(now,1000))continue;
  if(!peer.pose.Accept(packet,now))continue;peer.port=port;
  if(packet.snapSerial&&peer.snaps.Accept(packet.session,packet.snapSerial,now,120)){peer.pendingSnap=packet.snapDegrees;peer.snapTime=now;}
  if(packet.throwSerial&&peer.throws.Accept(packet.session,packet.throwSerial,now,200)){peer.pendingThrow=packet;peer.throwTime=now;peer.throwWeapon=actor.weapon;}
  if(!peer.reported){peer.reported=true;Log("Pose channel accepted live human player %u; loopback peer port %u.",id,port);}
  auto relay=packet;relay.kind=Relay;relay.body=actor.body;
  for(unsigned other=0;other<256;++other)if(other!=id&&peers[other].port&&peers[other].pose.Read(now,1000))transport.Send(relay,peers[other].port);
  if(settings.mirror){
   if(peer.mirror>255||!actors[peer.mirror].soldier||!actors[peer.mirror].weapon||Distance(actor.body,actors[peer.mirror].body)>60.f){
    unsigned best=256;float distance=std::numeric_limits<float>::max();
    for(unsigned bot=0;bot<256;++bot)if(actors[bot].ai&&actors[bot].soldier&&actors[bot].weapon){const float d=Distance(actor.body,actors[bot].body);if(d<distance){distance=d;best=bot;}}
    if(best!=peer.mirror){peer.mirror=best;if(best<256)Log("Explicit one-PC pose mirror: human %u -> bot %u (%.1f m).",id,best,distance);}
   }
   if(peer.mirror<256){relay.kind=Mirror;relay.player=peer.mirror;relay.body=actors[peer.mirror].body;transport.Send(relay,port);}
  }
 }
}
void* __fastcall ConstructorHook(void* self,void*){void* result=originalConstructor(self);manager=self;ownerThread=GetCurrentThreadId();actors={};peers={};lastPump=0;Log("Dedicated PlayerManager captured on the game thread.");return result;}
// Dedicated servers skip render/animation finalization. These ordinary manager
// queries run during player input, AI and scripting; native results are retained.
void* __fastcall LookupHook(void* self,void*,int id){void* result=originalLookup(self,id);if(self==manager)Pump();return result;}
void* __fastcall ListHook(void* self,void*){void* result=originalList(self);if(self==manager)Pump();return result;}
bool Owner(void* receiver,const Actor& actor){
 __try {
  return actor.player&&actor.soldier&&actor.weapon&&Read<void*>(actor.player,0xcc)==actor.weak&&Read<void*>(actor.weak,4)==actor.soldier&&
   Read<void*>(actor.weapon,0x34)==actor.soldier&&Read<void*>(receiver,0xc)==actor.weapon&&
   (Read<void*>(receiver,0x10)==game+ServerProfile.fireVt||(Crate(actor)&&(Read<void*>(receiver,0x10)==game+0x3dbdf0||Read<void*>(receiver,0x10)==game+0x3dbf90)))&&Read<BYTE*>(actor.weapon,0x1b4)==static_cast<BYTE*>(receiver)+0x10;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
const Packet* PoseFor(void* receiver,unsigned* id){
 if(GetCurrentThreadId()!=ownerThread)return nullptr;
 const auto now=GetTickCount64();for(unsigned i=0;i<256;++i)if(const auto pose=peers[i].pose.Read(now)){
  if(Owner(receiver,actors[i])&&pose->weaponName==actors[i].name){*id=i;return pose;}
 }return nullptr;
}
bool Foot(const Actor& a,void* soldier){
 __try{return !a.ai&&a.weapon&&a.soldier==soldier&&a.weak&&Read<void*>(a.player,0xcc)==a.weak&&Read<void*>(a.weak,4)==soldier&&
  !(Read<DWORD>(soldier,0x14)&0x20)&&!Read<void*>(soldier,0x34)&&!Read<void*>(soldier,0x28c);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
float ServerLookInput(void* soldier,const void* caller,float native){
 if(caller!=game+0x12d49e||GetCurrentThreadId()!=ownerThread)return native;
 Pump();const auto now=GetTickCount64();
 for(unsigned id=0;id<256;++id)if(actors[id].soldier==soldier){
  auto& p=peers[id];const float turn=p.pendingSnap;p.pendingSnap=0;
  if(!turn||!Foot(actors[id],soldier)||!p.pose.Read(now)||now<p.snapTime||now-p.snapTime>150)return native;
  if(snaps++<24)Log("Authoritative snap turn: player=%u event=%u degrees=%.1f.",id,p.snaps.serial,turn);
  return native+turn;
 }return native;
}
float __fastcall LookHook(void* soldier,void*){const auto caller=_ReturnAddress();return ServerLookInput(soldier,caller,originalLook(soldier));}
bool CopyLaunch(const Matrix* p,Matrix* out){__try{if(!p)return false;*out=*p;return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
const Matrix* __fastcall LaunchHook(void* self,void*){
 Pump();const auto native=originalLaunch(self);mappedReceiver=nullptr;mappedId=256;mappedThrowSerial=0;
 unsigned id=256;void* receiver=static_cast<BYTE*>(self)-0x10;const auto p=PoseFor(receiver,&id);Matrix source{};
 if(!p||!(p->flags&WeaponHeld)||!CopyLaunch(native,&source))return native;
 const auto gun=Multiply(p->weapon,actors[id].body),camera=Multiply(p->camera,actors[id].body);
 const auto result=std::string_view(p->weaponName.data())=="unl_grenade_frag"?std::optional<Matrix>(TrackedFragLaunch(gun)):MapTrackedFire(source,camera,gun);
 if(!result||!InverseRigid(*result))return native;
 mappedLaunch=*result;mappedGun=gun;mappedReceiver=receiver;mappedId=id;mappedSequence=p->sequence;return &mappedLaunch;
}
const Matrix* __fastcall CrateLaunchHook(void* self,void*){
 Pump();const auto native=originalCrateLaunch(self);mappedReceiver=nullptr;mappedId=256;mappedThrowSerial=0;
 unsigned id=256;auto receiver=static_cast<BYTE*>(self)-0x10;const auto pose=PoseFor(receiver,&id);
 if(!pose||!(pose->flags&LeftCrateHeld)||!Crate(actors[id]))return native;
 auto& peer=peers[id];const auto now=GetTickCount64();
 if(!peer.throwTime||now<peer.throwTime||now-peer.throwTime>700||peer.throwWeapon!=actors[id].weapon||!peer.pendingThrow.throwSerial)return native;
 mappedLaunch=Multiply(peer.pendingThrow.throwLaunch,actors[id].body);
 mappedVelocity=TransformVelocity(peer.pendingThrow.throwVelocity,actors[id].body);
 if(!InverseRigid(mappedLaunch))return native;
 mappedGun=mappedLaunch;mappedReceiver=receiver;mappedId=id;mappedThrowSerial=peer.pendingThrow.throwSerial;return &mappedLaunch;
}
void* __fastcall FireHook(void* receiver,void*,const Matrix* launch,const Matrix* parent,const stereo::Vec3* velocity){
 if(launch==&mappedLaunch&&mappedReceiver==receiver&&mappedId<256&&Owner(receiver,actors[mappedId])){
  if(mappedThrowSerial){
   auto& peer=peers[mappedId];const auto now=GetTickCount64();
   if(peer.pendingThrow.throwSerial==mappedThrowSerial&&peer.throwTime&&now>=peer.throwTime&&now-peer.throwTime<=700&&peer.throwWeapon==actors[mappedId].weapon&&Crate(actors[mappedId])){
    peer.pendingThrow.throwSerial=0;peer.throwTime=0;mappedThrowSerial=0;
    if(crateShots++<24)Log("Authoritative left-hand support throw: player=%u event=%u speed=(%.2f,%.2f,%.2f).",mappedId,peer.throws.serial,mappedVelocity.x,mappedVelocity.y,mappedVelocity.z);
    return originalFire(receiver,&mappedLaunch,&mappedGun,&mappedVelocity);
   }
   mappedThrowSerial=0;return originalFire(receiver,launch,parent,velocity);
  }
  if(shots<12||shots%120==0)Log("Tracked server shot %u: player=%u seq=%u origin=(%.3f,%.3f,%.3f) forward=(%.4f,%.4f,%.4f).",shots,mappedId,mappedSequence,
   mappedLaunch.values[3][0],mappedLaunch.values[3][1],mappedLaunch.values[3][2],mappedLaunch.values[2][0],mappedLaunch.values[2][1],mappedLaunch.values[2][2]);
  ++shots;return originalFire(receiver,launch,&mappedGun,velocity);
 }
 return originalFire(receiver,launch,parent,velocity);
}
}
}
extern "C" __declspec(dllexport) DWORD WINAPI BF2142VRInitialize(void* parameter){
 using namespace bfvr::bf2142::net;
 if(!parameter||game)return 0;game=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));settings=LoadSettings();
 logFile=CreateFileW(static_cast<const wchar_t*>(parameter),GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
 if(logFile==INVALID_HANDLE_VALUE)return 0;
 if(!settings.enabled||!ProfileValid()||!transport.Open(true,settings.port)){Log("Server extension unavailable: explicit configuration, verified profile or loopback port missing.");return 0;}
 if(MH_Initialize()!=MH_OK)return 0;
 void* targets[]={game+0x3f540,game+0x3e6f0,game+0x3dd50,game+0x1999d0,game+0x19a8b0,game+0x129160,game+0x18c720};
 void* hooks[]={reinterpret_cast<void*>(ConstructorHook),reinterpret_cast<void*>(LookupHook),reinterpret_cast<void*>(ListHook),reinterpret_cast<void*>(LaunchHook),reinterpret_cast<void*>(FireHook),reinterpret_cast<void*>(LookHook),reinterpret_cast<void*>(CrateLaunchHook)};
 void** originals[]={reinterpret_cast<void**>(&originalConstructor),reinterpret_cast<void**>(&originalLookup),reinterpret_cast<void**>(&originalList),reinterpret_cast<void**>(&originalLaunch),reinterpret_cast<void**>(&originalFire),reinterpret_cast<void**>(&originalLook),reinterpret_cast<void**>(&originalCrateLaunch)};
 const unsigned count=unsigned(std::size(targets));unsigned created=0;for(;created<count;++created)if(MH_CreateHook(targets[created],hooks[created],originals[created])!=MH_OK)break;
 if(created!=count){for(unsigned i=0;i<created;++i)MH_RemoveHook(targets[i]);return 0;}
 for(unsigned i=0;i<count;++i)if(MH_EnableHook(targets[i])!=MH_OK){for(void* t:targets){MH_DisableHook(t);MH_RemoveHook(t);}return 0;}
 Log("BF2142VR experimental dedicated extension ready: authoritative tracked fire, snap turns, support throws and pose relay (protocol v2); loopback UDP %u; mirror=%u.",settings.port,settings.mirror?1:0);return 1;
}
