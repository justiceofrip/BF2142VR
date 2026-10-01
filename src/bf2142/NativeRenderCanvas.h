#pragma once
#include "StereoSession.h"
#include <d3d9.h>
namespace bfvr::bf2142 {
bool InstallRenderCanvas(LogFunction logger);
void ConfigureRenderCanvas(D3DPRESENT_PARAMETERS* parameters,HWND window=nullptr);
void ConfirmRenderCanvas(IDirect3DDevice9* device);
}
