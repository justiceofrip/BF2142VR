#include "ComfortCamera.h"
#include "TrackingMath.h"
#include <cmath>
namespace bfvr::bf2142 {
float PhysicalCameraHeight::Update(std::uintptr_t identity,int stance,float nativeY,float bodyY,std::uint64_t now) noexcept {
    if(owner!=identity){*this={};owner=identity;}
    if(!identity || stance<0||stance>2||!std::isfinite(nativeY)||!std::isfinite(bodyY))return nativeY;
    const float height=nativeY-bodyY;
    // The native first-person eye is near the soldier root +0.47m. Spawn /
    // deployment cameras can pass metres above it; never calibrate from them.
    if(!ready && stance==0 && height>.15f && height<1.1f){
        if(!since||now<since||std::abs(height-candidate)>.025f){since=now;candidate=height;}
        else if(now-since>=300){offset=candidate;ready=true;}
    }else if(!ready)since=0;
    return ready?bodyY+offset:nativeY;
}
void ComfortYaw::Bind(std::uintptr_t identity) noexcept {
    if(owner!=identity){owner=identity;recoilDegrees=0;}
}
void ComfortYaw::AddRecoil(float degrees) noexcept {
    if(owner && std::isfinite(degrees))recoilDegrees=std::remainder(recoilDegrees+double(degrees),360.0);
}
std::optional<float> ComfortYaw::Heading(float bodyDegrees,float localDegrees) const noexcept {
    if(!owner || !std::isfinite(bodyDegrees) || !std::isfinite(localDegrees))return {};
    return float(std::remainder(double(bodyDegrees)+double(localDegrees)-recoilDegrees,360.0));
}
std::optional<stereo::Matrix4> MakeComfortCamera(const stereo::Matrix4& nativeCamera,float headingDegrees) noexcept {
    if(!std::isfinite(headingDegrees) || !InverseRigid(nativeCamera))return {};
    constexpr double radians=0.017453292519943295;
    const double yaw=std::remainder(double(headingDegrees),360.0)*radians;
    const float c=float(std::cos(yaw)),s=float(std::sin(yaw));
    stereo::Matrix4 result{};
    result.values[0]={c,0,-s,0};result.values[1]={0,1,0,0};result.values[2]={s,0,c,0};
    result.values[3]=nativeCamera.values[3];
    return result;
}
}
