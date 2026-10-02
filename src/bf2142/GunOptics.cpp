#include "GunOptics.h"
#include <algorithm>
#include <cmath>
#include <limits>
namespace bfvr::bf2142 {
namespace {
using V=stereo::Vec3;using M=stereo::Matrix4;
// Measured rear apertures of stock geom 0 / LOD 0, in metres. No meshes or
// textures are bundled. Unknown weapons keep their native presentation.
constexpr OpticDefinition definitions[]={
    // EU LMG glass bounds: x +/- .01364, y .07386..09740, z -.1329.
    // Inset from the bezel. A reflex sight overlays only its dot, with no
    // magnified replay or zoomed rectangle surrounding the gun.
    {"eu_mg",{0,.08563f,-.1332f},.0123f,.0104f,true,.59f,1.f},
    {"eu_ar_rifle",{0,.095f,-.081f},.0114f,.0137f,false,.484f,2.f},
    {"as_ar_rifle",{-.0012f,.0815f,-.069f},.0098f,.0098f,false,.484f,2.f},
    {"eu_sni",{0,.0974f,-.073f},.0210f,.0084f,true,.312f,4.f},
    {"as_sni",{.0008f,.091f,-.225f},.0118f,.0118f,false,.312f,4.f},
    {"unl_adv_sni",{0,.089f,-.233f},.0140f,.0110f,true,.312f,4.f},
    // SMG rear sight faces, inset from their bezels. Keep 1x reflex rendering.
    {"eu_smg",{-.01135f,.13425f,-.1056f},.0062f,.0082f,true,.749f,1.f,true},
    {"as_smg",{.00029f,.0542f,.0605f},.0080f,.0070f,false,.8f,1.f,true},
    // Launcher side displays. Stock native zoom is .85, not the rifle .484.
    // Their off-bore position shares the existing finite bore-zero projection.
    {"eu_av",{-.07075f,.0778f,.0665f},.0310f,.0110f,true,.85f,1.5f},
    {"as_av",{-.16296f,.06845f,.0980f},.0300f,.0225f,true,.85f,1.5f},
    {"unl_av_rifle",{-.00025f,.09275f,-.3257f},.0150f,.0140f,false,.59f,2.f}
};
V Sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V Add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
V Mul(V v,float x){return {v.x*x,v.y*x,v.z*x};}
float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
V Unit(V v){const float n=std::sqrt(Dot(v,v));return n>1.e-6f?Mul(v,1/n):V{};}
V Pos(const M& m){return {m.values[3][0],m.values[3][1],m.values[3][2]};}
V Direction(V v,const M& m){return {v.x*m.values[0][0]+v.y*m.values[1][0]+v.z*m.values[2][0],v.x*m.values[0][1]+v.y*m.values[1][1]+v.z*m.values[2][1],v.x*m.values[0][2]+v.y*m.values[1][2]+v.z*m.values[2][2]};}
V Transform(V v,const M& m){return Add(Direction(v,m),Pos(m));}
bool Valid(const GunOptic& o){return o.definition && std::isfinite(o.magnification) && o.magnification>=1 && o.magnification<=16 && InverseRigid(o.gun).has_value();}
bool Aperture(float u,float v,bool rect){return rect?std::max(std::abs(u),std::abs(v))<1 && (std::max(0.f,std::abs(u)-.84f)*std::max(0.f,std::abs(u)-.84f)+std::max(0.f,std::abs(v)-.84f)*std::max(0.f,std::abs(v)-.84f)<.0256f):u*u+v*v<1;}
std::optional<V> Project(V p,const M& view,const M& proj,unsigned w,unsigned h){
    p=Transform(p,view);if(p.z<.015f)return {};
    const auto& a=proj.values;const float d=p.x*a[0][3]+p.y*a[1][3]+p.z*a[2][3]+a[3][3];if(!std::isfinite(d)||d<=0)return {};
    const float x=(p.x*a[0][0]+p.y*a[1][0]+p.z*a[2][0]+a[3][0])/d;
    const float y=(p.x*a[0][1]+p.y*a[1][1]+p.z*a[2][1]+a[3][1])/d;
    if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>16||std::abs(y)>16)return {};
    return V{(x+1)*.5f*w,(1-y)*.5f*h,p.z};
}
std::uint32_t Blend(std::uint32_t a,std::uint32_t b,float t){
    std::uint32_t p=0xff000000;for(unsigned s=0;s<24;s+=8){const float x=float((a>>s)&255),y=float((b>>s)&255);p|=std::uint32_t(std::clamp(x+(y-x)*t,0.f,255.f))<<s;}return p;
}
std::uint32_t Sample(const std::vector<DWORD>& p,unsigned w,unsigned h,float u,float v){
    if(u<0||v<0||u>1||v>1)return 0xff080b0e;
    const float x=u*(w-1),y=v*(h-1);const unsigned a=unsigned(x),b=unsigned(y),c=std::min(a+1,w-1),d=std::min(b+1,h-1);
    return Blend(Blend(p[size_t(b)*w+a],p[size_t(b)*w+c],x-a),Blend(p[size_t(d)*w+a],p[size_t(d)*w+c],x-a),y-b);
}
}
void IsolateOpticHud(std::vector<DWORD>& hud,const std::vector<DWORD>& baseline,unsigned w,unsigned h){
 if(!w||!h||hud.size()!=size_t(w)*h||baseline.size()!=hud.size()){hud.clear();return;}
 size_t visible=0;
 for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){auto& p=hud[size_t(y)*w+x];const auto b=baseline[size_t(y)*w+x];
  if(x<w/4||x>=w*3/4||y<h/4||y>=h*3/4){p=0;continue;}
  unsigned difference=0;for(unsigned s=0;s<32;s+=8)difference+=unsigned(std::abs(int((p>>s)&255)-int((b>>s)&255)));
  if(difference<8)p=0;visible+=(p>>24)>0;
 }
 if(!visible)hud.clear(); // unrecognized/custom HUD keeps the tested fallback reticle
}
const OpticDefinition* FindGunOptic(std::string_view name) noexcept{for(const auto& d:definitions)if(name==d.name)return &d;return nullptr;}
std::optional<OpticView> MakeOpticView(const GunOptic& o,const std::array<EyeCamera,2>& eyes) noexcept{
    if(!Valid(o))return {};const auto inv=InverseRigid(o.gun);if(!inv)return {};
    const auto& d=*o.definition;OpticView out;float best=0;
    for(size_t i=0;i<eyes.size();++i){
        if(!InverseRigid(eyes[i].world))return {};
        const V e=Transform(Pos(eyes[i].world),*inv),forward=Direction({eyes[i].world.values[2][0],eyes[i].world.values[2][1],eyes[i].world.values[2][2]},*inv);
        const float relief=d.center.z-e.z;
        if(relief<.045f||relief>.55f||forward.z<.85f)continue;
        const float x=(e.x-d.center.x)/(.012f+d.halfWidth),y=(e.y-d.center.y)/(.012f+d.halfHeight);
        const float radial=std::sqrt(x*x+y*y);
        const float v=std::clamp((1.15f-radial)*3.f,0.f,1.f)*std::clamp((relief-.045f)/.03f,0.f,1.f)*std::clamp((.55f-relief)/.1f,0.f,1.f);
        out.visibility[i]=v;if(v>best){best=v;out.relief=relief;}
    }
    if(best<.02f)return {};
    // Finite 100 m bore zero retains sight-over-bore at close range. No shot
    // state is changed. The scope camera and reticle use this same line.
    const V localOrigin=Add(d.center,{0,0,.025f}),z=Unit(Sub({0,0,100},localOrigin));
    const V x=Unit(Cross({0,1,0},z)),y=Cross(z,x);M local{};
    local.values[0]={x.x,x.y,x.z,0};local.values[1]={y.x,y.y,y.z,0};local.values[2]={z.x,z.y,z.z,0};local.values[3]={localOrigin.x,localOrigin.y,localOrigin.z,1};
    out.world=Multiply(local,o.gun);
    const float tx=d.halfWidth/(out.relief*o.magnification),ty=d.halfHeight/(out.relief*o.magnification);
    out.fov={-tx,tx,ty,-ty};return out;
}
size_t CompositeGunOptic(std::vector<DWORD>& pixels,const std::vector<DWORD>& scope,
    unsigned w,unsigned h,const GunOptic& o,const EyeCamera& eye,const OpticView& frame,unsigned index,bool rgba,const std::vector<DWORD>& nativeHud){
    if(!Valid(o)||!w||!h||w>8192||h>8192||index>1||pixels.size()!=size_t(w)*h||(o.magnification>1.01f && scope.size()!=pixels.size())||frame.visibility[index]<.02f)return 0;
    const auto inverse=InverseRigid(eye.world),gunInverse=InverseRigid(o.gun);if(!inverse||!gunInverse)return 0;
    const auto& d=*o.definition;const V e=Transform(Pos(eye.world),*gunInverse);const float relief=d.center.z-e.z;
    if(relief<=.04f||!std::isfinite(frame.relief)||frame.relief<=0)return 0;
    float minX=float(w),minY=float(h),maxX=0,maxY=0;
    for(float x:{-1.f,1.f})for(float y:{-1.f,1.f}){
        const auto p=Project(Transform(Add(d.center,{x*d.halfWidth,y*d.halfHeight,0}),o.gun),*inverse,eye.projection,w,h);
        if(!p)return 0;minX=std::min(minX,p->x);minY=std::min(minY,p->y);maxX=std::max(maxX,p->x);maxY=std::max(maxY,p->y);
    }
    const int x0=int(std::clamp(std::floor(minX),0.f,float(w))),x1=int(std::clamp(std::ceil(maxX),0.f,float(w)));
    const int y0=int(std::clamp(std::floor(minY),0.f,float(h))),y1=int(std::clamp(std::ceil(maxY),0.f,float(h)));
    const float t=relief/(100-e.z);const V dot=Add(e,Mul(Sub({0,0,100},e),t));
    const auto& p=eye.projection.values;if(!std::isfinite(p[0][0])||!std::isfinite(p[1][1])||p[0][0]<=0||p[1][1]<=0)return 0;
    const M eyeToGun=Multiply(eye.world,*gunInverse);const float scale=frame.relief/relief;
    // Reticle angular thickness stays legible at different render resolutions.
    const float line=std::clamp(relief/(h*p[1][1]),.00010f,.0008f);
    const std::uint32_t ink=d.redDot?(rgba?0xff4040ff:0xffff4040):(rgba?0xff70dcff:0xffffdc70);
    size_t marked=0;
    for(int y=y0;y<y1;++y)for(int x=x0;x<x1;++x){
        const V ray=Direction({(2*(x+.5f)/w-1-p[2][0])/p[0][0],(1-2*(y+.5f)/h-p[2][1])/p[1][1],1},eyeToGun);
        if(ray.z<=.01f)continue;const V hit=Add(e,Mul(ray,relief/ray.z));
        const float u=(hit.x-d.center.x)/d.halfWidth,v=(hit.y-d.center.y)/d.halfHeight;
        if(!Aperture(u,v,d.rectangular))continue;
        const float dx=hit.x-dot.x,dy=hit.y-dot.y;
        const float su=.5f+dx/d.halfWidth*.5f*scale,sv=.5f-dy/d.halfHeight*.5f*scale;
        const bool reflex=o.magnification<=1.01f;
        auto color=reflex?pixels[size_t(y)*w+x]:Sample(scope,w,h,su,sv);
        const bool center=dx*dx+dy*dy<line*line*2.25f;
        const bool cross=(std::abs(dx)<line && std::abs(dy)<d.halfHeight*.45f && std::abs(dy)>line*3) ||
            (std::abs(dy)<line && std::abs(dx)<d.halfWidth*.45f && std::abs(dx)>line*3);
        const bool hasHud=!d.redDot && nativeHud.size()==pixels.size();
        if(hasHud){
            // Native 800x600 UI coordinates: central 400x300 contains the
            // stock scope reticle, compass, rangefinder and stabilizer.
            const float hu=.5f+(su-.5f)*.5f,hv=.5f+(sv-.5f)*.5f;
            if(hu>=.25f&&hu<.75f&&hv>=.25f&&hv<.75f){
                const DWORD hud=nativeHud[size_t(hv*h)*w+unsigned(hu*w)];const unsigned a=hud>>24;
                if(reflex&&!a)continue;
                DWORD blended=0xff000000;for(unsigned channel=0;channel<24;channel+=8)
                    blended|=std::min<DWORD>(255u,((hud>>channel)&255)+(((color>>channel)&255)*(255-a)+127)/255)<<channel;
                color=blended;
            }else if(reflex)continue;
        }else {
            if(reflex && !center)continue;
            if(center||(d.magnification>=4 && cross))color=ink;
        }
        const float edge=d.rectangular?std::max(std::abs(u),std::abs(v)):std::sqrt(u*u+v*v);
        const float alpha=frame.visibility[index]*std::clamp((1-edge)*25.f,0.f,1.f);
        auto& dest=pixels[size_t(y)*w+x];dest=Blend(dest,color,alpha);++marked;
    }
    return marked;
}
}
