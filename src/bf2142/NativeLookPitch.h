#pragma once
#include "stereo/StereoMath.h"
#include <algorithm>
#include <cmath>
#include <optional>
namespace bfvr::bf2142 {
// Native ladder entry tests the stock look pitch, even when the headset uses
// an independent comfort camera. Keep that hidden pitch tied to the HMD.
inline std::optional<float> NativeHeadPitch(const stereo::Pose& head) noexcept {
 const auto& q=head.orientation;
 const float norm=q.x*q.x+q.y*q.y+q.z*q.z+q.w*q.w;
 if(!std::isfinite(norm)||norm<.99f||norm>1.01f)return {};
 const float x=-2.f*(q.x*q.z+q.w*q.y)/norm;
 const float y=2.f*(q.w*q.x-q.y*q.z)/norm;
 const float z=-(1.f-2.f*(q.x*q.x+q.y*q.y)/norm);
 // Stock entry rejects exactly level/downward look at the bottom. A small
 // native-only upward bias allows a level HMD to enter; looking down still
 // permits descending from above. This never rotates the rendered headset.
 return std::clamp(-std::atan2(y,std::hypot(x,z))*57.295779513f-5.f,-80.f,80.f);
}
inline std::optional<float> NativePitchAxis(float current,float target,float factor) noexcept {
 if(!std::isfinite(current)||!std::isfinite(target)||!std::isfinite(factor)||
    std::abs(current)>180.f||std::abs(target)>80.f||std::abs(factor)<.001f||std::abs(factor)>100.f)return {};
 // Bound catch-up after rejoining/refocusing and respect the signed-short
 // 1/100 stock network codec. Do not accumulate physical mouse pitch in VR.
 const float delta=std::clamp(target-current,-30.f,30.f);
 return std::round(std::clamp(delta/factor,-327.67f,327.67f)*100.f)*.01f;
}
}
