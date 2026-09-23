#include "ControllerHandPose.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
using M=stereo::Matrix4;using V=stereo::Vec3;
V Sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V Add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
V Mul(V a,float s){return {a.x*s,a.y*s,a.z*s};}
float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
V Unit(V a){const float n=std::sqrt(Dot(a,a));return n>1.e-6f?Mul(a,1/n):V{};}
V Row(const M& m,int r){return {m.values[r][0],m.values[r][1],m.values[r][2]};}
void Set(M& m,int r,V a){m.values[r]={a.x,a.y,a.z,r==3?1.f:0.f};}
}
std::optional<ControllerHandPose> CaptureControllerHand(const std::array<M,70>& native,int wrist) noexcept {
    if(wrist!=7 && wrist!=33)return {};
    for(int i=wrist;i<wrist+21;++i)if(!InverseRigid(native[i]))return {};
    const auto wp=Row(native[wrist],3);
    const auto index=Row(native[wrist+5],3),little=Row(native[wrist+17],3);
    const auto middle=Mul(Add(Row(native[wrist+9],3),Row(native[wrist+13],3)),.5f);
    const auto across=Sub(index,little),along=Sub(middle,wp);
    const float width=std::sqrt(Dot(across,across)),length=std::sqrt(Dot(along,along));
    if(width<.025f||width>.12f||length<.035f||length>.16f)return {};
    // OpenXR grip -Z is little finger -> index/thumb. Its D3D-converted +Z
    // has that direction. +X points out of the right palm, into the left.
    const V z=Unit(across),proximal=Mul(along,-1);
    const V x=Unit(Cross(proximal,z)),y=Cross(z,x);
    if(Dot(x,x)<.9f)return {};
    M palm{};Set(palm,0,x);Set(palm,1,y);Set(palm,2,z);
    Set(palm,3,Add(Add(wp,Mul(along,.9f)),Mul(x,wrist==33?-.024f:.024f)));
    const auto inverse=InverseRigid(palm);if(!inverse)return {};
    ControllerHandPose result;
    for(int i=0;i<21;++i)result.bones[i]=Multiply(native[wrist+i],*inverse);
    return result;
}
void ControllerHandCache::Update(const std::array<M,70>& native,bool holdingPoseReady) noexcept {
    if(settled)return;
    for(const auto& bone:native)if(!InverseRigid(bone))return;
    const auto l=CaptureControllerHand(native,7),r=CaptureControllerHand(native,33);
    if(!l||!r)return;
    if(!left||!right||holdingPoseReady){left=l;right=r;reference=native;settled=holdingPoseReady;}
}
M KnifeInGrip(const M& grip) noexcept {
    // Measured stock mesh: thick handle is -Z; thin blade and tip are +Z.
    // Put the handle centre at the tracked palm and the blade at the thumb.
    M attachment{};for(int i=0;i<4;++i)attachment.values[i][i]=1;
    attachment.values[3]={0,-.006f,.100f,1};return Multiply(attachment,grip);
}
bool PistolWeapon(std::string_view name) noexcept {return name=="eu_handgun"||name=="as_handgun";}
bool PistolSupportEligible(bool pressed,bool previous,float separation) noexcept {
    return pressed && std::isfinite(separation)&&separation>=0&&separation<(previous?.21f:.14f);
}
M PistolAimFilter::Update(const M& aim,bool supported,std::int64_t time) noexcept {
    if(!supported||!InverseRigid(aim)||time<=0){Reset();return aim;}
    const auto dt=time-lastTime;
    // Never drag aim through recenter/snap turns, a long tracking gap, or a
    // deliberate fast turn. Only steady small rotations while palms meet.
    float trace=0;if(active)for(int i=0;i<3;++i)trace+=Dot(Row(aim,i),Row(filtered,i));
    if(!active||dt<0||dt>100000000||trace<2.9563f){filtered=aim;active=true;lastTime=time;return aim;}
    const float blend=dt?1-std::exp(-40.f*float(dt)*1.e-9f):0.f;
    const V z=Unit(Add(Mul(Row(filtered,2),1-blend),Mul(Row(aim,2),blend)));
    V x=Add(Mul(Row(filtered,0),1-blend),Mul(Row(aim,0),blend));x=Unit(Sub(x,Mul(z,Dot(x,z))));
    M next=aim;Set(next,0,x);Set(next,1,Cross(z,x));Set(next,2,z);
    if(!InverseRigid(next)){filtered=aim;}else filtered=next;
    lastTime=time;return filtered;
}
}
