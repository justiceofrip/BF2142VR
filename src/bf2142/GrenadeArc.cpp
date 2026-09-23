#include "GrenadeArc.h"
#include "TrackingMath.h"
#include <cmath>
#include <algorithm>
namespace bfvr::bf2142 {
std::optional<stereo::Vec3> GrenadePoint(const GrenadeTrajectory& a,float t) noexcept {
    const auto& p=a.start;const auto& v=a.velocity;
    if(!std::isfinite(t)||t<0||t>3||!std::isfinite(a.gravity)||a.gravity<0||a.gravity>50||
        !std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z)||!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||v.x*v.x+v.y*v.y+v.z*v.z>2500)return {};
    return stereo::Vec3{p.x+v.x*t,p.y+v.y*t-.5f*a.gravity*t*t,p.z+v.z*t};
}
size_t DrawGrenadeArc(std::vector<DWORD>& pixels,unsigned w,unsigned h,DXGI_FORMAT format,const EyeCamera& eye,const GrenadeTrajectory& a){
    if(!w||!h||w>8192||h>8192||pixels.size()!=size_t(w)*h)return 0;
    const auto inverse=InverseRigid(eye.world);if(!inverse)return 0;size_t drawn=0;
    const auto project=[&](float t)->std::optional<stereo::Vec3>{
        const auto world=GrenadePoint(a,t);if(!world)return {};const auto& m=inverse->values;
        const float x=world->x*m[0][0]+world->y*m[1][0]+world->z*m[2][0]+m[3][0];
        const float y=world->x*m[0][1]+world->y*m[1][1]+world->z*m[2][1]+m[3][1];
        const float z=world->x*m[0][2]+world->y*m[1][2]+world->z*m[2][2]+m[3][2];if(z<.03f)return {};
        const auto& q=eye.projection.values;const float d=x*q[0][3]+y*q[1][3]+z*q[2][3]+q[3][3];if(d<=0||!std::isfinite(d))return {};
        const float u=(x*q[0][0]+y*q[1][0]+z*q[2][0]+q[3][0])/d,v=(x*q[0][1]+y*q[1][1]+z*q[2][1]+q[3][1])/d;
        if(!std::isfinite(u)||!std::isfinite(v)||std::abs(u)>4||std::abs(v)>4)return {};
        return stereo::Vec3{(u+1)*.5f*w,(1-v)*.5f*h,z};
    };
    const bool rgba=format==DXGI_FORMAT_R8G8B8A8_UNORM||format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    const DWORD color=rgba?0xffffd960:0xff60d9ff;
    // Short dotted aim guide, not a collision/landing predictor. No wall or
    // ground marker is invented without a verified native scene query.
    for(unsigned segment=0;segment<32;++segment){
        if(segment%4>=2)continue;const auto p=project(segment*.04f),q=project((segment+1)*.04f);if(!p||!q)continue;
        const int count=std::clamp(int(std::hypot(q->x-p->x,q->y-p->y)),1,512);
        for(int k=0;k<=count;++k){const float t=float(k)/count;const int x=int(p->x+t*(q->x-p->x)),y=int(p->y+t*(q->y-p->y));
            for(int dy=-1;dy<=1;++dy)for(int dx=-1;dx<=1;++dx)if(x+dx>=0&&x+dx<int(w)&&y+dy>=0&&y+dy<int(h)){pixels[size_t(y+dy)*w+x+dx]=color;++drawn;}}
    }
    return drawn;
}
}
