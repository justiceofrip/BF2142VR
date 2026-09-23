#include "StereoCamera.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
std::optional<CameraInput> CloseWeaponCamera(const CameraInput& source) noexcept {
    if(!std::isfinite(source.nearPlane)||source.nearPlane<=0||!std::isfinite(source.farDelta)||source.farDelta<=0)return {};
    auto out=source;out.nearPlane=std::min(source.nearPlane,.006f);out.farDelta=source.nearPlane+source.farDelta-out.nearPlane;return out;
}
std::optional<EyeCamera> MakeEyeCamera(const CameraInput& source,
    const stereo::Pose& reference, const stereo::Pose& eye,
    const stereo::FovTangents& fov,float worldScale,float heightOffset) noexcept {
    if (!std::isfinite(source.farDelta) || source.farDelta <= 0) return {};
    if(!std::isfinite(heightOffset)) return {};
    auto world = stereo::ComposeRuntimeHeadWithD3D8Camera(source.world, reference, eye, worldScale);
    if(world)world->values[3][1]+=heightOffset;
    const auto projection = stereo::MakeD3D8ProjectionFromFovTangents(
        fov, source.nearPlane, source.nearPlane + source.farDelta);
    if (!world || !projection) return {};
    const float horizontal = std::max(std::abs(fov.left), std::abs(fov.right));
    const float vertical = std::max(std::abs(fov.down), std::abs(fov.up));
    if (horizontal <= 0 || vertical <= 0) return {};
    // Native culling is symmetric. Enclose the asymmetric runtime frustum;
    // the actual shader projection remains the exact per-eye projection.
    return EyeCamera{*world, *projection, 2.0f * std::atan(vertical), vertical / horizontal};
}
}
