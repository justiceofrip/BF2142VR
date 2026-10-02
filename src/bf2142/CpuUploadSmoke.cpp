// Manual 32-bit hardware regression: full-size repeated CPU uploads, exact
// patterned pixels with padded pitch, alpha, ownership and bounded memory.
#include "presenter/SharedTextureProducer.h"
#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <wrl/client.h>
#include <psapi.h>
#include <cstdio>
#include <vector>
#include <array>
using Microsoft::WRL::ComPtr;
using namespace bfvr;
void Check(HRESULT hr){if(FAILED(hr)){printf("FAIL %08lx\n",hr);exit(1);}}
SIZE_T Memory(unsigned frame){PROCESS_MEMORY_COUNTERS_EX m{};m.cb=sizeof(m);GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&m),sizeof(m));printf("frame=%u private-MiB=%.1f\n",frame,m.PrivateUsage/1048576.);return m.PrivateUsage;}
int main(){
 setvbuf(stdout,nullptr,_IONBF,0);const UINT w=2064,h=2208,pitch=w+13;
 ComPtr<IDXGIFactory1> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));ComPtr<IDXGIAdapter1> adapter;Check(factory->EnumAdapters1(0,&adapter));DXGI_ADAPTER_DESC1 ad{};Check(adapter->GetDesc1(&ad));
 shared::SharedTextureRequirements r{};r.boundedCpuUpload=true;r.adapterLuid=ad.AdapterLuid;r.format=DXGI_FORMAT_B8G8R8A8_UNORM;r.leftWorldWidth=r.rightWorldWidth=r.uiWidth=w;r.leftWorldHeight=r.rightWorldHeight=r.uiHeight=h;
 shared::SharedTextureProducer producer;auto name=L"Local\\BF2142CpuMemory-"+std::to_wstring(GetCurrentProcessId());if(!producer.Initialize(name.c_str(),r,nullptr,nullptr))return 2;Memory(0);
 ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;Check(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context));ComPtr<ID3D11Device1> d1;Check(device.As(&d1));
 std::array<shared::SharedTextureDescription,3> desc;producer.CopyDescriptions(desc.data(),3);std::array<ComPtr<ID3D11Texture2D>,3> textures;std::array<ComPtr<IDXGIKeyedMutex>,3> locks;
 for(unsigned i=0;i<3;++i){Check(d1->OpenSharedResourceByName(desc[i].name,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,IID_PPV_ARGS(&textures[i])));Check(textures[i].As(&locks[i]));}
 D3D11_TEXTURE2D_DESC td{};textures[0]->GetDesc(&td);td.Usage=D3D11_USAGE_STAGING;td.BindFlags=td.MiscFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;ComPtr<ID3D11Texture2D> staging;Check(device->CreateTexture2D(&td,nullptr,&staging));
 std::array<std::vector<DWORD>,3> images;std::array<shared::SharedTexturePixels,3> frames;
 for(unsigned i=0;i<3;++i){images[i].resize(size_t(pitch)*h);frames[i]={images[i].data(),pitch*4,w,h,r.format};}
 const auto start=GetTickCount64();SIZE_T firstFrameBytes=0;
 for(unsigned f=1;f<=120;++f){
  for(unsigned i=0;i<3;++i){
   std::fill(images[i].begin(),images[i].end(),0x12345678u); // padding must not become image rows
   for(UINT y=0;y<h;++y)for(UINT x=0;x<w;++x)
    images[i][size_t(y)*pitch+x]=(i==2?0x80000000u:0xff000000u)|(f<<16)|((i+1)<<12)|((x+y)&0xfff);
  }
  if(!producer.PublishFrame(frames))return 3;
  for(unsigned i=0;i<3;++i){if(locks[i]->AcquireSync(1,1000)!=S_OK)return 4;context->CopyResource(staging.Get(),textures[i].Get());D3D11_MAPPED_SUBRESOURCE m{};Check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&m));
   for(UINT y=0;y<h;++y){const DWORD* row=reinterpret_cast<const DWORD*>(static_cast<const BYTE*>(m.pData)+size_t(y)*m.RowPitch);for(UINT x=0;x<w;++x)if(row[x]!=images[i][size_t(y)*pitch+x]){puts("FAIL pixel content/alpha/pitch");return 5;}}
   context->Unmap(staging.Get(),0);Check(locks[i]->ReleaseSync(0));
  }
  if(f==1)firstFrameBytes=Memory(f);
  else if(f%20==0 && Memory(f)>firstFrameBytes+256ull*1024*1024){puts("FAIL unbounded CPU upload memory growth");return 6;}
 }
 printf("120 complete CPU publications: %llu ms; current color, alpha, row pitch and ownership passed\n",GetTickCount64()-start);return 0;
}
