#pragma once
#include "StereoSession.h"
#include <cstdint>
#include "TraversalControls.h"
namespace bfvr::bf2142 {
// Profiled infantry camera source and local look-input/recoil bridge.
bool InstallNativeComfort(LogFunction logger);
bool ReadNativeComfortCamera(const stereo::Matrix4& nativeWorld,stereo::Matrix4* stableWorld,
    const void* expectedSoldier=nullptr);
bool ReadNativeStance(int* stance);
// Returns true only when the movement-only native adapter owns this infantry.
bool ConfigureNativeMovement(bool enabled,float trackedYawRadians);
// Match the hidden stock infantry pitch without rotating the comfort camera.
bool ConfigureNativeLookPitch(bool enabled,const stereo::Pose& head);
bool ReadNativeMovementYaw(const void* soldier,float* offsetDegrees);
void ConfigurePhysicalCamera(bool enabled);
bool NativeSnapTurnAvailable();
bool RequestNativeSnapTurn(float degrees,std::int64_t sampleTime);
void ClearNativeSnapTurn();
bool ReadNativeTraversal(TraversalSample*);
}
