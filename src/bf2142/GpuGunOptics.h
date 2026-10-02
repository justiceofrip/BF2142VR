#pragma once
#include "GunOptics.h"
#include <d3d9.h>
#include <wrl/client.h>
namespace bfvr::bf2142 {
// No CPU eye readback/upload: compose directly into already staged GPU eyes.
class GpuGunOptics {
public:
 bool Prepare(IDirect3DDevice9*,unsigned width,unsigned height);
 bool CaptureScope();
 bool Draw(IDirect3DSurface9*,const GunOptic&,const EyeCamera&,const OpticView&,
           unsigned eye,IDirect3DTexture9* hud=nullptr,IDirect3DTexture9* baseline=nullptr);
 void Reset();
private:
 IDirect3DDevice9* device=nullptr;
 unsigned width=0,height=0;
 bool shaderFailed=false;
 Microsoft::WRL::ComPtr<IDirect3DPixelShader9> shader;
 Microsoft::WRL::ComPtr<IDirect3DTexture9> scope;
 Microsoft::WRL::ComPtr<IDirect3DSurface9> scopeSurface;
};
}
