#pragma once
#include "StereoCamera.h"
#include <optional>
namespace bfvr::bf2142 {
struct WorldMarkerPoint { float x=0,y=0,z=0,distance=0;bool edge=false; };
// Clamp a direction once in a common head frustum, then project the same
// virtual point into each asymmetric eye. Native NDC clamps are not stereo.
std::optional<WorldMarkerPoint> ProjectWorldMarker(const stereo::Vec3& world,
    const EyeCamera& head,const EyeCamera& eye,float limit=.65f) noexcept;
}
