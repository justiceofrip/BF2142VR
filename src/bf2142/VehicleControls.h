#pragma once
#include "ControllerPolicy.h"
namespace bfvr::bf2142 {
enum class VehicleRole {Unknown,Driver,Passenger,Aimed,PitchAimed,Pilot};
VehicleRole StockVehicleRole(const char* name) noexcept;
struct VehicleSample {std::uint64_t owner=0,seat=0,root=0;VehicleRole role=VehicleRole::Unknown;stereo::Matrix4 rootWorld{};};
class VehicleView {
public:
 std::optional<stereo::Matrix4> Update(const VehicleSample&,const stereo::Matrix4& camera) noexcept;
 void Reset() noexcept {*this={};}
private:
 std::uint64_t owner=0,seat=0,root=0;float yawOffset=0;stereo::Matrix4 localCamera{};
};
class VehicleControls {
public:
 void Update(const VehicleSample&,const shared::SharedControllerSample&,const stereo::Matrix4* nativeCamera,
             const stereo::Matrix4* targetCamera,ControllerCommand&) noexcept;
 void Reset() noexcept {*this={};}
private:
 std::uint64_t owner=0,root=0,seat=0;std::int64_t last=0;bool driverHeld=false,nextHeld=false;int nextSeat=2;
 float remainderX=0,remainderY=0;
};
}
