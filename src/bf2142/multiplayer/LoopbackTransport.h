#pragma once
#include "PoseProtocol.h"
namespace bfvr::bf2142::net {
struct Settings {bool enabled=false,mirror=false;unsigned port=17568;Secret secret{};};
Settings LoadSettings();
class Transport {
public:
 bool Open(bool server,unsigned port);
 bool Receive(Packet*,unsigned* sourcePort,const Secret&);
 bool Send(const Packet&,unsigned port);
 unsigned LocalPort() const;
 void Close();
 ~Transport(){Close();}
private:
 std::uintptr_t socket=~std::uintptr_t{};bool started=false;
};
}
