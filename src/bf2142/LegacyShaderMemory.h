#pragma once
#include <windows.h>
namespace bfvr::bf2142 {
// d3dx9_29 tags the high address bit in compiler/effect pointers. Its heap
// and VirtualAlloc arenas must remain below 2 GiB in a large-address-aware host.
// Installed before the first game effect; retained for the runtime's lifetime.
using ShaderMemoryLog = void (*)(const char*, ...);
bool InstallLegacyShaderMemory(HMODULE module, ShaderMemoryLog log);
struct ShaderMemoryStats { size_t reserved, active, peak, allocations, failures; };
ShaderMemoryStats GetLegacyShaderMemoryStats();
}
