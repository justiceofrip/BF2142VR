#pragma once
#include "StereoCamera.h"
#include <windows.h>
#include <dxgiformat.h>
#include <vector>
namespace bfvr::bf2142 {
struct GrenadeTrajectory {stereo::Vec3 start{},velocity{};float gravity=9.81f*.55f;};
// Shared by the actual pre-velocity launch hook and the stereo guide.
inline stereo::Matrix4 TrackedFragLaunch(const stereo::Matrix4& weapon) noexcept {
    auto launch=weapon;for(int i=0;i<3;++i)launch.values[3][i]+=.18f*weapon.values[2][i];return launch;
}
std::optional<stereo::Vec3> GrenadePoint(const GrenadeTrajectory&,float seconds) noexcept;
size_t DrawGrenadeArc(std::vector<DWORD>& pixels,unsigned width,unsigned height,DXGI_FORMAT,const EyeCamera&,const GrenadeTrajectory&);
}
