#include "ComfortCamera.h"
#include "StereoCamera.h"
#include "TrackingMath.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace bfvr;
namespace {
using M=stereo::Matrix4;
M Identity(){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;return m;}
bool Near(float a,float b){return std::abs(a-b)<.0001f;}
bool Same(const M& a,const M& b){for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(!Near(a.values[i][j],b.values[i][j]))return false;return true;}
M Pitch(float angle){M m=Identity();m.values[1]={0,std::cos(angle),std::sin(angle),0};m.values[2]={0,-std::sin(angle),std::cos(angle),0};return m;}
M Roll(float angle){M m=Identity();m.values[0]={std::cos(angle),std::sin(angle),0,0};m.values[1]={-std::sin(angle),std::cos(angle),0,0};return m;}
}
int main(){
    bf2142::ComfortYaw yaw;yaw.Bind(1);
    const auto identity=Identity();
    // Continuous fire/recovery can change all native camera axes. An unmoving
    // headset must not tilt or gradually acquire the game's recoil heading.
    double nativeYaw=20;
    const auto baseline=bf2142::MakeComfortCamera(identity,20);if(!baseline)return 1;
    for(int frame=0;frame<20000;++frame){
        const float recoil=frame%19<9?.125f:-.0625f;
        nativeYaw=std::remainder(nativeYaw+recoil,360.0);yaw.AddRecoil(recoil);
        const float local=float(frame%31)*.25f; // Native body/look redistribution.
        auto heading=yaw.Heading(float(nativeYaw)-local,local);if(!heading)return 2;
        M animated=bf2142::Multiply(Pitch(.2f*std::sin(frame*.1f)),Roll(.1f*std::cos(frame*.2f)));
        auto stable=bf2142::MakeComfortCamera(animated,*heading);
        if(!stable || !Same(*stable,*baseline))return 3;
        // Reapplying an absolute source must never integrate the previous eye.
        if(!Same(*bf2142::MakeComfortCamera(*stable,*heading),*baseline))return 4;
    }
    // Deliberate turning while firing is retained, including angle wraparound.
    auto heading=yaw.Heading(float(nativeYaw)+95,0);
    auto turned=heading?bf2142::MakeComfortCamera(identity,*heading):std::optional<M>{};
    if(!turned || !Same(*turned,*bf2142::MakeComfortCamera(identity,115)))return 5;
    yaw.Bind(1);if(!Near(*yaw.Heading(float(nativeYaw),0),20))return 6;
    yaw.Bind(2);if(!Near(*yaw.Heading(359,3),2))return 7;
    yaw.Bind(0);if(yaw.Heading(20,0))return 8;
    yaw.Bind(3);yaw.AddRecoil(std::numeric_limits<float>::quiet_NaN());
    if(!Near(*yaw.Heading(20,0),20) || yaw.Heading(INFINITY,0))return 9;
    // Position/stance stays native; physical leaning, yaw, pitch and roll still
    // come from OpenXR. Both eyes have the same stable world-up base.
    M animated=bf2142::Multiply(Pitch(.5f),Roll(.2f));animated.values[3]={100,1.4f,200,1};
    const M original=animated;auto stable=bf2142::MakeComfortCamera(animated,90);
    if(!stable || !Same(animated,original) || !Near(stable->values[3][1],1.4f))return 10;
    bf2142::CameraInput camera{*stable,.04f,300};
    stereo::Pose reference{},left{},right{};reference.position.y=left.position.y=right.position.y=1.7f;
    left.position.x=-.032f;right.position.x=.032f;
    const stereo::FovTangents fov{-1,1,1,-1};
    auto l=bf2142::MakeEyeCamera(camera,reference,left,fov),r=bf2142::MakeEyeCamera(camera,reference,right,fov);
    if(!l||!r || !Near(l->world.values[3][2]-r->world.values[3][2],.064f))return 11;
    left.position.z=-.3f;left.orientation={std::sin(.15f),0,0,std::cos(.15f)};
    l=bf2142::MakeEyeCamera(camera,reference,left,fov);
    if(!l || !Near(l->world.values[3][0],100.3f) || std::abs(l->world.values[2][1])<.2f)return 12;
    // Converting the shared stable frame into skeleton space must reconstruct
    // exactly the world frame used by the eyes; no controller/weapon lag frame.
    M body=*bf2142::MakeComfortCamera(identity,75);body.values[3]={100,0,200,1};
    const auto inv=bf2142::InverseRigid(body);if(!inv)return 13;
    const M model=bf2142::Multiply(*stable,*inv);
    auto hand=stereo::ComposeRuntimeHeadWithD3D8Camera(model,reference,left,1);
    auto worldHand=stereo::ComposeRuntimeHeadWithD3D8Camera(*stable,reference,left,1);
    if(!hand||!worldHand||!Same(bf2142::Multiply(*hand,body),*worldHand))return 14;
    M invalid=identity;invalid.values[0][0]=2;
    if(bf2142::MakeComfortCamera(invalid,0)||bf2142::MakeComfortCamera(identity,NAN))return 15;
    bf2142::PhysicalCameraHeight height;
    if(!Near(height.Update(1,0,12.2f,10,100),12.2f)||height.ready)return 16;
    height.Update(1,0,10.47f,10,200);height.Update(1,0,10.48f,10,510);
    if(!height.ready||!Near(height.offset,.47f))return 17;
    if(!Near(height.Update(1,2,9.37f,10,600),10.47f))return 18;
    // The user's physical 1.1m drop is applied once by the shared eye/hand
    // tracking transform, instead of adding the native prone camera drop too.
    const float visible=height.Update(1,2,9.37f,10,610)-1.1f;if(!Near(visible,9.37f))return 19;
    height.Update(2,2,9.37f,10,620);if(height.ready)return 20;
    puts("VR comfort: 20,000 recoil/settling frames, intentional turn, owner reset, stereo/6DoF and hand-frame agreement passed.");
}
