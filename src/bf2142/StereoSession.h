#pragma once
#include <windows.h>
#include <d3d9.h>
#include <string>
#include "presenter/SharedPresentationProtocol.h"
#include "stereo/StereoMath.h"
namespace bfvr::bf2142 {
using LogFunction = void (*)(const char*, ...);
using NativeRender = bool (__thiscall*)(void*, double, float);
bool StartStereo(const std::wstring& presenter, const std::wstring& log, LogFunction logger);
void StereoDeviceCreated(IDirect3DDevice9* device);
void StereoPresent(IDirect3DDevice9* device);
void StereoReset();
bool StereoHudBegin(bool standaloneMenu=false);
void StereoHudEnd();
bool StereoOpticHudBegin();
void StereoOpticHudEnd();
bool SuppressStereoPresent(IDirect3DDevice9* device);
bool IsSecondStereoEye();
bool IsScopeRender();
void ConfigureNativeTracking(float scale,float height,const stereo::Pose* head,const stereo::Pose* referenceGrip,const stereo::Pose* grip);
bool RenderStereo(void* renderer, NativeRender original, double delta, float interpolation);
bool InstallNativeStereo(LogFunction logger);
bool NativeViewsAvailable(void* renderer);
bool NativeWorldActive(void* renderer);
bool InstallNativeMenus(BYTE* renderer,LogFunction logger);
bool BeginNativeEye(void* renderer, const stereo::Pose& reference,
    const shared::SharedPresentationView& eye);
bool BeginNativeScope(void* renderer,const stereo::Matrix4& world,const stereo::FovTangents& fov);
struct EyeCamera;
bool ReadNativeEyeCamera(EyeCamera* camera);
bool ReadNativeWeaponProjection(stereo::Matrix4* projection);
bool ReadStereoMarkerFrame(EyeCamera* head,EyeCamera* eye);
bool HideStereoWorldMarkers();
void EndNativeEye();
}

namespace bfvr::bf2142 {void HideNativeWeaponForReplay(bool hide);}
