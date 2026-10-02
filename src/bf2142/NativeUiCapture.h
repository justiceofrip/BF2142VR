#pragma once
#include "StereoSession.h"
#include "FrameCapture.h"
#include <wrl/client.h>
namespace bfvr::bf2142 {
class NativeUiCapture {
public:
    bool Connect(IDirect3DDevice9* device, LogFunction log);
    bool Begin(IDirect3DDevice9* device,bool clear=true);
    bool Finish(DXGI_FORMAT format,std::vector<DWORD>& pixels);
    bool Detach();
    bool Read(DXGI_FORMAT format,std::vector<DWORD>& pixels);
    bool CompositeDesktop();
    bool BeginOptic(IDirect3DDevice9*);
    bool EndOptic();
    bool ReadOptic(DXGI_FORMAT,std::vector<DWORD>&);
    IDirect3DTexture9* ResolveTexture(bool optic=false); // borrowed, GPU-only MSAA resolve

    IDirect3DSurface9* Surface() const { return active || opticActive ? nullptr : surface.Get(); } // borrowed
    bool Active() const { return active; }
    void Reset();
private:
    using SetTarget=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,DWORD,IDirect3DSurface9*);
    using SetState=HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,D3DRENDERSTATETYPE,DWORD);
    static HRESULT STDMETHODCALLTYPE TargetHook(IDirect3DDevice9*,DWORD,IDirect3DSurface9*);
    static HRESULT STDMETHODCALLTYPE StateHook(IDirect3DDevice9*,D3DRENDERSTATETYPE,DWORD);
    inline static NativeUiCapture* instance=nullptr;
    SetTarget originalTarget=nullptr;
    SetState originalState=nullptr;
    IDirect3DDevice9* device=nullptr;
    LogFunction logger=nullptr;
    bool active=false, connected=false, failureLogged=false;
    D3DMULTISAMPLE_TYPE samples=D3DMULTISAMPLE_NONE;
    DWORD sampleQuality=0;
    bool Failure(const char* step,HRESULT result);
    UINT width=0,height=0;
    DWORD alphaStates[5]{};
    Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;
    Microsoft::WRL::ComPtr<IDirect3DSurface9> surface,backbuffer;
    Microsoft::WRL::ComPtr<IDirect3DTexture9> opticTexture;
    Microsoft::WRL::ComPtr<IDirect3DSurface9> opticSurface;
    bool opticActive=false;
    FrameCapture capture;
};
}
