#pragma once
#include "PoseProtocol.h"
namespace bfvr::bf2142::net {
using BodyBones=std::array<Matrix,80>;
struct RemoteArmContinuity {std::array<stereo::Vec3,2> poles{};std::array<bool,2> valid{};};
struct PalmBinding {Matrix wristFromPalm{};};
// Native animation blends can contain small scale/shear errors. Keep the same
// rigid-frame acceptance bound, but invert the accepted 3x3 basis exactly.
std::optional<Matrix> InverseAnimatedBone(const Matrix&) noexcept;
// Rotate only the verified head/face subtree around its native attachment.
// The packet already expresses the HMD look basis in soldier-local space.
std::optional<BodyBones> SolveRemoteHead(const BodyBones&,const Matrix& trackedHead) noexcept;
std::optional<PalmBinding> CaptureBodyPalm(const BodyBones&,bool left) noexcept;
// The verified third-person rig has thumb/index/grouped remaining-finger chains.
// Invalid or collapsed finger geometry leaves the already-solved arm untouched.
bool PoseRemoteFingers(BodyBones&,bool left,const Matrix& palm,
 const std::array<float,5>& curls,bool holding) noexcept;
std::optional<BodyBones> SolveRemoteArms(const BodyBones&,const Packet&,
 const PalmBinding& left,const PalmBinding& right,const BodyBones* detailedReference=nullptr,
 RemoteArmContinuity* continuity=nullptr,float elapsedSeconds=0) noexcept;
}
