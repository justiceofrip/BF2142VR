#include "EyeRestore.h"
namespace bfvr::bf2142 {
bool EyeRestore::Draw(IDirect3DDevice9* device,const std::vector<DWORD>& pixels,UINT w,UINT h,DXGI_FORMAT format){
    if(!device || !w || !h || pixels.size()!=size_t(w)*h)return false;
    if(width!=w || height!=h)Reset();
    if(!texture){
        if(FAILED(device->CreateTexture(w,h,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture,nullptr)))return false;
        width=w;height=h;
    }
    D3DLOCKED_RECT row{};if(FAILED(texture->LockRect(0,&row,nullptr,D3DLOCK_DISCARD)))return false;
    const bool rgba=format==DXGI_FORMAT_R8G8B8A8_UNORM||format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    for(UINT y=0;y<h;++y){auto* dest=reinterpret_cast<DWORD*>(static_cast<BYTE*>(row.pBits)+y*row.Pitch);
        for(UINT x=0;x<w;++x){const DWORD p=pixels[size_t(y)*w+x];dest[x]=rgba?(p&0xff00ff00)|((p&255)<<16)|((p>>16)&255):p;}}
    if(FAILED(texture->UnlockRect(0)))return false;
    Microsoft::WRL::ComPtr<IDirect3DStateBlock9> state;
    if (FAILED(device->CreateStateBlock(D3DSBT_ALL,&state))) return false;
    Microsoft::WRL::ComPtr<IDirect3DSurface9> previous,back,depth;
    if(FAILED(device->GetRenderTarget(0,&previous)) || FAILED(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)))return false;
    device->GetDepthStencilSurface(&depth);
    if(FAILED(device->SetRenderTarget(0,back.Get())))return false;
    device->SetDepthStencilSurface(nullptr);
    if (FAILED(device->BeginScene())) { device->SetRenderTarget(0,previous.Get());device->SetDepthStencilSurface(depth.Get());state->Apply();return false; }
    device->SetVertexShader(nullptr); device->SetPixelShader(nullptr);
    device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
    device->SetTexture(0,texture.Get());
    device->SetRenderState(D3DRS_FOGENABLE,FALSE); device->SetRenderState(D3DRS_LIGHTING,FALSE);
    device->SetRenderState(D3DRS_ZENABLE,FALSE); device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE); device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
    device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE); device->SetRenderState(D3DRS_STENCILENABLE,FALSE);
    device->SetRenderState(D3DRS_COLORWRITEENABLE,15); device->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);
    device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE); device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
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
    const float rightEdge=static_cast<float>(width)-.5f,bottomEdge=static_cast<float>(height)-.5f;
    const Vertex vertices[]={{-.5f,-.5f,0,1,0,0},{rightEdge,-.5f,0,1,1,0},{-.5f,bottomEdge,0,1,0,1},{rightEdge,bottomEdge,0,1,1,1}};
    const HRESULT draw=device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex));
    const HRESULT end=device->EndScene();
    const HRESULT targetRestore=device->SetRenderTarget(0,previous.Get());
    const HRESULT depthRestore=device->SetDepthStencilSurface(depth.Get());
    const HRESULT restore=state->Apply();
    return SUCCEEDED(draw) && SUCCEEDED(end) && SUCCEEDED(restore) && SUCCEEDED(targetRestore) && SUCCEEDED(depthRestore);
}
}
