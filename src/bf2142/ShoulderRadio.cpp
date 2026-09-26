#include "ShoulderRadio.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
using V=stereo::Vec3;using Q=stereo::Quaternion;
V Rotate(Q q,V v){const V t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
 return {v.x+q.w*t.x+q.y*t.z-q.z*t.y,v.y+q.w*t.y+q.z*t.x-q.x*t.z,v.z+q.w*t.z+q.x*t.y-q.y*t.x};}
V Point(const stereo::Pose& a,V p){p=Rotate(a.orientation,p);return {a.position.x+p.x,a.position.y+p.y,a.position.z+p.z};}
Q Product(Q a,Q b){return {a.w*b.x+a.x*b.w+a.y*b.z-a.z*b.y,a.w*b.y-a.x*b.z+a.y*b.w+a.z*b.x,a.w*b.z+a.x*b.y-a.y*b.x+a.z*b.w,a.w*b.w-a.x*b.x-a.y*b.y-a.z*b.z};}
stereo::Pose Pose(const shared::SharedPresentationPose& p){return {{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}};}
constexpr DWORD tracked=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripOrientationValid|
 shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked|shared::kControllerHandFlagSqueezeActive|
 shared::kControllerHandFlagAimActive|shared::kControllerHandFlagAimPositionValid|shared::kControllerHandFlagAimOrientationValid|
 shared::kControllerHandFlagAimPositionTracked|shared::kControllerHandFlagAimOrientationTracked;
}
stereo::Pose RadioGrip(const stereo::Pose& a) noexcept {
 // Left palm faces the casing; thumb runs up its side to the top PTT switch.
 return {Point(a,{kRadioPosition.x,kRadioPosition.y-.008f,kRadioPosition.z-.034f}),Product(a.orientation,{.5f,-.5f,.5f,.5f})};
}
V RadioButton(const stereo::Pose& a,float press) noexcept {
 return Point(a,{kRadioPosition.x-.016f,kRadioPosition.y+.052f-.004f*std::clamp(press,0.f,1.f),kRadioPosition.z-.008f});
}
RadioFrame ShoulderRadio::Update(const RadioObservation& o) noexcept {
 RadioFrame f{};const auto& l=o.sample.hands[0];const auto now=o.sample.predictedDisplayTime;
 const auto anchor=stereo::MakeRelativePose({},o.anchor);
 if(!o.active||!o.owner||!anchor||now<=0||!(o.sample.flags&shared::kControllerSampleFlagSessionFocused)){Reset();return f;}
 f.visible=true;f.anchor=*anchor;f.time=now;f.grip=RadioGrip(f.anchor);f.button=RadioButton(f.anchor,0);
 const auto hand=stereo::MakeRelativePose(f.grip,Pose(l.gripPose));
 if(!o.leftAvailable||(l.flags&tracked)!=tracked||!std::isfinite(l.squeezeValue)||!hand){Reset();return f;}
 const bool squeeze=l.squeezeValue>(held?.35f:.65f);
 const auto p=hand->position;const float distance=std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);
 const bool gap=!observed||o.owner!=owner||now<time||now-time>150000000;
 const float dt=gap?0.f:std::min(.05f,float(now-time)*1.e-9f);
 if(gap){held=false;previousPressed=squeeze;depression=0;}
 observed=true;time=now;owner=o.owner;f.hovered=distance<.115f;
 if(held&&(!squeeze||distance>.27f))held=false;
 if(!held&&squeeze&&!previousPressed&&f.hovered)held=true;
 previousPressed=squeeze;
 f.held=held;
 // The grip gesture physically depresses the switch. PTT follows the contact,
 // not an arbitrary stick/button chord or a proximity-only hot microphone.
 const float before=depression;
 depression=held?std::min(1.f,depression+dt*18.f):0.f;
 f.depression=depression;f.pressed=held&&depression>=.8f;f.click=f.pressed&&before<.8f;
 f.button=RadioButton(f.anchor,depression);
 return f;
}
}
