#pragma once
#include "PoseProtocol.h"
namespace bfvr::bf2142::net {
// Cosmetic local contact only. No damage, force, or synthetic remote events.
struct FistBumpContact {
 stereo::Vec3 previous{};std::uint64_t time=0,lastPulse=0;bool armed=false;
 bool Update(stereo::Vec3 separation,bool fists,std::uint64_t now) noexcept;
};
struct FistBumpPeer {
 std::array<FistBumpContact,4> contacts{};
 std::uint64_t localSession=0,remoteSession=0;
 unsigned Update(const Packet& local,const Packet& remote,std::uint64_t now) noexcept;
};
}
