#pragma once
#include "TrackingMath.h"
#include "ControllerHandPose.h"
#include <array>
namespace bfvr::bf2142 {
using HandBones=std::array<stereo::Matrix4,70>;
struct HandBindings {stereo::Matrix4 leftFromWeapon{},rightFromWeapon{};};
std::optional<HandBindings> CaptureHandBindings(const HandBones& native) noexcept;
struct HandFrame {
    stereo::Matrix4 head{},leftGrip{},leftAim{},rightGrip{},rightAim{};
    std::optional<HandBindings> bindings;
    std::optional<stereo::Matrix4> torso; // level input frame in skeleton coordinates
    bool leftValid=false,rightValid=false,supportPressed=false,wasSupporting=false;
    bool supportReady=true;
    bool fingerPoses=false;
    std::array<float,5> leftCurls{},rightCurls{};
    bool knifeGrip=false,pistolGrip=false,weaponHeld=true;
    std::optional<ControllerHandPose> leftPalm,rightPalm;
    const HandBones* handReference=nullptr; // borrowed only during the synchronous hand solve
};
struct HandResult { HandBones bones{}; bool supporting=false; };
// Pure model-space solve. Native animation remains the source for finger poses,
// arm proportions, wrist-to-item binding, recoil and all sixteen mesh parts.
std::optional<HandResult> SolveTrackedHands(const HandBones& native,const HandFrame& frame) noexcept;
bool PoseEmptyFingers(HandBones&,int wrist,const stereo::Matrix4& palm,const std::array<float,5>& curls) noexcept;
bool PoseFreeFingers(HandBones&,int wrist,const std::array<float,5>& curls) noexcept;
struct ArmSolution { stereo::Vec3 elbow{},wrist{}; };
std::optional<ArmSolution> SolveArm(stereo::Vec3 shoulder,stereo::Vec3 wrist,
    float upperLength,float lowerLength,bool left) noexcept;
bool SupportGripEligible(bool pressed,bool previous,float distance,float handSeparation) noexcept;
}
