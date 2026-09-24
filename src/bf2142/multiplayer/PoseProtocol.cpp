#include "PoseProtocol.h"
#include <cmath>
#include <cstring>
#include <type_traits>
namespace bfvr::bf2142::net {
static_assert(std::is_trivially_copyable_v<Packet>);
float Distance(const Matrix& a,const Matrix& b) noexcept {
 float d=0;for(int i=0;i<3;++i){const auto x=a.values[3][i]-b.values[3][i];d+=x*x;}return std::sqrt(d);
}
bool Validate(const Packet& p,const Secret& secret) noexcept {
 if(p.magic!=Magic||p.version!=Version||p.bytes!=sizeof(Packet)||p.player>255||!p.session||
    p.kind<Pose||p.kind>Mirror||(p.flags&~7u))return false;
 unsigned difference=0;for(std::size_t i=0;i<secret.size();++i)difference|=secret[i]^p.secret[i];
 if(difference)return false;
 for(const auto* m:{&p.body,&p.camera,&p.head,&p.left,&p.right,&p.weapon})if(!InverseRigid(*m))return false;
 for(int i=0;i<3;++i)if(std::abs(p.body.values[3][i])>100000)return false;
 Matrix origin{};
 for(const auto* m:{&p.camera,&p.head,&p.left,&p.right,&p.weapon})if(Distance(*m,origin)>4)return false;
 for(float f:p.curls)if(!std::isfinite(f)||f<0||f>1)return false;
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
