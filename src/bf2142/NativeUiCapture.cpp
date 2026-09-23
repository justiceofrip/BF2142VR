#include "NativeUiCapture.h"
#include <MinHook.h>
namespace bfvr::bf2142 {
namespace {
constexpr D3DRENDERSTATETYPE alphaTypes[]={D3DRS_SEPARATEALPHABLENDENABLE,D3DRS_SRCBLENDALPHA,D3DRS_DESTBLENDALPHA,D3DRS_BLENDOPALPHA,D3DRS_COLORWRITEENABLE};
constexpr DWORD alphaValues[]={TRUE,D3DBLEND_ONE,D3DBLEND_INVSRCALPHA,D3DBLENDOP_ADD,15};
}
HRESULT STDMETHODCALLTYPE NativeUiCapture::TargetHook(IDirect3DDevice9* device,DWORD index,IDirect3DSurface9* target) {
    auto& ui=*instance;
    if (ui.active && device==ui.device && index==0 && target==ui.backbuffer.Get()) target=ui.surface.Get();
    return ui.originalTarget(device,index,target);
}
HRESULT STDMETHODCALLTYPE NativeUiCapture::StateHook(IDirect3DDevice9* device,D3DRENDERSTATETYPE type,DWORD value) {
    auto& ui=*instance;
    if (ui.active && device==ui.device) for (int i=0;i<5;++i) if (type==alphaTypes[i]) {
        ui.alphaStates[i]=value; value=i==4?(value|D3DCOLORWRITEENABLE_ALPHA):alphaValues[i]; break;
    }
    return ui.originalState(device,type,value);
}
bool NativeUiCapture::Connect(IDirect3DDevice9* source,LogFunction log) {
    if (connected) return source==device;
    logger=log; device=source; instance=this;
    auto** table=*reinterpret_cast<void***>(source);
    if (MH_CreateHook(table[37],reinterpret_cast<void*>(&TargetHook),reinterpret_cast<void**>(&originalTarget))!=MH_OK) return false;
    if (MH_CreateHook(table[57],reinterpret_cast<void*>(&StateHook),reinterpret_cast<void**>(&originalState))!=MH_OK) {
        MH_RemoveHook(table[37]); return false;
    }
    if (MH_EnableHook(table[37])!=MH_OK || MH_EnableHook(table[57])!=MH_OK) {
        MH_DisableHook(table[37]); MH_DisableHook(table[57]);
        MH_RemoveHook(table[37]); MH_RemoveHook(table[57]); return false;
    }
    connected=true; return true;
}
bool NativeUiCapture::Failure(const char* step,HRESULT result) {
    if(!failureLogged && logger) {failureLogged=true;logger("UI capture failed: %s hr=0x%08lX samples=%u quality=%lu size=%ux%u.",step,static_cast<unsigned long>(result),unsigned(samples),sampleQuality,width,height);}
    backbuffer.Reset();return false;
}
bool NativeUiCapture::Begin(IDirect3DDevice9* source,bool clear) {
    if (!connected || active || source!=device) return false;
    HRESULT hr=device->GetRenderTarget(0,&backbuffer);
    if(FAILED(hr))return Failure("get native render target",hr);
    D3DSURFACE_DESC desc{};hr=backbuffer->GetDesc(&desc);
    if(FAILED(hr))return Failure("describe native target",hr);
    if (!texture || width!=desc.Width || height!=desc.Height || samples!=desc.MultiSampleType || sampleQuality!=desc.MultiSampleQuality) {
        surface.Reset();texture.Reset();capture.Reset();width=desc.Width;height=desc.Height;
        samples=desc.MultiSampleType;sampleQuality=desc.MultiSampleQuality;clear=true;
        hr=device->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,nullptr);
        if(FAILED(hr))return Failure("create alpha texture",hr);
        if(samples==D3DMULTISAMPLE_NONE)hr=texture->GetSurfaceLevel(0,&surface);
        else hr=device->CreateRenderTarget(width,height,D3DFMT_A8R8G8B8,samples,sampleQuality,FALSE,&surface,nullptr);
        if(FAILED(hr)){texture.Reset();return Failure("create matching alpha render target",hr);}
    }
    for (int i=0;i<5;++i) {
        hr=device->GetRenderState(alphaTypes[i],&alphaStates[i]);
        if(FAILED(hr))return Failure("read native alpha state",hr);
    }
    D3DVIEWPORT9 viewport{};hr=device->GetViewport(&viewport);
    if(FAILED(hr))return Failure("read native viewport",hr);
    hr=originalTarget(device,0,surface.Get());
    if(FAILED(hr))return Failure("bind isolated UI target",hr);
    if(clear)hr=device->Clear(0,nullptr,D3DCLEAR_TARGET,0,1,0);
    if(FAILED(hr)) {
        originalTarget(device,0,backbuffer.Get());device->SetViewport(&viewport);
        return Failure("clear isolated UI",hr);
    }
    device->SetViewport(&viewport);
    for (int i=0;i<5;++i) originalState(device,alphaTypes[i],i==4?(alphaStates[i]|D3DCOLORWRITEENABLE_ALPHA):alphaValues[i]);
    active=true;return true;
}
bool NativeUiCapture::Detach() {
    if (!active) return false;
    active=false;
    D3DVIEWPORT9 viewport{};device->GetViewport(&viewport);
    const HRESULT restored=originalTarget(device,0,backbuffer.Get());
    device->SetViewport(&viewport);
    for (int i=0;i<5;++i) originalState(device,alphaTypes[i],alphaStates[i]);
    backbuffer.Reset();
    return SUCCEEDED(restored);
}
bool NativeUiCapture::Read(DXGI_FORMAT format,std::vector<DWORD>& pixels) {
    return surface && SUCCEEDED(capture.ReadSurface(device,surface.Get(),format,pixels,true));
}
bool NativeUiCapture::Finish(DXGI_FORMAT format,std::vector<DWORD>& pixels) {
    if (!Detach()) return false;
    return Read(format,pixels) && CompositeDesktop();
}
bool NativeUiCapture::CompositeDesktop() {
    if(!texture || !surface)return false;
    if(samples!=D3DMULTISAMPLE_NONE) {
        Microsoft::WRL::ComPtr<IDirect3DSurface9> resolved;
        if(FAILED(texture->GetSurfaceLevel(0,&resolved)) || FAILED(device->StretchRect(surface.Get(),nullptr,resolved.Get(),nullptr,D3DTEXF_NONE)))return false;
    }
    Microsoft::WRL::ComPtr<IDirect3DStateBlock9> state;
    if (FAILED(device->CreateStateBlock(D3DSBT_ALL,&state))) return false;
    Microsoft::WRL::ComPtr<IDirect3DSurface9> previous,back,depth;
    if(FAILED(device->GetRenderTarget(0,&previous)) || FAILED(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)))return false;
    device->GetDepthStencilSurface(&depth);
    if(FAILED(originalTarget(device,0,back.Get())))return false;
    device->SetDepthStencilSurface(nullptr);
    if (FAILED(device->BeginScene())) { originalTarget(device,0,previous.Get());device->SetDepthStencilSurface(depth.Get());state->Apply();return false; }
    device->SetVertexShader(nullptr); device->SetPixelShader(nullptr);
    device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
    device->SetTexture(0,texture.Get());
    device->SetRenderState(D3DRS_FOGENABLE,FALSE); device->SetRenderState(D3DRS_LIGHTING,FALSE);
    device->SetRenderState(D3DRS_ZENABLE,FALSE); device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE); device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE); device->SetRenderState(D3DRS_STENCILENABLE,FALSE);
    device->SetRenderState(D3DRS_COLORWRITEENABLE,15); device->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE); device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE); device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1); device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1); device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);
    device->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
    device->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
    device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
    device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
    device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
    device->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);
    device->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT); device->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
    D3DVIEWPORT9 viewport{0,0,width,height,0,1}; device->SetViewport(&viewport);
    struct Vertex { float x,y,z,w,u,v; };
    const float w=static_cast<float>(width)-.5f,h=static_cast<float>(height)-.5f;
    const Vertex vertices[]={{-.5f,-.5f,0,1,0,0},{w,-.5f,0,1,1,0},{-.5f,h,0,1,0,1},{w,h,0,1,1,1}};
    const HRESULT draw=device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex));
    const HRESULT end=device->EndScene();
    const HRESULT targetRestore=originalTarget(device,0,previous.Get());
    const HRESULT depthRestore=device->SetDepthStencilSurface(depth.Get());
    const HRESULT restore=state->Apply();
    return SUCCEEDED(draw) && SUCCEEDED(end) && SUCCEEDED(restore) && SUCCEEDED(targetRestore) && SUCCEEDED(depthRestore);
}
void NativeUiCapture::Reset() {
    if (active && device) Detach();
    capture.Reset(); backbuffer.Reset(); surface.Reset(); texture.Reset(); width=height=0;
    samples=D3DMULTISAMPLE_NONE;sampleQuality=0;failureLogged=false;
}
}
