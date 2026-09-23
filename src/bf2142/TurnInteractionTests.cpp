#include "HandPoseMath.h"
#include "ComfortControls.h"
#include "ComfortCamera.h"
#include "ControllerPolicy.h"
#include "FingerPose.h"
#include "VrControlsMenu.h"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <limits>
using namespace bfvr;using namespace bfvr::bf2142;using M=stereo::Matrix4;
#define CHECK(x) do{if(!(x)){printf("Turn interaction failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
M At(float x=0,float y=0,float z=0){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
bool Same(const M& a,const M& b){for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(std::abs(a.values[i][j]-b.values[i][j])>.0003f)return false;return true;}
float Distance(const M& a,const M& b){float v=0;for(int i=0;i<3;++i){const float d=a.values[3][i]-b.values[3][i];v+=d*d;}return std::sqrt(v);}
HandBones Rig(){
 HandBones b;for(auto& m:b)m=At();b[0]=At(0,1.6f,0);
 for(int w:{7,33}){const float sign=w==7?-1.f:1.f;
  b[w-4]=At(sign*.18f,1.38f,0);b[w-3]=b[w-4];b[w-2]=At(sign*.22f,1.12f,.1f);b[w-1]=b[w-2];b[w]=At(sign*.14f,1.23f,.3f);
  for(int finger=0;finger<5;++finger)for(int joint=0;joint<4;++joint){
   float across=finger==0?-.048f:(finger==1?-.032f:(finger==2?.01f:(finger==3?-.01f:.035f)));
   b[w+1+finger*4+joint]=Multiply(At(sign*across,sign*(-.09f-.013f*joint),sign*(.024f+.010f*joint)),b[w]);
  }
 }
 for(int i=54;i<70;++i)b[i]=At(.14f,1.3f,.42f+.002f*(i-54));return b;
}
int main(int argc,char** argv){
 NativeTurnPulse pulse;CHECK(pulse.Queue(1,30,100,1000));CHECK(!pulse.Queue(1,30,100,1001));
 CHECK(pulse.Consume(1,true,1010)==30);CHECK(pulse.Consume(1,true,1011)==0);
 CHECK(pulse.Queue(1,-45,101,1020));CHECK(!pulse.Consume(1,false,1021));CHECK(!pulse.Consume(1,true,1022));
 CHECK(pulse.Queue(1,45,102,1030));CHECK(!pulse.Consume(2,true,1031));
 CHECK(pulse.Queue(1,60,103,1040));CHECK(!pulse.Consume(1,true,1191));
 CHECK(pulse.Queue(1,30,104,1200));pulse.Cancel();CHECK(!pulse.Consume(1,true,1201));
 CHECK(!pulse.Queue(1,0,105,1210));CHECK(!pulse.Queue(1,91,105,1210));CHECK(!pulse.Queue(1,std::numeric_limits<float>::quiet_NaN(),105,1210));
 // Native body/world yaw rotates the full rig together, preserving exact
 // shoulder/wrist/weapon geometry through repeated full revolutions.
 const auto native=Rig();HandFrame f;f.head=native[0];f.torso=At();f.leftValid=f.rightValid=true;
 f.leftGrip=At(-.25f,1.25f,.32f);f.rightGrip=At(.22f,1.3f,.38f);f.leftAim=f.leftGrip;f.rightAim=f.rightGrip;
 f.bindings=CaptureHandBindings(native);f.leftPalm=CaptureControllerHand(native,7);f.rightPalm=CaptureControllerHand(native,33);f.handReference=&native;
 for(int kind=0;kind<3;++kind){f.knifeGrip=kind==1;f.weaponHeld=kind!=2;const auto initial=SolveTrackedHands(native,f);CHECK(initial);
  for(int step=0;step<96;++step){const auto rotation=*MakeComfortCamera(At(),float(step*30));auto turned=f;
   turned.head=Multiply(f.head,rotation);turned.torso=rotation;
   turned.leftGrip=Multiply(f.leftGrip,rotation);turned.rightGrip=Multiply(f.rightGrip,rotation);turned.leftAim=Multiply(f.leftAim,rotation);turned.rightAim=Multiply(f.rightAim,rotation);
   auto source=native;for(auto& bone:source)bone=Multiply(bone,rotation);turned.handReference=&source;
   const auto out=SolveTrackedHands(source,turned);CHECK(out);
   for(int i=3;i<70;++i){if(i==28)continue;CHECK(Same(out->bones[i],Multiply(initial->bones[i],rotation)));}
  }
 }
 // An empty squeeze changes fingers, not wrists, camera, gun binding or arms.
 f.knifeGrip=false;f.weaponHeld=false;f.fingerPoses=true;f.leftCurls.fill(0);f.rightCurls.fill(0);
 const auto open=SolveTrackedHands(native,f);CHECK(open);f.leftCurls.fill(1);f.rightCurls.fill(1);const auto fist=SolveTrackedHands(native,f);CHECK(fist);
 for(int w:{7,33}){CHECK(Same(open->bones[w],fist->bones[w]));
  for(int finger=1;finger<5;++finger){const int b=w+1+finger*4;CHECK(Distance(open->bones[b+3],fist->bones[b+3])>.01f);
   for(int j=0;j<3;++j)CHECK(std::abs(Distance(open->bones[b+j],open->bones[b+j+1])-Distance(fist->bones[b+j],fist->bones[b+j+1]))<.0001f);
  }
 }
 for(int i:{0,1,2,3,4,5,6,28,29,30,31,32,54,69})CHECK(Same(open->bones[i],fist->bones[i]));
 // Heading selection uses only yaw and the left aim pose, independent of gun aim.
 stereo::Pose reference{},head{};shared::SharedControllerSample input{};
 input.flags=shared::kControllerSampleFlagSessionFocused;input.hands[0].flags=shared::kControllerHandFlagAimActive|shared::kControllerHandFlagAimOrientationValid|shared::kControllerHandFlagAimOrientationTracked|shared::kControllerHandFlagThumbstickActive;
 auto& aim=input.hands[0].aimPose;aim.orientationY=-.70710678f;aim.orientationW=.70710678f;
 CHECK(std::abs(LocomotionYaw(false,reference,head,input))<.0001f);CHECK(std::abs(LocomotionYaw(true,reference,head,input)-1.5707963f)<.0001f);
 ControllerPolicyState move;input.predictedDisplayTime=1000000000;input.hands[0].thumbstickY=1;
 auto command=MapControllers(move,input,true,LocomotionYaw(true,reference,head,input),600);CHECK(command.keys[0x20]&&!command.keys[0x11]);
 aim.orientationY=0;aim.orientationX=.70710678f;CHECK(std::abs(LocomotionYaw(true,reference,head,input))<.0001f);
 aim.orientationX=0;input.hands[0].flags=0;CHECK(std::abs(LocomotionYaw(true,reference,head,input))<.0001f);
 // The ninth menu row matches drawing bounds and persists into a fresh load.
 CHECK(VrMenuHit(.2f,.77f,true)==9);CHECK(VrMenuHit(.2f,.817f,true)==-1);
 VrSettings settings;settings.configPath=(std::filesystem::temp_directory_path()/L"BF2142VR-v27-prefs.ini").wstring();
 VrControlsMenu menu;menu.Hotkey(true);menu.Hotkey(false);CHECK(menu.Interact(.2f,.77f,true,false,settings,1.7f)&&settings.controllerRelativeMovement);
 SetEnvironmentVariableW(L"BF2142VR_CONFIG",settings.configPath.c_str());CHECK(LoadVrSettings(L"").controllerRelativeMovement);
 if(argc==2){
  constexpr unsigned w=1280,h=800;std::vector<DWORD> pixels(w*h);menu.Draw(pixels,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,settings);
  std::ofstream output(argv[1],std::ios::binary);output<<"P6\n"<<w<<" "<<h<<"\n255\n";
  for(auto pixel:pixels){const char rgb[]={char(pixel>>16),char(pixel>>8),char(pixel)};output.write(rgb,3);}
 }
 CHECK(menu.Interact(.2f,.77f,true,false,settings,1.7f)&&!settings.controllerRelativeMovement);CHECK(!LoadVrSettings(L"").controllerRelativeMovement);
 WritePrivateProfileStringW(L"VR",L"MovementDirection",L"invalid",settings.configPath.c_str());CHECK(!LoadVrSettings(L"").controllerRelativeMovement);
 SetEnvironmentVariableW(L"BF2142VR_CONFIG",nullptr);DeleteFileW(settings.configPath.c_str());
 puts("Snap pulses, 288 full-body yaw poses, independent empty fists, head/controller movement and saved menu preference passed.");
}
