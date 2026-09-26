#pragma once
#include "PoseProtocol.h"
#include "NativeJoinProof.h"
namespace bfvr::bf2142::net {
struct Settings {bool enabled=false,mirror=false,voiceEnabled=false,enemyProximity=true;unsigned port=17568,voicePort=17569;float voiceRange=20;Secret secret{};};
Settings LoadSettings();
class Transport {
public:
 bool Open(bool server,unsigned port);
 bool Receive(Packet*,unsigned* sourcePort,const Secret&);
 bool Send(const Packet&,unsigned port);
 unsigned LocalPort() const;
 bool TakeJoinChallenge(JoinChallenge& out){if(!haveChallenge)return false;out=challenge;haveChallenge=false;return true;}
 void Close();
 ~Transport(){Close();}
private:
 JoinChallenge challenge{};bool haveChallenge=false;unsigned expectedPort=0;
 std::uintptr_t socket=~std::uintptr_t{};bool started=false;
};
}
