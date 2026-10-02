#include "MenuOverlayGpu.h"
#include "MenuRoomGpu.h"
#include "GpuFrameTransfer.h"
#include "NativeExResources.h"
#include <MinHook.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <algorithm>
using namespace bfvr;using namespace bfvr::bf2142;
using Microsoft::WRL::ComPtr;
namespace bfvr::bf2142 {void PublishNativeHudPointer(bool,float,float,bool){} }
#define CHECK(x) do{if(!(x)){printf("Menu GPU failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int wmain(int argc,wchar_t** argv){
 const UINT w=argc>1?2064u:640u,h=argc>1?2208u:360u;
 const HWND window=CreateWindowExW(0,L"STATIC",L"Hidden GPU menu check",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(window);
 ComPtr<IDirect3D9Ex> api;CHECK(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&api)));
 D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;
 pp.BackBufferWidth=w;pp.BackBufferHeight=h;pp.BackBufferFormat=D3DFMT_X8R8G8B8;
 pp.EnableAutoDepthStencil=TRUE;pp.AutoDepthStencilFormat=D3DFMT_D24S8;pp.MultiSampleType=D3DMULTISAMPLE_4_SAMPLES;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 ComPtr<IDirect3DDevice9Ex> device;
 CHECK(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,nullptr,&device)));
 SetEnvironmentVariableW(L"BF2142VR_EX_MANAGED_UPLOAD",L"1");CHECK(MH_Initialize()==MH_OK);CHECK(InstallNativeExResources(device.Get(),nullptr));
 ComPtr<ID3D11Device> receiver;CHECK(SUCCEEDED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&receiver,nullptr,nullptr)));
 GpuFrameTransfer transfer;CHECK(SUCCEEDED(transfer.Initialize(device.Get(),w,h,receiver.Get())));
 MenuRoomGpu room;if(argc>2)CHECK(room.LoadScene(argv[2]));
 MenuOverlayGpu overlay;CHECK(overlay.Prepare(device.Get(),w,h));
 VrControlsMenu controls;VrSettings settings;
 shared::SharedRenderRequest request{};
 for(unsigned i=0;i<2;++i){auto& v=request.views[i];v.pose.orientationW=1;v.pose.positionX=i?.032f:-.032f;v.fov={-.85f,.85f,.65f,-.65f};}
 ComPtr<IDirect3DSurface9> nativeTarget,nativeDepth;CHECK(SUCCEEDED(device->GetRenderTarget(0,&nativeTarget)));CHECK(SUCCEEDED(device->GetDepthStencilSurface(&nativeDepth)));
 CHECK(SUCCEEDED(device->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff345678,1,0)));
 const D3DVIEWPORT9 vp{3,5,321,211,.1f,.9f};CHECK(SUCCEEDED(device->SetViewport(&vp)));
 const float constant[4]={.123f,.456f,.789f,1};device->SetPixelShaderConstantF(0,constant,1);
 device->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);device->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);device->SetRenderState(D3DRS_COLORWRITEENABLE,3);
 const auto stateIntact=[&](){
  ComPtr<IDirect3DSurface9> rt,depth;device->GetRenderTarget(0,&rt);device->GetDepthStencilSurface(&depth);
  D3DVIEWPORT9 v{};device->GetViewport(&v);DWORD scissor=0,fill=0,mask=0;float c[4]{};
  device->GetRenderState(D3DRS_SCISSORTESTENABLE,&scissor);device->GetRenderState(D3DRS_FILLMODE,&fill);device->GetRenderState(D3DRS_COLORWRITEENABLE,&mask);device->GetPixelShaderConstantF(0,c,1);
  return rt==nativeTarget&&depth==nativeDepth&&!std::memcmp(&v,&vp,sizeof(vp))&&!std::memcmp(c,constant,sizeof(c))&&scissor==TRUE&&fill==D3DFILL_WIREFRAME&&mask==3;
 };
 const auto stage=[&](){return SUCCEEDED(transfer.Stage(0,nullptr))&&SUCCEEDED(transfer.Stage(1,nullptr))&&SUCCEEDED(transfer.StageBackbuffer(2));};
 std::vector<DWORD> referenceLeft,referenceRight;
 CHECK(room.Draw(device.Get(),referenceLeft,referenceRight,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,request,{}));CHECK(stateIntact());
 CHECK(stage());CHECK(room.DrawTo(device.Get(),{transfer.Surface(0),transfer.Surface(1)},w,h,request,{}));CHECK(SUCCEEDED(transfer.Export()));CHECK(stateIntact());
 FrameCapture capture;std::vector<DWORD> pixels;
 CHECK(SUCCEEDED(capture.ReadSurface(device.Get(),transfer.Surface(0),DXGI_FORMAT_B8G8R8A8_UNORM,pixels)));CHECK(pixels==referenceLeft);
 CHECK(SUCCEEDED(capture.ReadSurface(device.Get(),transfer.Surface(1),DXGI_FORMAT_B8G8R8A8_UNORM,pixels)));CHECK(pixels==referenceRight);
 CHECK(referenceLeft!=referenceRight);
 MenuPointerVisual pointer;pointer.visible=true;pointer.point.pixelX=w*.62f;pointer.point.pixelY=h*.43f;
 unsigned cases=0;
 for(bool wide:{false,true})for(bool open:{false,true})for(bool pressed:{false,true}){
  if(controls.Open()!=open){controls.Hotkey(false);controls.Hotkey(true);controls.Hotkey(false);}
  pointer.pressed=pressed;
  const auto& art=controls.Artwork(w,h,DXGI_FORMAT_B8G8R8A8_UNORM,settings,wide);CHECK(art.size()==size_t(w)*h);
  const auto revision=controls.ArtworkRevision();(void)controls.Artwork(w,h,DXGI_FORMAT_B8G8R8A8_UNORM,settings,wide);CHECK(revision==controls.ArtworkRevision());
  std::vector<DWORD> expected(size_t(w)*h,0xff345678),left(expected.size()),right(expected.size());
  controls.Draw(expected,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,settings,wide);
  DrawMenuPointer(left,right,expected,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,request,{},pointer.point,{},pressed);
  CHECK(stage());CHECK(overlay.Draw({transfer.Surface(0),transfer.Surface(1),transfer.Surface(2)},pointer,request,art,revision));CHECK(stateIntact());
  CHECK(SUCCEEDED(capture.ReadSurface(device.Get(),transfer.Surface(2),DXGI_FORMAT_B8G8R8A8_UNORM,pixels,true)));
  size_t differences=0;for(size_t i=0;i<pixels.size();++i)differences+=pixels[i]!=expected[i];
  printf("Overlay wide=%d open=%d pressed=%d pixel differences=%zu\n",wide,open,pressed,differences);if(differences)for(size_t i=0,n=0;i<pixels.size()&&n<12;++i)if(pixels[i]!=expected[i]){printf("diff %zu,%zu got=%08lx wanted=%08lx\n",i%w,i/w,pixels[i],expected[i]);++n;}CHECK(differences==0);++cases;
 }
 // Laser projection is shared with the CPU path; eyes differ with IPD and preserve UI hit coordinates.
 pointer.ray=MenuRayHit{pointer.point,{.15f,-.1f,-.2f},{0,0,-1.5f}};
 auto a=ProjectMenuBeam(w,h,{},*pointer.ray,request.views[0]),b=ProjectMenuBeam(w,h,{},*pointer.ray,request.views[1]);CHECK(a&&b&&a->x1>b->x1);
 CHECK(!ProjectMenuBeam(0,h,{},*pointer.ray,request.views[0]));
 MenuRayHit behind{pointer.point,{0,0,.2f},{0,0,1.f}};CHECK(!ProjectMenuBeam(w,h,{},behind,request.views[0]));
 const auto& art=controls.Artwork(w,h,DXGI_FORMAT_B8G8R8A8_UNORM,settings,true);
 CHECK(stage());CHECK(overlay.Draw({transfer.Surface(0),transfer.Surface(1),transfer.Surface(2)},pointer,request,art,controls.ArtworkRevision()));CHECK(stateIntact());
 for(unsigned e=0;e<2;++e){CHECK(SUCCEEDED(capture.ReadSurface(device.Get(),transfer.Surface(e),DXGI_FORMAT_B8G8R8A8_UNORM,pixels)));size_t count=0;for(auto c:pixels)count+=c==0xffffb830;CHECK(count>20);}
 CHECK(SUCCEEDED(capture.Read(device.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,pixels)));CHECK(pixels.front()==0xff345678&&pixels.back()==0xff345678);
 // Compare completed GPU work, not just queue submission. CPU number excludes its later upload (conservative).
 constexpr int count=20;auto start=std::chrono::steady_clock::now();
 for(int n=0;n<count;++n)CHECK(room.Draw(device.Get(),referenceLeft,referenceRight,w,h,DXGI_FORMAT_B8G8R8A8_UNORM,request,{}));
 const double oldMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/count;
 start=std::chrono::steady_clock::now();
 for(int n=0;n<count;++n){CHECK(stage());CHECK(room.DrawTo(device.Get(),{transfer.Surface(0),transfer.Surface(1)},w,h,request,{}));CHECK(overlay.Draw({transfer.Surface(0),transfer.Surface(1),transfer.Surface(2)},pointer,request,art,controls.ArtworkRevision()));CHECK(SUCCEEDED(transfer.Export()));}
 const double newMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/count;
 printf("%ux%u completed stereo room: CPU readback %.2f ms; GPU room + UI + laser + fence %.2f ms\n",w,h,oldMs,newMs);
 overlay.Reset();room.Reset();capture.Reset();transfer.Reset();nativeTarget.Reset();nativeDepth.Reset();CHECK(SUCCEEDED(device->ResetEx(&pp,nullptr)));
 CHECK(SUCCEEDED(transfer.Initialize(device.Get(),w,h,receiver.Get())));CHECK(overlay.Prepare(device.Get(),w,h));CHECK(stage());
 CHECK(room.DrawTo(device.Get(),{transfer.Surface(0),transfer.Surface(1)},w,h,request,{}));
 CHECK(overlay.Draw({transfer.Surface(0),transfer.Surface(1),transfer.Surface(2)},pointer,request,art,controls.ArtworkRevision()));CHECK(SUCCEEDED(transfer.Export()));
 printf("%u overlay comparisons, exact room pixels, stereo laser, native state/backbuffer preservation, cached upload and ResetEx passed.\n",cases);
 overlay.Reset();room.Reset();transfer.Reset();device.Reset();DestroyWindow(window);return 0;
}
