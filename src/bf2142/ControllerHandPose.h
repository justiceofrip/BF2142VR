#pragma once
#include "TrackingMath.h"
#include <array>
#include <cstdint>
#include <string_view>
namespace bfvr::bf2142 {
// Captured once per local skeleton, in anatomical palm space. No weapon
// binding or subsequently evaluated attack/reload animation is retained.
struct ControllerHandPose { std::array<stereo::Matrix4,21> bones{}; };
std::optional<ControllerHandPose> CaptureControllerHand(
    const std::array<stereo::Matrix4,70>& native,int wrist) noexcept;
struct ControllerHandCache {
    std::optional<ControllerHandPose> left,right;bool settled=false;
    std::array<stereo::Matrix4,70> reference{};
    void Update(const std::array<stereo::Matrix4,70>& native,bool holdingPoseReady) noexcept;
};
stereo::Matrix4 KnifeInGrip(const stereo::Matrix4& grip) noexcept;
bool PistolWeapon(std::string_view name) noexcept;
bool PistolSupportEligible(bool pressed,bool previous,float separation) noexcept;
struct PistolAimFilter {
    stereo::Matrix4 Update(const stereo::Matrix4& aim,bool supported,std::int64_t time) noexcept;
    void Reset() noexcept {active=false;lastTime=0;}
private:
    stereo::Matrix4 filtered{};std::int64_t lastTime=0;bool active=false;
};
}
