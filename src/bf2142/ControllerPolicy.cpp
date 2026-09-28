#include "ControllerPolicy.h"
#include "TrackingMath.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
bool MatchingControllerSample(const shared::SharedControllerSample& s,LONG sequence,
    const shared::SharedRenderRequest& r,LONG expected) noexcept {
    return expected>0 && sequence==expected && s.predictedDisplayTime==r.predictedDisplayTime &&
        r.headPoseValid && r.headPoseTracked && (s.flags&shared::kControllerSampleFlagSessionFocused)!=0;
}
float LocomotionYaw(bool controllerRelative,const stereo::Pose& reference,const stereo::Pose& head,
    const shared::SharedControllerSample& sample) noexcept {
    const auto relativeHead=stereo::MakeRelativePose(reference,head);
    const float fallback=relativeHead?PoseYaw(*relativeHead):0.f;
    if(!controllerRelative)return fallback;
    const auto& hand=sample.hands[0];
    constexpr DWORD tracked=shared::kControllerHandFlagAimActive|shared::kControllerHandFlagAimOrientationValid|shared::kControllerHandFlagAimOrientationTracked;
    if((hand.flags&tracked)!=tracked)return fallback;
    const auto& p=hand.aimPose;
    const auto pose=stereo::MakeRelativePose(reference,{{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}});
    if(!pose)return fallback;
    const auto q=pose->orientation;
    const float x=-2.f*(q.x*q.z+q.w*q.y),z=-(1.f-2.f*(q.x*q.x+q.y*q.y));
    // A vertical or untracked controller has no usable horizontal heading.
    return std::hypot(x,z)>.2f?PoseYaw(*pose):fallback;
}
ControllerCommand MapControllers(ControllerPolicyState& state,const shared::SharedControllerSample& sample,
    bool gameplay,float movementYaw,float turnSpeed,bool motionHands,bool snapTurning,float snapAngle) noexcept {
    ControllerCommand out{};
    if (!(sample.flags&shared::kControllerSampleFlagSessionFocused)) {state={};return out;}
    const auto& l=sample.hands[0];const auto& r=sample.hands[1];
    float dt=state.previousTime?float(sample.predictedDisplayTime-state.previousTime)*1.e-9f:0;
    state.previousTime=sample.predictedDisplayTime;
    if(dt<0 || dt>.25f)dt=0;
    dt=std::min(dt,.05f);
    const auto axis=[](float value) { if(!std::isfinite(value))return 0.f; const float v=std::clamp(value,-1.f,1.f);return std::abs(v)<.2f?0.f:std::copysign((std::abs(v)-.2f)/.8f,v); };
    const auto pressed=[](const auto& hand,DWORD bit) {return (hand.buttons&bit)!=0;};
    const bool modifier=(l.flags&shared::kControllerHandFlagTriggerActive) && std::isfinite(l.triggerValue) && l.triggerValue>.65f;
    const bool leftStick=(l.flags&shared::kControllerHandFlagThumbstickActive)!=0;
    const bool rightStick=(r.flags&shared::kControllerHandFlagThumbstickActive)!=0;
    if(gameplay && leftStick) {
        const auto d=stereo::QuantizeDigitalLocomotion(axis(l.thumbstickX),axis(l.thumbstickY),movementYaw,state.movement);
        out.keys[DIK_W]=d.forward>0?0x80:0;out.keys[DIK_S]=d.forward<0?0x80:0;
        out.keys[DIK_D]=d.horizontal>0?0x80:0;out.keys[DIK_A]=d.horizontal<0?0x80:0;
    } else stereo::ResetDigitalLocomotion(state.movement);
    out.snapDegrees=state.snap.Update(gameplay && rightStick && snapTurning,r.thumbstickX,sample.predictedDisplayTime,snapAngle);
    if(rightStick) {
        const float speed=std::clamp(std::isfinite(turnSpeed)?turnSpeed:600.f,50.f,2000.f);
        const float x=gameplay && snapTurning?0.f:axis(r.thumbstickX)*dt*speed+state.remainderX;
        const float y=gameplay && motionHands?0.f:-axis(r.thumbstickY)*dt*speed+state.remainderY;
        out.mouseX=static_cast<LONG>(x);out.mouseY=static_cast<LONG>(y);
        state.remainderX=x-float(out.mouseX);state.remainderY=y-float(out.mouseY);
    }
    if((r.flags&shared::kControllerHandFlagTriggerActive) && std::isfinite(r.triggerValue) && r.triggerValue>.55f)out.buttons[0]=0x80;
    if(gameplay) {
        const bool a=pressed(r,shared::kControllerHandButtonPrimary);
        out.keys[DIK_SPACE]=a&&!modifier?0x80:0;
        out.keys[DIK_G]=a&&modifier?0x80:0;
        const bool leftPrimary=pressed(l,shared::kControllerHandButtonPrimary);
        const bool leftSecondary=pressed(l,shared::kControllerHandButtonSecondary);
        out.keys[DIK_R]=leftPrimary && !modifier?0x80:0;
        out.keys[DIK_E]=leftSecondary && !modifier?0x80:0;
        const bool next=modifier && leftPrimary,previous=modifier && leftSecondary;
        if(next && !state.nextWeaponHeld)out.wheel=-120;
        if(previous && !state.previousWeaponHeld)out.wheel=120;
        state.nextWeaponHeld=next;state.previousWeaponHeld=previous;
        const bool stickClick=pressed(l,shared::kControllerHandButtonThumbstick);
        out.keys[DIK_LSHIFT]=stickClick?0x80:0;
        out.keys[DIK_LCONTROL]=(!motionHands || modifier) && (l.flags&shared::kControllerHandFlagSqueezeActive) && std::isfinite(l.squeezeValue) && l.squeezeValue>.65f?0x80:0;
        out.buttons[1]= (r.flags&shared::kControllerHandFlagSqueezeActive) && std::isfinite(r.squeezeValue) && r.squeezeValue>.65f?0x80:0;
    } else {
        out.keys[DIK_RETURN]=pressed(r,shared::kControllerHandButtonPrimary)?0x80:0;
        if(leftStick) {out.keys[DIK_UP]=l.thumbstickY>.6f?0x80:0;out.keys[DIK_DOWN]=l.thumbstickY<-.6f?0x80:0;}
        state.nextWeaponHeld=false;state.previousWeaponHeld=false;
    }
    const bool recenter=pressed(r,shared::kControllerHandButtonThumbstick);
    out.recenter=recenter && !state.recenterHeld;state.recenterHeld=recenter;
    out.keys[DIK_TAB]=gameplay && pressed(l,shared::kControllerHandButtonMenu)?0x80:0;
    out.keys[DIK_ESCAPE]=pressed(r,shared::kControllerHandButtonSecondary)?0x80:0;
    return out;
}
}
