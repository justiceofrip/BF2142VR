#pragma once
#include "VehicleControls.h"
namespace bfvr::bf2142 {
bool InstallNativeVehicle(LogFunction);
bool ReadNativeVehicle(VehicleSample*);
void UpdateNativeVehicle(const VehicleSample*,const shared::SharedControllerSample*,const stereo::Pose& reference,
 const stereo::Pose& head,float scale,float height,ControllerCommand&,bool horizonLock=true);
bool ReadNativeVehicleCamera(const stereo::Matrix4& native,stereo::Matrix4* stable);
void RecenterNativeVehicle();
std::optional<stereo::Pose> ReadVehicleHudAnchor(const stereo::Pose&,float scale);
}
