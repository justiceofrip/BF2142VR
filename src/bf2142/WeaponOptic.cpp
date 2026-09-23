#include "WeaponOptic.h"
#include <cmath>
#include <algorithm>
namespace bfvr::bf2142 {
namespace {
using V=stereo::Vec3;using M=stereo::Matrix4;
constexpr float lensY=.055f,lensZ=.08f,radius=.034f;
V Transform(V p,const M& m){return {p.x*m.values[0][0]+p.y*m.values[1][0]+p.z*m.values[2][0]+m.values[3][0],p.x*m.values[0][1]+p.y*m.values[1][1]+p.z*m.values[2][1]+m.values[3][1],p.x*m.values[0][2]+p.y*m.values[1][2]+p.z*m.values[2][2]+m.values[3][2]};}
std::optional<V> Pixel(V p,const M& view,const M& projection,unsigned w,unsigned h){
    const auto q=Transform(p,view);if(q.z<.025f)return {};
    const auto& a=projection.values;const float d=q.x*a[0][3]+q.y*a[1][3]+q.z*a[2][3]+a[3][3];if(d<.001f)return {};
    const float x=(q.x*a[0][0]+q.y*a[1][0]+q.z*a[2][0]+a[3][0])/d;
    const float y=(q.x*a[0][1]+q.y*a[1][1]+q.z*a[2][1]+a[3][1])/d;
    if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>8||std::abs(y)>8)return {};
    return V{(x+1)*.5f*w,(1-y)*.5f*h,q.z};
}
void Mark(std::vector<DWORD>& p,unsigned w,unsigned h,int x,int y,std::uint32_t color){if(x>=0&&y>=0&&x<int(w)&&y<int(h))p[size_t(y)*w+x]=color;}
}
std::optional<V> OpticDot(const M& gun,const M& eye) noexcept {
    const auto inverse=InverseRigid(gun);if(!inverse||!InverseRigid(eye))return {};
    const V e=Transform({eye.values[3][0],eye.values[3][1],eye.values[3][2]},*inverse);
    if(e.z>=lensZ-.025f||e.z<-.9f)return {};
    // Sight zero is along the gun bore at 50 metres. Eye translation changes
    // the visible point on the glass while its direction remains bore-aligned.
    const float t=(lensZ-e.z)/(50-e.z);const V p{e.x*(1-t),e.y*(1-t),lensZ};
    if(p.x*p.x+(p.y-lensY)*(p.y-lensY)>radius*radius)return {};
    return Transform(p,gun);
}
void DrawWeaponOptic(std::vector<DWORD>& pixels,unsigned w,unsigned h,const M& gun,const M& eye,const M& projection,bool rgba){
    if(!w||!h||pixels.size()!=size_t(w)*h)return;const auto inverse=InverseRigid(eye),gunInverse=InverseRigid(gun);if(!inverse||!gunInverse)return;
    const V e=Transform({eye.values[3][0],eye.values[3][1],eye.values[3][2]},*gunInverse);
    if(e.z>=lensZ-.025f||e.z<-.9f||std::abs(e.x)>.45f||std::abs(e.y-lensY)>.45f)return;
    constexpr unsigned segments=32;
    for(unsigned i=0;i<segments;++i){const float a=float(i)*6.2831853f/segments,b=float(i+1)*6.2831853f/segments;
        const auto p=Pixel(Transform({radius*std::cos(a),lensY+radius*std::sin(a),lensZ},gun),*inverse,projection,w,h);
        const auto q=Pixel(Transform({radius*std::cos(b),lensY+radius*std::sin(b),lensZ},gun),*inverse,projection,w,h);if(!p||!q)continue;
        const int count=std::min(2048,std::max(1,int(std::hypot(q->x-p->x,q->y-p->y))));
        for(int j=0;j<=count;++j){const float t=float(j)/count;Mark(pixels,w,h,int(p->x+t*(q->x-p->x)),int(p->y+t*(q->y-p->y)),0xff777777);}
    }
    const auto world=OpticDot(gun,eye);if(!world)return;const auto hit=Pixel(*world,*inverse,projection,w,h);if(!hit)return;
    const std::uint32_t red=rgba?0xff4050ff:0xffff5040;
    for(int y=-2;y<=2;++y)for(int x=-2;x<=2;++x)if(x*x+y*y<=4)Mark(pixels,w,h,int(hit->x)+x,int(hit->y)+y,red);
}
}
