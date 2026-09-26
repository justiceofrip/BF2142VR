#pragma once
#include "RemoteArmMath.h"
namespace bfvr::bf2142::net {
// Bounded cosmetic capsules, not server physics/hitboxes. Uses native anatomy.
Packet ConstrainRemoteHands(const BodyBones&,const Packet&) noexcept;
stereo::Vec3 ConstrainRemoteElbow(const BodyBones&,stereo::Vec3 shoulder,
 stereo::Vec3 wrist,stereo::Vec3 elbow) noexcept;
}
