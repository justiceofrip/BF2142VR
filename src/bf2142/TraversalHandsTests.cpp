#include "HandPoseMath.h"
#include "TraversalControls.h"
#include "ComfortCamera.h"
#include <cstdio>
#include <cmath>
using namespace bfvr;using namespace bfvr::bf2142;using M=stereo::Matrix4;
#define CHECK(x) do{if(!(x)){printf("Traversal/hands failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
M At(float x=0,float y=0,float z=0){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
bool Same(const M& a,const M& b){for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(std::abs(a.values[i][j]-b.values[i][j])>.0002f)return false;return true;}
HandBones Rig(){
 HandBones b;for(auto& m:b)m=At(0,0,0);b[0]=At(0,1.6f,0);
 for(int w:{7,33}){bool left=w==7;float sign=left?-1.f:1.f;
  b[w-4]=At(sign*.18f,1.38f,0);b[w-3]=b[w-4];b[w-2]=At(sign*.22f,1.12f,.1f);b[w-1]=b[w-2];b[w]=At(sign*.14f,1.23f,.3f);
  for(int finger=0;finger<5;++finger)for(int joint=0;joint<4;++joint){
   float across=finger==0?-.048f:(finger==1?-.032f:(finger==2?.01f:(finger==3?-.01f:.035f)));
   auto local=At(sign*across,sign*(-.09f-.013f*joint),sign*(.024f+.010f*joint));b[w+1+finger*4+joint]=Multiply(local,b[w]);
  }
 }
 for(int i=54;i<70;++i)b[i]=At(.14f,1.3f,.42f+.002f*(i-54));return b;
}
int main(){
 // Anatomical flexion: left fingers close toward +palm X, right toward -X.
 // The old policy preserved lengths yet bent every digit backwards.
 for(int wrist:{7,33}){
  auto bones=Rig();bones[wrist]=At();
  for(int f=0;f<5;++f)for(int j=0;j<4;++j)bones[wrist+1+4*f+j]=At(0,-.09f-j*.025f,(f-2)*.015f);
  auto curled=bones;std::array<float,5> curls{};curls.fill(1);CHECK(PoseEmptyFingers(curled,wrist,At(),curls));
  for(int f=1;f<5;++f){const int tip=wrist+4+4*f;CHECK(curled[tip].values[3][0]*(wrist==7?1.f:-1.f)>.02f);}
  curls.fill(0);auto open=bones;CHECK(PoseEmptyFingers(open,wrist,At(),curls));CHECK(Same(open[wrist],curled[wrist]));
 }
 const auto native=Rig();HandFrame f;f.head=native[0];f.torso=At();f.leftValid=f.rightValid=true;f.weaponHeld=true;f.fingerPoses=true;
 f.leftGrip=At(-.25f,1.25f,.32f);f.rightGrip=At(.22f,1.3f,.38f);f.leftAim=f.leftGrip;f.rightAim=f.rightGrip;
 f.bindings=CaptureHandBindings(native);f.leftPalm=CaptureControllerHand(native,7);f.rightPalm=CaptureControllerHand(native,33);f.handReference=&native;f.leftCurls.fill(.4f);f.rightCurls.fill(.7f);
 const auto rifle=SolveTrackedHands(native,f);CHECK(rifle);f.knifeGrip=true;const auto knife=SolveTrackedHands(native,f);CHECK(knife);
 for(int i=2;i<28;++i)CHECK(Same(rifle->bones[i],knife->bones[i]));
 auto animated=native;const auto rotate=*MakeComfortCamera(At(),37);
 for(int i=2;i<28;++i)animated[i]=Multiply(native[i],rotate);
 f.knifeGrip=false;const auto reload=SolveTrackedHands(animated,f);CHECK(reload);
 for(int i=2;i<28;++i)CHECK(Same(rifle->bones[i],reload->bones[i]));
 f.leftCurls.fill(1);const auto close=SolveTrackedHands(native,f);CHECK(close&&!Same(close->bones[15],rifle->bones[15]));CHECK(Same(close->bones[33],rifle->bones[33])&&Same(close->bones[54],rifle->bones[54]));
 TraversalView view;TraversalSample state;state.owner=1;state.mount=9;state.mode=TraversalMode::Ladder;state.world=At();
 const M foot=At(0,1.7f,0);view.RecordFoot(1,foot,1000);auto nativeCamera=*MakeComfortCamera(At(0,2,0),90);CHECK(Same(*view.Mounted(state,nativeCamera,1010),foot));
 for(int i=0;i<200;++i){state.world=*MakeComfortCamera(At(0,i*.02f,0),float(i));nativeCamera=*MakeComfortCamera(At(.2f,i*.02f+1.9f,.1f),float(i+20));
  const auto out=view.Mounted(state,nativeCamera,1020+i*10);CHECK(out);CHECK(Same(*out,At(0,1.7f+i*.02f,0)));}
 state.mode=TraversalMode::Parachute;state.mount++;nativeCamera=At(0,5.7f,0);const auto canopy=view.Mounted(state,nativeCamera,4000);CHECK(canopy&&canopy->values[1][1]==1);
 // Real downward motion automatically pulses the native jump/chute action;
 // no ordinary standing jump, stale sample, death/menu or other vehicle pulse.
 TraversalControls controls;shared::SharedControllerSample input{};input.flags=shared::kControllerSampleFlagSessionFocused;input.predictedDisplayTime=1000000000;
 state.mode=TraversalMode::Foot;state.mount=0;state.world=At();state.velocityValid=true;state.verticalSpeed=0;
 ControllerCommand command;controls.Update(state,input,{},command);int pulses=0;
 for(int i=0;i<100;++i){input.predictedDisplayTime+=20000000;command={};controls.Update(state,input,{},command);CHECK(!command.keys[0x39]);}
 state.verticalSpeed=-6;
 for(int i=0;i<40;++i){input.predictedDisplayTime+=20000000;command={};controls.Update(state,input,{},command);pulses+=command.keys[0x39]!=0;}
 CHECK(pulses>0&&pulses<20);state.verticalSpeed=0;input.predictedDisplayTime+=20000000;command={};controls.Update(state,input,{},command);CHECK(!command.keys[0x39]);
 // One hand's downward pull creates upward debt; observed native travel consumes
 // it. Two simultaneous grips cannot double the stroke, release stops motion.
 controls.Reset();state.mode=TraversalMode::Ladder;state.mount=4;state.world=At();input.predictedDisplayTime+=20000000;controls.Update(state,input,{},command);
 const DWORD flags=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagSqueezeActive;
 for(auto& h:input.hands){h.flags=flags;h.squeezeValue=1;h.gripPose.positionY=.2f;}
 input.predictedDisplayTime+=20000000;command={};controls.Update(state,input,{},command);CHECK(!command.keys[0x11]);
 for(auto& h:input.hands)h.gripPose.positionY-=.04f;
 input.predictedDisplayTime+=20000000;command={};controls.Update(state,input,{},command);CHECK(command.keys[0x11]&&!command.keys[0x1f]);
 state.world.values[3][1]+=.04f;input.predictedDisplayTime+=20000000;command={};controls.Update(state,input,{},command);CHECK(!command.keys[0x11]);
 for(auto& h:input.hands)h.squeezeValue=0;input.predictedDisplayTime+=20000000;command={};controls.Update(state,input,{},command);CHECK(!command.keys[0x11]&&!command.keys[0x1f]);
 input.hands[0].flags|=shared::kControllerHandFlagThumbstickActive;input.hands[0].thumbstickY=1;
 input.predictedDisplayTime+=20000000;command={};command.keys[0x1e]=0x80;command.mouseY=120;command.snapDegrees=30;
 controls.Update(state,input,{},command);CHECK(command.keys[0x11]&&!command.keys[0x1e]&&!command.mouseY&&!command.snapDegrees);
 puts("Mirrored fist flexion, independent off-hand, mounted horizon/translation, native chute pulses and ladder pull debt passed.");
}
