#pragma once
#include "stereo/StereoMath.h"
namespace bfvr::bf2142 {
stereo::Matrix4 Multiply(const stereo::Matrix4& a,const stereo::Matrix4& b) noexcept;
std::optional<stereo::Matrix4> InverseRigid(const stereo::Matrix4& matrix) noexcept;
std::optional<stereo::Matrix4> TrackedWeaponCamera(const stereo::Matrix4& sourceCamera,
    const stereo::Matrix4& eyeCamera,const stereo::Pose& calibrationHead,
    const stereo::Pose& referenceGrip,const stereo::Pose& currentGrip,float scale) noexcept;
std::optional<stereo::Matrix4> MapTrackedFire(const stereo::Matrix4& nativeFire,
    const stereo::Matrix4& nativeCamera,const stereo::Matrix4& gun) noexcept;
float PoseYaw(const stereo::Pose& pose) noexcept;
}
