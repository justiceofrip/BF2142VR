#pragma once
#include <d3d9.h>
#include <dxgiformat.h>
#include <vector>
namespace bfvr::bf2142 {
class FrameCapture {
public:
    ~FrameCapture() { Reset(); }
    void Reset();
    HRESULT Read(IDirect3DDevice9* device, DXGI_FORMAT format, std::vector<DWORD>& pixels);
    HRESULT ReadSurface(IDirect3DDevice9* device, IDirect3DSurface9* source,
        DXGI_FORMAT format, std::vector<DWORD>& pixels, bool preserveAlpha = false);
    UINT Width() const { return width; }
    UINT Height() const { return height; }
private:
    IDirect3DSurface9* staging = nullptr;
    IDirect3DSurface9* resolved = nullptr;
    UINT width=0, height=0;
    D3DFORMAT sourceFormat=D3DFMT_UNKNOWN;
};
}
