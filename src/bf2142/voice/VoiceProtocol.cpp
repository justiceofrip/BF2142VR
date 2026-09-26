#include <winsock2.h>
#include <ws2tcpip.h>
#include "VoiceProtocol.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace bfvr::bf2142::voice {
namespace {bool Finite(stereo::Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z)&&std::abs(p.x)<100000&&std::abs(p.y)<100000&&std::abs(p.z)<100000;}
float Distance(stereo::Vec3 a,stereo::Vec3 b){return std::hypot(std::hypot(a.x-b.x,a.y-b.y),a.z-b.z);}}
bool Decode(const void* bytes,std::size_t n,const net::Secret& key,Packet& p) noexcept {
 if(!bytes||n<HeaderBytes||n>sizeof(Packet))return false;Packet next{};std::memcpy(&next,bytes,n);
 if(next.magic!=0x31564e42||next.version!=1||next.player>255||!next.session||next.secret!=key||next.reserved||next.size>MaxPayload||n!=HeaderBytes+next.size)return false;
 if(next.kind!=Hello&&next.kind!=Audio&&next.kind!=Relay)return false;
 if(next.kind==Hello?(next.size||next.sequence):(!next.size||!next.sequence))return false;
 if(!Finite(next.position)||!std::isfinite(next.range)||next.range<2||next.range>100)return false;p=next;return true;
}
bool Socket::Open(bool server,unsigned port){
 Close();if(!port||port>65535)return false;WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))return false;started=true;
 socket=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(socket==INVALID_SOCKET){Close();return false;}
 if(server){BOOL exclusive=TRUE;setsockopt(socket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&exclusive),sizeof(exclusive));}
 sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=htons(u_short(server?port:0));u_long nonblocking=1;
 if(bind(socket,reinterpret_cast<sockaddr*>(&a),sizeof(a))||ioctlsocket(socket,FIONBIO,&nonblocking)){Close();return false;}return true;
}
void Socket::Close(){if(socket!=INVALID_SOCKET){closesocket(socket);socket=INVALID_SOCKET;}if(started){WSACleanup();started=false;}}
bool Socket::Send(const Packet& p,unsigned port){if(socket==INVALID_SOCKET||!port||port>65535||p.size>MaxPayload)return false;
 sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=htons(u_short(port));const int n=int(HeaderBytes+p.size);
 return sendto(socket,reinterpret_cast<const char*>(&p),n,0,reinterpret_cast<sockaddr*>(&a),sizeof(a))==n;
}
bool Socket::Receive(Packet& p,unsigned& port,const net::Secret& key){if(socket==INVALID_SOCKET)return false;
 std::array<char,1201> b{};for(int work=0;work<32;++work){sockaddr_in a{};int size=sizeof(a);const int n=recvfrom(socket,b.data(),int(b.size()),0,reinterpret_cast<sockaddr*>(&a),&size);
  if(n==SOCKET_ERROR)return false;if(a.sin_family!=AF_INET||a.sin_addr.s_addr!=htonl(INADDR_LOOPBACK)||!Decode(b.data(),std::size_t(n),key,p))continue;port=ntohs(a.sin_port);return true;}return false;
}
unsigned Socket::Port()const{sockaddr_in a{};int n=sizeof(a);return socket!=INVALID_SOCKET&&!getsockname(socket,reinterpret_cast<sockaddr*>(&a),&n)?ntohs(a.sin_port):0;}
bool CanHear(const Member& from,const Member& to,const VoiceRules& rules) noexcept {
 return from.alive&&to.alive&&from.owner&&to.owner&&from.owner!=to.owner&&Finite(from.position)&&Finite(to.position)&&std::isfinite(rules.range)&&rules.range>=2&&rules.range<=100&&
  (rules.enemies||(from.team>=1&&from.team<=2&&to.team==from.team))&&Distance(from.position,to.position)<rules.range;
}
Gains SpatialGains(stereo::Vec3 source,const stereo::Matrix4& listener,float range) noexcept {
 if(!Finite(source)||!InverseRigid(listener)||!std::isfinite(range)||range<2||range>100)return {};
 const auto& t=listener.values[3];const float x=source.x-t[0],y=source.y-t[1],z=source.z-t[2];const float distance=std::hypot(std::hypot(x,y),z);
 const float gain=std::clamp((range-distance)/(range-1.5f),0.f,1.f);const auto& right=listener.values[0];
 const float pan=distance>.01f?std::clamp((x*right[0]+y*right[1]+z*right[2])/distance,-1.f,1.f):0;
 return {gain*gain*std::sqrt((1-pan)*.5f),gain*gain*std::sqrt((1+pan)*.5f)};
}
bool Gate::Update(const std::array<short,FrameSamples>& pcm,float db,bool enabled) noexcept {
 if(!enabled||!std::isfinite(db)){Reset();return false;}double sum=0;for(short n:pcm){const double a=double(n)/32768;sum+=a*a;}
 const double threshold=std::pow(10.,std::clamp(db,-65.f,-15.f)/20.);
 if(sum/FrameSamples>=threshold*threshold)remaining=12;else if(remaining) --remaining;return remaining>0;
}
bool Server::Open(unsigned port,const net::Secret& secret,VoiceRules value){Reset();key=secret;rules=value;return socket.Open(true,port);}
void Server::Reset(){socket.Close();peers={};relayed=0;}
void Server::Pump(const Members& members,std::uint64_t now){
 for(unsigned i=0;i<256;++i){auto& p=peers[i];const auto& m=members[i];if(!m.alive||m.owner!=p.owner||m.session!=p.session||!m.session)p={};}
 Packet p;unsigned port=0;
 for(unsigned work=0;work<96&&socket.Receive(p,port,key);++work){
  const auto& m=members[p.player];auto& peer=peers[p.player];
  if(!m.alive||!m.owner||!m.session||p.session!=m.session)continue;
  if(peer.port&&peer.port!=port&&now-peer.seen<1000)continue;
  if(p.kind==Hello){
   if(peer.port!=port||peer.session!=p.session)peer={m.owner,m.session,now,now,port,0,8};else peer.seen=now;
   continue;
  }
  if(p.kind!=Audio||peer.port!=port||now-peer.seen>1000||!net::Newer(p.sequence,peer.sequence))continue;
  peer.sequence=p.sequence;peer.tokens=std::min(8.f,peer.tokens+float(now-peer.rateTime)*.05f);peer.rateTime=now;
  if(peer.tokens<1)continue;peer.tokens-=1;
  p.kind=Relay;p.position=m.position;p.position.y+=1.6f;p.range=rules.range;
  for(unsigned id=0;id<256;++id)if(peers[id].port&&now-peers[id].seen<=1000&&CanHear(m,members[id],rules)&&socket.Send(p,peers[id].port))++relayed;
 }
}
}
