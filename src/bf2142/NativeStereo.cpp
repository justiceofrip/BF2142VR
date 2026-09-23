#include "StereoSession.h"
#include "StereoCamera.h"
#include "TrackingMath.h"
#include "NativeQueryGuard.h"
#include "NativeComfort.h"
#include "NativeVehicle.h"
#include "NativeHudPointer.h"
#include "NativeCrosshair.h"
#include <MinHook.h>
#include <array>
#include <cmath>
#include <cstring>
#include <initializer_list>
namespace bfvr::bf2142 {
namespace {
BYTE* image=nullptr;
NativeRender nativeRender=nullptr;
using NativeHud=void (__thiscall*)(void*);
NativeHud nativeHud=nullptr;
BYTE* gameImage=nullptr;
using ProjectionBuilder=void (__thiscall*)(void*);
using CameraSetter=void (__thiscall*)(void*,const stereo::Matrix4*);
ProjectionBuilder nativeProjection=nullptr;
CameraSetter setCamera=nullptr;
float worldScale=1.f,heightOffset=0.f;
bool handTracking=false;
stereo::Pose handHead{},referenceHand{},currentHand{};
LogFunction logMessage=nullptr;
constexpr size_t viewSize=0x434, stateStart=0x18;
struct Override {
    void* view=nullptr;
    std::array<BYTE,viewSize-stateStart> saved{};
    EyeCamera eye{};float nearPlane=0,farDelta=0;
};
// Native rendering and all camera overrides occur on the game's render thread.
thread_local std::array<Override,2> overrides{};
thread_local bool eyeActive=false;
thread_local bool replayActive=false;
thread_local std::array<stereo::Matrix4,2> comfortSources{};
thread_local std::array<bool,2> comfortValid{};
bool installed=false;thread_local bool hideWeapon=false;

template<class T> T Read(void* object,size_t offset) {
    T value{}; std::memcpy(&value,static_cast<BYTE*>(object)+offset,sizeof(T)); return value;
}
template<class T> void Write(void* object,size_t offset,const T& value) {
    std::memcpy(static_cast<BYTE*>(object)+offset,&value,sizeof(T));
}
bool Match(const BYTE* target,std::initializer_list<int> bytes) {
    size_t i=0;
    for (int value:bytes) { if (value>=0 && target[i]!=value) return false; ++i; }
    return true;
}
bool ExecutableRva(DWORD rva) {
    const auto* nt=reinterpret_cast<const IMAGE_NT_HEADERS*>(image+reinterpret_cast<IMAGE_DOS_HEADER*>(image)->e_lfanew);
    const auto* section=IMAGE_FIRST_SECTION(nt);
    for (WORD i=0;i<nt->FileHeader.NumberOfSections;++i)
        if ((section[i].Characteristics&IMAGE_SCN_MEM_EXECUTE) && rva>=section[i].VirtualAddress &&
            rva+64<section[i].VirtualAddress+section[i].Misc.VirtualSize) return true;
    return false;
}
bool ValidateProfile() {
    __try {
        const auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);
        if (dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>0x1000) return false;
        const auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(image+dos->e_lfanew);
        if (nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
            nt->OptionalHeader.SizeOfImage<0x220000 || nt->OptionalHeader.SizeOfImage>0x1000000) return false;
        if (!ExecutableRva(0x2cef0) || !ExecutableRva(0x66f10) || !ExecutableRva(0x67090)) return false;
        if (!Match(image+0x2cef0,{0x55,0x8b,0xec,0x81,0xec,0x04,0x03,0,0,0x53,0x56,0x8b,0xf1,0x8b,0x0d})) return false;
        if (!Match(image+0x66f10,{0x55,0x8b,0xec,0x51,0x56,0x8b,0xf1,0xb8,1,0,0,0,0x84,0x86,0x28,4,0,0})) return false;
        if (!Match(image+0x67090,{0x55,0x8b,0xec,0x8b,0x45,8,0x56,0x8b,0xf1,0x50,0x8d,0x4e,0x40,0xe8,-1,-1,-1,-1,
            0xb0,1,0xc7,0x86,0x24,4,0,0,3,0,0,0,0x88,0x86,0x2c,4,0,0,0x88,0x86,0x2d,4,0,0,0x5e,0x5d,0xc2,4,0})) return false;
        if (!Match(image+0xd0c0,{0x8b,0x81,0xf8,0,0,0,0xc3}) ||
            !Match(image+0xd0d0,{0x8b,0x81,0xfc,0,0,0,0xc3}) ||
            !Match(image+0x66dd0,{0x8d,0x41,0x40,0xc3})) return false;
        auto** rendererTable=reinterpret_cast<void**>(image+0x1c1868);
        auto** viewTable=reinterpret_cast<void**>(image+0x1c6610);
        return rendererTable[10]==image+0x2cef0 && rendererTable[21]==image+0xd0c0 &&
            rendererTable[55]==image+0xd0d0 && viewTable[25]==image+0x67090 &&
            viewTable[26]==image+0x66dd0 && viewTable[28]==image+0x670c0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool ValidateGameProfile() {
    __try {
        const auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(gameImage);
        if(!dos || dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>0x1000)return false;
        const auto* nt=reinterpret_cast<IMAGE_NT_HEADERS*>(gameImage+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
            nt->OptionalHeader.SizeOfImage<0x5a6b70 || nt->OptionalHeader.SizeOfImage>0x4000000)return false;
        return Match(gameImage+0x34e610,{0x83,0x3d,-1,-1,-1,-1,0,0x56,0x8b,0xf1,0x75,0x3b,0x83,0x3d}) &&
            reinterpret_cast<void**>(gameImage+0x5a6b50)[7]==gameImage+0x34e610 &&
            Match(gameImage+0xb7530,{0x8b,0x41,0x6c,0xc3}) &&
            reinterpret_cast<void**>(gameImage+0x5292a0)[12]==gameImage+0xb7530;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool WorldActive(void* renderer) {
    __try {
        if (!renderer || *static_cast<void**>(renderer)!=image+0x1c1868) return false;
        void* players=Read<void*>(image,0x221a58);
        return players && *static_cast<void**>(players)==gameImage+0x5292a0 && Read<void*>(players,0x6c);
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
bool GetViews(void* renderer,void** world,void** weapon) {
    __try {
        // Login has a camera but no local player. Conversely a transient camera
        // mode change must never reclassify an existing player as a flat menu.
        if(!WorldActive(renderer))return false;
        *world=Read<void*>(renderer,0xf8); *weapon=Read<void*>(renderer,0xfc);
        if (!*world || !*weapon || *world==*weapon) return false;
        if (*static_cast<void**>(*world)!=image+0x1c6610 || *static_cast<void**>(*weapon)!=image+0x1c6610) return false;
        return Read<DWORD>(*world,0x18)==0 && Read<DWORD>(*weapon,0x18)==0;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
bool SaveView(void* view,Override* target,CameraInput* camera) {
    __try {
        std::memcpy(target->saved.data(),static_cast<BYTE*>(view)+stateStart,target->saved.size());
        camera->world=Read<stereo::Matrix4>(view,0x40);
        camera->nearPlane=Read<float>(view,0x2c); camera->farDelta=Read<float>(view,0x30);
        target->view=view;
        return true;
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
void __fastcall CameraSetterHook(void* view,void*,const stereo::Matrix4* world) {
    if(eyeActive)for(const auto& item:overrides)if(item.view==view) {setCamera(view,&item.eye.world);return;}
    setCamera(view,world);
}
void __fastcall ProjectionHook(void* view,void*) {
    nativeProjection(view);
    if (!eyeActive) return;
    for (const auto& item:overrides) if (item.view==view) {
        Write(view,0x140,item.eye.projection);
        // Preserve the native derived-matrix dirty bits; the base projection is current.
        Write<DWORD>(view,0x428,Read<DWORD>(view,0x428)&~1u);
        return;
    }
}
void __fastcall HudHook(void* hud,void*) {
    if(IsScopeRender())return; // Scope texture contains the world only.
    const bool captured=StereoHudBegin();
    {CrosshairScope crosshair;nativeHud(hud);}
    if (captured) StereoHudEnd();
}
bool __fastcall RenderHook(void* renderer,void*,double delta,float interpolation) {
    if (replayActive) return nativeRender(renderer,delta,interpolation);
    replayActive=true;
    const bool result=RenderStereo(renderer,nativeRender,delta,interpolation);
    replayActive=false;
    return result;
}
}
bool NativeWorldActive(void* renderer) {return installed && WorldActive(renderer);}
bool NativeViewsAvailable(void* renderer) {
    void* world=nullptr; void* weapon=nullptr;
    return installed && GetViews(renderer,&world,&weapon);
}
bool BeginNativeEye(void* renderer,const stereo::Pose& reference,const shared::SharedPresentationView& eye) {
    void* views[2]{};
    if (eyeActive || !GetViews(renderer,&views[0],&views[1])) return false;
    const stereo::Pose pose{{eye.pose.positionX,eye.pose.positionY,eye.pose.positionZ},
        {eye.pose.orientationX,eye.pose.orientationY,eye.pose.orientationZ,eye.pose.orientationW}};
    const stereo::FovTangents fov{std::tan(eye.fov.angleLeft),std::tan(eye.fov.angleRight),
        std::tan(eye.fov.angleUp),std::tan(eye.fov.angleDown)};
    // Validate both views before the first write.
    for (size_t i=0;i<2;++i) {
        CameraInput input{};
        if (!SaveView(views[i],&overrides[i],&input)) return false;
        // Pin the native comfort source across the pair. Neither the previous
        // eye nor renderer-side animation is allowed to become the next base.
        if(!IsSecondStereoEye())comfortValid[i]=ReadNativeVehicleCamera(input.world,&comfortSources[i])||ReadNativeComfortCamera(input.world,&comfortSources[i]);
        if(comfortValid[i])input.world=comfortSources[i];
        if(i==1){const auto close=CloseWeaponCamera(input);if(!close)return false;input=*close;}
        overrides[i].nearPlane=input.nearPlane;overrides[i].farDelta=input.farDelta;
        const auto transformed=MakeEyeCamera(input,reference,pose,fov,worldScale,heightOffset);
        if (!transformed) return false;
        overrides[i].eye=*transformed;
        if(i==1 && handTracking) {
            const auto weapon=TrackedWeaponCamera(input.world,transformed->world,handHead,referenceHand,currentHand,worldScale);
            if(weapon)overrides[i].eye.world=*weapon;
        }
    }
    eyeActive=true;
    for (auto& item:overrides) {
        Write(item.view,0x2c,item.nearPlane);Write(item.view,0x30,item.farDelta);
        Write(item.view,0x24,item.eye.cullingFov);
        Write(item.view,0x34,item.eye.cullingAspect);
        Write<DWORD>(item.view,0x428,15);
        setCamera(item.view,&item.eye.world);
    }
    return true;
}
bool BeginNativeScope(void* renderer,const stereo::Matrix4& world,const stereo::FovTangents& fov){
    void* views[2]{};if(eyeActive || !InverseRigid(world) || !GetViews(renderer,&views[0],&views[1]))return false;
    for(size_t i=0;i<2;++i){
        CameraInput source{};if(!SaveView(views[i],&overrides[i],&source))return false;
        source.world=world;
        // Keep the local first-person model out of the telescope source, while
        // retaining the world's native far plane and normal geometry. All view
        // fields are restored by EndNativeEye, including on a failed replay.
        if(i==1){source.nearPlane=4;source.farDelta=1;}
        overrides[i].nearPlane=source.nearPlane;overrides[i].farDelta=source.farDelta;
        const auto camera=MakeEyeCamera(source,{}, {},fov);
        if(!camera)return false;overrides[i].eye=*camera;
    }
    eyeActive=true;
    for(auto& item:overrides){
        Write(item.view,0x2c,item.nearPlane);Write(item.view,0x30,item.farDelta);
        Write(item.view,0x24,item.eye.cullingFov);Write(item.view,0x34,item.eye.cullingAspect);
        Write<DWORD>(item.view,0x428,15);setCamera(item.view,&item.eye.world);
    }
    return true;
}
bool ReadNativeEyeCamera(EyeCamera* camera){if(!camera||!eyeActive)return false;*camera=overrides[0].eye;return true;}
void EndNativeEye() {
    if (!eyeActive) return;
    eyeActive=false;
    for (auto& item:overrides) {
        std::memcpy(static_cast<BYTE*>(item.view)+stateStart,item.saved.data(),item.saved.size());
        item.view=nullptr;
    }
}
void ConfigureNativeTracking(float scale,float height,const stereo::Pose* head,const stereo::Pose* referenceGrip,const stereo::Pose* grip) {
    worldScale=scale;heightOffset=height;handTracking=head && referenceGrip && grip;
    if(handTracking){handHead=*head;referenceHand=*referenceGrip;currentHand=*grip;}
}
bool InstallNativeStereo(LogFunction logger) {
    if (installed) return true;
    logMessage=logger;
    image=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9_ori.dll"));
    if (!image) image=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9.dll"));
    if (!image || !ValidateProfile()) {
        logMessage("Native stereo disabled: renderer camera/frame signatures or vtables do not match."); return false;
    }
    gameImage=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
    if (!ValidateGameProfile()) {
        logMessage("Native stereo disabled: HUD manager signature/vtable mismatch."); return false;
    }
    const auto setter=MH_CreateHook(image+0x67090,reinterpret_cast<void*>(&CameraSetterHook),reinterpret_cast<void**>(&setCamera));
    if(setter!=MH_OK)return false;
    const auto a=MH_CreateHook(image+0x66f10,reinterpret_cast<void*>(&ProjectionHook),reinterpret_cast<void**>(&nativeProjection));
    if (a!=MH_OK) {MH_RemoveHook(image+0x67090);return false;}
    const auto b=MH_CreateHook(image+0x2cef0,reinterpret_cast<void*>(&RenderHook),reinterpret_cast<void**>(&nativeRender));
    if (b!=MH_OK) { MH_RemoveHook(image+0x66f10); MH_RemoveHook(image+0x67090);return false; }
    const auto c=MH_CreateHook(gameImage+0x34e610,reinterpret_cast<void*>(&HudHook),reinterpret_cast<void**>(&nativeHud));
    if (c!=MH_OK) { MH_RemoveHook(image+0x66f10); MH_RemoveHook(image+0x2cef0);MH_RemoveHook(image+0x67090);return false; }
    if (MH_EnableHook(image+0x66f10)!=MH_OK || MH_EnableHook(image+0x2cef0)!=MH_OK || MH_EnableHook(gameImage+0x34e610)!=MH_OK || MH_EnableHook(image+0x67090)!=MH_OK) {
        for (BYTE* target:{image+0x66f10,image+0x2cef0,gameImage+0x34e610,image+0x67090}) { MH_DisableHook(target); MH_RemoveHook(target); }
        return false;
    }
    installed=true;
    if(!InstallNativeHudPointer(logger))logger("Deployment HUD pointer hook unavailable: profile mismatch.");
    if(!InstallNativeComfort(logger))logger("Infantry VR comfort unavailable: native heading/recoil profile or hook mismatch.");
    if(!InstallNativeQueryGuard(image,logger))logger("Native renderer query guard unavailable: profile or hook mismatch.");
    if(!InstallNativeMenus(image,logger)) logger("Native Flash menu hook unavailable; HUD capture remains active.");
    logMessage("Native stereo camera/frame/HUD hooks installed; renderer=%p; native aiming retained.",image);
    return true;
}
}

namespace bfvr::bf2142 {void HideNativeWeaponForReplay(bool hide){hideWeapon=hide;}}
