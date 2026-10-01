#pragma once
#include <d3d9.h>
namespace bfvr::bf2142 {
using ExResourceLog=void(*)(const char*,...);
bool InstallNativeExResources(IDirect3DDevice9*,ExResourceLog);
}
