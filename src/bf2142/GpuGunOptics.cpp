#include "GpuGunOptics.h"
#include <d3dcompiler.h>
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
using Microsoft::WRL::ComPtr;
constexpr char program[]=R"(
sampler2D scene : register(s0);
sampler2D hud : register(s1);
sampler2D baseline : register(s2);
float4 dimensions : register(c0);
float4 projection : register(c1);
float4 eye : register(c2);
float4 axisX : register(c3);
float4 axisY : register(c4);
float4 axisZ : register(c5);
float4 aperture : register(c6);
float4 reticle : register(c7);
float4 modes : register(c8);
float4 main(float2 uv:TEXCOORD0):COLOR0 {
 float3 r=float3((2*uv.x-1-projection.z)/projection.x,
                 (1-2*uv.y-projection.w)/projection.y,1);
 float3 ray=r.x*axisX.xyz+r.y*axisY.xyz+r.z*axisZ.xyz;
 clip(ray.z-.01);
 float2 hit=eye.xy+ray.xy*(eye.w/ray.z);
 float2 a=(hit-aperture.xy)/aperture.zw;
 float2 b=abs(a);float radial=dot(a,a);
 if(modes.y>.5){clip(1-max(b.x,b.y));float2 rounded=max(0,b-.84);clip(.0256-dot(rounded,rounded));}
 else clip(1-radial);
 float2 delta=hit-reticle.xy;
 float2 su=.5+float2(delta.x,-delta.y)/aperture.zw*.5*reticle.w;
 float4 color=0;
 if(modes.z<.5){
   color=tex2D(scene,su*(1-dimensions.xy)+.5*dimensions.xy);color.a=1;
   if(any(su<0)||any(su>1))color=float4(8,11,14,255)/255;
 }
 float center=dot(delta,delta)<reticle.z*reticle.z*2.25;
 float crossLine=(abs(delta.x)<reticle.z&&abs(delta.y)<aperture.w*.45&&abs(delta.y)>reticle.z*3)||
   (abs(delta.y)<reticle.z&&abs(delta.x)<aperture.z*.45&&abs(delta.x)>reticle.z*3);
 if(modes.w>.5){
   float2 hu=.5+(su-.5)*.5;float4 ink=0;
   if(all(hu>=.25)&&all(hu<.75)){
     ink=tex2D(hud,hu);float4 ordinary=tex2D(baseline,hu);
     if(dot(abs(ink-ordinary),float4(255,255,255,255))<8)ink=0;
   }
   color=ink+color*(1-ink.a);
 } else if(center>.5||(modes.z<.5&&axisX.w>=4&&crossLine>.5))color=float4(1,220./255,112./255,1);
 float edge=modes.y>.5?max(b.x,b.y):sqrt(radial);
 return color*(modes.x*saturate((1-edge)*25));
})";
struct State {
 IDirect3DDevice9* d;
 ComPtr<IDirect3DStateBlock9> block;
 ComPtr<IDirect3DSurface9> targets[4],depth;
 unsigned count=1;
 bool started=false;
 explicit State(IDirect3DDevice9* source):d(source){}
 bool Save(){
  D3DCAPS9 caps{};if(FAILED(d->GetDeviceCaps(&caps)))return false;
  count=std::min(4u,unsigned(caps.NumSimultaneousRTs));
  if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&block)))return false;
  for(unsigned i=0;i<count;++i)d->GetRenderTarget(i,&targets[i]);
  d->GetDepthStencilSurface(&depth);return bool(targets[0]);
 }
 bool Restore(){
  if(!block)return true;
  bool ok=!started||SUCCEEDED(d->EndScene());started=false;
  d->SetDepthStencilSurface(nullptr);
  for(unsigned i=0;i<count;++i)ok=SUCCEEDED(d->SetRenderTarget(i,targets[i].Get()))&&ok;
  ok=SUCCEEDED(d->SetDepthStencilSurface(depth.Get()))&&ok;
  ok=SUCCEEDED(block->Apply())&&ok;block.Reset();return ok;
 }
 ~State(){Restore();}
};
}
bool GpuGunOptics::Prepare(IDirect3DDevice9* d,unsigned w,unsigned h){
 if(!d||!w||!h||w>8192||h>8192)return false;
 if(device!=d||width!=w||height!=h)Reset();
 device=d;width=w;height=h;
 if(shaderFailed)return false;
 if(!shader){
  ComPtr<ID3DBlob> code,error;
  if(FAILED(D3DCompile(program,sizeof(program)-1,"BF2142 GPU optic",nullptr,nullptr,"main","ps_3_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error))){shaderFailed=true;return false;}
  if(FAILED(d->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()),&shader)))return false;
 }
 return true;
}
bool GpuGunOptics::CaptureScope(){
 if(!device||!shader)return false;
 if(!scope){
  if(FAILED(device->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&scope,nullptr)))return false;
  if(FAILED(scope->GetSurfaceLevel(0,&scopeSurface))){scope.Reset();return false;}
 }
 ComPtr<IDirect3DSurface9> back;
 return SUCCEEDED(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back))&&
  SUCCEEDED(device->StretchRect(back.Get(),nullptr,scopeSurface.Get(),nullptr,D3DTEXF_NONE));
}
bool GpuGunOptics::Draw(IDirect3DSurface9* target,const GunOptic& gun,const EyeCamera& eye,
 const OpticView& frame,unsigned index,IDirect3DTexture9* hud,IDirect3DTexture9* baseline){
 if(!device||!shader||!target||!gun.definition||index>1||!std::isfinite(gun.magnification)||gun.magnification<1||gun.magnification>16)return false;
 if(frame.visibility[index]<.02f)return true;
 if(gun.magnification>1.01f&&!scope)return false;
 const auto inv=InverseRigid(gun.gun),view=InverseRigid(eye.world);
 if(!inv||!view||!std::isfinite(frame.relief)||frame.relief<=0)return false;
 const auto matrix=Multiply(eye.world,*inv);const auto& e=matrix.values[3];
 const auto& def=*gun.definition;const float relief=def.center.z-e[2];
 // Shade only the projected glass rectangle; the rest of each eye stays
 // untouched, including the native HUD and weapon geometry.
 const auto gunToEye=Multiply(gun.gun,*view);
 float minX=float(width),minY=float(height),maxX=0,maxY=0;
 const auto& p=eye.projection.values;
 for(float sx:{-1.f,1.f})for(float sy:{-1.f,1.f}){
  const float local[]={def.center.x+sx*def.halfWidth,def.center.y+sy*def.halfHeight,def.center.z,1};
  float point[4]{};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)point[j]+=local[i]*gunToEye.values[i][j];
  if(point[2]<.015f)return true;
  float clip[4]{};for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)clip[j]+=point[i]*p[i][j];
  if(!std::isfinite(clip[3])||clip[3]<=0)return false;
  const float x=(clip[0]/clip[3]+1)*.5f*width,y=(1-clip[1]/clip[3])*.5f*height;
  if(!std::isfinite(x)||!std::isfinite(y))return false;
  minX=std::min(minX,x);maxX=std::max(maxX,x);minY=std::min(minY,y);maxY=std::max(maxY,y);
 }
 const RECT bounds{LONG(std::clamp(std::floor(minX),0.f,float(width))),LONG(std::clamp(std::floor(minY),0.f,float(height))),
  LONG(std::clamp(std::ceil(maxX),0.f,float(width))),LONG(std::clamp(std::ceil(maxY),0.f,float(height)))};
 if(bounds.left>=bounds.right||bounds.top>=bounds.bottom)return true;
 if(relief<=.04f||!std::isfinite(relief)||p[0][0]<=0||p[1][1]<=0)return false;
 const float t=relief/(100-e[2]),dotX=e[0]*(1-t),dotY=e[1]*(1-t);
 const float line=std::clamp(relief/(height*p[1][1]),.00010f,.0008f);
 float constants[9][4]={{1.f/width,1.f/height,float(width),float(height)},
  {p[0][0],p[1][1],p[2][0],p[2][1]}, {e[0],e[1],e[2],relief},
  {matrix.values[0][0],matrix.values[0][1],matrix.values[0][2],def.magnification},
  {matrix.values[1][0],matrix.values[1][1],matrix.values[1][2],0},
  {matrix.values[2][0],matrix.values[2][1],matrix.values[2][2],0},
  {def.center.x,def.center.y,def.halfWidth,def.halfHeight},
  {dotX,dotY,line,frame.relief/relief},
  {frame.visibility[index],def.rectangular?1.f:0.f,gun.magnification<=1.01f?1.f:0.f,hud&&baseline?1.f:0.f}};
 for(const auto& row:constants)for(float x:row)if(!std::isfinite(x))return false;
 D3DSURFACE_DESC desc{};if(FAILED(target->GetDesc(&desc))||desc.Width!=width||desc.Height!=height)return false;
 State saved(device);if(!saved.Save())return false;
 bool ok=true;const auto set=[&](HRESULT hr){ok=SUCCEEDED(hr)&&ok;};
 set(device->SetDepthStencilSurface(nullptr));
 for(unsigned i=1;i<saved.count;++i)set(device->SetRenderTarget(i,nullptr));
 set(device->SetRenderTarget(0,target));
 D3DVIEWPORT9 viewport{0,0,width,height,0,1};set(device->SetViewport(&viewport));
 set(device->SetVertexShader(nullptr));set(device->SetPixelShader(shader.Get()));
 set(device->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1));
 set(device->SetPixelShaderConstantF(0,&constants[0][0],9));
 for(auto state:{D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ALPHATESTENABLE,D3DRS_STENCILENABLE,D3DRS_SCISSORTESTENABLE,
    D3DRS_FOGENABLE,D3DRS_LIGHTING,D3DRS_SRGBWRITEENABLE,D3DRS_SEPARATEALPHABLENDENABLE,D3DRS_CLIPPLANEENABLE})set(device->SetRenderState(state,FALSE));
 set(device->SetScissorRect(&bounds));set(device->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE));
 set(device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE));set(device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE));
 set(device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA));set(device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD));
 set(device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE));set(device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID));
 set(device->SetRenderState(D3DRS_COLORWRITEENABLE,15));set(device->SetRenderState(D3DRS_MULTISAMPLEMASK,0xffffffff));
 set(device->SetStreamSourceFreq(0,1));set(device->SetStreamSourceFreq(1,1));
 set(device->SetTexture(0,scope.Get()));set(device->SetTexture(1,hud));set(device->SetTexture(2,baseline));
 for(unsigned i=0;i<3;++i){
  set(device->SetSamplerState(i,D3DSAMP_MINFILTER,i?D3DTEXF_POINT:D3DTEXF_LINEAR));
  set(device->SetSamplerState(i,D3DSAMP_MAGFILTER,i?D3DTEXF_POINT:D3DTEXF_LINEAR));
  set(device->SetSamplerState(i,D3DSAMP_MIPFILTER,D3DTEXF_NONE));
  set(device->SetSamplerState(i,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP));set(device->SetSamplerState(i,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP));
  set(device->SetSamplerState(i,D3DSAMP_SRGBTEXTURE,FALSE));
 }
 if(!ok)return false;
 if(FAILED(device->BeginScene()))return false;saved.started=true;
 struct Vertex{float x,y,z,rhw,u,v;};
 const float w=float(width)-.5f,h=float(height)-.5f;
 const Vertex vertices[]={{-.5f,-.5f,0,1,0,0},{w,-.5f,0,1,1,0},{-.5f,h,0,1,0,1},{w,h,0,1,1,1}};
 ok=SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex)));
 return saved.Restore()&&ok;
}
void GpuGunOptics::Reset(){scopeSurface.Reset();scope.Reset();shader.Reset();device=nullptr;width=height=0;shaderFailed=false;}
}
