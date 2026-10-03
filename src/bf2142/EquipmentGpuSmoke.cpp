#include "BodyEquipment.h"
#include "NativeExResources.h"
#include <MinHook.h>
#include "WristMenu.h"
#include "FrameCapture.h"
#include "StereoCamera.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <cstdio>
#include <cmath>
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Equipment GPU line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int wmain(int argc,wchar_t** argv){
 CHECK(argc>=2);BodyEquipment body;CHECK(body.Load(argv[1]));
 auto window=CreateWindowExW(0,L"STATIC",L"Hidden equipment GPU check",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(window);
 const bool ex=argc>2&&wcscmp(argv[2],L"--native-ex")==0;
 Microsoft::WRL::ComPtr<IDirect3D9Ex> apiEx;Microsoft::WRL::ComPtr<IDirect3D9> api;
 if(ex){CHECK(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&apiEx)));api=apiEx;}
 else api.Attach(Direct3DCreate9(D3D_SDK_VERSION));CHECK(api);
 D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;pp.BackBufferWidth=1600;pp.BackBufferHeight=900;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.EnableAutoDepthStencil=TRUE;pp.AutoDepthStencilFormat=D3DFMT_D24S8;pp.MultiSampleType=D3DMULTISAMPLE_4_SAMPLES;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 Microsoft::WRL::ComPtr<IDirect3DDevice9> d;Microsoft::WRL::ComPtr<IDirect3DDevice9Ex> deviceEx;
 for(UINT adapter=0;adapter<api->GetAdapterCount()&&!d;++adapter){const auto create=ex?apiEx->CreateDeviceEx(adapter,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,nullptr,&deviceEx):api->CreateDevice(adapter,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&d);if(deviceEx)d=deviceEx;D3DADAPTER_IDENTIFIER9 id{};api->GetAdapterIdentifier(adapter,0,&id);printf("D3D9 adapter %u (%s) create=0x%08lX\n",adapter,id.Description,create);}CHECK(d);
 if(ex){SetEnvironmentVariableW(L"BF2142VR_EX_MANAGED_UPLOAD",L"1");CHECK(MH_Initialize()==MH_OK);CHECK(InstallNativeExResources(d.Get(),nullptr));}
 shared::SharedRenderRequest request{};for(int i=0;i<2;++i){auto& v=request.views[i];v.pose.positionY=1.7f;v.pose.positionX=i?.032f:-.032f;v.pose.orientationX=std::sin(-.58f);v.pose.orientationW=std::cos(-.58f);v.fov={-.72f,.72f,.55f,-.55f};}
 BodyInventoryResult placement;placement.anchor={{0,1.7f,0},{0,0,0,1}};placement.anchorValid=true;
 InventoryNames names{};strcpy_s(names[1].data(),49,"knife");strcpy_s(names[2].data(),49,"eu_handgun");strcpy_s(names[3].data(),49,"eu_ar_rifle");strcpy_s(names[7].data(),49,"unl_grenade_frag");strcpy_s(names[4].data(),49,"unl_hub_ammo");
 CameraInput base{};for(int i=0;i<4;++i)base.world.values[i][i]=1;base.nearPlane=.035f;base.farDelta=100;
 auto camera=MakeEyeCamera(base,{}, {},{std::tan(-.72f),std::tan(.72f),std::tan(.55f),std::tan(-.55f)});CHECK(camera);
 Microsoft::WRL::ComPtr<IDirect3DSurface9> target,depth;d->GetRenderTarget(0,&target);d->GetDepthStencilSurface(&depth);
 FrameCapture capture;std::vector<DWORD> eyes[2];
 for(int eye=0;eye<2;++eye){
  CHECK(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff18202a,0,0)));CHECK(SUCCEEDED(d->BeginScene()));
  d->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);d->SetRenderState(D3DRS_COLORWRITEENABLE,3);
  D3DVIEWPORT9 hostile{17,19,300,200,.75f,.8f};d->SetViewport(&hostile);d->SetRenderState(D3DRS_MULTISAMPLEMASK,0);
  CHECK(body.DrawGpu(d.Get(),request.views[eye],camera->projection,1,placement,names,3));
  D3DVIEWPORT9 restored{};d->GetViewport(&restored);CHECK(restored.X==17&&restored.MinZ==.75f);
  DWORD value=0;d->GetRenderState(D3DRS_MULTISAMPLEMASK,&value);CHECK(value==0);d->GetRenderState(D3DRS_FILLMODE,&value);CHECK(value==D3DFILL_WIREFRAME);d->GetRenderState(D3DRS_SCISSORTESTENABLE,&value);CHECK(value==TRUE);d->GetRenderState(D3DRS_COLORWRITEENABLE,&value);CHECK(value==3);
  CHECK(SUCCEEDED(d->EndScene()));CHECK(SUCCEEDED(capture.Read(d.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,eyes[eye])));
 }
 unsigned painted=0;for(auto color:eyes[0])painted+=color!=0xff18202a;CHECK(painted>200&&eyes[0]!=eyes[1]);
 if(argc>2&&!ex){std::ofstream file(std::filesystem::path(argv[2]),std::ios::binary);file<<"P6\n1600 900\n255\n";for(auto c:eyes[0]){const char rgb[]={char(c>>16),char(c>>8),char(c)};file.write(rgb,3);}}
 // The offhand prop is visible with an otherwise empty inventory layer and
 // follows the tracked palm, independent of the body equipment anchor.
 HeldEquipment held;strcpy_s(held.name.data(),49,"unl_hub_ammo");held.pose={{-.16f,1.35f,-.38f},{0,0,0,1}};
 InventoryNames hidden{};std::vector<DWORD> heldPixels[2];
 for(int e=0;e<2;++e){
  CHECK(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff18202a,0,0)));CHECK(SUCCEEDED(d->BeginScene()));
  CHECK(body.DrawGpu(d.Get(),request.views[e],camera->projection,1,placement,hidden,3,&held));CHECK(SUCCEEDED(d->EndScene()));
  CHECK(SUCCEEDED(capture.Read(d.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,heldPixels[e])));
 }
 unsigned propPixels=0;for(auto color:heldPixels[0])propPixels+=color!=0xff18202a;CHECK(propPixels>200&&heldPixels[0]!=heldPixels[1]);
 D3DVIEWPORT9 full{0,0,1600,900,0,1};d->SetViewport(&full);d->SetRenderState(D3DRS_MULTISAMPLEMASK,0xffffffff);
 WristMenuGpu wrist;WristFrame f;f.visible=true;f.panel={{0,0,-.35f},{0,0,0,1}};shared::SharedPresentationView eye{};eye.pose.orientationW=1;
 CHECK(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff18202a,0,0)));CHECK(SUCCEEDED(d->BeginScene()));CHECK(wrist.Draw(d.Get(),f,eye,camera->projection,1));CHECK(SUCCEEDED(d->EndScene()));std::vector<DWORD> wristPixels;CHECK(SUCCEEDED(capture.Read(d.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,wristPixels)));unsigned letters=0;for(auto c:wristPixels)letters+=((c>>16)&255)>150;CHECK(letters>100);
 Microsoft::WRL::ComPtr<IDirect3DSurface9> afterTarget,afterDepth;d->GetRenderTarget(0,&afterTarget);d->GetDepthStencilSurface(&afterDepth);CHECK(afterTarget.Get()==target.Get()&&afterDepth.Get()==depth.Get());
 body.ResetGpu();wrist.Reset();capture.Reset();afterTarget.Reset();afterDepth.Reset();target.Reset();depth.Reset();CHECK(SUCCEEDED(deviceEx?deviceEx->ResetEx(&pp,nullptr):d->Reset(&pp)));
 CHECK(SUCCEEDED(d->BeginScene()));CHECK(body.DrawGpu(d.Get(),request.views[0],camera->projection,1,placement,names,3));CHECK(wrist.Draw(d.Get(),f,eye,camera->projection,1));CHECK(SUCCEEDED(d->EndScene()));
 printf("Full-resolution MSAA equipment: %u pixels, stereo parallax, textured wrist label, exact native state/targets and device reset passed.\n",painted);DestroyWindow(window);
}
