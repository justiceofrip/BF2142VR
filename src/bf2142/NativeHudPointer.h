#pragma once
#include "StereoSession.h"
namespace bfvr::bf2142 {
bool InstallNativeHudPointer(LogFunction);
// Only called with an accepted, focused controller ray. Normalized UI content coordinates.
void PublishNativeHudPointer(bool valid,float x=0,float y=0,bool held=false);
}
