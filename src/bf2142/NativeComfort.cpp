#include "NativeComfort.h"
#include "StockTurnAxis.h"
#include "NativeLookPitch.h"
#include "ComfortCamera.h"
#include "MovementFrame.h"
#include "ComfortControls.h"
#include "TrackingMath.h"
#include <MinHook.h>
#include <intrin.h>
#include <cmath>
#include <cstring>
#include <initializer_list>
namespace bfvr::bf2142 {
namespace {
BYTE* game=nullptr;BYTE* render=nullptr;
using RecoilGetter=float(__thiscall*)(void*);
RecoilGetter nativeRecoil=nullptr;
using MoveCameraGetter=const stereo::Matrix4*(__thiscall*)(void*);
MoveCameraGetter nativeMoveCamera=nullptr;bool movementInstalled=false;
struct MovementSample {std::uint64_t owner=0;ULONGLONG tick=0;float trackedYaw=0;} movement;
thread_local stereo::Matrix4 movementCamera{};
SRWLOCK lock=SRWLOCK_INIT;
ComfortYaw yaw;NativeTurnPulse turnPulse;TraversalView traversalView;bool traversalProfile=false;
bool physicalCamera=false;PhysicalCameraHeight physicalHeight;
bool installed=false;volatile LONG reported=0;
using InputBatch=void(__thiscall*)(void*,int,float);
using LookScale=float(__thiscall*)(void*);
InputBatch nativeInputBatch=nullptr;LookScale nativeLookScale=nullptr;
bool stockTurnInstalled=false,stockPitchInstalled=false;
struct PitchSample {std::uint64_t owner=0;ULONGLONG tick=0;float degrees=0;} lookPitch;
LogFunction logLine=nullptr;
template<class T>T Read(const void* p,size_t offset){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+offset,sizeof(v));return v;}
bool Match(const BYTE* p,std::initializer_list<int> values){size_t i=0;for(int v:values){if(v>=0&&p[i]!=v)return false;++i;}return true;}
bool Profile(){
    __try {
        // These sites prove the getter ABI, its exact input-update call, and
        // the independent body/local heading and visual recoil additions.
        return Read<void*>(game+0x566150,63*4)==game+0x18a6d0 &&
            Read<void*>(game+0x5292a0,12*4)==game+0xb7530 &&
            Match(game+0xb7530,{0x8b,0x41,0x6c,0xc3}) &&
            Match(game+0x186140,{0x8b,0x81,0x18,2,0,0,0x83,0xf8,0xff,0x74,0x3f,0x8b,0x91,0x88,1,0,0}) &&
            Match(game+0x186183,{0xff,0xa0,0x20,1,0,0,0x5e,0xd9,5,-1,-1,-1,-1,0xc3}) &&
            Match(game+0x18a6d0,{0x55,0x8b,0xec,0x81,0xec,0xd0,2,0,0,0x8b,0x45,0x14}) &&
            Match(game+0x18acf7,{0x8b,0xcb,0xe8,-1,-1,-1,-1,0xd8,0x45,0xf0,0x8b,0xcb,0xd9,0x5d,0xf0}) &&
            game+0x18acfe+Read<INT32>(game,0x18acfa)==game+0x186140 &&
            Match(game+0x18ae82,{0xd9,0x83,0x80,2,0,0,0xd8,0xe1,0xd9,0x9b,0x80,2,0,0,
                0xd9,0x83,0x78,2,0,0,0xd8,0xe1,0xd9,0x9b,0x78,2,0,0}) &&
            Match(game+0x18adc5,{0xd9,0x45,0xf0,0xd8,0x83,0x78,2,0,0}) &&
            Match(game+0x18aea4,{0xd9,0x45,0xd8,0xd8,0x83,0x5c,2,0,0,0xd9,0x93,0x5c,2,0,0}) &&
            Match(game+0x1890fd,{0xd9,0x86,0x7c,2,0,0,0x8b,0x4f,4,0xd8,0x86,0x78,2,0,0}) &&
            Match(game+0x189111,{0xd8,0x86,0x74,2,0,0,0xd9,0x5d,0xf8}) &&
            Match(game+0x2f6e51,{0x8d,0x86,0xa0,0,0,0,0x5e,0xc3}) &&
            Read<void*>(game+0x566150,30*4)==game+0x2f6e20 &&
            Match(game+0x18b015,{0xc7,0x83,0x6c,2,0,0,0,0,0,0});
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct Infantry {void* soldier=nullptr;void* identity=nullptr;float bodyYaw=0,localYaw=0;stereo::Matrix4 body{};int stance=-1;};
bool ReadInfantry(Infantry* out){
    __try {
        const auto pm=Read<void*>(render,0x221a58);
        if(!pm||Read<void*>(pm,0)!=game+0x5292a0)return false;
        const auto player=Read<void*>(pm,0x6c);
        if(!player||Read<void*>(player,0)!=game+0x566cd8)return false;
        const auto weak=Read<void*>(player,0xcc);if(!weak)return false;
        const auto soldier=Read<void*>(weak,4);
        if(!soldier||Read<void*>(soldier,0)!=game+0x566150 || (Read<DWORD>(soldier,0x14)&0x20) ||
            Read<void*>(soldier,0x34) || Read<void*>(soldier,0x28c) || Read<int>(soldier,0x2f0)!=0)return false;
        *out={soldier,weak,Read<float>(soldier,0x25c),Read<float>(soldier,0x278),Read<stereo::Matrix4>(soldier,0xa0),Read<int>(soldier,0x26c)};
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
std::uint64_t TurnOwner(const Infantry& owner){return (std::uint64_t(reinterpret_cast<std::uintptr_t>(owner.identity))<<32)|reinterpret_cast<std::uintptr_t>(owner.soldier);}
bool Focused(){DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);return pid==GetCurrentProcessId();}
float LocalLookInput(void* soldier,const void* caller,float result){
    // Observe actual recoil only at the local infantry input caller. Snap yaw
    // lives in its generated action, so comfort never subtracts intentional turns.
    if(caller==game+0x18acfe){
        Infantry owner{};
        if(ReadInfantry(&owner) && owner.soldier==soldier){
            AcquireSRWLockExclusive(&lock);
            yaw.Bind(reinterpret_cast<std::uintptr_t>(owner.identity));yaw.AddRecoil(result);
            ReleaseSRWLockExclusive(&lock);
            // Recoil stays native. Intentional yaw is already in the stock
            // action shared by local prediction and every multiplayer server.
            return result;
        }
    }
    return result;
}
// The verified input manager builds a ring of 0x118-byte actions. Insert
// into its first newly built action after all control maps have contributed,
// before GetAction, native prediction or the compact network codec can read it.
bool ApplyStockTurn(void* action,float factor){
 Infantry owner{};
 const bool valid=installed&&stockTurnInstalled&&Focused()&&ReadInfantry(&owner);
 AcquireSRWLockExclusive(&lock);
 const float degrees=turnPulse.Consume(valid?TurnOwner(owner):0,valid,GetTickCount64());
 ReleaseSRWLockExclusive(&lock);
 if(!degrees||!action)return false;
 __try {
  const auto mask=Read<DWORD>(action,0x100);
  const auto value=StockTurnAxis(mask&0x10?Read<float>(action,0x10):0.f,degrees,factor);
  if(!value)return false;
  std::memcpy(static_cast<BYTE*>(action)+0x10,&*value,sizeof(float));
  const DWORD present=mask|0x10;std::memcpy(static_cast<BYTE*>(action)+0x100,&present,sizeof(present));
  return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ReadInputBatch(void* input,int* sequence,int* capacity,BYTE** actions){
 __try {
  if(!input||Read<void*>(input,0)!=game+0x58de40||Read<int>(input,0x38)!=0)return false;
  *sequence=Read<int>(input,0x24);*capacity=Read<int>(input,0x2c);*actions=Read<BYTE*>(input,0x34);
  return *sequence>=0&&*capacity==16&&*actions;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
float StockLookFactor(){
 Infantry owner{};if(!installed||!ReadInfantry(&owner)||!nativeLookScale)return 0;
 __try {
  const auto factor=Read<const float*>(game,0x668e4c);
  return factor?*factor*nativeLookScale(owner.soldier):0;
 }__except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
bool ApplyStockPitch(BYTE* actions,int start,int capacity,int count,float factor){
 Infantry owner{};
 if(!actions||start<0||capacity!=16||count<=0||count>capacity||!installed||!stockPitchInstalled||!Focused()||!ReadInfantry(&owner))return false;
 TraversalSample traversal;
 if(!ReadNativeTraversal(&traversal)||traversal.mode!=TraversalMode::Foot)return false;
 AcquireSRWLockShared(&lock);const auto sample=lookPitch;ReleaseSRWLockShared(&lock);
 const auto now=GetTickCount64();
 if(sample.owner!=TurnOwner(owner)||!sample.tick||now<sample.tick||now-sample.tick>150)return false;
 __try {
  const auto value=NativePitchAxis(Read<float>(owner.soldier,0x270),sample.degrees,factor);
  if(!value)return false;
  // One correction across the batch, not once per action. Later actions clear
  // native mouse pitch too, so a stall cannot apply the same correction twice.
  for(int i=0;i<count;++i){
   auto* a=actions+size_t((std::int64_t(start)+i)%capacity)*0x118;
   const float pitch=i?0.f:*value;const DWORD mask=Read<DWORD>(a,0x100)|0x20;
   std::memcpy(a+0x14,&pitch,sizeof(pitch));std::memcpy(a+0x100,&mask,sizeof(mask));
  }
  return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
float StockPitchFactor(){
 Infantry owner{};if(!installed||!stockPitchInstalled||!nativeLookScale||!ReadInfantry(&owner))return 0;
 __try {const auto factor=Read<const float*>(game,0x668f24);return factor?*factor*nativeLookScale(owner.soldier):0;}
 __except(EXCEPTION_EXECUTE_HANDLER){return 0;}
}
bool StockPitchProfile(){
 __try {
  return Match(game+0x18abe5,{0x83,0xe6,0x20})&&
   Match(game+0x18abf1,{0xd9,0x85,0x44,0xfe,0xff,0xff})&&
   Match(game+0x18abff,{0x8b,0x15})&&Read<void*>(game,0x18ac01)==game+0x668f24&&
   Match(game+0x18ac51,{0xe8,0xca,0xb7,0xff,0xff})&&
   Match(game+0x18ad21,{0xd8,0x83,0x70,2,0,0})&&Match(game+0x18ad35,{0xd9,0x9b,0x70,2,0,0})&&
   Match(game+0x183c70,{0x83,0xe0,0x20})&&Match(game+0x183c79,{0x8b,0x4f,0x14})&&
   Match(game+0x183c9c,{0x66,0x89,0x46,0x0e})&&Read<float>(game,0x56545c)==100.f;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void __fastcall InputBatchHook(void* input,void*,int count,float dt){
 int before=0,capacity=0;BYTE* actions=nullptr;
 const bool eligible=count>0&&count<=16&&ReadInputBatch(input,&before,&capacity,&actions);
 nativeInputBatch(input,count,dt);
 if(!eligible)return;
 int after=0,newCapacity=0;BYTE* current=nullptr;
 if(ReadInputBatch(input,&after,&newCapacity,&current)&&current==actions&&capacity==newCapacity&&
    std::int64_t(after)-before==count){
  ApplyStockTurn(actions+size_t(before%capacity)*0x118,StockLookFactor());
  ApplyStockPitch(actions,before,capacity,count,StockPitchFactor());
 }
}
bool InstallStockTurn(){
 __try {
  if(!Match(game+0x277b70,{0x55,0x8b,0xec,0x83,0xec,0x2c,0x53,0x56,0x8b,0xf1,0x8b,0x46,0x38})||
     !Match(game+0x277c3d,{0x8b,0x45,0xfc,0x99,0xf7,0x7e,0x2c,0x8b,0x7e,0x34})||
     !Match(game+0x277c94,{0x8b,0x4e,0x34,0x03,0xcb,0x8b,0xd0,0xe8,0x10,0xf1,1,0})||
     !Match(game+0x277d78,{0x8b,0x45,0xfc,0x83,0x46,0x3c,1,0x5f,0x89,0x46,0x24})||
     !Match(game+0x277dbd,{0xc7,6})||Read<void*>(game,0x277dbf)!=game+0x58de40||
     !Match(game+0x18abcf,{0xd9,0x85,0x40,0xfe,0xff,0xff})||
     !Match(game+0x18abdd,{0x8b,0x0d})||Read<void*>(game,0x18abdf)!=game+0x668e4c||
     !Match(game+0x186420,{0x55,0x8b,0xec,0x81,0xec,0,1,0,0,0x56,0x57})||
     !Match(game+0x183c43,{0x8b,0x57,0x10,0x89,0x55,8})||
     !Match(game+0x183c66,{0x66,0x89,0x46,0x0c})||Read<float>(game,0x56545c)!=100.f)return false;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
 if(MH_CreateHook(game+0x277b70,InputBatchHook,reinterpret_cast<void**>(&nativeInputBatch))!=MH_OK)return false;
 if(MH_EnableHook(game+0x277b70)!=MH_OK){MH_RemoveHook(game+0x277b70);return false;}
 nativeLookScale=reinterpret_cast<LookScale>(game+0x186420);return true;
}
bool CopyMovementCamera(const stereo::Matrix4* source,stereo::Matrix4* out){
 __try {if(!source)return false;*out=*source;return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
const stereo::Matrix4* LocalMovementCamera(void* soldier,const void* caller,const stereo::Matrix4* native){
 if(caller!=game+0x18b033)return native;
 float offset=0;stereo::Matrix4 source{};
 if(!ReadNativeMovementYaw(soldier,&offset)||!CopyMovementCamera(native,&source))return native;
 const auto mapped=MakeMovementCamera(source,offset);if(!mapped)return native;
 movementCamera=*mapped;return &movementCamera;
}
const stereo::Matrix4* __fastcall MovementCameraHook(void* soldier,void*){
 const auto caller=_ReturnAddress();return LocalMovementCamera(soldier,caller,nativeMoveCamera(soldier));
}
bool InstallMovementCamera(){
 // Native input reads projected forward/right from this returned matrix for
 // BOTH ordinary walking and sprint. All other callers keep the original.
 __try {
  if(!Match(game+0x189000,{0x55,0x8b,0xec,0x81,0xec,0xf8,0,0,0,0x8b,0x81,8,2,0,0,0x8b,8,0x56,0x8b,0x71,0x10})||
   !Match(game+0x18908b,{0x8b,0x10,0x8b,0xc8,0xff,0x52,0x78,0x5e,0x8b,0xe5,0x5d,0xc3})||
   !Match(game+0x18b029,{0x8b,0xcb,0x89,0x45,0xfc,0xe8,0xcd,0xdf,0xff,0xff,0x8b,0xf8,0x8b,0x4f,0x20,0x8b,0x57,0x28}))return false;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
 auto entry=game+0x189000;
 if(MH_CreateHook(entry,MovementCameraHook,reinterpret_cast<void**>(&nativeMoveCamera))!=MH_OK)return false;
 if(MH_EnableHook(entry)!=MH_OK){MH_RemoveHook(entry);return false;}return true;
}
float __fastcall RecoilHook(void* soldier,void*){
    const auto caller=_ReturnAddress();
    return LocalLookInput(soldier,caller,nativeRecoil(soldier));
}
}
bool ReadNativeTraversal(TraversalSample* out){
 if(!installed||!out)return false;*out={};
 __try{
  const auto pm=Read<void*>(render,0x221a58);if(!pm||Read<void*>(pm,0)!=game+0x5292a0)return false;
  const auto player=Read<void*>(pm,0x6c);if(!player||Read<void*>(player,0)!=game+0x566cd8)return false;
  const auto weak=Read<void*>(player,0xcc);const auto soldier=weak?Read<void*>(weak,4):nullptr;
  if(!soldier||Read<void*>(soldier,0)!=game+0x566150||(Read<DWORD>(soldier,0x14)&0x20))return false;
  TraversalSample s;s.owner=(std::uint64_t(reinterpret_cast<std::uintptr_t>(weak))<<32)|reinterpret_cast<std::uintptr_t>(soldier);
  const auto control=Read<void*>(player,0x80);const auto object=control?Read<void*>(control,4):nullptr;
  if(!object)return false;
  if(object==soldier&&!Read<void*>(soldier,0x34)&&!Read<void*>(soldier,0x28c)&&!Read<int>(soldier,0x2f0)){
   s.mode=TraversalMode::Foot;s.world=Read<stereo::Matrix4>(soldier,0xa0);
   const auto physics=Read<void*>(soldier,0x50);
   if(physics&&Read<void*>(physics,0)==game+0x593aa0&&Read<void*>(game+0x593aa0,0x64)==game+0x2c5560&&Match(game+0x2c5560,{0x8d,0x41,0x78,0xc3})){
    s.verticalSpeed=Read<float>(physics,0x7c);s.velocityValid=std::isfinite(s.verticalSpeed)&&std::abs(s.verticalSpeed)<150;
   }
  }else if(traversalProfile){
   if(Read<DWORD>(object,0x14)&0x20)return false;
   const auto definition=Read<void*>(object,0x24);if(!definition)return false;
   if(Read<void*>(object,0)==game+0x5673d8&&Read<void*>(definition,0)==game+0x567790)s.mode=TraversalMode::Ladder;
   else if(Read<void*>(object,0)==game+0x56e818&&Read<void*>(definition,0)==game+0x56e518){
    // Pods share Parachute's class: only the stock canopy is supported here.
    const unsigned length=Read<unsigned>(definition,0x20),capacity=Read<unsigned>(definition,0x24);
    if(length!=9||capacity<length||capacity>4096)return false;
    const auto name=capacity<16?static_cast<const char*>(definition)+0x10:Read<const char*>(definition,0x10);
    if(!name||_strnicmp(name,"parachute",10))return false;s.mode=TraversalMode::Parachute;
   }else return false;
   s.mount=reinterpret_cast<std::uintptr_t>(object);s.world=Read<stereo::Matrix4>(object,0xa0);
  }else return false;
  if(!InverseRigid(s.world))return false;*out=s;return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ConfigureNativeMovement(bool enabled,float trackedYawRadians){
 Infantry owner{};const bool valid=enabled&&installed&&movementInstalled&&Focused()&&std::isfinite(trackedYawRadians)&&ReadInfantry(&owner);
 AcquireSRWLockExclusive(&lock);movement=valid?MovementSample{TurnOwner(owner),GetTickCount64(),trackedYawRadians}:MovementSample{};ReleaseSRWLockExclusive(&lock);return valid;
}
bool ConfigureNativeLookPitch(bool enabled,const stereo::Pose& head){
 Infantry owner{};TraversalSample traversal;const auto pitch=NativeHeadPitch(head);
 const bool valid=enabled&&installed&&stockPitchInstalled&&Focused()&&pitch&&ReadInfantry(&owner)&&
  ReadNativeTraversal(&traversal)&&traversal.mode==TraversalMode::Foot;
 AcquireSRWLockExclusive(&lock);lookPitch=valid?PitchSample{TurnOwner(owner),GetTickCount64(),*pitch}:PitchSample{};ReleaseSRWLockExclusive(&lock);return valid;
}
bool ReadNativeMovementYaw(const void* soldier,float* offsetDegrees){
 Infantry owner{};if(!offsetDegrees||!installed||!movementInstalled||!Focused()||!ReadInfantry(&owner)||owner.soldier!=soldier)return false;
 AcquireSRWLockShared(&lock);const auto sample=movement;const auto recoil=yaw.owner==reinterpret_cast<std::uintptr_t>(owner.identity)?yaw.recoilDegrees:0;ReleaseSRWLockShared(&lock);
 const auto now=GetTickCount64();if(sample.owner!=TurnOwner(owner)||!sample.tick||now<sample.tick||now-sample.tick>150)return false;
 const auto offset=MovementYawOffset(recoil,sample.trackedYaw);if(!offset)return false;*offsetDegrees=*offset;return true;
}
bool NativeSnapTurnAvailable(){TraversalSample traversal;if(ReadNativeTraversal(&traversal)&&traversal.mode!=TraversalMode::Foot)return false;Infantry owner{};return installed && stockTurnInstalled && ReadInfantry(&owner);}
bool RequestNativeSnapTurn(float degrees,std::int64_t sampleTime){
    Infantry owner{};if(!installed||!stockTurnInstalled||!Focused()||!ReadInfantry(&owner))return false;
    AcquireSRWLockExclusive(&lock);
    const bool queued=turnPulse.Queue(TurnOwner(owner),degrees,sampleTime,GetTickCount64());
    ReleaseSRWLockExclusive(&lock);return queued;
}
void ClearNativeSnapTurn(){AcquireSRWLockExclusive(&lock);turnPulse.Cancel();ReleaseSRWLockExclusive(&lock);}
bool InstallNativeComfort(LogFunction logger){
    if(installed)return true;logLine=logger;
    game=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));render=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9_ori.dll"));
    if(!render)render=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9.dll"));
    if(!game||!render||!Profile())return false;
    const auto entry=game+0x186140;
    if(MH_CreateHook(entry,RecoilHook,reinterpret_cast<void**>(&nativeRecoil))!=MH_OK)return false;
    if(MH_EnableHook(entry)!=MH_OK){MH_RemoveHook(entry);return false;}
    traversalProfile=Read<void*>(game+0x566cd8,0x4c)==game+0x193df0 &&
        Match(game+0x193df0,{0x56,0x8b,0xf1,0x8b,0x86,0x80,0,0,0}) &&
        Read<void*>(game+0x56e518,0x10)==game+0x1cf950 && Match(game+0x1cf950,{0xa1,-1,-1,-1,-1,0xc3}) &&
        Read<void*>(game+0x567790,0x10)==game+0x19dcc0 && Match(game+0x19dcc0,{0xa1,-1,-1,-1,-1,0xc3}) &&
        Read<void*>(game+0x567790,0x50)==game+0x19e1a0 && Read<void*>(game+0x56e518,0x50)==game+0x1d0280 &&
        Match(game+0x19ded1,{0xc7,0x06}) && Read<void*>(game,0x19ded3)==game+0x5673d8 &&
        Match(game+0x1d0092,{0xc7,0x06}) && Read<void*>(game,0x1d0094)==game+0x56e818;
    stockTurnInstalled=InstallStockTurn();logger("Snap turn stock multiplayer input: %s; no server snap event.",stockTurnInstalled?"connected":"profile unavailable");
    stockPitchInstalled=stockTurnInstalled&&StockPitchProfile();logger("VR ladder entry pitch: %s.",stockPitchInstalled?"stock network input follows headset":"profile unavailable; original input retained");
    movementInstalled=InstallMovementCamera();logger("VR movement basis: %s.",movementInstalled?"native walking/sprint heading connected":"unavailable; original input retained");
    installed=true;logger("Traversal comfort profile: %s.",traversalProfile?"ladder and stock parachute":"unavailable");
    logger("Infantry VR comfort connected: stock action snap-turn input, level heading, head recoil excluded, native recoil retained.");return true;
}
bool ReadNativeComfortCamera(const stereo::Matrix4& nativeWorld,stereo::Matrix4* stableWorld,const void* expectedSoldier){
    if(!installed||!stableWorld)return false;
    TraversalSample traversal;const bool supported=ReadNativeTraversal(&traversal)&&traversal.mode!=TraversalMode::Foot&&(!expectedSoldier||(traversal.mode==TraversalMode::Parachute&&std::uint32_t(traversal.owner)==reinterpret_cast<uintptr_t>(expectedSoldier)));
    Infantry owner{};const bool valid=!supported&&ReadInfantry(&owner);
    if(!valid){
        float distance=0;for(int i=0;i<3;++i){const float d=nativeWorld.values[3][i]-traversal.world.values[3][i];distance+=d*d;}
        AcquireSRWLockExclusive(&lock);
        const auto camera=supported&&std::isfinite(distance)&&distance<25?traversalView.Mounted(traversal,nativeWorld,GetTickCount64()):std::optional<stereo::Matrix4>{};
        if(!camera)traversalView.Reset();ReleaseSRWLockExclusive(&lock);
        if(camera){*stableWorld=*camera;return true;}return false;
    }
    AcquireSRWLockExclusive(&lock);
    yaw.Bind(valid?reinterpret_cast<std::uintptr_t>(owner.identity):0);
    const auto heading=valid?yaw.Heading(owner.bodyYaw,owner.localYaw):std::optional<float>{};
    ReleaseSRWLockExclusive(&lock);
    if(!heading || (expectedSoldier && owner.soldier!=expectedSoldier) || !InverseRigid(owner.body))return false;
    float distance=0;for(int i=0;i<3;++i){const float d=nativeWorld.values[3][i]-owner.body.values[3][i];distance+=d*d;}
    // Do not redirect a death, deployment or distant cinematic camera.
    if(!std::isfinite(distance)||distance>9.f)return false;
    const auto stable=MakeComfortCamera(nativeWorld,*heading);if(!stable)return false;
    *stableWorld=*stable;
    if(physicalCamera)stableWorld->values[3][1]=physicalHeight.Update(reinterpret_cast<std::uintptr_t>(owner.identity),owner.stance,
        nativeWorld.values[3][1],owner.body.values[3][1],GetTickCount64());
    AcquireSRWLockExclusive(&lock);traversalView.RecordFoot(TurnOwner(owner),*stableWorld,GetTickCount64());ReleaseSRWLockExclusive(&lock);
    if(InterlockedCompareExchange(&reported,1,0)==0){logLine("Infantry headset and controller poses use the same recoil-free level frame.");}
    return true;
}
bool ReadNativeStance(int* stance){
    if(!installed||!stance)return false;Infantry owner{};if(!ReadInfantry(&owner))return false;
    if(owner.stance<0||owner.stance>2)return false;*stance=owner.stance;return true;
}
void ConfigurePhysicalCamera(bool enabled){if(!enabled)physicalHeight={};physicalCamera=enabled;}
}
