#pragma once
#include "stereo/StereoMath.h"
#include <optional>
namespace bfvr::bf2142 {
struct CameraInput {
    stereo::Matrix4 world;
    float nearPlane = 0;
    float farDelta = 0;
};
struct EyeCamera {
    stereo::Matrix4 world;
    stereo::Matrix4 projection;
    float cullingFov = 0;
    float cullingAspect = 0;
};
std::optional<CameraInput> CloseWeaponCamera(const CameraInput&) noexcept;
std::optional<EyeCamera> MakeEyeCamera(const CameraInput& source,
    const stereo::Pose& reference, const stereo::Pose& eye,
    const stereo::FovTangents& fov, float worldScale=1.f, float heightOffset=0.f) noexcept;
}
