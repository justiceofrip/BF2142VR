#pragma once
#include "GunOptics.h"
#include "StereoSession.h"
namespace bfvr::bf2142 {
bool InstallNativeOptics(LogFunction);
bool ReadNativeOptic(GunOptic*);
void DisableNativeOptics();
}

namespace bfvr::bf2142 {
void ConfigureAutomaticAds(bool enabled);
// Called once after the normal eye pair; never advances game input per eye.
void UpdateAutomaticAds(const std::array<EyeCamera,2>& eyes);
void RequestAutomaticAds(bool gameplay);
void ResetAutomaticAds();
bool NativeWeaponAds(void* weapon);
bool HandlesAutomaticAds();
// Once per controller command; never emit input in menus, vehicles or stale tracking.
bool AutomaticAdsButton(bool allowInput);
}

namespace bfvr::bf2142 {bool ReadNativeSightAlignmentOffset(stereo::Vec3* offset);}
