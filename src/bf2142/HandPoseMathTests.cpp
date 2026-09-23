#include "HandPoseMath.h"
#include "FingerPose.h"
#include "HandBindingCache.h"
#include "AdsHandPose.h"
#include "WeaponOptic.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace bfvr;using namespace bfvr::bf2142;
namespace {
using M=stereo::Matrix4;
M At(float x,float y,float z){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
float Dist(const M& a,const M& b){float n=0;for(int i=0;i<3;++i)n+=std::pow(a.values[3][i]-b.values[3][i],2.f);return std::sqrt(n);}
bool CloseEnough(float a,float b){return std::abs(a-b)<.001f;}
bool Same(const M& a,const M& b){for(int r=0;r<4;++r)for(int c=0;c<4;++c)if(std::abs(a.values[r][c]-b.values[r][c])>.001f)return false;return true;}
HandBones Rig(){HandBones b;for(auto& m:b)m=At(0,0,0);b[0]=At(0,1.6f,0);
    b[3]=At(-.18f,1.38f,0);b[5]=At(-.2f,1.17f,.15f);b[7]=At(-.06f,1.28f,.34f);
    b[29]=At(.18f,1.38f,0);b[31]=At(.24f,1.15f,.12f);b[33]=At(.15f,1.23f,.28f);
    b[4]=At(-.19f,1.21f,.12f);b[6]=At(-.13f,1.22f,.24f);b[30]=At(.23f,1.2f,.1f);b[32]=At(.19f,1.19f,.2f);
    for(int i=8;i<28;++i)b[i]=At(-.05f,1.29f,.36f);for(int i=34;i<54;++i)b[i]=At(.16f,1.24f,.3f);
    for(int i=54;i<70;++i)b[i]=At(.15f,1.3f,.42f+float(i-54)*.002f);return b;}
}
int main(){
    AdsHandPose adsShape;auto normal=Rig();adsShape.Apply(normal,false,true,1000);
    auto zoomPose=normal;zoomPose[8].values[3][0]+=.2f;zoomPose[3].values[3][1]+=.15f;
    zoomPose[55].values[3][2]+=.012f;const auto livePart=zoomPose[55];const auto liveCamera=zoomPose[0];
    adsShape.Apply(zoomPose,true,true,1100);
    if(!Same(zoomPose[8],normal[8])||!Same(zoomPose[3],normal[3])||!Same(zoomPose[55],livePart)||!Same(zoomPose[0],liveCamera))return 80;
    auto exitPose=Rig();exitPose[8].values[3][0]+=.2f;adsShape.Apply(exitPose,false,true,1200);
    if(!Same(exitPose[8],normal[8]))return 81;
    auto reload=Rig();reload[8].values[3][0]+=.2f;const auto reloadHand=reload[8];adsShape.Apply(reload,false,true,1600);
    if(!Same(reload[8],reloadHand))return 82;
    AdsHandPose fresh;auto noReference=Rig();noReference[8].values[3][0]+=.2f;fresh.Apply(noReference,true,true,2000);
    if(!Same(noReference[8],reloadHand))return 83;
    const auto b=Rig();HandFrame f{};f.head=b[0];f.rightGrip=At(.25f,1.25f,.32f);f.rightAim=At(0,0,0);f.rightValid=true;
    f.leftGrip=At(-.22f,1.23f,.28f);f.leftAim=At(0,0,0);f.leftValid=true;
    auto result=SolveTrackedHands(b,f);if(!result){puts("neutral solve rejected");return 1;}
    if(Dist(result->bones[33],f.rightGrip)>.001f || Dist(result->bones[7],f.leftGrip)>.001f)return 2;
    if(!Same(result->bones[0],b[0])||!Same(result->bones[1],b[1]))return 3;
    const auto before=Multiply(b[33],*InverseRigid(b[54]));const auto after=Multiply(result->bones[33],*InverseRigid(result->bones[54]));
    if(!Same(before,after))return 4;
    const auto fingerBefore=Multiply(b[34],*InverseRigid(b[33]));const auto fingerAfter=Multiply(result->bones[34],*InverseRigid(result->bones[33]));if(!Same(fingerBefore,fingerAfter))return 5;
    const auto partBefore=Multiply(b[65],*InverseRigid(b[54]));const auto partAfter=Multiply(result->bones[65],*InverseRigid(result->bones[54]));if(!Same(partBefore,partAfter))return 6;
    for(int i=0;i<36;++i){float a=i*.174533f;f.rightAim=At(0,0,0);f.rightAim.values[0]={std::cos(a),0,-std::sin(a),0};f.rightAim.values[2]={std::sin(a),0,std::cos(a),0};
        result=SolveTrackedHands(b,f);if(!result||Dist(result->bones[33],f.rightGrip)>.001f)return 7;
        for(const auto& m:result->bones)if(!InverseRigid(m))return 8;
    }
    f.rightAim=At(0,0,0);f.leftValid=false;result=SolveTrackedHands(b,f);if(!result)return 9;
    f.leftValid=true;f.leftGrip=result->bones[7];f.supportPressed=true;result=SolveTrackedHands(b,f);if(!result||!result->supporting)return 10;
    f.supportPressed=false;result=SolveTrackedHands(b,f);if(!result||result->supporting)return 11;
    if(SupportGripEligible(true,false,.25f,.3f)||!SupportGripEligible(true,true,.25f,.3f)||SupportGripEligible(false,true,.1f,.3f)||SupportGripEligible(true,true,.1f,.05f))return 12;
    f.rightValid=false;if(SolveTrackedHands(b,f))return 13;f.rightValid=true;f.rightGrip.values[3][0]=std::numeric_limits<float>::quiet_NaN();if(SolveTrackedHands(b,f))return 14;
    f.rightGrip=At(7,1.2f,0);if(SolveTrackedHands(b,f))return 15;
    for(float x:{-.4f,-.1f,.1f,.4f})for(float y:{-.4f,0.f,.4f})for(float z:{-.2f,.2f,.4f}){
        const auto arm=SolveArm({0,0,0},{x,y,z},.29f,.27f,x<0);if(!arm||!std::isfinite(arm->elbow.x))return 16;
        const float e=std::sqrt(arm->elbow.x*arm->elbow.x+arm->elbow.y*arm->elbow.y+arm->elbow.z*arm->elbow.z);if(e<.289f||e>.48f)return 17;
    }
    if(SolveArm({0,0,0},{0,0,0},.29f,.27f,true)||SolveArm({0,0,0},{0,0,4},.29f,.27f,true))return 18;
    f.rightGrip=At(.25f,1.25f,.32f);f.leftValid=true;f.leftGrip.values[3][1]=std::numeric_limits<float>::infinity();if(SolveTrackedHands(b,f))return 19;
    if(SupportGripEligible(true,false,-1,.3f))return 20;
    // A rotated gun must rotate both the actual projectile and native barrel
    // offset/deviation. Moving the ejected-shell matrix alone cannot pass this.
    const M camera=At(10,2,30);M gun=At(9,1.7f,30.5f);
    gun.values[0]={0,0,-1,0};gun.values[2]={1,0,0,0};
    M local=At(.05f,-.1f,.4f);const float spread=.015f;
    local.values[0]={std::cos(spread),0,-std::sin(spread),0};local.values[2]={std::sin(spread),0,std::cos(spread),0};
    auto launch=MapTrackedFire(Multiply(local,camera),camera,gun);
    if(!launch || !Same(*launch,Multiply(local,gun)) || !CloseEnough(launch->values[3][0],9.4f))return 21;
    // The native driver computes speed * launch.forward + player velocity
    // after the getter. A quarter turn must turn the shot, not player motion.
    const auto straight=MapTrackedFire(camera,camera,gun);if(!straight)return 31;
    const stereo::Vec3 nativeVelocity{3,0,2};const float muzzleSpeed=800;
    const stereo::Vec3 velocity{straight->values[2][0]*muzzleSpeed+nativeVelocity.x,
        straight->values[2][1]*muzzleSpeed+nativeVelocity.y,straight->values[2][2]*muzzleSpeed+nativeVelocity.z};
    if(!CloseEnough(velocity.x,803)||!CloseEnough(velocity.z,2))return 32;
    if(MapTrackedFire(At(100,0,0),camera,gun))return 22;
    auto invalid=camera;invalid.values[2][2]=std::numeric_limits<float>::infinity();if(MapTrackedFire(invalid,camera,gun))return 23;
    // Animation of the native wrists cannot change an already captured grip.
    f={};f.head=b[0];f.rightGrip=At(.25f,1.25f,.32f);f.rightAim=At(0,0,0);f.rightValid=true;
    f.leftGrip=At(-.22f,1.23f,.28f);f.leftAim=At(0,0,0);f.leftValid=true;f.bindings=CaptureHandBindings(b);
    const auto stable=SolveTrackedHands(b,f);auto animated=b;
    animated[7].values[3][0]+=.03f;animated[33].values[3][1]+=.02f;
    animated[7].values[0]={0,1,0,0};animated[7].values[1]={-1,0,0,0};
    animated[33].values[0]={0,1,0,0};animated[33].values[1]={-1,0,0,0};
    const auto moving=SolveTrackedHands(animated,f);
    if(!stable||!moving||!Same(stable->bones[54],moving->bones[54])||!Same(stable->bones[7],moving->bones[7]))return 24;
    const M optic=At(0,0,0);auto eye=At(0,.055f,-.35f);
    const auto center=OpticDot(optic,eye);if(!center || std::abs(center->z-.08f)>.0001f)return 25;
    eye.values[3][0]=.15f;if(OpticDot(optic,eye))return 26;
    eye=At(0,.055f,.1f);if(OpticDot(optic,eye))return 27;
    eye=At(.02f,.055f,-.35f);const auto offAxis=OpticDot(optic,eye);if(!offAxis || offAxis->x<=0)return 28;
    M projection{};projection.values[0][0]=projection.values[1][1]=1;projection.values[2][3]=1;
    std::vector<DWORD> image(320*240);DrawWeaponOptic(image,320,240,optic,eye,projection,false);
    unsigned red=0,gray=0;for(auto pixel:image){red+=pixel==0xffff5040;gray+=pixel==0xff777777;}
    if(red!=13 || gray<20 || gray>500 || image.front() || image.back())return 29;
    std::fill(image.begin(),image.end(),0);DrawWeaponOptic(image,320,240,optic,eye,projection,true);
    red=0;for(auto pixel:image)red+=pixel==0xff4050ff;if(red!=13)return 30;
    // A static but rearward deploy pose never becomes a foregrip. Accept a
    // stable authored hold only after the draw interval, including when the
    // player holds squeeze throughout equip. Preserve the working right bind.
    HandBindingCache cache;auto deploy=b;deploy[7]=At(.1f,1.1f,.03f);
    for(unsigned time=0;time<=800;time+=50)cache.Update(deploy,time);
    if(!cache.value || cache.supportReady)return 33;
    const auto rightBinding=cache.value->rightFromWeapon;
    for(unsigned time=850;time<=1150;time+=50)cache.Update(b,time);
    if(!cache.supportReady || !Same(cache.value->leftFromWeapon,CaptureHandBindings(b)->leftFromWeapon) || !Same(cache.value->rightFromWeapon,rightBinding))return 34;
    const auto settled=cache.value->leftFromWeapon;cache.Update(deploy,1200);
    if(!Same(cache.value->leftFromWeapon,settled))return 35; // reload cannot recapture
    cache={};for(unsigned time=0;time<=700;time+=50)cache.Update(b,time);
    if(cache.supportReady)return 36;cache.Update(b,750);if(!cache.supportReady)return 37;
    cache={};cache.Update(b,1000);cache.Update(b,2000);if(cache.supportReady)return 38; // a long gap is not stable observation
    for(unsigned time=2050;time<=2250;time+=50)cache.Update(b,time);if(!cache.supportReady)return 39;
    f.bindings=CaptureHandBindings(b);f.leftValid=false;f.supportPressed=false;result=SolveTrackedHands(b,f);if(!result)return 40;
    f.leftValid=true;f.leftGrip=result->bones[7];f.supportPressed=true;f.supportReady=false;
    result=SolveTrackedHands(b,f);if(!result || result->supporting)return 41;
    f.supportReady=true;result=SolveTrackedHands(b,f);if(!result || !result->supporting)return 42;
    // Reproduce equip with an off-to-the-side firing wrist. Provisional frames
    // retain the current authored relation, then lock the settled holding pose.
    auto badDraw=b;badDraw[33].values[3][0]+=.18f;badDraw[33].values[3][1]+=.09f;
    HandBindingCache firing;
    for(unsigned t=0;t<=400;t+=50)firing.Update(badDraw,t);
    if(firing.rightReady)return 100;
    for(unsigned t=450;t<=800;t+=50)firing.Update(b,t);
    if(!firing.rightReady || !Same(firing.value->rightFromWeapon,CaptureHandBindings(b)->rightFromWeapon))return 101;
    firing.Update(badDraw,900);
    if(!Same(firing.value->rightFromWeapon,CaptureHandBindings(b)->rightFromWeapon))return 102;
    // Scope entry before settling must not permanently capture the ADS pose.
    firing={};for(unsigned t=0;t<=1000;t+=50)firing.Update(badDraw,t,false);
    if(firing.rightReady||firing.supportReady)return 103;
    for(unsigned t=1050;t<=1400;t+=50)firing.Update(b,t);
    if(!firing.rightReady||!firing.supportReady||!Same(firing.value->rightFromWeapon,CaptureHandBindings(b)->rightFromWeapon))return 104;
    // An observation gap cannot count as a stable firing-grip interval.
    firing={};firing.Update(b,1000);firing.Update(b,2000);if(firing.rightReady)return 105;
    // Finger posing is isolated to the free hand, keeps lengths and wrist,
    // restores the authored curl exactly, and rejects corrupt data atomically.
    auto fingers=Rig();for(int finger=0;finger<5;++finger)for(int j=0;j<4;++j)
        fingers[8+finger*4+j]=At(-.04f+finger*.014f,1.29f+j*.012f,.37f+j*.018f);
    auto posed=fingers;std::array<float,5> curls{};
    if(!PoseFreeFingers(posed,7,curls)||!Same(posed[7],fingers[7])||!Same(posed[54],fingers[54]))return 90;
    for(int finger=0;finger<5;++finger)for(int j=0;j<3;++j){int k=8+finger*4+j;
        if(!CloseEnough(Dist(posed[k],posed[k+1]),Dist(fingers[k],fingers[k+1])))return 91;}
    posed=fingers;curls.fill(1);if(!PoseFreeFingers(posed,7,curls))return 92;
    for(int i=0;i<70;++i)if(!Same(posed[i],fingers[i]))return 93;
    curls[2]=std::numeric_limits<float>::quiet_NaN();if(PoseFreeFingers(posed,7,curls))return 94;
    for(int i=0;i<70;++i)if(!Same(posed[i],fingers[i]))return 95;
    shared::SharedControllerHandSample touch{};
    touch.flags=shared::kControllerHandFlagSqueezeActive|shared::kControllerHandFlagTriggerActive|shared::kControllerHandFlagTriggerTouchActive|shared::kControllerHandFlagTriggerTouched|shared::kControllerHandFlagThumbTouchActive;
    touch.squeezeValue=.7f;auto fc=ControllerFingerCurls(touch);
    if(fc[0]!=0 || fc[1]<.5f || !CloseEnough(fc[2],.7f))return 96;
    touch.triggerValue=1;touch.flags|=shared::kControllerHandFlagThumbTouched;fc=ControllerFingerCurls(touch);if(fc[0]!=1||fc[1]!=1)return 97;
    touch.squeezeValue=std::numeric_limits<float>::quiet_NaN();if(ControllerFingerCurls(touch)[2]!=0)return 98;
    // Both held-hand thumb/index joints respond without altering the gun/wrist
    // or the three authored fingers carrying the weapon.
    auto both=fingers;
    for(int i=8;i<28;++i){both[i+26]=both[i];both[i+26].values[3][0]+=.2f;both[i+26].values[3][1]-=.05f;}
    auto held=both;std::array<float,5> handCurls{0,0,1,1,1};
    if(!PoseFreeFingers(held,33,handCurls))return 110;
    if(!Same(held[33],both[33])||!Same(held[54],both[54]))return 111;
    for(int i=42;i<54;++i)if(!Same(held[i],both[i]))return 112;
    if(Same(held[39],both[39])||Same(held[35],both[35]))return 113;
    f={};f.head=b[0];f.rightValid=true;f.rightGrip=At(.2f,1.3f,.4f);f.rightAim=At(0,0,0);
    f.weaponHeld=false;f.fingerPoses=true;f.leftValid=true;f.leftGrip=At(-.22f,1.23f,.28f);f.leftAim=At(0,0,0);f.supportPressed=true;
    const auto empty=SolveTrackedHands(b,f);if(!empty||empty->supporting||Dist(empty->bones[33],f.rightGrip)>.001f||Dist(empty->bones[7],f.leftGrip)>.001f)return 117;
    if(empty->bones[54].values[3][2]>f.head.values[3][2]-3.9f||!Same(empty->bones[0],b[0]))return 118;
    f.leftValid=false;if(!SolveTrackedHands(b,f))return 119;
    puts("BF2142 controller wrists, both hands' fingers, weapon attachments, knife palm grip, IK and support policy passed.");return 0;
}
