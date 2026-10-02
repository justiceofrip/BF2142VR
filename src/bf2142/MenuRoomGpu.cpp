#include "MenuRoomGpu.h"
#include <d3dcompiler.h>
#include <cmath>
#include <algorithm>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
constexpr char program[]=R"(
float4 eyeOrigin:register(c0);
float4 basisX:register(c1);
float4 basisY:register(c2);
float4 basisZ:register(c3);
float4 fov:register(c4);
float4 style:register(c5);
float integral(float value,float period,float width){float u=value+width,n=floor(u/period);return n*(2*width)+min(u-n*period,2*width);}
float stripe(float value,float period,float width,float footprint){footprint=max(footprint,.0001);return saturate((integral(value+footprint*.5,period,width)-integral(value-footprint*.5,period,width))/footprint);}
float band(float value,float width,float footprint){footprint=max(footprint,.0001);return saturate((min(value+footprint*.5,width)-max(value-footprint*.5,-width))/footprint);}
float4 main(float2 uv:TEXCOORD0):COLOR0{
 float3 local=float3(lerp(fov.x,fov.y,uv.x),lerp(fov.z,fov.w,uv.y),-1);
 float3 ray=local.x*basisX.xyz+local.y*basisY.xyz+local.z*basisZ.xyz;
 bool hangar=style.x>0;
 float3 low=hangar?float3(-14,-1.6,-24):float3(-4,-1.6,-5),high=hangar?float3(14,8.5,8):float3(4,2.5,3);
 float3 edge=float3(ray.x>0?high.x:low.x,ray.y>0?high.y:low.y,ray.z>0?high.z:low.z);
 float3 times=(edge-eyeOrigin.xyz)/(ray+float3(abs(ray.x)<.000001?.000001:0,abs(ray.y)<.000001?.000001:0,abs(ray.z)<.000001?.000001:0));
 times=float3(times.x>0?times.x:10000,times.y>0?times.y:10000,times.z>0?times.z:10000);
 float t=min(times.x,min(times.y,times.z));
 float3 hit=eyeOrigin.xyz+ray*t,footprint=abs(ddx(hit))+abs(ddy(hit));
 float3 color;
 if(times.y<=times.x&&times.y<=times.z){
  if(ray.y<0){float a=stripe(hit.x,1,.018,footprint.x),b=stripe(hit.z,1,.018,footprint.z);
   color=lerp(float3(26,40,50),float3(13,25,34),1-(1-a)*(1-b));
   color=lerp(color,float3(30,138,160),band(abs(hit.x)-2.93,.03,footprint.x));
  }else color=float3(17,28,39);
 }else{
  float value=times.x<=times.z?hit.z:hit.x,fw=times.x<=times.z?footprint.z:footprint.x;
  color=lerp(float3(24,39,53),float3(12,24,34),stripe(value,1.6,.018,fw));
  color=lerp(color,float3(60,191,211),max(band(hit.y-1.75,.045,footprint.y),band(hit.y+1.35,.028,footprint.y)));
 }
 if(hangar){
  if(times.y<=times.x&&times.y<=times.z&&ray.y<0){
   float bay=max(band(abs(hit.x)-4.8,2.8,footprint.x)*band(hit.z+10.5,4.5,footprint.z),band(abs(hit.x)-6.0,2.8,footprint.x)*band(hit.z+15,4,footprint.z));
   color=lerp(color,float3(44,58,68),bay*.65);
   float lanes=band(abs(hit.x)-1.8,.045,footprint.x);
   color=lerp(color,float3(210,152,58),lanes);
   float rails=band(abs(hit.x)-9.8,.10,footprint.x);
   color=lerp(color,float3(94,187,216),rails);
  }else if(times.y<=times.x&&times.y<=times.z){
   float beam=stripe(hit.z,4,.13,footprint.z);color=lerp(float3(30,40,52),float3(10,19,27),beam);
   float light=band(abs(hit.x)-5,.38,footprint.x)*stripe(hit.z,4,1.2,footprint.z);color=lerp(color,float3(216,229,233),light);
  }else{
   float vertical=stripe(times.x<=times.z?hit.z:hit.x,4,.15,times.x<=times.z?footprint.z:footprint.x);
   color=lerp(float3(43,61,77),float3(16,30,42),vertical);
   float beam=max(band(hit.y-5.8,.10,footprint.y),band(hit.y+1.1,.05,footprint.y));color=lerp(color,float3(57,178,211),beam);
   if(times.z<times.x&&ray.z<0){float gate=band(hit.x,9,footprint.x)*band(hit.y-2.3,3.1,footprint.y);color=lerp(color,float3(26,38,49),gate*(1-stripe(hit.y,.7,.022,footprint.y)));}
  }
 }
 return float4(color*clamp(1-t*(hangar?.009:.025),.55,1)/255,1);
})";
using V=stereo::Vec3;
V Rotate(const stereo::Quaternion& q,V v){const V t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
 return {v.x+q.w*t.x+q.y*t.z-q.z*t.y,v.y+q.w*t.y+q.z*t.x-q.x*t.z,v.z+q.w*t.z+q.x*t.y-q.y*t.x};}
}
bool MenuRoomGpu::Draw(IDirect3DDevice9* d,std::vector<DWORD>& left,std::vector<DWORD>& right,UINT w,UINT h,
 DXGI_FORMAT format,const shared::SharedRenderRequest& request,const stereo::Pose& anchor){
 if(!Render(d,w,h,format,request,anchor,nullptr))return false;
 left.swap(completed[0]);right.swap(completed[1]);return true;
}
bool MenuRoomGpu::DrawTo(IDirect3DDevice9* d,const std::array<IDirect3DSurface9*,2>& destinations,UINT w,UINT h,
 const shared::SharedRenderRequest& request,const stereo::Pose& anchor){
 for(auto* target:destinations){D3DSURFACE_DESC desc{};
  if(!target||FAILED(target->GetDesc(&desc))||desc.Width!=w||desc.Height!=h||desc.Format!=D3DFMT_A8R8G8B8||desc.MultiSampleType!=D3DMULTISAMPLE_NONE)return false;
 }
 return Render(d,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,request,anchor,&destinations);
}
bool MenuRoomGpu::Render(IDirect3DDevice9* d,UINT w,UINT h,DXGI_FORMAT format,
 const shared::SharedRenderRequest& request,const stereo::Pose& anchor,const std::array<IDirect3DSurface9*,2>* destinations){
 if(!d||!w||!h||w>8192||h>8192)return false;
 if(d!=device||w!=width||h!=height){Reset();device=d;width=w;height=h;}
 if(failed)return false;
 if(!shader){Microsoft::WRL::ComPtr<ID3DBlob> code,error;
  if(FAILED(D3DCompile(program,sizeof(program)-1,"BF2142 menu room",nullptr,nullptr,"main","ps_3_0",D3DCOMPILE_OPTIMIZATION_LEVEL3,0,&code,&error))){failed=true;return false;}
  if(FAILED(d->CreatePixelShader(static_cast<const DWORD*>(code->GetBufferPointer()),&shader))){failed=true;return false;}
 }
 if(!surface){
  D3DMULTISAMPLE_TYPE aa=D3DMULTISAMPLE_NONE;
  if(scene.Ready()){
   Microsoft::WRL::ComPtr<IDirect3D9> api;D3DCAPS9 caps{};
   if(SUCCEEDED(d->GetDirect3D(&api))&&SUCCEEDED(d->GetDeviceCaps(&caps))&&
      SUCCEEDED(api->CheckDeviceMultiSampleType(caps.AdapterOrdinal,caps.DeviceType,D3DFMT_A8R8G8B8,TRUE,D3DMULTISAMPLE_4_SAMPLES,nullptr))&&
      SUCCEEDED(api->CheckDeviceMultiSampleType(caps.AdapterOrdinal,caps.DeviceType,D3DFMT_D24S8,TRUE,D3DMULTISAMPLE_4_SAMPLES,nullptr)))aa=D3DMULTISAMPLE_4_SAMPLES;
  }
  if(FAILED(d->CreateRenderTarget(w,h,D3DFMT_A8R8G8B8,aa,0,FALSE,&surface,nullptr)))return false;
  if(scene.Ready()&&FAILED(d->CreateDepthStencilSurface(w,h,D3DFMT_D24S8,aa,0,TRUE,&depthSurface,nullptr))){surface.Reset();return false;}
 }
 float constants[2][5][4]{};
 for(unsigned e=0;e<2;++e){const auto& v=request.views[e];const auto& p=v.pose;
  const auto local=stereo::MakeRelativePose(anchor,{{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}});if(!local)return false;
  const auto x=Rotate(local->orientation,{1,0,0}),y=Rotate(local->orientation,{0,1,0}),z=Rotate(local->orientation,{0,0,1});
  const float value[5][4]={{local->position.x,local->position.y,local->position.z,0},{x.x,x.y,x.z,0},{y.x,y.y,y.z,0},{z.x,z.y,z.z,0},
   {std::tan(v.fov.angleLeft),std::tan(v.fov.angleRight),std::tan(v.fov.angleUp),std::tan(v.fov.angleDown)}};
  for(const auto& row:value)for(float f:row)if(!std::isfinite(f))return false;
  if(value[4][1]<=value[4][0]||value[4][2]<=value[4][3])return false;
  std::memcpy(constants[e],value,sizeof(value));
 }
 Microsoft::WRL::ComPtr<IDirect3DStateBlock9> state;if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&state)))return false;
 D3DCAPS9 caps{};if(FAILED(d->GetDeviceCaps(&caps)))return false;
 const unsigned targets=std::min<unsigned>(caps.NumSimultaneousRTs,4);
 Microsoft::WRL::ComPtr<IDirect3DSurface9> previous[4],depth;d->GetDepthStencilSurface(&depth);
 for(unsigned i=0;i<targets;++i){d->GetRenderTarget(i,&previous[i]);if(i)d->SetRenderTarget(i,nullptr);}
 bool ok=SUCCEEDED(d->SetDepthStencilSurface(nullptr))&&SUCCEEDED(d->SetRenderTarget(0,surface.Get()));
 d->SetVertexShader(nullptr);d->SetPixelShader(shader.Get());d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
 for(auto renderState:{D3DRS_ZENABLE,D3DRS_ZWRITEENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_SEPARATEALPHABLENDENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_STENCILENABLE,D3DRS_FOGENABLE,D3DRS_LIGHTING,D3DRS_SRGBWRITEENABLE,D3DRS_CLIPPLANEENABLE})d->SetRenderState(renderState,FALSE);
 d->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS,TRUE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_COLORWRITEENABLE,15);d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
 D3DVIEWPORT9 viewport{0,0,w,h,0,1};d->SetViewport(&viewport);
 struct Vertex{float x,y,z,w,u,v;};const float r=float(w)-.5f,b=float(h)-.5f;
 const Vertex vertices[]={{-.5f,-.5f,0,1,0,0},{r,-.5f,0,1,1,0},{-.5f,b,0,1,0,1},{r,b,0,1,1,1}};
 for(unsigned e=0;e<2&&ok;++e){
  if(FAILED(d->BeginScene())){ok=false;break;}
  d->SetDepthStencilSurface(nullptr);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);d->SetRenderState(D3DRS_LIGHTING,FALSE);
  d->SetVertexShader(nullptr);d->SetPixelShader(shader.Get());d->SetFVF(D3DFVF_XYZRHW|D3DFVF_TEX1);
  const float style[4]={scene.Ready()?1.f:0.f,0,0,0};d->SetPixelShaderConstantF(5,style,1);
  ok=SUCCEEDED(d->SetPixelShaderConstantF(0,&constants[e][0][0],5))&&SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex)));
  if(ok&&scene.Ready()){
   const auto& v=request.views[e];const auto& p=v.pose;CameraInput source{};for(int i=0;i<4;++i)source.world.values[i][i]=1;source.nearPlane=.04f;source.farDelta=80;
   const auto eye=MakeEyeCamera(source,anchor,{{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}},
       {std::tan(v.fov.angleLeft),std::tan(v.fov.angleRight),std::tan(v.fov.angleUp),std::tan(v.fov.angleDown)});
   ok=eye&&SUCCEEDED(d->SetDepthStencilSurface(depthSurface.Get()))&&SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_ZBUFFER,0,1,0))&&scene.Draw(d,*eye);
  }
  ok=SUCCEEDED(d->EndScene())&&ok;
  if(ok)ok=destinations?SUCCEEDED(d->StretchRect(surface.Get(),nullptr,(*destinations)[e],nullptr,D3DTEXF_NONE)):
      SUCCEEDED(capture.ReadSurface(d,surface.Get(),format,completed[e]));
 }
 bool restored=SUCCEEDED(d->SetRenderTarget(0,previous[0].Get()));
 for(unsigned i=1;i<targets;++i)restored=SUCCEEDED(d->SetRenderTarget(i,previous[i].Get()))&&restored;
 restored=SUCCEEDED(d->SetDepthStencilSurface(depth.Get()))&&restored;
 restored=SUCCEEDED(state->Apply())&&restored;
 return ok&&restored;
}
}
