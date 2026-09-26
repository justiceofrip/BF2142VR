#include "ShoulderRadioGpu.h"
#include "FrameCapture.h"
#include <wrl/client.h>
#include <cstdio>
#include <vector>
#include <fstream>
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Radio GPU failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(int argc,char** argv){
 const auto window=CreateWindowExW(0,L"STATIC",L"Hidden radio depth test",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(window);
 Microsoft::WRL::ComPtr<IDirect3D9> api;api.Attach(Direct3DCreate9(D3D_SDK_VERSION));CHECK(api);
 for(bool msaa:{false,true}){
  D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window;pp.BackBufferWidth=960;pp.BackBufferHeight=720;
  pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.EnableAutoDepthStencil=TRUE;pp.AutoDepthStencilFormat=D3DFMT_D24S8;pp.MultiSampleType=msaa?D3DMULTISAMPLE_4_SAMPLES:D3DMULTISAMPLE_NONE;
  Microsoft::WRL::ComPtr<IDirect3DDevice9> d;CHECK(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&d)));
  const auto projection=stereo::MakeD3D8ProjectionFromFovTangents({-1,1,.75f,-.75f},.006f,100.f);CHECK(projection);
  RadioFrame f;f.visible=true;f.anchor={{-.22f,.19f,-.455f},{0,1,0,0}};
  shared::SharedPresentationView eye;eye.pose.orientationW=1;FrameCapture capture;
  const DWORD bg=0xff101820;
  std::vector<DWORD> left,right;
  for(int e=0;e<2;++e){
   eye.pose.positionX=e?.032f:-.032f;
   CHECK(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,bg,1,0)));
   CHECK(SUCCEEDED(d->BeginScene()));
   d->SetRenderState(D3DRS_FILLMODE,D3DFILL_WIREFRAME);d->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);d->SetRenderState(D3DRS_COLORWRITEENABLE,3);
   CHECK(DrawShoulderRadio(d.Get(),f,eye,*projection,1));
   DWORD fill=0,z=0,color=0;d->GetRenderState(D3DRS_FILLMODE,&fill);d->GetRenderState(D3DRS_ZWRITEENABLE,&z);d->GetRenderState(D3DRS_COLORWRITEENABLE,&color);
   CHECK(fill==D3DFILL_WIREFRAME&&z==FALSE&&color==3);
   CHECK(SUCCEEDED(d->EndScene()));CHECK(SUCCEEDED(capture.Read(d.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,e?right:left)));
  }
  unsigned painted=0;for(auto c:left)painted+=c!=bg;CHECK(painted>1000&&left!=right);
  if(argc>1&&!msaa){std::ofstream out(argv[1],std::ios::binary);out<<"P6\n960 720\n255\n";for(auto c:left){const char rgb[]={char(c>>16),char(c>>8),char(c)};out.write(rgb,3);}}
  // A foreground depth surface (e.g. fingers) must occlude the radio, not get
  // painted over by an after-the-fact CPU overlay.
  CHECK(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,bg,0,0)));
  CHECK(SUCCEEDED(d->BeginScene()));CHECK(DrawShoulderRadio(d.Get(),f,eye,*projection,1));CHECK(SUCCEEDED(d->EndScene()));
  CHECK(SUCCEEDED(capture.Read(d.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,right)));for(auto c:right)CHECK(c==bg);
  f.pressed=true;f.depression=1;CHECK(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,bg,1,0)));
  CHECK(SUCCEEDED(d->BeginScene()));CHECK(DrawShoulderRadio(d.Get(),f,eye,*projection,1));CHECK(SUCCEEDED(d->EndScene()));
  CHECK(SUCCEEDED(capture.Read(d.Get(),DXGI_FORMAT_B8G8R8A8_UNORM,right)));unsigned green=0;for(auto c:right)green+=((c>>8)&255)>180;CHECK(green>20);
  printf("Radio GPU: MSAA=%d, %u pixels, stereo parallax, depth occlusion, moving switch and restored state.\n",msaa,painted);
 }
 DestroyWindow(window);return 0;
}
