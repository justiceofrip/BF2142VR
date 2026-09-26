#include "ShoulderRadioGpu.h"
#include <wrl/client.h>
#include <array>
#include <cmath>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
struct Vertex {float x,y,z;DWORD color;};
using V=stereo::Vec3;
V Transform(const stereo::Pose& p,V v){const auto q=p.orientation;const V t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
 return {p.position.x+v.x+q.w*t.x+q.y*t.z-q.z*t.y,p.position.y+v.y+q.w*t.y+q.z*t.x-q.x*t.z,p.position.z+v.z+q.w*t.z+q.x*t.y-q.y*t.x};}
}
bool DrawShoulderRadio(IDirect3DDevice9* d,const RadioFrame& f,const shared::SharedPresentationView& eye,const stereo::Matrix4& projection,float scale) noexcept {
 if(!d||!f.visible||!std::isfinite(scale)||scale<=0)return false;
 const auto& p=eye.pose;const auto a=stereo::MakeRelativePose({{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}},f.anchor);
 if(!a)return false;for(const auto& row:projection.values)for(float v:row)if(!std::isfinite(v))return false;
 Microsoft::WRL::ComPtr<IDirect3DSurface9> depth;if(FAILED(d->GetDepthStencilSurface(&depth))||!depth)return false;
 Microsoft::WRL::ComPtr<IDirect3DStateBlock9> saved;if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&saved)))return false;
 // State blocks do not include render targets. This draw never changes them.
 std::array<Vertex,648> mesh{};unsigned count=0;
 const auto box=[&](V low,V high,DWORD color){
  const V points[]={{low.x,low.y,low.z},{high.x,low.y,low.z},{high.x,high.y,low.z},{low.x,high.y,low.z},
   {low.x,low.y,high.z},{high.x,low.y,high.z},{high.x,high.y,high.z},{low.x,high.y,high.z}};
  constexpr unsigned faces[6][6]={{0,1,2,0,2,3},{5,4,7,5,7,6},{4,0,3,4,3,7},{1,5,6,1,6,2},{3,2,6,3,6,7},{4,5,1,4,1,0}};
  for(unsigned face=0;face<6;++face){const float shade=face==0?1.f:face==4?.9f:.66f;
   const DWORD c=0xff000000|(DWORD(((color>>16)&255)*shade)<<16)|(DWORD(((color>>8)&255)*shade)<<8)|DWORD((color&255)*shade);
   for(unsigned index:faces[face]){auto v=points[index];v={v.x+kRadioPosition.x,v.y+kRadioPosition.y,v.z+kRadioPosition.z};v=Transform(*a,v);
    mesh[count++]={v.x*scale,v.y*scale,-v.z*scale,c};}
  }
 };
 box({-.032f,-.052f,-.018f},{.032f,.045f,.018f},0xff354452); // casing
 box({-.026f,-.043f,-.021f},{.026f,.034f,-.018f},0xff18212b); // rubber face
 for(int i=0;i<5;++i){const float y=-.029f+float(i)*.008f;box({-.020f,y,-.023f},{.020f,y+.003f,-.021f},0xff77838c);}
 box({.018f,.045f,-.002f},{.024f,.115f,.004f},0xff222a31); // antenna
 box({-.027f,.044f-.004f*f.depression,-.014f},{-.005f,.052f-.004f*f.depression,.005f},f.pressed?0xff79cc9a:0xff657987); // travel switch
 box({.009f,.020f,-.023f},{.020f,.029f,-.021f},f.pressed?0xff30e898:f.hovered?0xff60b9dd:0xff334b54); // TX/hover LED
 box({.006f,.045f,-.011f},{.017f,.047f,.002f},f.pressed?0xff30e898:0xff334b54); // top indicator visible when worn
 D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;D3DMATRIX proj{};std::memcpy(&proj,&projection,sizeof(proj));
 bool ok=true;const auto set=[&](HRESULT hr){ok=SUCCEEDED(hr)&&ok;};
 set(d->SetVertexShader(nullptr));set(d->SetPixelShader(nullptr));set(d->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE));
 set(d->SetTransform(D3DTS_WORLD,&identity));set(d->SetTransform(D3DTS_VIEW,&identity));set(d->SetTransform(D3DTS_PROJECTION,&proj));
 set(d->SetRenderState(D3DRS_LIGHTING,FALSE));set(d->SetRenderState(D3DRS_FOGENABLE,FALSE));set(d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE));
 set(d->SetRenderState(D3DRS_ZENABLE,D3DZB_TRUE));set(d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE));set(d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL));
 set(d->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE));set(d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE));set(d->SetRenderState(D3DRS_STENCILENABLE,FALSE));
 set(d->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE));set(d->SetRenderState(D3DRS_CLIPPLANEENABLE,0));set(d->SetRenderState(D3DRS_COLORWRITEENABLE,15));
 set(d->SetRenderState(D3DRS_DEPTHBIAS,0));set(d->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,0));set(d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID));
 set(d->SetTexture(0,nullptr));set(d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1));set(d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE));
 set(d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1));set(d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE));set(d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE));
 if(ok)set(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,count/3,mesh.data(),sizeof(Vertex)));
 const bool restored=SUCCEEDED(saved->Apply());return ok&&restored;
}
}
