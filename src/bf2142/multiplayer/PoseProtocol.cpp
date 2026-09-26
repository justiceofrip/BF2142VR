#include "PoseProtocol.h"
#include <cmath>
#include <cstring>
#include <type_traits>
namespace bfvr::bf2142::net {
static_assert(std::is_trivially_copyable_v<Packet>);
stereo::Vec3 TransformVelocity(stereo::Vec3 v,const Matrix& m) noexcept {
 return {v.x*m.values[0][0]+v.y*m.values[1][0]+v.z*m.values[2][0],
         v.x*m.values[0][1]+v.y*m.values[1][1]+v.z*m.values[2][1],
         v.x*m.values[0][2]+v.y*m.values[1][2]+v.z*m.values[2][2]};
}
bool EventWindow::Accept(std::uint64_t s,std::uint32_t e,std::uint64_t now,std::uint64_t interval) noexcept {
 if(!s||!e||!now||(accepted&&now<accepted))return false;
 if(session==s && (!Newer(e,serial)||(accepted&&now-accepted<interval)))return false;
 session=s;serial=e;accepted=now;return true;
}
float Distance(const Matrix& a,const Matrix& b) noexcept {
 float d=0;for(int i=0;i<3;++i){const auto x=a.values[3][i]-b.values[3][i];d+=x*x;}return std::sqrt(d);
}
bool Validate(const Packet& p,const Secret& secret) noexcept {
 if(p.magic!=Magic||p.version!=Version||p.bytes!=sizeof(Packet)||p.player>255||!p.session||
    p.kind<Pose||p.kind>Subscribe||(p.flags&~31u))return false;
 if(!std::isfinite(p.movementYawDegrees)||std::abs(p.movementYawDegrees)>180||(!(p.flags&MovementValid)&&p.movementYawDegrees!=0))return false;
 unsigned difference=0;for(std::size_t i=0;i<secret.size();++i)difference|=secret[i]^p.secret[i];
 if(difference)return false;
 if(p.kind==Subscribe){
  // A receive-only lease cannot smuggle aim, movement, grip or action state.
  if(p.flags||p.snapSerial||p.throwSerial||p.snapDegrees!=0||p.movementYawDegrees!=0||
     p.throwVelocity.x!=0||p.throwVelocity.y!=0||p.throwVelocity.z!=0)return false;
  for(const auto* m:{&p.body,&p.camera,&p.head,&p.left,&p.right,&p.weapon,&p.throwLaunch})
   for(const auto& row:m->values)for(float v:row)if(v!=0)return false;
  for(float v:p.curls)if(v!=0)return false;
  for(char c:p.weaponName)if(c)return false;
  return true;
 }
 for(const auto* m:{&p.body,&p.camera,&p.head,&p.left,&p.right,&p.weapon})if(!InverseRigid(*m))return false;
 for(int i=0;i<3;++i)if(std::abs(p.body.values[3][i])>100000)return false;
 Matrix origin{};
 for(const auto* m:{&p.camera,&p.head,&p.left,&p.right,&p.weapon})if(Distance(*m,origin)>4)return false;
 for(float f:p.curls)if(!std::isfinite(f)||f<0||f>1)return false;
 if(p.snapSerial){if(!std::isfinite(p.snapDegrees)||std::abs(p.snapDegrees)<15||std::abs(p.snapDegrees)>90)return false;}
 else if(p.snapDegrees!=0)return false;
 const bool crate=!std::memcmp(p.weaponName.data(),"unl_hub_medic",14)||!std::memcmp(p.weaponName.data(),"unl_hub_ammo",13);
 if((p.flags&LeftCrateHeld)&&(!(p.flags&LeftValid)||!crate))return false;
 if(p.throwSerial){
  const auto v=p.throwVelocity;const float speed=v.x*v.x+v.y*v.y+v.z*v.z;
  if(!crate||!(p.flags&LeftCrateHeld)||!InverseRigid(p.throwLaunch)||Distance(p.throwLaunch,origin)>4||
     !std::isfinite(speed)||speed<.01f||speed>1600.f)return false;
 }
 bool end=false;
 for(char c:p.weaponName){if(!c){end=true;continue;}if(end||!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return false;}
 return end&&p.weaponName[0];
}
bool Decode(const void* bytes,std::size_t size,const Secret& secret,Packet* out) noexcept {
 if(!bytes||!out||size!=sizeof(Packet))return false;
 Packet p;std::memcpy(&p,bytes,sizeof(p));if(!Validate(p,secret))return false;*out=p;return true;
}
bool Newer(std::uint32_t next,std::uint32_t previous) noexcept {const auto delta=next-previous;return delta && delta<0x80000000u;}
bool FreshPose::Accept(const Packet& p,std::uint64_t now) noexcept {
 if(!now||(received&&now<received))return false;
 if(received&&now>=received&&now-received<=1000){
  if(packet.session!=p.session||!Newer(p.sequence,packet.sequence))return false;
 }
 packet=p;received=now;return true;
}
const Packet* FreshPose::Read(std::uint64_t now,std::uint64_t lifetime) const noexcept {
 return received&&now>=received&&now-received<=lifetime?&packet:nullptr;
}
}
