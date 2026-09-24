#pragma once
#include "PoseProtocol.h"
namespace bfvr::bf2142::net {
using BodyBones=std::array<Matrix,80>;
struct PalmBinding {Matrix wristFromPalm{};};
// Native animation blends can contain small scale/shear errors. Keep the same
// rigid-frame acceptance bound, but invert the accepted 3x3 basis exactly.
std::optional<Matrix> InverseAnimatedBone(const Matrix&) noexcept;
std::optional<PalmBinding> CaptureBodyPalm(const BodyBones&,bool left) noexcept;
std::optional<BodyBones> SolveRemoteArms(const BodyBones&,const Packet&,
 const PalmBinding& left,const PalmBinding& right,const BodyBones* detailedReference=nullptr) noexcept;
}
