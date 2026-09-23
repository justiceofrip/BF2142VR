#include "BodyInventory.h"
#include "TrackingMath.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
constexpr std::array<BodySlot,7> slots{{
    {"Back / primary",{.20f,-.18f,.22f},.23f,3},
    {"Right holster / sidearm",{.27f,-.67f,.015f},.19f,2},
    {"Chest / knife",{-.11f,-.36f,-.14f},.16f,1},
    {"Left belt / utility",{-.29f,-.66f,.015f},.18f,4},
    {"Rear belt / utility",{-.17f,-.66f,.22f},.18f,5},
    {"Front belt / grenade",{.10f,-.64f,-.14f},.16f,7},
    {"Right rear belt / utility",{.24f,-.67f,.24f},.16f,6}
}};
constexpr unsigned scanCodes[]={0,DIK_1,DIK_2,DIK_3,DIK_4,DIK_5,DIK_6,DIK_7,DIK_8,DIK_9};
float Wrap(float a){return std::remainder(a,6.283185307f);}
stereo::Pose Pose(const shared::SharedPresentationPose& p){return {{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}};}
}
const std::array<BodySlot,7>& BodySlots() noexcept{return slots;}
BodyInventoryResult BodyInventory::Update(bool enabled,const shared::SharedControllerSample& sample,const stereo::Pose& head,
    const std::array<bool,10>& inventory) noexcept {
    BodyInventoryResult result;
    constexpr DWORD tracked=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|
        shared::kControllerHandFlagGripOrientationValid|shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked;
    const auto& right=sample.hands[1];
    const auto normalized=stereo::MakeRelativePose(stereo::Pose{{0,0,0},{0,0,0,1}},head);
    const bool pressed=(right.flags&shared::kControllerHandFlagSqueezeActive)&&std::isfinite(right.squeezeValue)&&right.squeezeValue>.65f;
    if(!enabled || !(sample.flags&shared::kControllerSampleFlagSessionFocused) || !normalized || sample.predictedDisplayTime<=0){
        // Keep torso heading through a brief menu/tracking interruption; cancel
        // all interaction edges. Recenter/respawn explicitly resets the anchor.
        gripTracked=false;pendingKey=0;keyUntil=0;lastTime=0;return result;
    }
    const auto q=normalized->orientation;
    // Flatten the actual forward vector, not UI Euler yaw. The latter changes
    // heading during pitch and spins the belt as the player looks down.
    const float forwardX=-2*(q.x*q.z+q.w*q.y),forwardZ=-(1-2*(q.x*q.x+q.y*q.y));
    const float horizontal=std::hypot(forwardX,forwardZ);
    const float yaw=horizontal>.2f?std::atan2(-forwardX,-forwardZ):bodyYaw;
    const LONGLONG now=sample.predictedDisplayTime;
    const bool gap=!lastTime || now<=lastTime || now-lastTime>250000000;
    const float dt=gap?0:std::min(.05f,float(now-lastTime)*1.e-9f);lastTime=now;
    if(!initialized){initialized=true;bodyYaw=yaw;}
    // Looking down at a holster or reaching it must not rotate it out of reach.
    const float error=Wrap(yaw-bodyYaw),dead=.65f;
    if(horizontal>.65f && !gripHeld && std::abs(error)>dead)
        bodyYaw=Wrap(bodyYaw+std::clamp(error-std::copysign(dead,error),-1.6f*dt,1.6f*dt));
    result.anchor={head.position,{0,std::sin(bodyYaw*.5f),0,std::cos(bodyYaw*.5f)}};
    // Account for the eyes moving around the neck when nodding. Preserve the
    // old eye-relative layout when upright; pitch never tilts the equipment.
    const auto rotate=[](const stereo::Quaternion& a,stereo::Vec3 v){
        const stereo::Vec3 t{2*(a.y*v.z-a.z*v.y),2*(a.z*v.x-a.x*v.z),2*(a.x*v.y-a.y*v.x)};
        return stereo::Vec3{v.x+a.w*t.x+a.y*t.z-a.z*t.y,v.y+a.w*t.y+a.z*t.x-a.x*t.z,v.z+a.w*t.z+a.x*t.y-a.y*t.x};
    };
    const auto neck=rotate(q,{0,-.12f,.08f}),upright=rotate(result.anchor.orientation,{0,-.12f,.08f});
    result.anchor.position.x+=neck.x-upright.x;result.anchor.position.y+=neck.y-upright.y;result.anchor.position.z+=neck.z-upright.z;
    result.anchorValid=true;
    if((right.flags&tracked)!=tracked){gripTracked=false;pendingKey=0;keyUntil=0;gripHeld=false;return result;}
    const auto hand=stereo::MakeRelativePose(result.anchor,Pose(right.gripPose));
    if(!hand){gripTracked=false;pendingKey=0;keyUntil=0;gripHeld=false;return result;}
    if(gap || !gripTracked){gripHeld=pressed;pendingKey=0;keyUntil=0;}
    gripTracked=true;
    float best=1;
    for(unsigned i=0;i<slots.size();++i){
        const auto& slot=slots[i];if(slot.item>=inventory.size()||!inventory[slot.item])continue;
        const auto p=hand->position;
        const float dx=p.x-slot.offset.x,dy=p.y-slot.offset.y,dz=p.z-slot.offset.z;
        const float distance=(dx*dx+dy*dy+dz*dz)/(slot.radius*slot.radius);
        if(std::isfinite(distance)&&distance<best){best=distance;result.hovered=int(i);}
    }
    if(pressed && !gripHeld && result.hovered>=0){
        result.selected=result.hovered;pendingKey=scanCodes[slots[result.hovered].item];keyUntil=now+120000000;
    }
    gripHeld=pressed;
    if(now<keyUntil)result.key=pendingKey;else pendingKey=0;
    return result;
}
}
