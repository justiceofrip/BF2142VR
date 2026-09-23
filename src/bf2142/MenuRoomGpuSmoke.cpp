#include "MenuRoomGpu.h"
#include <cstdio>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <cmath>
#include <cstring>
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Menu GPU failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char** argv){
 const auto window=CreateWindowExW(0,L"STATIC",L"BF2142 hidden room test",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(window);
 Microsoft::WRL::ComPtr<IDirect3D9> api;api.Attach(Direct3DCreate9(D3D_SDK_VERSION));CHECK(api);
 for(bool msaa:{false,true}){
  D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;pp.BackBufferWidth=1280;pp.BackBufferHeight=720;
  pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.EnableAutoDepthStencil=TRUE;pp.AutoDepthStencilFormat=D3DFMT_D24S8;pp.MultiSampleType=msaa?D3DMULTISAMPLE_4_SAMPLES:D3DMULTISAMPLE_NONE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
  Microsoft::WRL::ComPtr<IDirect3DDevice9> d;CHECK(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,argc==3?D3DCREATE_HARDWARE_VERTEXPROCESSING:D3DCREATE_SOFTWARE_VERTEXPROCESSING,&pp,&d)));
  Microsoft::WRL::ComPtr<IDirect3DSurface9> target,depth;CHECK(SUCCEEDED(d->GetRenderTarget(0,&target)));CHECK(SUCCEEDED(d->GetDepthStencilSurface(&depth)));
  CHECK(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff345678,1,0)));
  const D3DVIEWPORT9 viewport{3,5,321,211,.1f,.9f};d->SetViewport(&viewport);d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE);d->SetRenderState(D3DRS_COLORWRITEENABLE,3);d->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);
  const float shaderConstant[4]={.123f,.456f,.789f,1};d->SetPixelShaderConstantF(0,shaderConstant,1);
  shared::SharedRenderRequest request{};for(unsigned i=0;i<2;++i){auto& v=request.views[i];v.pose.orientationW=1;v.pose.positionX=i?.032f:-.032f;v.fov.angleLeft=-.85f;v.fov.angleRight=.85f;v.fov.angleUp=.6f;v.fov.angleDown=-.6f;}
  std::vector<DWORD> left,right;MenuRoomGpu room;if(argc==3)CHECK(room.LoadScene(std::filesystem::path(argv[2]).wstring()));CHECK(room.Draw(d.Get(),left,right,1280,720,DXGI_FORMAT_B8G8R8A8_UNORM,request,{}));CHECK(left.size()==1280*720&&right.size()==left.size());
  const auto start=std::chrono::steady_clock::now();for(int n=0;n<5;++n)CHECK(room.Draw(d.Get(),left,right,1280,720,DXGI_FORMAT_B8G8R8A8_UNORM,request,{}));
  printf("GPU room msaa=%d stereo 1280x720 average %.2f ms\n",msaa,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()/5);
  Microsoft::WRL::ComPtr<IDirect3DSurface9> afterTarget,afterDepth;d->GetRenderTarget(0,&afterTarget);d->GetDepthStencilSurface(&afterDepth);CHECK(afterTarget.Get()==target.Get()&&afterDepth.Get()==depth.Get());
  D3DVIEWPORT9 afterViewport{};d->GetViewport(&afterViewport);CHECK(std::memcmp(&viewport,&afterViewport,sizeof(viewport))==0);
  DWORD state=0;d->GetRenderState(D3DRS_COLORWRITEENABLE,&state);CHECK(state==3);d->GetRenderState(D3DRS_SCISSORTESTENABLE,&state);CHECK(state==TRUE);d->GetRenderState(D3DRS_FILLMODE,&state);CHECK(state==D3DFILL_WIREFRAME);
  float afterConstant[4]{};d->GetPixelShaderConstantF(0,afterConstant,1);CHECK(std::memcmp(afterConstant,shaderConstant,sizeof(afterConstant))==0);
  FrameCapture capture;std::vector<DWORD> background;CHECK(SUCCEEDED(capture.Read(d.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,background)));CHECK(background.front()==0xff345678&&background.back()==0xff345678);
  unsigned difference=0;for(size_t i=0;i<left.size();++i)difference+=left[i]!=right[i];CHECK(difference>1000);
  std::vector<DWORD> cpuLeft(left.size()),cpuRight(right.size());DrawMenuRoom(cpuLeft,cpuRight,1280,720,DXGI_FORMAT_B8G8R8A8_UNORM,request,{});
  double error=0;for(size_t i=0;i<left.size();++i)for(unsigned shift:{0u,8u,16u})error+=std::abs(int((left[i]>>shift)&255)-int((cpuLeft[i]>>shift)&255));
  error/=left.size()*3;printf("GPU/analytic reference mean pixel-channel error %.4f; stereo distinct pixels %u\n",error,difference);CHECK(argc==3?error>3:error<2);
  if(argc>=2&&!msaa){std::ofstream file(argv[1],std::ios::binary);file<<"P6\n1280 720\n255\n";for(auto pixel:left){const char rgb[]={char(pixel>>16),char(pixel>>8),char(pixel)};file.write(rgb,3);}}
  const auto bgra=left;CHECK(room.Draw(d.Get(),left,right,1280,720,DXGI_FORMAT_R8G8B8A8_UNORM,request,{}));for(size_t i=0;i<left.size();++i)CHECK(left[i]==((bgra[i]&0xff00ff00)|((bgra[i]&255)<<16)|((bgra[i]>>16)&255)));
  room.Reset();CHECK(room.Draw(d.Get(),left,right,640,360,DXGI_FORMAT_B8G8R8A8_UNORM,request,{}));CHECK(left.size()==640*360);
 }
 DestroyWindow(window);puts("GPU room AA, stereo, RGBA/BGRA, MSAA native target/state preservation and reset passed.");return 0;
}
