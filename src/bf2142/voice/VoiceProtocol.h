#pragma once
#include "../multiplayer/LoopbackTransport.h"
#include <array>
#include <cstdint>
namespace bfvr::bf2142::voice {
constexpr int SampleRate=16000,FrameSamples=320,FrameMs=20,MaxPayload=400;
enum :std::uint16_t {Hello=1,Audio=2,Relay=3};
#pragma pack(push,1)
struct Packet {
 std::uint32_t magic=0x31564e42;std::uint16_t version=1,kind=Hello;
 std::uint32_t player=256,sequence=0;std::uint64_t session=0;
 net::Secret secret{};stereo::Vec3 position{};float range=20;
 std::uint16_t size=0,reserved=0;std::array<unsigned char,MaxPayload> data{};
};
#pragma pack(pop)
constexpr std::size_t HeaderBytes=offsetof(Packet,data);
static_assert(HeaderBytes==60 && sizeof(Packet)==460);
bool Decode(const void*,std::size_t,const net::Secret&,Packet&) noexcept;
class Socket {
public:
 Socket()=default;Socket(const Socket&)=delete;Socket& operator=(const Socket&)=delete;
 bool Open(bool server,unsigned port);void Close();~Socket(){Close();}
 bool Send(const Packet&,unsigned port);bool Receive(Packet&,unsigned& port,const net::Secret&);
 unsigned Port() const;
private:std::uintptr_t socket=~std::uintptr_t{};bool started=false;
};
struct Member {bool alive=false;std::uint64_t owner=0,session=0;stereo::Vec3 position{};int team=0;};
using Members=std::array<Member,256>;
struct VoiceRules {float range=20;bool enemies=true;};
bool CanHear(const Member&,const Member&,const VoiceRules&) noexcept;
struct Gains {float left=0,right=0;};
Gains SpatialGains(stereo::Vec3 source,const stereo::Matrix4& listener,float range) noexcept;
class Gate {
public:
 bool Update(const std::array<short,FrameSamples>&,float thresholdDb,bool enabled) noexcept;
 void Reset() noexcept {remaining=0;}
private:int remaining=0;
};
class Server {
public:
 bool Open(unsigned port,const net::Secret&,VoiceRules);
 void Pump(const Members&,std::uint64_t now);
 void Reset();
 std::uint64_t Relayed()const{return relayed;}
private:
 struct Peer {std::uint64_t owner=0,session=0,seen=0,rateTime=0;unsigned port=0;std::uint32_t sequence=0;float tokens=8;};
 Socket socket;net::Secret key{};VoiceRules rules{};std::array<Peer,256> peers{};std::uint64_t relayed=0;
};
}
