#include "WristMenu.h"
#include "TrackingMath.h"
#include <cmath>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
stereo::Vec3 Transform(const stereo::Pose& p,stereo::Vec3 v){const auto q=p.orientation;const stereo::Vec3 t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};return {p.position.x+v.x+q.w*t.x+q.y*t.z-q.z*t.y,p.position.y+v.y+q.w*t.y+q.z*t.x-q.x*t.z,p.position.z+v.z+q.w*t.z+q.x*t.y-q.y*t.x};}
}
bool WristMenuGpu::Draw(IDirect3DDevice9* d,const WristFrame& f,const shared::SharedPresentationView& eye,const stereo::Matrix4& projection,float scale){
 if(!d||!f.visible||!std::isfinite(scale)||scale<=0)return false;
 const auto& p=eye.pose;const stereo::Pose eyePose{{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}};
 const auto a=stereo::MakeRelativePose(eyePose,f.panel);if(!a)return false;
 if(device!=d){Reset();device=d;}
 if(!texture){
  constexpr int w=512,h=192;BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
  HDC dc=CreateCompatibleDC(nullptr);if(!dc)return false;void* bits=nullptr;auto bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!bitmap){DeleteDC(dc);return false;}
  auto old=SelectObject(dc,bitmap);auto brush=CreateSolidBrush(RGB(17,45,59));RECT rect{0,0,w,h};FillRect(dc,&rect,brush);DeleteObject(brush);
  auto font=CreateFontW(-68,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");auto oldFont=SelectObject(dc,font);
  SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(217,246,252));DrawTextW(dc,L"DEPLOY",-1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE);
  bool ready=SUCCEEDED(d->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&texture,nullptr));D3DLOCKED_RECT lock{};
  if(ready){ready=SUCCEEDED(texture->LockRect(0,&lock,nullptr,0));if(ready){for(int y=0;y<h;++y){auto* dst=reinterpret_cast<DWORD*>(static_cast<BYTE*>(lock.pBits)+y*lock.Pitch);auto* src=static_cast<DWORD*>(bits)+y*w;for(int x=0;x<w;++x)dst[x]=src[x]|0xff000000;}ready=SUCCEEDED(texture->UnlockRect(0));}}
  SelectObject(dc,oldFont);DeleteObject(font);SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);if(!ready){texture.Reset();return false;}
 }
 Microsoft::WRL::ComPtr<IDirect3DStateBlock9> state;if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&state)))return false;
 D3DMATRIX id{};id._11=id._22=id._33=id._44=1;bool ok=true;const auto set=[&](HRESULT hr){ok=SUCCEEDED(hr)&&ok;};
 set(d->SetVertexShader(nullptr));set(d->SetPixelShader(nullptr));set(d->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1));
 set(d->SetTransform(D3DTS_WORLD,&id));set(d->SetTransform(D3DTS_VIEW,&id));set(d->SetTransform(D3DTS_PROJECTION,reinterpret_cast<const D3DMATRIX*>(&projection)));
 set(d->SetStreamSourceFreq(0,1));set(d->SetStreamSourceFreq(1,1));set(d->SetRenderState(D3DRS_VERTEXBLEND,D3DVBF_DISABLE));set(d->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE,FALSE));
 for(auto s:{D3DRS_LIGHTING,D3DRS_FOGENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_CLIPPLANEENABLE,D3DRS_SRGBWRITEENABLE,D3DRS_ZENABLE,D3DRS_ZWRITEENABLE})set(d->SetRenderState(s,FALSE));
 set(d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE));set(d->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS,TRUE));set(d->SetRenderState(D3DRS_COLORWRITEENABLE,15));set(d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID));
 set(d->SetRenderState(D3DRS_TEXTUREFACTOR,f.pressed?0xffffca80:f.hovered?0xff70dfff:0xffffffff));
 set(d->SetTexture(0,texture.Get()));set(d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE));set(d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE));set(d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_TFACTOR));
 set(d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1));set(d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TFACTOR));set(d->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0));set(d->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE));set(d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE));
 set(d->SetSamplerState(0,D3DSAMP_MAXMIPLEVEL,0));set(d->SetSamplerState(0,D3DSAMP_MIPMAPLODBIAS,0));
 set(d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR));set(d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR));set(d->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_NONE));set(d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE));set(d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP));set(d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP));
 struct V {float x,y,z,u,v;};V quad[4]{};unsigned n=0;for(float y:{wristHeight*.5f,-wristHeight*.5f})for(float x:{-wristWidth*.5f,wristWidth*.5f}){auto v=Transform(*a,{x,y,0});quad[n++]={v.x*scale,v.y*scale,-v.z*scale,x/wristWidth+.5f,.5f-y/wristHeight};}
 if(ok)set(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,quad,sizeof(V)));
 if(ok&&f.hovered){
  V beam[2]{};unsigned i=0;for(auto point:{f.rayStart,f.rayEnd}){const auto local=stereo::MakeRelativePose(eyePose,{point,{0,0,0,1}});if(!local){ok=false;break;}const auto v=local->position;beam[i++]={v.x*scale,v.y*scale,-v.z*scale,0,0};}
  set(d->SetTexture(0,nullptr));set(d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG2));if(ok)set(d->DrawPrimitiveUP(D3DPT_LINELIST,1,beam,sizeof(V)));
 }
 const bool restored=SUCCEEDED(state->Apply());return ok&&restored;
}
}
