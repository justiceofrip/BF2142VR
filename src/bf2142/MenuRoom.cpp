#include "MenuRoom.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
using V=stereo::Vec3;
stereo::Pose Pose(const shared::SharedPresentationPose& p){return {{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}};}
V Rotate(const stereo::Quaternion& q,V v){
 const V t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
 return {v.x+q.w*t.x+q.y*t.z-q.z*t.y,v.y+q.w*t.y+q.z*t.x-q.x*t.z,v.z+q.w*t.z+q.x*t.y-q.y*t.x};
}
float Band(float value,float halfWidth,float footprint){
 footprint=std::max(footprint,.0001f);
 return std::clamp((std::min(value+footprint*.5f,halfWidth)-std::max(value-footprint*.5f,-halfWidth))/footprint,0.f,1.f);
}
V Mix(V a,V b,float t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};}
}
float MenuStripeCoverage(float value,float period,float halfWidth,float footprint) noexcept {
 if(!std::isfinite(value)||!std::isfinite(period)||!std::isfinite(halfWidth)||!std::isfinite(footprint)||period<=0||halfWidth<=0||footprint<0)return 0;
 footprint=std::max(footprint,.0001f);halfWidth=std::min(halfWidth,period*.5f);
 // Integrate the periodic stripe over a pixel footprint. Distant lines keep
 // their average coverage rather than blinking as a sample crosses them.
 const auto area=[=](float x){const float u=x+halfWidth,n=std::floor(u/period);return n*(2*halfWidth)+std::min(u-n*period,2*halfWidth);};
 return std::clamp((area(value+footprint*.5f)-area(value-footprint*.5f))/footprint,0.f,1.f);
}
void DrawMenuRoom(std::vector<DWORD>& left,std::vector<DWORD>& right,UINT w,UINT h,DXGI_FORMAT format,
 const shared::SharedRenderRequest& request,const stereo::Pose& anchor){
 if(!w||!h||w>8192||h>8192)return;const size_t count=size_t(w)*h;
 const bool rgba=format==DXGI_FORMAT_R8G8B8A8_UNORM||format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
 for(unsigned eye=0;eye<2;++eye){auto& image=eye?right:left;if(image.size()!=count)return;
  const auto local=stereo::MakeRelativePose(anchor,Pose(request.views[eye].pose));if(!local)return;
  const auto& view=request.views[eye];const float l=std::tan(view.fov.angleLeft),rr=std::tan(view.fov.angleRight),u=std::tan(view.fov.angleUp),d=std::tan(view.fov.angleDown);
  if(!std::isfinite(l)||!std::isfinite(rr)||!std::isfinite(u)||!std::isfinite(d)||rr<=l||u<=d)return;
  const float origin[]={local->position.x,local->position.y,local->position.z};
  const float low[]={-4.f,-1.6f,-5.f},high[]={4.f,2.5f,3.f};
  const auto vx=Rotate(local->orientation,{(rr-l)/w,0,0}),vy=Rotate(local->orientation,{0,-(u-d)/h,0});
  const float dx[]={vx.x,vx.y,vx.z},dy[]={vy.x,vy.y,vy.z};
  // Full output resolution: no 2x2 nearest-neighbour blocks. Ray derivatives
  // supply analytic anti-aliasing without additional scene renders or history.
  for(UINT y=0;y<h;++y){const float py=u-(float(y)+.5f)/h*(u-d);
   const auto first=Rotate(local->orientation,{l+.5f/w*(rr-l),py,-1});
   for(UINT x=0;x<w;++x){float ray[]={first.x+x*vx.x,first.y+x*vx.y,first.z+x*vx.z};float distance=10000;int face=-1;
    for(int axis=0;axis<3;++axis)if(std::abs(ray[axis])>1e-6f){float t=((ray[axis]>0?high[axis]:low[axis])-origin[axis])/ray[axis];if(t>0&&t<distance){distance=t;face=axis;}}
    V color{12,20,29};
    if(face>=0){float point[3]{},footprint[3]{};
     for(int i=0;i<3;++i){point[i]=origin[i]+distance*ray[i];footprint[i]=distance*(std::abs(dx[i]-ray[i]*dx[face]/ray[face])+std::abs(dy[i]-ray[i]*dy[face]/ray[face]));}
     const bool floor=face==1&&ray[1]<0;
     if(floor){const float a=MenuStripeCoverage(point[0],1,.018f,footprint[0]),b=MenuStripeCoverage(point[2],1,.018f,footprint[2]);
      color=Mix({26,40,50},{13,25,34},1-(1-a)*(1-b));
      const float light=Band(std::abs(point[0])-2.93f,.03f,footprint[0]);color=Mix(color,{30,138,160},light);
     }else if(face==1){color={17,28,39};}
     else{const int horizontal=face==0?2:0;
      color=Mix({24,39,53},{12,24,34},MenuStripeCoverage(point[horizontal],1.6f,.018f,footprint[horizontal]));
      const float light=std::max(Band(point[1]-1.75f,.045f,footprint[1]),Band(point[1]+1.35f,.028f,footprint[1]));color=Mix(color,{60,191,211},light);
     }
     const float fog=std::clamp(1.f-distance*.025f,.55f,1.f);color={color.x*fog,color.y*fog,color.z*fog};
    }
    const DWORD red=DWORD(std::clamp(color.x,0.f,255.f)+.5f),green=DWORD(std::clamp(color.y,0.f,255.f)+.5f),blue=DWORD(std::clamp(color.z,0.f,255.f)+.5f);
    image[size_t(y)*w+x]=0xff000000|(rgba?blue:red)<<16|green<<8|(rgba?red:blue);
   }
  }
 }
}
}
