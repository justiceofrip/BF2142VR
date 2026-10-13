#pragma once
#include "TrackingMath.h"
#include <DirectXMath.h>
#include <cmath>
namespace bfvr::bf2142 {
// Inverse of ComposeRuntimeHeadWithD3D8Camera. Native aim uses the vehicle's
// real camera (turret for gunners, fixed weapon direction for pilots). Converting
// through the comfort base keeps head motion from rotating the sight a second time.
inline std::optional<stereo::Pose> VehicleHudAnchor(const stereo::Matrix4& aim,
    const stereo::Matrix4& base,const stereo::Pose& reference,float scale){
    using namespace DirectX;
    const auto inv=InverseRigid(base),refView=stereo::MakeD3D8ViewFromOpenXRPose(reference);
    if(!inv||!refView||!InverseRigid(aim)||!std::isfinite(scale)||scale<=0)return {};
    const auto refWorld=InverseRigid(*refView);if(!refWorld)return {};
    auto relative=Multiply(aim,*inv);
    for(int i=0;i<3;++i)relative.values[3][i]/=scale;
    const auto runtime=Multiply(relative,*refWorld);
    XMFLOAT4X4 matrix{};
    for(int r=0;r<4;++r)for(int c=0;c<4;++c)matrix.m[r][c]=runtime.values[r][c]*(r==2?-1.f:1.f)*(c==2?-1.f:1.f);
    XMFLOAT4 q{};XMStoreFloat4(&q,XMQuaternionNormalize(XMQuaternionRotationMatrix(XMLoadFloat4x4(&matrix))));
    return stereo::Pose{{runtime.values[3][0],runtime.values[3][1],-runtime.values[3][2]},{q.x,q.y,q.z,q.w}};
}
}
