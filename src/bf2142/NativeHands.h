#pragma once
#include "StereoSession.h"
#include "StereoCamera.h"
#include "ShoulderRadio.h"
#include <dxgiformat.h>
#include <vector>
namespace bfvr::bf2142 {
struct TrackedWeaponFrame {void* weapon=nullptr;stereo::Matrix4 world{};std::array<char,49> name{};};
bool ReadTrackedWeaponFrame(TrackedWeaponFrame* result,bool previousSample=false);
bool InstallNativeHands(LogFunction logger);
void PublishNativeHands(const shared::SharedControllerSample* sample,const stereo::Pose& reference,
    const stereo::Pose& head,float worldScale,float heightOffset,const RadioFrame& radio={});
void ClearNativeHands();
void DrawNativeOptic(std::vector<DWORD>& pixels,UINT width,UINT height,DXGI_FORMAT format,const EyeCamera& eye);
}

namespace bfvr::bf2142 {bool IsLocalTrackedWeapon(void* weapon);}

namespace bfvr::bf2142 {bool ReadNativeInventory(std::array<bool,10>* present,int* selected=nullptr,std::array<std::array<char,49>,10>* names=nullptr,std::uint64_t* owner=nullptr);}

namespace bfvr::bf2142 {bool ReadNativeRightGripOffset(stereo::Vec3* offset);}

namespace bfvr::bf2142 {void EnableFingerPoses(bool enabled);}

namespace bfvr::bf2142 {
inline bool InventoryParentMatches(const void* parent,const void* soldier) noexcept {
    return soldier && (!parent || parent==soldier);
}
}

namespace bfvr::bf2142 {
struct ControllerCommand;
void ConfigureMotionActions(bool enabled);
void ResetMotionActions();
bool UpdateNativeMotion(const shared::SharedControllerSample*,const stereo::Pose& head,ControllerCommand&);
}

namespace bfvr::bf2142 {
void SetNativeWeaponHeld(bool held);
bool NativeWeaponHeld();
bool NativeWeaponInputReady();
struct GrenadeTrajectory;
bool ReadNativeGrenadeTrajectory(GrenadeTrajectory*);
}
