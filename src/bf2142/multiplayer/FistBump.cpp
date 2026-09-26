#include "FistBump.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142::net {
namespace {
using V=stereo::Vec3;
V Sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
bool Finite(V a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
V Point(const Matrix& m){return {m.values[3][0],m.values[3][1],m.values[3][2]};}
bool Closed(const Packet& p,unsigned hand){
 if(!(p.flags&(hand?RightValid:LeftValid))||(p.flags&(hand?WeaponHeld:LeftCrateHeld)))return false;
 for(unsigned j=1;j<5;++j){const float c=p.curls[hand*5+j];if(!std::isfinite(c)||c<.65f||c>1)return false;}
 // A supporting hand near a held weapon must not count as a social fist.
 if(!hand&&(p.flags&WeaponHeld)&&Dot(Sub(Point(p.left),Point(p.right)),Sub(Point(p.left),Point(p.right)))<.49f)return false;
 return true;
}
std::optional<V> Knuckle(const Packet& p,unsigned hand){
 const auto& palm=hand?p.right:p.left;if(!InverseRigid(p.body)||!InverseRigid(palm))return {};
 auto m=palm;for(int j=0;j<3;++j)m.values[3][j]-=.035f*m.values[1][j];return Point(Multiply(m,p.body));
}
}
bool FistBumpContact::Update(V separation,bool fists,std::uint64_t now) noexcept {
 if(!Finite(separation)||!fists){*this={};return false;}
 const float distance2=Dot(separation,separation);
 if(!time||now<=time||now-time>100){previous=separation;time=now;armed=distance2>.04f;return false;}
 const V step=Sub(separation,previous);const float travel2=Dot(step,step);
 if(travel2>.1225f){previous=separation;time=now;armed=false;return false;} // teleport/recenter
 const float t=travel2>1.e-8f?std::clamp(-Dot(previous,step)/travel2,0.f,1.f):0.f;
 const V nearest{previous.x+step.x*t,previous.y+step.y*t,previous.z+step.z*t};
 const bool hit=armed&&Dot(nearest,nearest)<.0144f&&travel2>.000025f&&
  Dot(previous,step)<0&&(!lastPulse||now-lastPulse>=350);
 previous=separation;time=now;
 if(hit){armed=false;lastPulse=now;}else if(distance2>.04f)armed=true;
 return hit;
}
unsigned FistBumpPeer::Update(const Packet& local,const Packet& remote,std::uint64_t now) noexcept {
 if(remote.kind!=Relay||!local.session||!remote.session||local.player==remote.player){*this={};return 0;}
 if(localSession!=local.session||remoteSession!=remote.session){*this={};localSession=local.session;remoteSession=remote.session;}
 unsigned mask=0;
 for(unsigned l=0;l<2;++l)for(unsigned r=0;r<2;++r){
  const auto a=Knuckle(local,l),b=Knuckle(remote,r);const bool fists=a&&b&&Closed(local,l)&&Closed(remote,r);
  if(contacts[l*2+r].Update(fists?Sub(*a,*b):V{},fists,now))mask|=1u<<l;
 }
 return mask;
}
}
