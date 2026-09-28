#include "WristMenu.h"
#include "TrackingMath.h"
#include <cmath>
namespace bfvr::bf2142 {
namespace {
stereo::Pose Pose(const shared::SharedPresentationPose& p){return {{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}};}
stereo::Vec3 Rotate(stereo::Quaternion q,stereo::Vec3 v){const stereo::Vec3 t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};return {v.x+q.w*t.x+q.y*t.z-q.z*t.y,v.y+q.w*t.y+q.z*t.x-q.x*t.z,v.z+q.w*t.z+q.x*t.y-q.y*t.x};}
}
WristFrame WristMenu::Update(bool enabled,const shared::SharedControllerSample& sample,const stereo::Pose& head) noexcept {
 constexpr DWORD grip=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripOrientationValid|shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked;
 constexpr DWORD aim=shared::kControllerHandFlagAimActive|shared::kControllerHandFlagAimPositionValid|shared::kControllerHandFlagAimOrientationValid|shared::kControllerHandFlagAimPositionTracked|shared::kControllerHandFlagAimOrientationTracked;
 const auto& l=sample.hands[0];const auto& r=sample.hands[1];
 if(!enabled||!(sample.flags&shared::kControllerSampleFlagSessionFocused)||(l.flags&grip)!=grip||sample.predictedDisplayTime<=0){Reset();return {};}
 const bool gap=!previous||sample.predictedDisplayTime<previous||sample.predictedDisplayTime-previous>150000000;
 const bool trigger=(r.flags&shared::kControllerHandFlagTriggerActive)&&std::isfinite(r.triggerValue)&&r.triggerValue>.55f;
 if(gap){gazeSince=0;revealed=false;armed=false;pressedUntil=0;consuming=false;triggerHeld=trigger;}
 previous=sample.predictedDisplayTime;frame={};
 const auto hand=Pose(l.gripPose);if(!stereo::MakeRelativePose(head,hand)){Reset();return {};}
 const auto offset=Rotate(hand.orientation,{0,.045f,.09f});
 frame.panel={{hand.position.x+offset.x,hand.position.y+offset.y,hand.position.z+offset.z},head.orientation};
 const auto local=stereo::MakeRelativePose(head,frame.panel);if(!local){Reset();return {};}
 const auto p=local->position;const float distance=std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);
 const bool looking=distance>=.20f&&distance<=.65f&&p.z<-.1f&&std::hypot(p.x,p.y)<-p.z*(revealed?.24f:.14f);
 if(!looking){gazeSince=0;revealed=false;}
 else {if(!gazeSince)gazeSince=previous;if(previous-gazeSince>=400000000)revealed=true;}
 frame.visible=revealed;
 if(frame.visible&&(r.flags&aim)==aim){
  const auto ray=stereo::MakeRelativePose(frame.panel,Pose(r.aimPose));
  if(ray){const auto forward=Rotate(ray->orientation,{0,0,-1});
   if(ray->position.z>.005f&&forward.z<-.05f){const float t=-ray->position.z/forward.z;
    const float x=ray->position.x+t*forward.x,y=ray->position.y+t*forward.y;
    frame.hovered=t>=0&&t<=1.5f&&std::abs(x)<wristWidth*.5f&&std::abs(y)<wristHeight*.5f;
    if(frame.hovered){frame.rayStart=Pose(r.aimPose).position;const auto q=Rotate(frame.panel.orientation,{x,y,.001f});frame.rayEnd={frame.panel.position.x+q.x,frame.panel.position.y+q.y,frame.panel.position.z+q.z};}
   }
  }
 }
 // Looking into the button with a held trigger never deploys or fires the gun.
 if(!trigger){armed=frame.visible;consuming=false;}
 if(frame.hovered&&trigger)consuming=true;
 if(frame.hovered&&trigger&&!triggerHeld&&armed){pressedUntil=previous+120000000;armed=false;}
 if(!frame.visible)armed=false;
 frame.pressed=previous<pressedUntil;triggerHeld=trigger;return frame;
}
void WristMenu::Apply(ControllerCommand& command,bool menuVisible) const noexcept {
 if(frame.hovered||consuming){command.buttons[0]=0;command.blockedPhysicalButtons[0]=1;}
 if(frame.pressed&&!menuVisible)command.keys[0x1c]=0x80; // c_GIEnter / native deploy
}
}
