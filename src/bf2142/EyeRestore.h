#pragma once
#include <d3d9.h>
#include <dxgiformat.h>
#include <wrl/client.h>
#include <vector>
namespace bfvr::bf2142 {
// Restore a normal eye after the offscreen-only scope replay. Never present
// the scope camera as the game's desktop frame or create another window.
class EyeRestore {
public:
    bool Draw(IDirect3DDevice9*,const std::vector<DWORD>&,UINT,UINT,DXGI_FORMAT);
    bool DrawTexture(IDirect3DDevice9*,IDirect3DTexture9*,bool blend=false);
    HRESULT LastResult() const { return lastResult; }
    const char* LastStage() const { return lastStage; }
    void Reset(){texture.Reset();width=height=0;}
private:
    HRESULT lastResult=S_OK;
    const char* lastStage="none";
    bool Rejected(HRESULT hr,const char* stage){lastResult=hr;lastStage=stage;return FAILED(hr);}
    Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
    UINT width=0,height=0;
};
}
