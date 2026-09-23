#pragma once
#include "stereo/StereoMath.h"
#include <cstdint>
#include <optional>
namespace bfvr::bf2142 {
// Recoil remains in the game's simulation. Only the tracking-space heading
// removes its accumulated contribution. A pause/recenter is not a new owner.
struct ComfortYaw {
    std::uintptr_t owner=0;
    double recoilDegrees=0;
    void Bind(std::uintptr_t identity) noexcept;
    void AddRecoil(float degrees) noexcept;
    std::optional<float> Heading(float bodyDegrees,float localDegrees) const noexcept;
};
struct PhysicalCameraHeight {
    std::uintptr_t owner=0;std::uint64_t since=0;float candidate=0,offset=0;bool ready=false;
    float Update(std::uintptr_t identity,int stance,float nativeY,float bodyY,std::uint64_t now) noexcept;
};
// Use absolute input heading, never the animated/rendered eye orientation.
// Position remains native so movement and stance changes retain their origin.
std::optional<stereo::Matrix4> MakeComfortCamera(const stereo::Matrix4& nativeCamera,
    float headingDegrees) noexcept;
}
