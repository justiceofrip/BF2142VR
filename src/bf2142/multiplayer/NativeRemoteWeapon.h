#pragma once
#include "../NativeHands.h"
namespace bfvr::bf2142 {
bool InstallRemoteWeaponVisibility(BYTE* renderer,LogFunction);
bool HideRemoteWeaponGeometry(void* geometry);
}
