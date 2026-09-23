#include "HandPoseMath.h"
#include "MenuRoom.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <chrono>
#include <limits>
using namespace bfvr;using namespace bfvr::bf2142;
using M=stereo::Matrix4;
#define CHECK(x) do{if(!(x)){printf("Close grip failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
M At(float x,float y,float z){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
M Yaw(float a){auto m=At(0,0,0);m.values[0]={std::cos(a),0,-std::sin(a),0};m.values[2]={std::sin(a),0,std::cos(a),0};return m;}
bool Same(const M& a,const M& b){for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(std::abs(a.values[i][j]-b.values[i][j])>.0001f)return false;return true;}
float Distance(const M& a,const M& b){float sum=0;for(int i=0;i<3;++i){float d=a.values[3][i]-b.values[3][i];sum+=d*d;}return std::sqrt(sum);}
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
int main(int argc,char** argv){
 const auto native=Rig();auto lp=CaptureControllerHand(native,7),rp=CaptureControllerHand(native,33);CHECK(lp&&rp);
 auto invalid=native;invalid[12]=invalid[24];CHECK(!CaptureControllerHand(invalid,7));CHECK(!CaptureControllerHand(native,9));
 ControllerHandCache cache;cache.Update(native,false);CHECK(cache.left&&cache.right&&!cache.settled);
 auto holding=native;holding[43].values[3][2]-=.012f;
 cache.Update(holding,false);CHECK(Same(cache.right->bones[10],rp->bones[10]));
 cache.Update(holding,true);CHECK(cache.settled&&!Same(cache.right->bones[10],rp->bones[10]));
 const auto heldPose=cache.right;cache.Update(native,true);CHECK(Same(cache.right->bones[10],heldPose->bones[10]));
 HandFrame f{};f.head=native[0];f.rightValid=f.leftValid=true;f.rightGrip=At(.22f,1.32f,.34f);f.rightAim=Yaw(.3f);f.leftGrip=At(-.24f,1.3f,.27f);f.leftAim=Yaw(-.2f);
 f.leftPalm=lp;f.rightPalm=rp;f.handReference=&native;f.bindings=CaptureHandBindings(native);f.knifeGrip=true;
 for(int n=0;n<36;++n){auto grip=Yaw(n*.174533f);grip.values[3]=f.rightGrip.values[3];f.rightGrip=grip;
  auto solved=SolveTrackedHands(native,f);CHECK(solved);
  const auto handle=Multiply(At(0,.006f,-.100f),solved->bones[54]);CHECK(Distance(handle,f.rightGrip)<.0001f);
  for(int i=0;i<3;++i)CHECK(std::abs(solved->bones[54].values[2][i]-f.rightGrip.values[2][i])<.0001f);
  CHECK(Same(Multiply(*InverseRigid(rp->bones[0]),solved->bones[33]),f.rightGrip));
  CHECK(Same(Multiply(*InverseRigid(lp->bones[0]),solved->bones[7]),f.leftGrip));
  CHECK(Same(solved->bones[0],native[0])&&Same(solved->bones[1],native[1]));
 }
 f.rightGrip=At(.22f,1.32f,.34f);auto before=SolveTrackedHands(native,f);CHECK(before);
 auto attack=native;auto twist=Yaw(1.2f);twist.values[3]={.02f,.01f,0,1};
 for(int i=7;i<28;++i)attack[i]=Multiply(attack[i],twist);
 for(int i=33;i<54;++i)attack[i]=Multiply(attack[i],twist);
 // Keep the arm segments valid while changing wrist roll and every native
 // finger. Controller-space hands must not inherit this attack animation.
 attack[5]=At(-.15f,1.16f,.16f);attack[31]=At(.24f,1.13f,.16f);
 auto after=SolveTrackedHands(attack,f);CHECK(after);
 for(int i=2;i<54;++i)CHECK(Same(before->bones[i],after->bones[i]));
 CHECK(Same(before->bones[54],after->bones[54]));
 f.leftValid=false;before=SolveTrackedHands(native,f);f.rightGrip.values[3][0]+=.1f;after=SolveTrackedHands(native,f);CHECK(before&&after);
 for(int i=7;i<28;++i)CHECK(Same(before->bones[i],after->bones[i]));
 f.leftValid=true;f.weaponHeld=false;f.supportPressed=true;after=SolveTrackedHands(native,f);CHECK(after&&!after->supporting);CHECK(after->bones[54].values[3][2]<-3.9f);
 f.weaponHeld=true;f.knifeGrip=false;f.pistolGrip=true;f.leftGrip=f.rightGrip;f.leftGrip.values[3][0]-=.06f;f.supportReady=true;
 auto brace=SolveTrackedHands(native,f);CHECK(brace&&brace->supporting);
 const auto firingPalm=Multiply(*InverseRigid(rp->bones[0]),brace->bones[33]);
 const auto supportPalm=Multiply(*InverseRigid(lp->bones[0]),brace->bones[7]);
 const auto cupOffset=Multiply(supportPalm,*InverseRigid(firingPalm));CHECK(cupOffset.values[3][0]<-.03f&&cupOffset.values[3][2]<0);
 f.supportPressed=false;auto single=SolveTrackedHands(native,f);CHECK(single&&!single->supporting);
 CHECK(Same(brace->bones[54],single->bones[54]));CHECK(Same(brace->bones[33],single->bones[33]));CHECK(!Same(brace->bones[7],single->bones[7]));
 f.supportPressed=true;f.leftGrip=f.rightGrip;after=SolveTrackedHands(native,f);CHECK(after&&after->supporting&&Same(brace->bones[54],after->bones[54]));
 f.leftGrip.values[3][0]-=.3f;after=SolveTrackedHands(native,f);CHECK(after&&!after->supporting);
 CHECK(PistolSupportEligible(true,false,0));CHECK(!PistolSupportEligible(true,false,.18f));CHECK(PistolSupportEligible(true,true,.18f));CHECK(!PistolSupportEligible(false,true,.05f));
 CHECK(!PistolSupportEligible(true,true,std::numeric_limits<float>::quiet_NaN()));
 CHECK(PistolWeapon("eu_handgun")&&PistolWeapon("as_handgun")&&!PistolWeapon("eu_handgun_custom")&&!PistolWeapon("knife"));
 PistolAimFilter filter;const auto neutral=Yaw(0),jitter=Yaw(.04f);
 CHECK(Same(filter.Update(neutral,true,1000000000),neutral));const auto steady=filter.Update(jitter,true,1011000000);
 CHECK(InverseRigid(steady)&&steady.values[2][0]>0&&steady.values[2][0]<jitter.values[2][0]);
 CHECK(Same(filter.Update(jitter,true,1011000000),steady));CHECK(Same(filter.Update(jitter,false,1022000000),jitter));
 CHECK(Same(filter.Update(neutral,true,1033000000),neutral));CHECK(Same(filter.Update(Yaw(1),true,1044000000),Yaw(1)));
 CHECK(Same(filter.Update(neutral,true,1400000000),neutral));
 CHECK(std::abs(MenuStripeCoverage(0,1,.018f,1)-.036f)<.0001f);
 CHECK(std::abs(MenuStripeCoverage(.123f,1,.018f,3)-.036f)<.0001f);
 CHECK(MenuStripeCoverage(.017f,1,.018f,.02f)>.5f&&MenuStripeCoverage(.017f,1,.018f,.02f)<1);
 CHECK(MenuStripeCoverage(.5f,1,.018f,.002f)==0);
 if(argc==2){
  constexpr unsigned w=1280,h=720;std::vector<DWORD> left(w*h),right(w*h);shared::SharedRenderRequest request{};
  for(unsigned i=0;i<2;++i){auto& v=request.views[i];v.pose.orientationW=1;v.pose.positionX=i?.032f:-.032f;v.fov.angleLeft=-.85f;v.fov.angleRight=.85f;v.fov.angleUp=.6f;v.fov.angleDown=-.6f;}
  const auto start=std::chrono::steady_clock::now();DrawMenuRoom(left,right,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,request,{});
  printf("Room stereo 1280x720: %.2f ms\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());
  std::ofstream image(argv[1],std::ios::binary);image<<"P6\n"<<w<<" "<<h<<"\n255\n";for(auto px:left){const char rgb[]={char(px>>16),char(px>>8),char(px)};image.write(rgb,3);}
 }
 puts("Knife blade/handle alignment, animation-independent hands, pistol cup/release/aim and analytic menu AA passed.");return 0;
}
