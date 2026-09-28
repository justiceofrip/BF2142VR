#pragma once
#include "StereoSession.h"
#include "ComfortControls.h"
#include "stereo/DirectionalLocomotion.h"
#include <array>
namespace bfvr::bf2142 {
// One body-slot gesture, consumed by either DirectInput API exactly once.
// The native inventory is checked again at consumption, not just at XR polling.
struct EquipmentSelection {
    std::uint64_t owner=0;LONGLONG gesture=0;unsigned item=0;bool allowed=false;
};
struct ControllerCommand {
    EquipmentSelection selection{};
    std::array<BYTE,256> keys{},blockedPhysicalKeys{};
    std::array<BYTE,8> buttons{},blockedPhysicalButtons{};
    LONG mouseX=0,mouseY=0,wheel=0;
    bool recenter=false;float snapDegrees=0;
};
struct ControllerPolicyState {
    stereo::DigitalLocomotionState movement{};SnapTurn snap;
    LONGLONG previousTime=0;
    bool recenterHeld=false;
    bool nextWeaponHeld=false,previousWeaponHeld=false;
    float remainderX=0,remainderY=0;
};
bool MatchingControllerSample(const shared::SharedControllerSample& sample,LONG sequence,
    const shared::SharedRenderRequest& request,LONG expected) noexcept;
float LocomotionYaw(bool controllerRelative,const stereo::Pose& reference,const stereo::Pose& head,
    const shared::SharedControllerSample& sample) noexcept;
ControllerCommand MapControllers(ControllerPolicyState& state,
    const shared::SharedControllerSample& sample,bool gameplay,float movementYaw,float turnSpeed,bool motionHands=false,bool snapTurning=false,float snapAngle=30.f) noexcept;
}
