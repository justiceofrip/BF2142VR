#pragma once
#include "TrackingMath.h"
#include <cstdint>
#include <string_view>
namespace bfvr::bf2142 {
enum class MotionKind { None, Knife, Frag };
inline MotionKind MotionWeapon(std::string_view name) noexcept {
    return name=="knife"?MotionKind::Knife:name=="unl_grenade_frag"?MotionKind::Frag:MotionKind::None;
}
struct MotionInput {
    std::uint64_t owner=0;std::int64_t time=0;
    stereo::Vec3 hand{},head{};
    MotionKind kind=MotionKind::None;
    bool active=false,grip=false,trigger=false;
};
struct MotionResult {
    MotionKind action=MotionKind::None;
    stereo::Vec3 position{},velocity{};
    bool primary=false,primed=false,primeEdge=false;
};
class MotionActions {
public:
    MotionResult Update(const MotionInput&) noexcept;
    void Reset() noexcept {*this={};}
private:
    MotionInput previous{};
    stereo::Vec3 velocity{},relativeVelocity{};
    float travel=0;
    std::int64_t started=0,restSince=0,swingSince=0,lastStrike=0,pressedUntil=0,primeTime=0;
    bool rested=false,primed=false;
};
// Convert a runtime-space displacement/velocity into the native world frame.
stereo::Vec3 TransformMotionVector(stereo::Vec3,const stereo::Matrix4&) noexcept;
}
