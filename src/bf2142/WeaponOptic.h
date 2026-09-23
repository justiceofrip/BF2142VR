#pragma once
#include "TrackingMath.h"
#include <Windows.h>
#include <vector>
#include <cstdint>
namespace bfvr::bf2142 {
// Provisional non-magnifying optic above the weapon's authored origin. The dot
// is collimated per eye and clipped to the lens, rather than painted on the HUD.
std::optional<stereo::Vec3> OpticDot(const stereo::Matrix4& gun,const stereo::Matrix4& eye) noexcept;
void DrawWeaponOptic(std::vector<DWORD>& pixels,unsigned width,unsigned height,
    const stereo::Matrix4& gun,const stereo::Matrix4& eye,const stereo::Matrix4& projection,bool rgba);
}
