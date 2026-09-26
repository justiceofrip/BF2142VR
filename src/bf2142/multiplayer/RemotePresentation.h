#pragma once
#include "RemoteArmMath.h"
namespace bfvr::bf2142::net {
// Presentation only: never send filtered values back into gameplay/networking.
struct RemotePresentation {
 Packet displayed{},previous{};std::uint64_t time=0;bool reset=true;
 Packet Update(const Packet&,std::uint64_t now) noexcept;
};
// Verified on-foot 80-bone rig only; hips/legs and their attachments stay native.
std::optional<BodyBones> SolveRemoteTorso(const BodyBones&,const Packet&) noexcept;
}
