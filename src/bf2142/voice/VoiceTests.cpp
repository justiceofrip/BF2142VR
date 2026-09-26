#include "VoiceProtocol.h"
#include "VoiceCodec.h"
#include <cstdio>
#include <cmath>
#include <limits>
using namespace bfvr;using namespace bfvr::bf2142;using namespace bfvr::bf2142::voice;
#define CHECK(x) do{if(!(x)){std::printf("Voice check failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
stereo::Matrix4 Identity(){stereo::Matrix4 m{};for(int i=0;i<4;++i)m.values[i][i]=1;return m;}
int main(){
 net::Secret key{};key[0]=42;Packet p;p.player=3;p.session=123;p.secret=key;Packet decoded;
 CHECK(Decode(&p,HeaderBytes,key,decoded));CHECK(!Decode(&p,HeaderBytes-1,key,decoded));
 p.version=9;CHECK(!Decode(&p,HeaderBytes,key,decoded));p.version=1;p.sequence=1;CHECK(!Decode(&p,HeaderBytes,key,decoded));
 p.kind=Audio;p.size=3;CHECK(Decode(&p,HeaderBytes+3,key,decoded));CHECK(!Decode(&p,HeaderBytes+2,key,decoded));
 p.secret[1]=1;CHECK(!Decode(&p,HeaderBytes+3,key,decoded));p.secret=key;p.position.x=std::numeric_limits<float>::quiet_NaN();CHECK(!Decode(&p,HeaderBytes+3,key,decoded));p.position={};
 Member a{true,1,123,{0,0,0},1},b{true,2,456,{0,0,5},2};CHECK(CanHear(a,b,{20,true}));CHECK(!CanHear(a,b,{20,false}));
 b.team=1;CHECK(CanHear(a,b,{20,false}));b.team=0;CHECK(!CanHear(a,b,{20,false}));b.position.z=20;CHECK(!CanHear(a,b,{20,true}));b.position.z=5;b.alive=false;CHECK(!CanHear(a,b,{20,true}));b.alive=true;
 auto gains=SpatialGains({3,0,0},Identity(),20);CHECK(gains.right>.5f&&gains.left<.001f);auto left=SpatialGains({-3,0,0},Identity(),20);CHECK(left.left>.5f&&left.right<.001f);CHECK(SpatialGains({0,0,21},Identity(),20).left==0);
 std::array<short,FrameSamples> quiet{},pcm{},out{};for(int i=0;i<FrameSamples;++i)pcm[i]=short(std::sin(i*.17)*7000);
 Gate gate;CHECK(!gate.Update(quiet,-40,true));CHECK(gate.Update(pcm,-40,true));CHECK(gate.Update(quiet,-40,true));CHECK(!gate.Update(pcm,-40,false));CHECK(!gate.Update(quiet,-40,true));
 gate.Update(pcm,-40,true);for(int i=0;i<20;++i)gate.Update(quiet,-40,true);CHECK(!gate.Update(quiet,-40,true));
 Encoder encoder;Decoder decoder;CHECK(encoder.Open()&&decoder.Open());const int size=encoder.Encode(pcm,p.data);CHECK(size>0&&size<=MaxPayload);CHECK(decoder.Decode(p.data.data(),size,out));long long energy=0;for(short s:out)energy+=int(s)*s;CHECK(energy>1000000);CHECK(decoder.Decode(nullptr,0,out));CHECK(!decoder.Decode(p.data.data(),MaxPayload+1,out));
 Socket sender,receiver,enemy,intruder;CHECK(sender.Open(false,1)&&receiver.Open(false,1)&&enemy.Open(false,1)&&intruder.Open(false,1));Socket reserve;CHECK(reserve.Open(false,1));const auto port=reserve.Port();reserve.Close();Server server;CHECK(server.Open(port,key,{20,true}));
 Members members{};members[3]=a;members[4]={true,2,456,{0,0,5},1};members[5]={true,3,789,{0,0,8},2};
 const auto hello=[&](Socket& socket,unsigned id){Packet h;h.player=id;h.session=members[id].session;h.secret=key;return socket.Send(h,port);};
 CHECK(hello(sender,3)&&hello(receiver,4)&&hello(enemy,5));server.Pump(members,1000);
 p.kind=Audio;p.player=3;p.session=123;p.sequence=1;p.secret=key;p.size=std::uint16_t(size);p.position={999,999,999};p.range=100;
 CHECK(sender.Send(p,port));server.Pump(members,1020);unsigned from=0;CHECK(receiver.Receive(decoded,from,key)&&from==port&&decoded.kind==Relay&&decoded.position.x==0&&std::abs(decoded.position.y-1.6f)<.001f&&decoded.range==20);CHECK(enemy.Receive(decoded,from,key));CHECK(server.Relayed()==2);
 CHECK(sender.Send(p,port));server.Pump(members,1040);CHECK(!receiver.Receive(decoded,from,key)); // replay
 CHECK(hello(intruder,3));p.sequence=2;CHECK(intruder.Send(p,port));server.Pump(members,1060);CHECK(!receiver.Receive(decoded,from,key)); // cannot steal endpoint
 p.session=9;CHECK(sender.Send(p,port));server.Pump(members,1080);CHECK(!receiver.Receive(decoded,from,key));p.session=123;
 members[4].position.z=21;p.sequence=3;CHECK(sender.Send(p,port));server.Pump(members,1100);CHECK(!receiver.Receive(decoded,from,key));CHECK(enemy.Receive(decoded,from,key));members[4].position.z=5;
 members[3].alive=false;p.sequence=4;CHECK(sender.Send(p,port));server.Pump(members,1120);CHECK(!enemy.Receive(decoded,from,key));members[3].alive=true;
 CHECK(server.Open(port,key,{20,false}));CHECK(hello(sender,3)&&hello(receiver,4)&&hello(enemy,5));server.Pump(members,2000);p.sequence=5;CHECK(sender.Send(p,port));server.Pump(members,2020);CHECK(receiver.Receive(decoded,from,key));CHECK(!enemy.Receive(decoded,from,key));
 // Bounded sender work; excess frames do not exceed token budget.
 for(unsigned seq=6;seq<40;++seq){p.sequence=seq;CHECK(sender.Send(p,port));}server.Pump(members,2020);unsigned frames=0;while(receiver.Receive(decoded,from,key))++frames;CHECK(frames<=7);
 p.sequence=40;CHECK(sender.Send(p,port));server.Pump(members,4000);CHECK(!receiver.Receive(decoded,from,key)); // expired hello lease
 CHECK(hello(sender,3)&&hello(receiver,4));server.Pump(members,4020);p.sequence=41;CHECK(sender.Send(p,port));server.Pump(members,4040);CHECK(receiver.Receive(decoded,from,key));
 members[3].owner=17;members[3].session=999;p.sequence=42;CHECK(sender.Send(p,port));server.Pump(members,4060);CHECK(!receiver.Receive(decoded,from,key)); // replaced native owner
 puts("Opus PCM roundtrip, PLC, voice activation, spatial gains, actual UDP fan-out, distance/team policy, replay, endpoint, session/owner, expiry and rate limits passed.");
}
