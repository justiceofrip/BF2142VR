#include "MenuOverlayGpu.h"
#include <d3dcompiler.h>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
using Microsoft::WRL::ComPtr;
constexpr char program[]=R"(
sampler2D artwork:register(s0);
float4 geometry:register(c0);
float4 style:register(c1);
float4 color:register(c2);
float4 main(float2 uv:TEXCOORD0):COLOR0 {
 if(style.x<.5)return tex2D(artwork,uv);
 float2 p=floor(uv*style.zw);
 if(style.x<1.5){
  float2 delta=geometry.zw-geometry.xy;
  float t=saturate(dot(p-geometry.xy,delta)/max(dot(delta,delta),.0001));
  clip(1.5-length(p-(geometry.xy+t*delta)));return color;
 }
 float2 offset=p-geometry.xy;float distance2=dot(offset,offset);
 // Pixel centers and radii are integral; tolerate sub-unit shader rounding at the border.
 clip((style.y+2)*(style.y+2)+.25-distance2);
 float center=max(1,floor(style.y/3));if(distance2<=center*center+.25)return 1;
 return distance2<=style.y*style.y+.25?color:float4(17./255,17./255,17./255,1);
})";
struct Saved {
 IDirect3DDevice9* d;ComPtr<IDirect3DStateBlock9> state;ComPtr<IDirect3DSurface9> targets[4],depth;
 unsigned count=0;bool scene=false;
 explicit Saved(IDirect3DDevice9* p):d(p){}
 bool Save(){
  D3DCAPS9 caps{};if(FAILED(d->GetDeviceCaps(&caps)))return false;count=std::min(4u,unsigned(caps.NumSimultaneousRTs));
  if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&state)))return false;
  for(unsigned i=0;i<count;++i)d->GetRenderTarget(i,&targets[i]);d->GetDepthStencilSurface(&depth);
  return bool(targets[0]);
 }
 bool Restore(){
  if(!state)return true;bool ok=!scene||SUCCEEDED(d->EndScene());scene=false;
  d->SetDepthStencilSurface(nullptr);for(unsigned i=0;i<count;++i)ok=SUCCEEDED(d->SetRenderTarget(i,targets[i].Get()))&&ok;
  ok=SUCCEEDED(d->SetDepthStencilSurface(depth.Get()))&&ok;ok=SUCCEEDED(state->Apply())&&ok;state.Reset();return ok;
 }
 ~Saved(){Restore();}
};
}
bool MenuOverlayGpu::Prepare(IDirect3DDevice9* d,UINT w,UINT h){
 if(!d||!w||!h||w>8192||h>8192)return false;
 if(device!=d||width!=w||height!=h){Reset();device=d;width=w;height=h;}
 if(failed)return false;
 if(!shader){
  ComPtr<ID3DBlob> code,error;
  if(FAILED(D3DCompile(program,sizeof(program)-1,"BF2142 menu overlays",nullptr,nullptr,"main","ps_3_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error))||
     FAILED(d->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()),&shader))){failed=true;return false;}
 }
 return true;
}
bool MenuOverlayGpu::Draw(const std::array<IDirect3DSurface9*,3>& targets,const MenuPointerVisual& pointer,
 const shared::SharedRenderRequest& request,const std::vector<DWORD>& art,UINT64 revision){
 if(!device||!shader||!targets[0]||!targets[1]||!targets[2]||art.size()!=size_t(width)*height)return false;
 if(!artwork){
  if(FAILED(device->CreateTexture(width,height,1,0,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&upload,nullptr))||
     FAILED(device->CreateTexture(width,height,1,0,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&artwork,nullptr))){upload.Reset();artwork.Reset();return false;}
  artRevision=~revision;
 }
 if(artRevision!=revision){
  D3DLOCKED_RECT mapped{};if(FAILED(upload->LockRect(0,&mapped,nullptr,0)))return false;
  for(UINT y=0;y<height;++y)std::memcpy(static_cast<BYTE*>(mapped.pBits)+size_t(y)*mapped.Pitch,art.data()+size_t(y)*width,width*4);
  if(FAILED(upload->UnlockRect(0))||FAILED(device->UpdateTexture(upload.Get(),artwork.Get())))return false;
  artRevision=revision;
 }
 Saved saved(device);if(!saved.Save())return false;
 bool ok=true;const auto set=[&](HRESULT hr){ok=SUCCEEDED(hr)&&ok;};
 set(device->SetDepthStencilSurface(nullptr));for(unsigned i=1;i<saved.count;++i)set(device->SetRenderTarget(i,nullptr));
 set(device->SetVertexShader(nullptr));set(device->SetPixelShader(shader.Get()));set(device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1));
 for(auto s:{D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_FOGENABLE,D3DRS_LIGHTING,
            D3DRS_SRGBWRITEENABLE,D3DRS_CLIPPLANEENABLE})set(device->SetRenderState(s,FALSE));
 set(device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE));set(device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID));
 set(device->SetRenderState(D3DRS_COLORWRITEENABLE,15));set(device->SetRenderState(D3DRS_MULTISAMPLEMASK,0xffffffff));
 set(device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE));set(device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE));
 set(device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA));set(device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD));
 set(device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,TRUE));set(device->SetRenderState(D3DRS_SRCBLENDALPHA,D3DBLEND_ONE));
 set(device->SetRenderState(D3DRS_DESTBLENDALPHA,D3DBLEND_INVSRCALPHA));set(device->SetRenderState(D3DRS_BLENDOPALPHA,D3DBLENDOP_ADD));
 set(device->SetStreamSourceFreq(0,1));set(device->SetStreamSourceFreq(1,1));
 set(device->SetTexture(0,artwork.Get()));
 for(auto s:{D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER})set(device->SetSamplerState(0,s,D3DTEXF_POINT));
 set(device->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE));set(device->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE));
 set(device->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP));set(device->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP));
 const float color[]={pointer.pressed?1.f:32.f/255,pointer.pressed?184.f/255:239.f/255,pointer.pressed?48.f/255:1.f,1};
 set(device->SetPixelShaderConstantF(2,color,1));if(!ok)return false;
 if(FAILED(device->BeginScene()))return false;saved.scene=true;
 const auto draw=[&](unsigned index,float mode,const float* geometry,float radius,const RECT& box){
  set(device->SetRenderTarget(0,targets[index]));D3DVIEWPORT9 vp{0,0,width,height,0,1};set(device->SetViewport(&vp));
  set(device->SetScissorRect(&box));set(device->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE));
  const float style[]={mode,radius,float(width),float(height)};set(device->SetPixelShaderConstantF(0,geometry,1));set(device->SetPixelShaderConstantF(1,style,1));
  struct V{float x,y,z,w,u,v;};const float w=float(width)-.5f,h=float(height)-.5f;
  const V vertices[]={{-.5f,-.5f,0,1,0,0},{w,-.5f,0,1,1,0},{-.5f,h,0,1,0,1},{w,h,0,1,1,1}};
  if(ok)set(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V)));
 };
 const float empty[4]{};draw(2,0,empty,0,{0,0,LONG(width),LONG(height)});
 if(pointer.visible){
  if(pointer.ray)for(unsigned e=0;e<2;++e)if(const auto line=ProjectMenuBeam(width,height,pointer.anchor,*pointer.ray,request.views[e])){
   const float endpoints[]={line->x0,line->y0,line->x1,line->y1};
   const RECT box{LONG(std::max(0.f,std::floor(std::min(line->x0,line->x1)-2))),LONG(std::max(0.f,std::floor(std::min(line->y0,line->y1)-2))),
    LONG(std::min(float(width),std::ceil(std::max(line->x0,line->x1)+3))),LONG(std::min(float(height),std::ceil(std::max(line->y0,line->y1)+3)))};
   draw(e,1,endpoints,0,box);
  }
  const float x=std::floor(pointer.point.pixelX),y=std::floor(pointer.point.pixelY),radius=float(std::max(4u,width/180));
  if(std::isfinite(x)&&std::isfinite(y)&&x>=0&&y>=0&&x<=width&&y<=height){
   const float center[]={x,y,0,0};const RECT box{LONG(std::max(0.f,x-radius-2)),LONG(std::max(0.f,y-radius-2)),
    LONG(std::min(float(width),x+radius+3)),LONG(std::min(float(height),y+radius+3))};draw(2,2,center,radius,box);
  }
 }
 return saved.Restore()&&ok;
}
void MenuOverlayGpu::Reset(){shader.Reset();artwork.Reset();upload.Reset();device=nullptr;width=height=0;artRevision=0;failed=false;}
}
