#include "ShoulderRadio.h"
#include "HandPoseMath.h"
#include <cstdio>
#include <cmath>
#include <limits>
#include <fstream>
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Shoulder radio failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
float Dist(stereo::Vec3 a,stereo::Vec3 b){return std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z));}
stereo::Vec3 Pos(const stereo::Matrix4& m){return {m.values[3][0],m.values[3][1],m.values[3][2]};}
void Hand(RadioObservation& o,const stereo::Pose& p){auto& h=o.sample.hands[0];h.gripPose={p.orientation.x,p.orientation.y,p.orientation.z,p.orientation.w,p.position.x,p.position.y,p.position.z};h.aimPose=h.gripPose;}
}
int main(int argc,char** argv){
 RadioObservation o;o.active=true;o.owner=1;o.anchor.position.y=1.7f;o.sample.flags=shared::kControllerSampleFlagSessionFocused;o.sample.predictedDisplayTime=1000000000;
 o.sample.hands[0].flags=~0u;Hand(o,RadioGrip(o.anchor));ShoulderRadio policy;
 const auto step=[&](){o.sample.predictedDisplayTime+=20000000;return policy.Update(o);};
 auto f=step();CHECK(f.visible&&f.hovered&&!f.held&&!f.pressed);
 o.sample.hands[0].squeezeValue=1;f=step();CHECK(f.held&&!f.pressed);
 f=step();CHECK(f.held&&!f.pressed);f=step();CHECK(f.held&&f.pressed&&f.click);
 CHECK(f.button.y<RadioButton(o.anchor,0).y);f=step();CHECK(f.pressed&&!f.click);
 o.sample.hands[0].squeezeValue=.5f;CHECK(step().pressed); // hysteresis
 o.sample.hands[0].squeezeValue=0;f=step();CHECK(!f.held&&!f.pressed&&f.depression==0);
 // Holding away from the shoulder then entering it must not silently open voice.
 auto away=RadioGrip(o.anchor);away.position.x+=.5f;Hand(o,away);o.sample.hands[0].squeezeValue=1;CHECK(!step().held);
 Hand(o,RadioGrip(o.anchor));CHECK(!step().held);o.sample.hands[0].squeezeValue=0;step();o.sample.hands[0].squeezeValue=1;step();step();CHECK(step().pressed);
 Hand(o,away);CHECK(!step().pressed);Hand(o,RadioGrip(o.anchor));CHECK(!step().held);
 // Every ownership/focus/menu/dropout transition requires a fresh release/press.
 for(int transition=0;transition<7;++transition){
  o.sample.hands[0].squeezeValue=0;step();o.sample.hands[0].squeezeValue=1;step();step();CHECK(step().pressed);
  switch(transition){case 0:o.active=false;break;case 1:o.sample.flags=0;break;case 2:o.leftAvailable=false;break;case 3:o.sample.hands[0].flags=0;break;case 4:o.owner=2;break;case 5:o.sample.predictedDisplayTime+=200000000;break;case 6:policy.Reset();break;}
  CHECK(!step().held);o.active=true;o.sample.flags=shared::kControllerSampleFlagSessionFocused;o.leftAvailable=true;o.sample.hands[0].flags=~0u;
  CHECK(!step().held);
 }
 o.sample.hands[0].squeezeValue=std::numeric_limits<float>::quiet_NaN();CHECK(!step().held);
 // Pose is a body-relative mount: yaw/translation cannot move the hand off it.
 o.anchor={{3,2,-4},{0,.70710678f,0,.70710678f}};const auto grip=RadioGrip(o.anchor);const auto local=stereo::MakeRelativePose(o.anchor,grip);
 CHECK(local&&Dist(local->position,{kRadioPosition.x,kRadioPosition.y-.008f,kRadioPosition.z-.034f})<.0001f);
 // Contact IK reaches the physical switch without changing the wrist, other
 // fingers, arm, weapon, or individual thumb-bone lengths.
 HandBones bones{};for(auto& b:bones)for(int i=0;i<4;++i)b.values[i][i]=1;
 for(int i=0;i<4;++i)bones[8+i].values[3]={.024f*i,0,0,1};const auto before=bones;
 CHECK(PoseFingerContact(bones,8,{.035f,.035f,0}));CHECK(Dist(Pos(bones[11]),{.035f,.035f,0})<.003f);
 for(int i=0;i<3;++i)CHECK(std::abs(Dist(Pos(bones[8+i]),Pos(bones[9+i]))-.024f)<.00001f);
 for(int i=0;i<70;++i)if(i<8||i>11)CHECK(bones[i].values==before[i].values);
 const auto valid=bones;CHECK(!PoseFingerContact(bones,8,{1,0,0}));CHECK(bones[11].values==valid[11].values);
 if(argc==2){
  HandBones native{};std::ifstream in(argv[1],std::ios::binary);CHECK(in.read(reinterpret_cast<char*>(native.data()),sizeof(native)));
  const auto leftPalm=CaptureControllerHand(native,7),rightPalm=CaptureControllerHand(native,33);CHECK(leftPalm&&rightPalm);
  HandFrame hands;hands.head=native[0];hands.torso=native[0];hands.leftValid=hands.rightValid=true;hands.weaponHeld=true;
  hands.leftPalm=leftPalm;hands.rightPalm=rightPalm;hands.handReference=&native;hands.fingerPoses=true;hands.leftCurls={.65f,.70f,.74f,.78f,.82f};
  const stereo::Pose anchor{};const auto radioGrip=RadioGrip(anchor);
  const auto mapped=[&](const stereo::Pose& p){return stereo::ComposeRuntimeHeadWithD3D8Camera(native[0],{},p,1);};
  const auto left=mapped(radioGrip),right=mapped({{.20f,-.30f,-.35f},{}});CHECK(left&&right);
  hands.leftGrip=hands.leftAim=*left;hands.rightGrip=hands.rightAim=*right;
  auto button=radioGrip;button.position=RadioButton(anchor,1);const auto press=mapped(button);CHECK(press);hands.leftThumbContact=Pos(*press);
  const auto solved=SolveTrackedHands(native,hands);CHECK(solved&&!solved->supporting);
  const auto palm=Multiply(*InverseRigid(leftPalm->bones[0]),solved->bones[7]);CHECK(Dist(Pos(palm),Pos(*left))<.001f);
  printf("Captured radio thumb contact error %.2f mm\n",1000*Dist(Pos(solved->bones[11]),*hands.leftThumbContact));
  CHECK(Dist(Pos(solved->bones[11]),*hands.leftThumbContact)<.003f);
  auto noRadio=hands;noRadio.leftThumbContact.reset();const auto compare=SolveTrackedHands(native,noRadio);CHECK(compare);
  for(int i=28;i<70;++i)CHECK(solved->bones[i].values==compare->bones[i].values);
 }
 printf("Shoulder radio: physical press/release, takeover prevention, dropout/recenter, body frame, thumb contact lengths.\n");return 0;
}
