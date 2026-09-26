#include "NativeComfort.h"
#include "multiplayer/NativeNetwork.h"
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
    // The native caller adds this exact horizontal delta to its look input.
    // Only this local input call may consume a snap. Native heading, soldier
    // orientation and HUD/minimap then advance through the normal game path.
    // Record actual recoil separately so comfort never subtracts intentional turns.
    if(caller==game+0x18acfe){
        Infantry owner{};
        if(ReadInfantry(&owner) && owner.soldier==soldier){
            AcquireSRWLockExclusive(&lock);
            yaw.Bind(reinterpret_cast<std::uintptr_t>(owner.identity));yaw.AddRecoil(result);
            const float turn=turnPulse.Consume(TurnOwner(owner),Focused(),GetTickCount64());
            ReleaseSRWLockExclusive(&lock);
            return result+turn;
        }
    }
    return result;
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
bool ReadNativeMovementYaw(const void* soldier,float* offsetDegrees){
 Infantry owner{};if(!offsetDegrees||!installed||!movementInstalled||!Focused()||!ReadInfantry(&owner)||owner.soldier!=soldier)return false;
 AcquireSRWLockShared(&lock);const auto sample=movement;const auto recoil=yaw.owner==reinterpret_cast<std::uintptr_t>(owner.identity)?yaw.recoilDegrees:0;ReleaseSRWLockShared(&lock);
 const auto now=GetTickCount64();if(sample.owner!=TurnOwner(owner)||!sample.tick||now<sample.tick||now-sample.tick>150)return false;
 const auto offset=MovementYawOffset(recoil,sample.trackedYaw);if(!offset)return false;*offsetDegrees=*offset;return true;
}
bool NativeSnapTurnAvailable(){TraversalSample traversal;if(ReadNativeTraversal(&traversal)&&traversal.mode!=TraversalMode::Foot)return false;Infantry owner{};return installed && ReadInfantry(&owner);}
bool RequestNativeSnapTurn(float degrees,std::int64_t sampleTime){
    Infantry owner{};if(!installed||!Focused()||!ReadInfantry(&owner))return false;
    AcquireSRWLockExclusive(&lock);
    const bool queued=turnPulse.Queue(TurnOwner(owner),degrees,sampleTime,GetTickCount64());
    ReleaseSRWLockExclusive(&lock);if(queued)PublishNetworkSnap(owner.soldier,degrees);return queued;
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
    movementInstalled=InstallMovementCamera();logger("VR movement basis: %s.",movementInstalled?"native walking/sprint heading connected":"unavailable; original input retained");
    installed=true;logger("Traversal comfort profile: %s.",traversalProfile?"ladder and stock parachute":"unavailable");
    logger("Infantry VR comfort connected: native snap-turn input, level heading, head recoil excluded, native recoil retained.");return true;
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
