#include "GpuGunOptics.h"
#include "FrameCapture.h"
#include <cstdio>
#include <cmath>
#include <chrono>
using namespace bfvr;using namespace bfvr::bf2142;using Microsoft::WRL::ComPtr;
#define CHECK(x) do{if(!(x)){printf("GPU optic check line %d: %s\n",__LINE__,#x);return 1;}}while(0)
stereo::Matrix4 Identity(){stereo::Matrix4 m{};for(unsigned i=0;i<4;++i)m.values[i][i]=1;return m;}
bool Texture(IDirect3DDevice9* d,unsigned w,unsigned h,const std::vector<DWORD>& pixels,ComPtr<IDirect3DTexture9>& result){
 ComPtr<IDirect3DTexture9> upload;
 if(FAILED(d->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&upload,nullptr))||
    FAILED(d->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&result,nullptr)))return false;
 D3DLOCKED_RECT map{};if(FAILED(upload->LockRect(0,&map,nullptr,0)))return false;
 for(unsigned y=0;y<h;++y)memcpy(static_cast<BYTE*>(map.pBits)+size_t(y)*map.Pitch,pixels.data()+size_t(y)*w,w*4);
 upload->UnlockRect(0);return SUCCEEDED(d->UpdateTexture(upload.Get(),result.Get()));
}
int wmain(int argc,wchar_t**){
 const unsigned w=argc>1?2064:512,h=argc>1?2208:512;
 HWND window=CreateWindowExW(0,L"STATIC",L"Hidden GPU optic check",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(window);
 ComPtr<IDirect3D9Ex> api;CHECK(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&api)));
 D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.hDeviceWindow=window;p.BackBufferWidth=w;p.BackBufferHeight=h;p.BackBufferFormat=D3DFMT_X8R8G8B8;
 p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.MultiSampleType=D3DMULTISAMPLE_8_SAMPLES;p.EnableAutoDepthStencil=TRUE;p.AutoDepthStencilFormat=D3DFMT_D24S8;
 p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 ComPtr<IDirect3DDevice9Ex> d;CHECK(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_PUREDEVICE,&p,nullptr,&d)));
 GpuGunOptics gpu;CHECK(gpu.Prepare(d.Get(),w,h));
 ComPtr<IDirect3DTexture9> eyeTexture;ComPtr<IDirect3DSurface9> target,back,depth;
 CHECK(SUCCEEDED(d->CreateTexture(w,h,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&eyeTexture,nullptr)));
 CHECK(SUCCEEDED(eyeTexture->GetSurfaceLevel(0,&target)));d->GetRenderTarget(0,&back);d->GetDepthStencilSurface(&depth);
 CHECK(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff336699,1,0)));
 D3DRECT block{LONG(w/3),LONG(h/3),LONG(w*2/3),LONG(h*2/3)};CHECK(SUCCEEDED(d->Clear(1,&block,D3DCLEAR_TARGET,0xff66aa44,1,0)));
 FrameCapture capture;std::vector<DWORD> scene;
 CHECK(SUCCEEDED(capture.Read(d.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,scene)));CHECK(gpu.CaptureScope());
 std::vector<DWORD> base(size_t(w)*h),ink=base;
 // Shared HUD content must subtract away; scope artwork remains premultiplied.
 for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
  if(x<w/8&&y<h/8)base[size_t(y)*w+x]=ink[size_t(y)*w+x]=0xff22aa77;
  if(x>w/4&&x<w*3/4&&y>h/4&&y<h*3/4&&(abs(int(x)-int(w/2))<3||abs(int(y)-int(h/2))<3))ink[size_t(y)*w+x]=0xc0609020;
 }
 ComPtr<IDirect3DTexture9> hud,baseline;CHECK(Texture(d.Get(),w,h,ink,hud));CHECK(Texture(d.Get(),w,h,base,baseline));
 auto isolated=ink;IsolateOpticHud(isolated,base,w,h);CHECK(!isolated.empty());
 size_t cases=0;
 for(const char* name:{"eu_ar_rifle","as_ar_rifle","eu_mg","eu_sni","as_sni","unl_adv_sni","eu_smg","as_smg","eu_av","as_av","unl_av_rifle"})for(bool nativeHud:{false,true})for(unsigned index:{0u,1u}){
  const auto* def=FindGunOptic(name);CHECK(def);GunOptic gun{def,Identity(),def->magnification};
  auto world=Identity();const float roll=.19f;world.values[0]={cosf(roll),sinf(roll),0,0};world.values[1]={-sinf(roll),cosf(roll),0,0};world.values[3]={17,23,8,1};
  CameraInput input{Identity(),.01f,1000};input.world.values[3]={def->center.x,def->center.y,def->center.z-.20f,1};
  auto camera=MakeEyeCamera(input,{}, {},{-1,1,1,-1});CHECK(camera);
  std::array<EyeCamera,2> eyes{*camera,*camera};eyes[1].world.values[3][0]+=.064f;
  for(auto& e:eyes)e.world=Multiply(e.world,world);gun.gun=world;
  const auto view=MakeOpticView(gun,eyes);CHECK(view);
  std::vector<DWORD> expected(size_t(w)*h,0xff112233),actual;
  CompositeGunOptic(expected,scene,w,h,gun,eyes[index],*view,index,false,nativeHud?isolated:std::vector<DWORD>{});
  CHECK(SUCCEEDED(d->ColorFill(target.Get(),nullptr,0xff112233)));
  // Hostile state must not change either optic output or later native draws.
  d->SetRenderState(D3DRS_COLORWRITEENABLE,3);d->SetRenderState(D3DRS_MULTISAMPLEMASK,0);
  d->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);
  D3DVIEWPORT9 hostile{11,17,200,100,.2f,.7f};d->SetViewport(&hostile);
  CHECK(gpu.Draw(target.Get(),gun,eyes[index],*view,index,nativeHud?hud.Get():nullptr,nativeHud?baseline.Get():nullptr));
  DWORD value=0;d->GetRenderState(D3DRS_COLORWRITEENABLE,&value);CHECK(value==3);
  d->GetRenderState(D3DRS_MULTISAMPLEMASK,&value);CHECK(value==0);
  d->GetRenderState(D3DRS_FILLMODE,&value);CHECK(value==D3DFILL_WIREFRAME);
  D3DVIEWPORT9 restored{};d->GetViewport(&restored);CHECK(restored.X==11&&restored.MinZ==.2f);
  ComPtr<IDirect3DSurface9> after,afterDepth;d->GetRenderTarget(0,&after);d->GetDepthStencilSurface(&afterDepth);CHECK(after.Get()==back.Get()&&afterDepth.Get()==depth.Get());
  CHECK(SUCCEEDED(capture.ReadSurface(d.Get(),target.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,actual)));
  size_t errors=0,changed=0;unsigned maxDifference=0;
  for(size_t i=0;i<expected.size();++i){changed+=expected[i]!=0xff112233;unsigned difference=0;
   for(unsigned shift=0;shift<32;shift+=8)difference=std::max(difference,unsigned(abs(int((expected[i]>>shift)&255)-int((actual[i]>>shift)&255))));
   errors+=difference>4;maxDifference=std::max(maxDifference,difference);
  }
  if(errors>std::max(size_t(3),changed/1000)){printf("Mismatch %s hud=%d eye=%u changed=%zu errors=%zu max=%u\n",name,nativeHud,index,changed,errors,maxDifference);return 1;}
  if(index==1)CHECK(changed==0&&errors==0);
  ++cases;
 }
 // All resources must release before device reset, then recreate successfully.
 gpu.Reset();capture.Reset();hud.Reset();baseline.Reset();target.Reset();eyeTexture.Reset();back.Reset();depth.Reset();
 CHECK(SUCCEEDED(d->ResetEx(&p,nullptr)));CHECK(gpu.Prepare(d.Get(),w,h));CHECK(gpu.CaptureScope());gpu.Reset();
 printf("GPU optics %ux%u: %zu CPU-reference cases, scopes/reflex, HUD isolation, off-eye clipping, rigid transforms, alpha, native state and reset passed.\n",w,h,cases);
 DestroyWindow(window);return 0;
}
