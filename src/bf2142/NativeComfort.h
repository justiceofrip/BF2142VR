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
void ConfigurePhysicalCamera(bool enabled);
bool NativeSnapTurnAvailable();
bool RequestNativeSnapTurn(float degrees,std::int64_t sampleTime);
void ClearNativeSnapTurn();
bool ReadNativeTraversal(TraversalSample*);
}
