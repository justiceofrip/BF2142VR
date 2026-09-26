#include "WorldMarker.h"
#include "TrackingMath.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
stereo::Vec3 Point(const stereo::Vec3& p,const stereo::Matrix4& m){
 return {p.x*m.values[0][0]+p.y*m.values[1][0]+p.z*m.values[2][0]+m.values[3][0],
         p.x*m.values[0][1]+p.y*m.values[1][1]+p.z*m.values[2][1]+m.values[3][1],
         p.x*m.values[0][2]+p.y*m.values[1][2]+p.z*m.values[2][2]+m.values[3][2]};
}
bool Projection(const stereo::Matrix4& m){
 for(const auto& row:m.values)for(float v:row)if(!std::isfinite(v))return false;
 return m.values[0][0]>.01f && m.values[1][1]>.01f &&
  std::abs(m.values[2][3]-1.f)<.001f && std::abs(m.values[3][3])<.001f;
}
}
std::optional<WorldMarkerPoint> ProjectWorldMarker(const stereo::Vec3& world,
    const EyeCamera& head,const EyeCamera& eye,float limit) noexcept {
 if(!std::isfinite(world.x)||!std::isfinite(world.y)||!std::isfinite(world.z)||
    !std::isfinite(limit)||limit<=0||limit>=1||!Projection(head.projection)||!Projection(eye.projection))return {};
 const auto hi=InverseRigid(head.world),ei=InverseRigid(eye.world);if(!hi||!ei)return {};
 const auto p=Point(world,*hi);const auto& hp=head.projection.values;
 const float z=std::max(.01f,p.z);
 float x=p.x*hp[0][0]/z+hp[2][0],y=p.y*hp[1][1]/z+hp[2][1];
 const bool edge=p.z<=.01f||std::abs(x)>limit||std::abs(y)>limit;
 stereo::Vec3 target=world;
 if(edge){
  const float length=std::hypot(x,y);if(!std::isfinite(length))return {};
  if(length<1e-6f){x=0;y=-limit;}else{x=x/length*limit;y=y/length*limit;}
  // Direction arrows are distant cues. A common point at 100 metres retains
  // natural binocular disparity, including head roll and canted eye poses.
  constexpr float depth=100.f;
  target=Point({(x-hp[2][0])*depth/hp[0][0],(y-hp[2][1])*depth/hp[1][1],depth},head.world);
 }
 const auto q=Point(target,*ei);if(q.z<=.001f)return {};
 const auto& ep=eye.projection.values;
 x=q.x*ep[0][0]/q.z+ep[2][0];y=q.y*ep[1][1]/q.z+ep[2][1];
 const auto actual=Point(world,*ei);const float distance=std::sqrt(actual.x*actual.x+actual.y*actual.y+actual.z*actual.z);
 if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(distance))return {};
 return WorldMarkerPoint{x,y,edge?0.f:1.f,edge?1.f:distance,edge};
}
}
