#pragma once
#include "TrackingMath.h"
#include <cmath>
namespace bfvr::bf2142 {
// Offset from native input yaw to the selected recoil-free tracking heading.
// Apply it only to the movement-basis read, never to the stored camera/body.
inline std::optional<float> MovementYawOffset(double recoilDegrees,float trackedRadians) noexcept {
 if(!std::isfinite(recoilDegrees)||!std::isfinite(trackedRadians))return {};
 return float(std::remainder(double(trackedRadians)*57.29577951308232-recoilDegrees,360.0));
}
inline std::optional<stereo::Matrix4> MakeMovementCamera(const stereo::Matrix4& native,float offsetDegrees) noexcept {
 if(!InverseRigid(native)||!std::isfinite(offsetDegrees)||std::abs(offsetDegrees)>180)return {};
 const float x=native.values[2][0],z=native.values[2][2];if(std::hypot(x,z)<.01f)return {};
 const double a=std::atan2(double(x),double(z))+double(offsetDegrees)*.017453292519943295;
 const float c=float(std::cos(a)),s=float(std::sin(a));stereo::Matrix4 out{};
 out.values[0]={c,0,-s,0};out.values[1]={0,1,0,0};out.values[2]={s,0,c,0};out.values[3]=native.values[3];return out;
}
}
