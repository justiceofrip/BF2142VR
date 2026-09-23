#include "TraversalControls.h"
#include "ComfortCamera.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
void TraversalView::RecordFoot(std::uint64_t id,const stereo::Matrix4& view,std::uint64_t time) noexcept {
 if(!id||!InverseRigid(view)){Reset();return;}
 owner=id;mount=0;camera=view;lastFoot=time;haveFoot=true;
}
std::optional<stereo::Matrix4> TraversalView::Mounted(const TraversalSample& s,const stereo::Matrix4& native,std::uint64_t now) noexcept {
 if(!s.owner||!s.mount||(s.mode!=TraversalMode::Ladder&&s.mode!=TraversalMode::Parachute)||!InverseRigid(s.world)||!InverseRigid(native)){Reset();return {};}
 if(owner!=s.owner){Reset();owner=s.owner;}
 if(mount!=s.mount){
  float distance=0;for(int i=0;i<3;++i){const float d=camera.values[3][i]-native.values[3][i];distance+=d*d;}
  if(!haveFoot||now<lastFoot||now-lastFoot>500||distance>16){
   const float heading=std::atan2(s.world.values[2][0],s.world.values[2][2])*57.295779513f;
   const auto level=MakeComfortCamera(native,heading);if(!level)return {};camera=*level;
  }
  mount=s.mount;mountWorld=s.world;haveFoot=false;
 }else{
  // Follow actual mount translation only. Seat animation, forced look pitch,
  // canopy banking and ladder-end camera offsets cannot rotate/bob the head.
  for(int i=0;i<3;++i)camera.values[3][i]+=s.world.values[3][i]-mountWorld.values[3][i];
  mountWorld=s.world;
 }
 return camera;
}
void TraversalControls::Update(const TraversalSample& s,const shared::SharedControllerSample& input,const stereo::Pose& head,ControllerCommand& c) noexcept {
 const auto now=input.predictedDisplayTime;
 if(!s.owner||s.mode==TraversalMode::Unavailable||!(input.flags&shared::kControllerSampleFlagSessionFocused)||now<=0||!InverseRigid(s.world)){Reset();return;}
 if(s.mode!=TraversalMode::Foot){c.mouseX=c.mouseY=0;c.snapDegrees=0;}
 if(s.mode==TraversalMode::Ladder){
  const auto& stick=input.hands[0];
  const float axis=(stick.flags&shared::kControllerHandFlagThumbstickActive)&&std::isfinite(stick.thumbstickY)?stick.thumbstickY:0;
  // Ladder throttle is up/down regardless of HMD/controller locomotion heading.
  c.keys[0x11]=axis>.25f?0x80:0;c.keys[0x1f]=axis<-.25f?0x80:0;c.keys[0x1e]=c.keys[0x20]=0;
  c.buttons[0]=c.buttons[1]=0;
 }
 if(owner!=s.owner||now<=last||now-last>250000000){Reset();owner=s.owner;last=now;previousBodyY=s.world.values[3][1];return;}
 const float dt=float(now-last)*1.e-9f;last=now;
 if(s.mode==TraversalMode::Foot){
  hand=-1;climbing=false;debt=0;mount=0;
  if(s.velocityValid&&std::isfinite(s.verticalSpeed)&&s.verticalSpeed<-4.5f){
   if(!fallSince)fallSince=now;
   if(now-fallSince>=180000000){
    if(!deploySince)deploySince=now;
    const auto phase=(now-deploySince)%550000000;
    // Release first even if jump is still held, then use the game's own
    // jump/parachute action. Native fall-height rules decide deployment.
    c.keys[0x39]=phase>=100000000&&phase<220000000?0x80:0;
   }
  }else{fallSince=deploySince=0;}
  return;
 }
 fallSince=deploySince=0;c.mouseX=c.mouseY=0;c.snapDegrees=0;
 if(s.mode!=TraversalMode::Ladder){hand=-1;climbing=false;debt=0;mount=s.mount;return;}
 const float bodyY=s.world.values[3][1];
 if(mount!=s.mount){mount=s.mount;hand=-1;climbing=false;debt=0;previousBodyY=bodyY;}
 const float travelled=bodyY-previousBodyY;previousBodyY=bodyY;
 if(std::abs(travelled)>.75f){hand=-1;debt=0;}
 else if(climbing)debt=std::clamp(debt-travelled,-.35f,.35f);
 bool gripped[2]{};float relativeY[2]{};
 constexpr DWORD flags=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagSqueezeActive;
 for(int i=0;i<2;++i){const auto& h=input.hands[i];relativeY[i]=h.gripPose.positionY-head.position.y;
  gripped[i]=(h.flags&flags)==flags&&std::isfinite(h.squeezeValue)&&h.squeezeValue>.65f&&std::isfinite(relativeY[i])&&std::abs(relativeY[i])<1.3f;
 }
 const int selected=hand>=0&&gripped[hand]?hand:(gripped[0]?0:(gripped[1]?1:-1));
 if(selected>=0){
  climbing=true;
  if(selected==hand){const float pull=previousHandY-relativeY[selected];if(std::abs(pull)<=std::min(.25f,3.f*dt))debt=std::clamp(debt+pull,-.35f,.35f);else debt=0;}
  else debt=0;
  previousHandY=relativeY[selected];
 }else{
  debt=0;
  const auto& stick=input.hands[0];
  if((stick.flags&shared::kControllerHandFlagThumbstickActive)&&std::isfinite(stick.thumbstickY)&&std::abs(stick.thumbstickY)>.25f)climbing=false;
 }
 hand=selected;
 if(climbing){c.keys[0x11]=selected>=0&&debt>.018f?0x80:0;c.keys[0x1f]=selected>=0&&debt<-.018f?0x80:0;c.keys[0x1e]=c.keys[0x20]=0;}
 c.buttons[0]=c.buttons[1]=0;
}
}
