#pragma once
#include "ControllerPolicy.h"
#include "TrackingMath.h"
namespace bfvr::bf2142 {
enum class TraversalMode {Unavailable,Foot,Ladder,Parachute};
struct TraversalSample {
 TraversalMode mode=TraversalMode::Unavailable;std::uint64_t owner=0,mount=0;
 stereo::Matrix4 world{};float verticalSpeed=0;bool velocityValid=false;
};
class TraversalView {
public:
 void RecordFoot(std::uint64_t owner,const stereo::Matrix4& camera,std::uint64_t time) noexcept;
 std::optional<stereo::Matrix4> Mounted(const TraversalSample&,const stereo::Matrix4& nativeCamera,std::uint64_t time) noexcept;
 void Reset() noexcept {*this={};}
private:
 std::uint64_t owner=0,mount=0,lastFoot=0;bool haveFoot=false;
 stereo::Matrix4 camera{},mountWorld{};
};
class TraversalControls {
public:
 void Update(const TraversalSample&,const shared::SharedControllerSample&,const stereo::Pose& head,ControllerCommand&) noexcept;
 void Reset() noexcept {*this={};}
private:
 std::uint64_t owner=0,mount=0;std::int64_t last=0,fallSince=0,deploySince=0;
 int hand=-1;bool climbing=false;float previousHandY=0,previousBodyY=0,debt=0;
};
}
