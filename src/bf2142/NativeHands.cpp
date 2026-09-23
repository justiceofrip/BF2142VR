#include "NativeHands.h"
#include "HandPoseMath.h"
#include "FingerPose.h"
#include "MotionActions.h"
#include "GrenadeArc.h"
#include "ControllerPolicy.h"
#include "NativeOptics.h"
#include "HandBindingCache.h"
#include "NativeComfort.h"
#include "WeaponOptic.h"
#include <MinHook.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>
namespace bfvr::bf2142 {
namespace {
using M=stereo::Matrix4;
using Finalize=void(__thiscall*)(void*,float);
Finalize original=nullptr;BYTE* game=nullptr;BYTE* render=nullptr;LogFunction logMessage=nullptr;
using Fire=void*(__thiscall*)(void*,const M*,const M*,const stereo::Vec3*);
Fire originalFire=nullptr;
using LaunchMatrix=const M*(__thiscall*)(void*);
LaunchMatrix originalLaunch=nullptr;thread_local M trackedLaunch{};
struct FirePose {void* soldier=nullptr;void* weapon=nullptr;M world{},camera{},tracking{};LONGLONG generation=0;ULONGLONG tick=0;};
FirePose firePose{};
MotionActions motionPolicy;
bool motionEnabled=true;volatile LONG weaponHeld=1,gripTransition=0;
struct Gesture {FirePose owner{};M launch{};stereo::Vec3 velocity{};MotionKind kind=MotionKind::None;ULONGLONG tick=0;unsigned serial=0;};
Gesture gesture{};unsigned nextGesture=0;thread_local unsigned launchGesture=0;
void* bindingWeapon=nullptr;void* bindingSoldier=nullptr;void* bindingSkeleton=nullptr;HandBindingCache bindings;
bool fingerPosesEnabled=true;
ControllerHandCache palms;PistolAimFilter pistolAim;
LONGLONG handPoseTime=0;
std::array<std::array<float,5>,2> filteredCurls{};LONGLONG fingerTime=0;
wchar_t tracePath[32768]{};unsigned traceFrames=0,traceRecords=0;
bool installed=false,reported=false,fireReported=false;volatile LONG supporting=0;
SRWLOCK sampleLock=SRWLOCK_INIT;
struct Sample { shared::SharedControllerSample input{};stereo::Pose reference{},head{};float scale=1,height=0;ULONGLONG tick=0;bool enabled=false; } current;
template<class T>T Read(const void* p,size_t offset){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+offset,sizeof(v));return v;}
stereo::Pose Pose(const shared::SharedPresentationPose& p){return {{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}};}
constexpr DWORD gripFlags=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripOrientationValid|
    shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked;
constexpr DWORD aimFlags=shared::kControllerHandFlagAimActive|shared::kControllerHandFlagAimPositionValid|shared::kControllerHandFlagAimOrientationValid|
    shared::kControllerHandFlagAimPositionTracked|shared::kControllerHandFlagAimOrientationTracked;
bool Tracked(const shared::SharedControllerHandSample& hand){return (hand.flags&(gripFlags|aimFlags))==(gripFlags|aimFlags);}
bool Profile(){
    __try {
        const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(game);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>4096)return false;
        const auto* pe=reinterpret_cast<const IMAGE_NT_HEADERS*>(game+dos->e_lfanew);
        if(pe->Signature!=IMAGE_NT_SIGNATURE||pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||pe->OptionalHeader.SizeOfImage<0x660000)return false;
        const BYTE finalize[]={0x55,0x8b,0xec,0x81,0xec,0xb8,0,0,0,0x57,0x8b,0xf9,0x83,0xbf,0x8c,2,0,0,0};
        const BYTE inventory[]={0x55,0x8b,0xec,0x56,0x8b,0x75,8,0x85,0xf6,0x7e,0x2d,0x8b,0x91,0xac,0,0,0};
        const BYTE fire[]={0x55,0x8b,0xec,0x83,0xec,0x28,0x53,0x56,0x8b,0xf1,0x8b,0x4e,0x0c,0x8b,0x81,0xa0,1,0,0,0x57};
        const BYTE launch[]={0x55,0x8b,0xec,0x83,0xec,0x10,0x53,0x56,0x8b,0xf1,0x8b,0x4e,0xfc,0x57};
        const BYTE playerGetter[]={0x8b,0x41,0x6c,0xc3};
        const BYTE transform[]={0x55,0x8b,0xec,0x81,0xec,8,1,0,0,0x53,0x56,0x8b,0xf1};
        return !std::memcmp(game+0x1f1a20,launch,sizeof(launch)) && game[0x1f1cb4]==0xc3 &&
            Read<void*>(game+0x5703c8,0xd4)==game+0x1f1a20 && !std::memcmp(game+0x1f2bb0,fire,sizeof(fire)) && !std::memcmp(game+0x1ee980,finalize,sizeof(finalize)) && !std::memcmp(game+0x1896a0,inventory,sizeof(inventory)) &&
            !std::memcmp(game+0xb7530,playerGetter,sizeof(playerGetter)) && !std::memcmp(game+0x4b8880,transform,sizeof(transform)) &&
            Read<void*>(game+0x5292a0,12*4)==game+0xb7530 && Read<void*>(game+0x566150,30*4)==game+0x2f6e20 &&
            Read<void*>(game+0x56cd88,58*4)==game+0x2f79b0;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ParachuteRig(void* soldier){TraversalSample s;return ReadNativeTraversal(&s)&&s.mode==TraversalMode::Parachute&&std::uint32_t(s.owner)==reinterpret_cast<uintptr_t>(soldier);}
struct Target {void* weapon=nullptr;void* skeleton=nullptr;M* bones=nullptr;};
bool FindTarget(void* soldier,Target* target){
    __try {
        const auto pm=Read<void*>(render,0x221a58);
        if(!pm||Read<void*>(pm,0)!=game+0x5292a0)return false;
        const auto player=Read<void*>(pm,0x6c);if(!player||Read<void*>(player,0)!=game+0x566cd8)return false;
        const auto weak=Read<void*>(player,0xcc);
        if(!weak||Read<void*>(weak,4)!=soldier||Read<void*>(soldier,0)!=game+0x566150 || (Read<DWORD>(soldier,0x14)&0x20))return false;
        if((Read<int>(soldier,0x2f0)!=0 || Read<void*>(soldier,0x28c))&&!ParachuteRig(soldier))return false;
        const int index=Read<int>(soldier,0x218);const auto begin=Read<BYTE*>(soldier,0x234),end=Read<BYTE*>(soldier,0x238);
        if(index<=0||index>=32||!begin||end<begin||size_t(end-begin)>32*4||size_t(index*4)>=size_t(end-begin))return false;
        const auto weapon=Read<void*>(begin,index*4);
        if(!weapon||Read<void*>(weapon,0)!=game+0x56cd88||Read<void*>(weapon,0x34)!=soldier)return false;
        const auto skel=Read<void*>(soldier,0x2c0);
        if(!skel||Read<int>(skel,0x14)!=70)return false;
        const auto meta=Read<BYTE*>(skel,8);const auto bones=Read<M*>(skel,0x10);if(!meta||!bones)return false;
        // Exact parent topology is checked on each owner before any write.
        const int ids[]={0,1,2,3,4,5,6,7,28,29,30,31,32,33,54,69};
        const short parents[]={-1,-1,1,2,3,4,5,6,1,28,29,30,31,32,1,1};
        for(size_t i=0;i<std::size(ids);++i)if(Read<int>(meta,ids[i]*0x24)!=ids[i]||Read<short>(meta,ids[i]*0x24+6)!=parents[i])return false;
        *target={weapon,skel,bones};return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool WeaponName(void* weapon,std::array<char,49>* output){
    __try {
        if(!weapon||Read<void*>(weapon,0)!=game+0x56cd88)return false;
        const auto t=Read<BYTE*>(weapon,0x24);if(!t||Read<void*>(t,0)!=game+0x56c758)return false;
        const unsigned n=Read<unsigned>(t,0x20),cap=Read<unsigned>(t,0x24);
        if(n<1||n>48||cap<n||cap>4096)return false;
        const auto name=cap<16?reinterpret_cast<char*>(t+0x10):Read<char*>(t,0x10);
        if(!name||name[n])return false;
        std::array<char,49> text{};for(unsigned i=0;i<n;++i){text[i]=name[i];if(text[i]>='A'&&text[i]<='Z')text[i]+=32;
            if(!((text[i]>='a'&&text[i]<='z')||(text[i]>='0'&&text[i]<='9')||text[i]=='_'))return false;}
        *output=text;return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
MotionKind Kind(void* weapon){std::array<char,49> name{};return WeaponName(weapon,&name)?MotionWeapon(name.data()):MotionKind::None;}
bool OwnedFireInterface(const FirePose& pose){
    __try {Target t;if(!FindTarget(pose.soldier,&t)||t.weapon!=pose.weapon)return false;
        const auto p=Read<BYTE*>(pose.weapon,0x1b4);return p && Read<void*>(p,0)==game+0x5703c8 && Read<void*>(p-4,0)==pose.weapon;}
    __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool NativeSpeed(void* weapon,float* speed){
    __try {
        if(Read<void*>(weapon,0x1a0)!=game+0x56cb58 || Read<void*>(game+0x56cb58,0x1b0)!=game+0x1c1ac0)return false;
        const BYTE signature[]={0x56,0x8b,0xf1,0x83,0x7e,0x2c,0,0x74,0x1e,0x8b,0x4e,0x2c,0x8b,1,0xff,0x90,0xc0,0,0,0};
        if(std::memcmp(game+0x1c1ac0,signature,sizeof(signature)))return false;
        using Speed=float(__thiscall*)(void*);
        *speed=reinterpret_cast<Speed>(game+0x1c1ac0)(static_cast<BYTE*>(weapon)+0x1a0);
        return std::isfinite(*speed)&&*speed>=0&&*speed<=100;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool FingerTopology(const Target& target){
    __try{
        const auto meta=Read<BYTE*>(target.skeleton,8);if(!meta)return false;
        for(int wrist:{7,33})for(int finger=0;finger<5;++finger)for(int j=0;j<4;++j){
            const int b=wrist+1+finger*4+j;
            if(Read<int>(meta,b*0x24)!=b || Read<short>(meta,b*0x24+6)!=(j?b-1:wrist))return false;
        }
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool SoldierWorld(void* soldier,M* world){__try{if(Read<void*>(soldier,0x34)&&!ParachuteRig(soldier))return false;*world=Read<M>(soldier,0xa0);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
bool CopyBones(const Target& t,HandBones* bones){__try{std::memcpy(bones->data(),t.bones,sizeof(HandBones));return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
bool WriteBones(const Target& t,const HandBones& bones){__try{std::memcpy(t.bones+2,bones.data()+2,sizeof(M)*68);return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
std::optional<M> Compose(const M& camera,const Sample& s,const stereo::Pose& pose){
    auto result=stereo::ComposeRuntimeHeadWithD3D8Camera(camera,s.reference,pose,s.scale);
    if(result)result->values[3][1]+=s.height;return result;
}
void Apply(void* soldier){
    Sample s;AcquireSRWLockShared(&sampleLock);s=current;ReleaseSRWLockShared(&sampleLock);
    if(!s.enabled||GetTickCount64()-s.tick>150)return;
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);if(foreground!=GetCurrentProcessId())return;
    Target target;if(!FindTarget(soldier,&target))return;
    AcquireSRWLockExclusive(&sampleLock);firePose={};ReleaseSRWLockExclusive(&sampleLock);
    HandBones native;if(!CopyBones(target,&native))return;
    if(bindingWeapon!=target.weapon || bindingSoldier!=soldier || bindingSkeleton!=target.skeleton){
        if(bindingSoldier!=soldier || bindingSkeleton!=target.skeleton){palms={};}
        pistolAim.Reset();InterlockedExchange(&supporting,0);
        bindings={};bindingWeapon=target.weapon;bindingSoldier=soldier;bindingSkeleton=target.skeleton;
    }
    // Keep anatomical references across weapon swaps on the same skeleton.
    // They are used for the knife, empty hands and cupped pistol support.
    const bool canopy=ParachuteRig(soldier);
    const bool fingersValid=FingerTopology(target);
    
    // Restore the accepted v16 binding/animation path. Do not replace the
    // native arm/finger pose with a cached weapon-relative ADS pose.
    const bool wasReady=bindings.supportReady;if(!canopy)bindings.Update(native,GetTickCount64(),!NativeWeaponAds(target.weapon));
    if(fingersValid)palms.Update(native,!canopy&&bindings.rightReady);
    if(!wasReady && bindings.supportReady)logMessage("Authored firing/support grips settled after weapon draw; controller aim preserved.");
    // Express the same level, recoil-free world frame in skeleton space.
    // Bone 0 and the native camera remain untouched for gameplay/fire mapping.
    M trackingCamera=native[0],body{};
    if(SoldierWorld(soldier,&body)){
        const auto inverse=InverseRigid(body);M stable{};
        if(inverse && ReadNativeComfortCamera(Multiply(native[0],body),&stable,soldier))
            trackingCamera=Multiply(stable,*inverse);
    }
    HandFrame f{};f.torso=trackingCamera;f.bindings=bindings.value;f.supportReady=bindings.supportReady;const auto head=Compose(trackingCamera,s,s.head);if(!head)return;f.head=*head;
    const auto& right=s.input.hands[1];const auto& left=s.input.hands[0];
    if(!Tracked(right))return;
    const auto rg=Compose(trackingCamera,s,Pose(right.gripPose)),ra=Compose(trackingCamera,s,Pose(right.aimPose));if(!rg||!ra)return;
    f.weaponHeld=NativeWeaponHeld()&&!canopy;f.rightGrip=*rg;f.rightAim=*ra;f.rightValid=true;f.knifeGrip=Kind(target.weapon)==MotionKind::Knife;
    std::array<char,49> name{};f.pistolGrip=WeaponName(target.weapon,&name)&&PistolWeapon(name.data());
    f.leftPalm=palms.left;f.rightPalm=palms.right;
    if(palms.left&&palms.right)f.handReference=&palms.reference;
    if(f.knifeGrip&&!fingersValid)return;
    if(f.pistolGrip)f.supportReady=bindings.rightReady;
    if(Tracked(left)){
        const auto lg=Compose(trackingCamera,s,Pose(left.gripPose)),la=Compose(trackingCamera,s,Pose(left.aimPose));
        if(lg&&la){f.leftGrip=*lg;f.leftAim=*la;f.leftValid=true;}
    }
    f.supportPressed=(left.flags&shared::kControllerHandFlagSqueezeActive)&&std::isfinite(left.squeezeValue)&&left.squeezeValue>.65f;
    f.wasSupporting=InterlockedCompareExchange(&supporting,0,0)!=0;
    if(handPoseTime && (s.input.predictedDisplayTime<handPoseTime || s.input.predictedDisplayTime-handPoseTime>100000000)){
        f.wasSupporting=false;pistolAim.Reset();
    }
    handPoseTime=s.input.predictedDisplayTime;
    float separation=0;for(int i=0;i<3;++i){const float d=f.leftGrip.values[3][i]-f.rightGrip.values[3][i];separation+=d*d;}
    const bool brace=f.pistolGrip&&f.leftValid&&f.leftPalm&&f.rightPalm&&
        PistolSupportEligible(f.weaponHeld&&f.supportPressed&&f.supportReady,f.wasSupporting,std::sqrt(separation));
    f.rightAim=pistolAim.Update(f.rightAim,brace,s.input.predictedDisplayTime);
    f.fingerPoses=fingerPosesEnabled && fingersValid;
    if(f.fingerPoses){
        const auto dt=s.input.predictedDisplayTime-fingerTime;
        const float blend=fingerTime && dt>0 && dt<250000000 ? 1-std::exp(-18.f*float(dt)*1.e-9f):1.f;
        for(unsigned h=0;h<2;++h){const auto goal=ControllerFingerCurls(s.input.hands[h]);
            if(dt!=0)for(size_t i=0;i<goal.size();++i)filteredCurls[h][i]+=(goal[i]-filteredCurls[h][i])*blend;}
        fingerTime=s.input.predictedDisplayTime;f.leftCurls=filteredCurls[0];f.rightCurls=filteredCurls[1];
    }
    const auto solved=SolveTrackedHands(native,f);
    // Explicit private pose capture for diagnosing native animation callbacks.
    // No files or per-frame inspection are enabled in ordinary player sessions.
    if(tracePath[0] && traceRecords<16 && ((traceFrames++%30)==0)){
        FILE* trace=nullptr;
        if(_wfopen_s(&trace,tracePath,traceRecords?L"ab":L"wb")==0 && trace){
            const DWORD header[]={0x33444842,DWORD(sizeof(HandBones)),DWORD(s.input.predictedDisplayTime),DWORD(s.input.predictedDisplayTime>>32),DWORD(soldier),DWORD(target.weapon),solved?1u:0u,bindings.supportReady?1u:0u};
            fwrite(header,sizeof(header),1,trace);fwrite(native.data(),sizeof(native),1,trace);
            fwrite(solved?solved->bones.data():native.data(),sizeof(native),1,trace);fwrite(&body,sizeof(body),1,trace);fclose(trace);++traceRecords;
        }
    }
    if(!solved){InterlockedExchange(&supporting,0);return;}
    // No camera, root, asset data or remote-player animation is changed.
    // The game binds bone 54 to the active weapon immediately after this call.
    if(WriteBones(target,solved->bones)){
        if(!canopy && SoldierWorld(soldier,&body) && InverseRigid(body)){
            AcquireSRWLockExclusive(&sampleLock);firePose={soldier,target.weapon,Multiply(solved->bones[54],body),Multiply(native[0],body),Multiply(trackingCamera,body),s.input.predictedDisplayTime,s.tick};ReleaseSRWLockExclusive(&sampleLock);
        }
        InterlockedExchange(&gripTransition,0);
        InterlockedExchange(&supporting,solved->supporting?1:0);
        if(!reported){reported=true;logMessage("Native controller hands active: local 1P skeleton, wrists/arms and 16 weapon parts; fresh focused sample.");}
    }
}
bool FireOwner(void* receiver,const FirePose& pose){
    __try {
        Target t;if(!pose.soldier||!pose.weapon||!FindTarget(pose.soldier,&t)||t.weapon!=pose.weapon)return false;
        return Read<void*>(receiver,0xc)==t.weapon && Read<void*>(receiver,0x10)==game+0x5703c8 &&
            Read<BYTE*>(t.weapon,0x1b4)==static_cast<BYTE*>(receiver)+0x10;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ReadFire(const M* source,M* target){__try{if(!source)return false;*target=*source;return true;}__except(EXCEPTION_EXECUTE_HANDLER){return false;}}
bool TrackedFire(void* receiver,FirePose* result){
    Sample s;FirePose pose;AcquireSRWLockShared(&sampleLock);s=current;pose=firePose;ReleaseSRWLockShared(&sampleLock);
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    if(!s.enabled || foreground!=GetCurrentProcessId() || GetTickCount64()-pose.tick>150 ||
        pose.generation!=s.input.predictedDisplayTime || !FireOwner(receiver,pose) || !InverseRigid(pose.world))return false;
    *result=pose;return true;
}
bool PendingGesture(void* receiver,const FirePose& pose,Gesture* out){
    Gesture g;AcquireSRWLockShared(&sampleLock);g=gesture;ReleaseSRWLockShared(&sampleLock);
    if(!motionEnabled||g.kind!=MotionKind::Knife||!g.serial||GetTickCount64()-g.tick>900 || g.owner.soldier!=pose.soldier || g.owner.weapon!=pose.weapon ||
        Kind(pose.weapon)!=g.kind||!FireOwner(receiver,pose)||!InverseRigid(g.launch))return false;
    *out=g;return true;
}
const M* __fastcall LaunchHook(void* interfaceSelf,void*){
    const M* native=originalLaunch(interfaceSelf);launchGesture=0;
    FirePose pose;M source{};
    // FireBase::LaunchMatrix runs before 0x1f31d0 calculates muzzle velocity.
    // Return owned thread-local storage (same lifetime as the native cached
    // result), never a stack pointer or an in-place edit of native fire state.
    if(TrackedFire(static_cast<BYTE*>(interfaceSelf)-0x10,&pose) && ReadFire(native,&source)){
        Gesture g;
        if(PendingGesture(static_cast<BYTE*>(interfaceSelf)-0x10,pose,&g)){
            trackedLaunch=g.launch;launchGesture=g.serial;return &trackedLaunch;
        }
        if(Kind(pose.weapon)==MotionKind::Frag){
            // ThrownFireComp may source an already transformed weapon matrix.
            // Reinterpreting it as camera-local applies the tracked turn twice.
            // Both native velocity and the guide consume this one launch frame.
            trackedLaunch=TrackedFragLaunch(pose.world);return &trackedLaunch;
        }
        if(const auto mapped=MapTrackedFire(source,pose.camera,pose.world)){
            trackedLaunch=*mapped;return &trackedLaunch;
        }
    }
    return native;
}
void* __fastcall FireHook(void* receiver,void*,const M* launch,const M* parent,const stereo::Vec3* velocity){
    FirePose pose;
    if(!NativeWeaponInputReady()){
        Sample s;AcquireSRWLockShared(&sampleLock);s=current;pose=firePose;ReleaseSRWLockShared(&sampleLock);
        if(s.enabled && FireOwner(receiver,pose))return nullptr; // Local hidden item cannot fire, including queued native input.
    }
    if(TrackedFire(receiver,&pose)){
        Gesture g;
        if(launch==&trackedLaunch && PendingGesture(receiver,pose,&g) && launchGesture==g.serial){
            float speed=0;stereo::Vec3 v{};
            if(velocity && NativeSpeed(pose.weapon,&speed)){
                __try {v=*velocity;}__except(EXCEPTION_EXECUTE_HANDLER){return originalFire(receiver,launch,parent,velocity);}
                const auto result=originalFire(receiver,&g.launch,&g.launch,&v);
                AcquireSRWLockExclusive(&sampleLock);if(gesture.serial==g.serial)gesture={};ReleaseSRWLockExclusive(&sampleLock);
                return result;
            }
        }
        // The main matrix and its computed velocity already use LaunchHook's
        // pose. Do not rotate either a second time, or rotate inherited motion.
        if(launch==&trackedLaunch && !fireReported){
            fireReported=true;logMessage("v12 main projectile launch and velocity follow held-gun aim; native speed, inherited movement and deviation retained.");
        }
        return originalFire(receiver,launch,&pose.world,velocity);
    }
    return originalFire(receiver,launch,parent,velocity);
}
void __fastcall Hook(void* soldier,void*,float delta){original(soldier,delta);Apply(soldier);}
}
bool ReadTrackedWeaponFrame(TrackedWeaponFrame* result,bool previousSample){
    if(!result || !NativeWeaponInputReady())return false;
    Sample s;FirePose pose;AcquireSRWLockShared(&sampleLock);s=current;pose=firePose;ReleaseSRWLockShared(&sampleLock);
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    const auto age=s.input.predictedDisplayTime-pose.generation;
    if(!s.enabled || foreground!=GetCurrentProcessId() || GetTickCount64()-pose.tick>150 ||
        age<0 || age>(previousSample?100000000LL:0LL) || !InverseRigid(pose.world))return false;
    Target target;if(!pose.soldier || !FindTarget(pose.soldier,&target) || target.weapon!=pose.weapon)return false;
    *result={pose.weapon,pose.world};WeaponName(pose.weapon,&result->name);return true;
}
bool InstallNativeHands(LogFunction logger){
    if(installed)return true;GetEnvironmentVariableW(L"BF2142VR_HAND_CAPTURE",tracePath,DWORD(std::size(tracePath)));logMessage=logger;game=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));render=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9_ori.dll"));
    if(!render)render=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9.dll"));
    if(!game||!render||!Profile()){logger("Native hands disabled: unrecognized BF2142 animation profile.");return false;}
    auto target=game+0x1ee980;
    if(MH_CreateHook(target,Hook,reinterpret_cast<void**>(&original))!=MH_OK)return false;
    const auto fire=game+0x1f2bb0;
    if(MH_CreateHook(fire,FireHook,reinterpret_cast<void**>(&originalFire))!=MH_OK){MH_RemoveHook(target);return false;}
    const auto launch=game+0x1f1a20;
    if(MH_CreateHook(launch,LaunchHook,reinterpret_cast<void**>(&originalLaunch))!=MH_OK){MH_RemoveHook(fire);MH_RemoveHook(target);return false;}
    if(MH_EnableHook(target)!=MH_OK || MH_EnableHook(launch)!=MH_OK || MH_EnableHook(fire)!=MH_OK){
        for(auto entry:{target,launch,fire}){MH_DisableHook(entry);MH_RemoveHook(entry);}return false;
    }
    installed=true;logger("Native hands connected: local first-person animation, pre-velocity launch pose and firearm adapters; tracking and owner guards enabled.");return true;
}
void PublishNativeHands(const shared::SharedControllerSample* sample,const stereo::Pose& reference,const stereo::Pose& head,float scale,float height){
    Sample s{};s.reference=reference;s.head=head;s.scale=scale;s.height=height;s.tick=GetTickCount64();
    if(sample && (sample->flags&shared::kControllerSampleFlagSessionFocused) && Tracked(sample->hands[1])){s.input=*sample;s.enabled=true;}
    AcquireSRWLockExclusive(&sampleLock);current=s;ReleaseSRWLockExclusive(&sampleLock);if(!s.enabled)InterlockedExchange(&supporting,0);
}
void ClearNativeHands(){AcquireSRWLockExclusive(&sampleLock);current={};firePose={};gesture={};ReleaseSRWLockExclusive(&sampleLock);InterlockedExchange(&supporting,0);}
void DrawNativeOptic(std::vector<DWORD>& pixels,UINT width,UINT height,DXGI_FORMAT format,const EyeCamera& eye){
    Sample s;FirePose pose;AcquireSRWLockShared(&sampleLock);s=current;pose=firePose;ReleaseSRWLockShared(&sampleLock);
    const auto& hand=s.input.hands[1];
    if(!s.enabled || !pose.weapon || GetTickCount64()-pose.tick>150 || pose.generation!=s.input.predictedDisplayTime ||
        !(hand.flags&shared::kControllerHandFlagSqueezeActive)||!std::isfinite(hand.squeezeValue)||hand.squeezeValue<.65f)return;
    DrawWeaponOptic(pixels,width,height,pose.world,eye.world,eye.projection,format==DXGI_FORMAT_R8G8B8A8_UNORM || format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
}

}

namespace bfvr::bf2142 {
bool IsLocalTrackedWeapon(void* weapon){Target target;return weapon && bindingSoldier && FindTarget(bindingSoldier,&target) && target.weapon==weapon;}
}

namespace bfvr::bf2142 {
bool ReadNativeInventory(std::array<bool,10>* present,int* selected,std::array<std::array<char,49>,10>* names,std::uint64_t* owner){
    if(owner)*owner=0;if(!installed||!present)return false;present->fill(false);if(names)*names={};
    __try {
        const auto pm=Read<void*>(render,0x221a58);
        if(!pm||Read<void*>(pm,0)!=game+0x5292a0)return false;
        const auto player=Read<void*>(pm,0x6c);if(!player||Read<void*>(player,0)!=game+0x566cd8)return false;
        const auto weak=Read<void*>(player,0xcc);if(!weak)return false;
        const auto soldier=Read<void*>(weak,4);
        if(!soldier || Read<void*>(soldier,0)!=game+0x566150 || (Read<DWORD>(soldier,0x14)&0x20) ||
            Read<int>(soldier,0x2f0)!=0 || Read<void*>(soldier,0x28c))return false;
        const auto begin=Read<BYTE*>(soldier,0x234),end=Read<BYTE*>(soldier,0x238);
        if(!begin||end<begin||size_t(end-begin)>32*4)return false;
        for(size_t i=1;i<present->size() && i*4<size_t(end-begin);++i){
            const auto item=Read<void*>(begin,i*4);
            // Membership in this verified local inventory owns an unequipped item.
            // Only the equipped item's physical parent points back to soldier.
            const auto parent=item?Read<void*>(item,0x34):nullptr;
            (*present)[i]=item && Read<void*>(item,0)==game+0x56cd88 && InventoryParentMatches(parent,soldier);
            if((*present)[i] && names){
                const auto t=Read<BYTE*>(item,0x24);if(!t||Read<void*>(t,0)!=game+0x56c758)continue;
                const unsigned n=Read<unsigned>(t,0x20),capacity=Read<unsigned>(t,0x24);
                if(n<1||n>48||capacity<n||capacity>4096)continue;
                const char* name=capacity<16?reinterpret_cast<char*>(t+0x10):Read<const char*>(t,0x10);
                if(!name||name[n])continue;bool valid=true;
                for(unsigned c=0;c<n;++c){char value=name[c];if(value>='A'&&value<='Z')value+=32;
                    if(!((value>='a'&&value<='z')||(value>='0'&&value<='9')||value=='_')){valid=false;break;}
                    (*names)[i][c]=value;
                }
                if(!valid)(*names)[i]={};
            }
        }
        if(selected)*selected=Read<int>(soldier,0x218);
        if(owner)*owner=reinterpret_cast<std::uintptr_t>(soldier);
        return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){present->fill(false);if(names)*names={};return false;}
}
}

namespace bfvr::bf2142 {
bool ReadNativeRightGripOffset(stereo::Vec3* offset){
    if(!offset||!bindings.value||!IsLocalTrackedWeapon(bindingWeapon))return false;
    const auto& m=bindings.value->rightFromWeapon;
    *offset={m.values[3][0],m.values[3][1],m.values[3][2]};return true;
}
}

namespace bfvr::bf2142 {void EnableFingerPoses(bool enabled){fingerPosesEnabled=enabled;fingerTime=0;}}

namespace bfvr::bf2142 {
void ConfigureMotionActions(bool enabled){motionEnabled=enabled;motionPolicy.Reset();}
void ResetMotionActions(){motionPolicy.Reset();AcquireSRWLockExclusive(&sampleLock);gesture={};ReleaseSRWLockExclusive(&sampleLock);}
bool UpdateNativeMotion(const shared::SharedControllerSample* input,const stereo::Pose& head,ControllerCommand& command){
    FirePose pose;Sample s;AcquireSRWLockShared(&sampleLock);pose=firePose;s=current;ReleaseSRWLockShared(&sampleLock);
    Target local;const auto kind=motionEnabled && input && FindTarget(bindingSoldier,&local)?Kind(local.weapon):MotionKind::None;
    // Consume virtual fire as soon as a supported item is equipped, including
    // its initial draw and tracking gaps. Menu mouse clicks remain separate.
    if(kind==MotionKind::Knife){command.buttons[0]=0;command.buttons[1]=0;}
    float speed=0;
    if(!NativeWeaponInputReady() || !input || !s.enabled || kind!=MotionKind::Knife || !Tracked(input->hands[1]) || !IsLocalTrackedWeapon(pose.weapon) || !OwnedFireInterface(pose) ||
        GetTickCount64()-pose.tick>100 || !NativeSpeed(pose.weapon,&speed)) {ResetMotionActions();return false;}
    if(!Compose(pose.tracking,s,Pose(input->hands[1].gripPose))){ResetMotionActions();return false;}
    const auto& hand=input->hands[1];
    MotionInput in{};in.owner=(std::uint64_t(reinterpret_cast<std::uintptr_t>(pose.soldier))<<32)|reinterpret_cast<std::uintptr_t>(pose.weapon);
    in.time=input->predictedDisplayTime;in.kind=kind;in.active=true;in.hand=Pose(hand.gripPose).position;in.head=head.position;
    in.grip=(hand.flags&shared::kControllerHandFlagSqueezeActive)&&std::isfinite(hand.squeezeValue)&&hand.squeezeValue>.55f;
    in.trigger=(hand.flags&shared::kControllerHandFlagTriggerActive)&&std::isfinite(hand.triggerValue)&&hand.triggerValue>.55f;
    const auto action=motionPolicy.Update(in);
    command.buttons[0]=action.primary?0x80:0;command.buttons[1]=0;
    if(action.action==MotionKind::None)return action.primeEdge;
    auto runtime=Pose(hand.gripPose);runtime.position=action.position;
    const auto start=Compose(pose.tracking,s,runtime);runtime.position.x+=action.velocity.x;runtime.position.y+=action.velocity.y;runtime.position.z+=action.velocity.z;
    const auto end=Compose(pose.tracking,s,runtime);
    if(!start||!end){command.buttons[0]=0;return false;}
    Gesture g{};g.owner=pose;g.kind=action.action;g.tick=GetTickCount64();g.launch=*start;
    g.velocity={end->values[3][0]-start->values[3][0],end->values[3][1]-start->values[3][1],end->values[3][2]-start->values[3][2]};
    const float n=std::sqrt(g.velocity.x*g.velocity.x+g.velocity.y*g.velocity.y+g.velocity.z*g.velocity.z);
    if(n>.01f){
        const stereo::Vec3 z{g.velocity.x/n,g.velocity.y/n,g.velocity.z/n};
        stereo::Vec3 x{z.z,0,-z.x};float a=std::sqrt(x.x*x.x+x.z*x.z);if(a<.01f){x={1,0,0};a=1;}
        x={x.x/a,x.y/a,x.z/a};const stereo::Vec3 y{z.y*x.z-z.z*x.y,z.z*x.x-z.x*x.z,z.x*x.y-z.y*x.x};
        g.launch.values[0]={x.x,x.y,x.z,0};g.launch.values[1]={y.x,y.y,y.z,0};g.launch.values[2]={z.x,z.y,z.z,0};
    }
    g.serial=++nextGesture;if(!g.serial)g.serial=++nextGesture;
    AcquireSRWLockExclusive(&sampleLock);gesture=g;ReleaseSRWLockExclusive(&sampleLock);return action.primeEdge;
}
}

namespace bfvr::bf2142 {
void SetNativeWeaponHeld(bool held){
    if(InterlockedExchange(&weaponHeld,held?1:0)!=(held?1:0)){InterlockedExchange(&gripTransition,1);ResetMotionActions();}
}
bool NativeWeaponHeld(){return InterlockedCompareExchange(&weaponHeld,0,0)!=0;}
bool NativeWeaponInputReady(){return NativeWeaponHeld() && InterlockedCompareExchange(&gripTransition,0,0)==0;}
bool ReadNativeGrenadeTrajectory(GrenadeTrajectory* result){
    if(!result)return false;TrackedWeaponFrame f;
    if(!ReadTrackedWeaponFrame(&f)||Kind(f.weapon)!=MotionKind::Frag)return false;
    Sample s;FirePose pose;AcquireSRWLockShared(&sampleLock);s=current;pose=firePose;ReleaseSRWLockShared(&sampleLock);
    if(pose.weapon!=f.weapon||pose.generation!=s.input.predictedDisplayTime||!OwnedFireInterface(pose))return false;
    float speed=0;if(!NativeSpeed(f.weapon,&speed)||speed<1||speed>40)return false;
    stereo::Vec3 inherited{};
    __try {
        // Native 0x1f31d0 adds the owning infantry physics velocity after
        // speed * launch.forward. Exact accessor/vtable guard; no game call.
        const auto physics=Read<BYTE*>(pose.soldier,0x50);
        if(physics){
            const BYTE getter[]={0x8d,0x41,0x78,0xc3};
            if(Read<void*>(physics,0)!=game+0x593aa0 || Read<void*>(game+0x593aa0,0x64)!=game+0x2c5560 ||
                std::memcmp(game+0x2c5560,getter,sizeof(getter)))return false;
            inherited=Read<stereo::Vec3>(physics,0x78);
        }
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
    if(!std::isfinite(inherited.x)||!std::isfinite(inherited.y)||!std::isfinite(inherited.z)||
        inherited.x*inherited.x+inherited.y*inherited.y+inherited.z*inherited.z>400)return false;
    const auto launch=TrackedFragLaunch(f.world);const auto& m=launch.values;
    *result={{m[3][0],m[3][1],m[3][2]},
        {speed*m[2][0]+inherited.x,speed*m[2][1]+inherited.y,speed*m[2][2]+inherited.z},9.81f*.55f};
    return true;
}
}
