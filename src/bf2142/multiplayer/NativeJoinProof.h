#pragma once
#include "PoseProtocol.h"
namespace bfvr::bf2142::net {
constexpr std::uint32_t JoinProofMagic=0x31414642,JoinProofCommand=0x42564631;
struct JoinChallenge {std::uint32_t magic=JoinProofMagic,version=1,player=256,reserved=0;std::uint64_t session=0;Secret secret{},nonce{};};
static_assert(sizeof(JoinChallenge)==56);
bool DecodeJoinChallenge(const void*,std::size_t,const Secret&,JoinChallenge&) noexcept;
class JoinProofPolicy {
public:bool Accept(const JoinChallenge&,unsigned player,std::uint64_t session,std::uint64_t now) noexcept;
private:Secret previous{};std::uint64_t last=0,session=0;
};
// Called on the game's owning thread only; native ClientCommand supplies issuer.
bool SendNativeJoinProof(void* game,void* localPlayer,const Secret& nonce) noexcept;
}
