#include "FrameCapture.h"
#include "FramePixels.h"
#include <cstring>
#include <wrl/client.h>
namespace bfvr::bf2142 {
void FrameCapture::Reset() {
    if (staging) staging->Release();
    if (resolved) resolved->Release();
    staging=nullptr; resolved=nullptr; width=height=0; sourceFormat=D3DFMT_UNKNOWN;
}
HRESULT FrameCapture::Read(IDirect3DDevice9* device, DXGI_FORMAT format, std::vector<DWORD>& pixels) {
    if (!device) return E_POINTER;
    Microsoft::WRL::ComPtr<IDirect3DSurface9> source;
    HRESULT hr=device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&source);
    if (FAILED(hr)) return hr;
    hr=ReadSurface(device,source.Get(),format,pixels);
    return hr;
}
HRESULT FrameCapture::ReadSurface(IDirect3DDevice9* device, IDirect3DSurface9* source,
    DXGI_FORMAT format, std::vector<DWORD>& pixels, bool preserveAlpha) {
    if (!device || !source) return E_POINTER;
    HRESULT hr=S_OK;
    D3DSURFACE_DESC desc{}; hr=source->GetDesc(&desc);
    if (FAILED(hr)) { return hr; }
    if (desc.Format!=D3DFMT_X8R8G8B8 && desc.Format!=D3DFMT_A8R8G8B8) {
        return D3DERR_NOTAVAILABLE;
    }
    const bool rgba=format==DXGI_FORMAT_R8G8B8A8_UNORM || format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    const bool bgra=format==DXGI_FORMAT_B8G8R8A8_UNORM || format==DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
    if ((!rgba && !bgra) || !desc.Width || !desc.Height || desc.Width>8192 || desc.Height>8192) {
        return D3DERR_NOTAVAILABLE;
    }
    if (width!=desc.Width || height!=desc.Height || sourceFormat!=desc.Format) Reset();
    if (!staging) {
        hr=device->CreateOffscreenPlainSurface(desc.Width,desc.Height,desc.Format,D3DPOOL_SYSTEMMEM,&staging,nullptr);
        if (FAILED(hr)) { return hr; }
        width=desc.Width; height=desc.Height; sourceFormat=desc.Format;
    }
    IDirect3DSurface9* copy=source;
    if (desc.MultiSampleType!=D3DMULTISAMPLE_NONE) {
        if (!resolved) hr=device->CreateRenderTarget(width,height,sourceFormat,D3DMULTISAMPLE_NONE,0,FALSE,&resolved,nullptr);
        if (SUCCEEDED(hr)) hr=device->StretchRect(source,nullptr,resolved,nullptr,D3DTEXF_NONE);
        copy=resolved;
    }
    if (SUCCEEDED(hr)) hr=device->GetRenderTargetData(copy,staging);
    if (FAILED(hr)) return hr;
    pixels.resize(static_cast<size_t>(width)*height);
    D3DLOCKED_RECT locked{}; hr=staging->LockRect(&locked,nullptr,D3DLOCK_READONLY);
    if (FAILED(hr)) return hr;
    for (UINT y=0;y<height;++y) {
        const auto* row=reinterpret_cast<const DWORD*>(static_cast<const BYTE*>(locked.pBits)+y*locked.Pitch);
        CopyFramePixels(pixels.data()+size_t(y)*width,row,width,rgba,!(preserveAlpha&&sourceFormat==D3DFMT_A8R8G8B8));
    }
    return staging->UnlockRect();
}
}
