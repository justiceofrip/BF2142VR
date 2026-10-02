#pragma once
#include "MenuPointer.h"
#include <array>
#include <d3d9.h>
#include <wrl/client.h>
namespace bfvr::bf2142 {
class MenuOverlayGpu {
public:
 bool Prepare(IDirect3DDevice9*,UINT width,UINT height);
 bool Draw(const std::array<IDirect3DSurface9*,3>&,const MenuPointerVisual&,
           const shared::SharedRenderRequest&,const std::vector<DWORD>& art,UINT64 revision);
 void Reset();
private:
 IDirect3DDevice9* device=nullptr;UINT width=0,height=0;UINT64 artRevision=0;
 bool failed=false;
 Microsoft::WRL::ComPtr<IDirect3DPixelShader9> shader;
 Microsoft::WRL::ComPtr<IDirect3DTexture9> artwork,upload;
};
}
