#include "GpuFrameTransfer.h"
#include "NativeExResources.h"
#include <MinHook.h>
#include "presenter/SharedTextureProducer.h"
#include <dxgi1_2.h>
#include <cstdio>
#include <cstdlib>
#include <vector>
using Microsoft::WRL::ComPtr;
using namespace bfvr;
void Check(HRESULT hr,const char* name){if(FAILED(hr)){printf("FAIL %s: %08lx\n",name,hr);exit(1);}}
void Require(bool value,const char* name){if(!value){printf("FAIL %s\n",name);exit(1);}}
void Log(void*,const wchar_t* text){wprintf(L"%ls\n",text);}
int wmain(int argc,wchar_t** argv){
    bool full=false,nativeEx=false;for(int i=1;i<argc;++i){full|=wcscmp(argv[i],L"--full-size")==0;nativeEx|=wcscmp(argv[i],L"--native-ex")==0;}
    if(nativeEx)SetEnvironmentVariableW(L"BF2142VR_GPU_TRANSFER",L"dx9ex");
    const UINT w=full?2528:320,h=full?2704:240;
    HWND window=CreateWindowExW(0,L"STATIC",L"Hidden GPU publication check",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Require(window!=nullptr,"window");
    ComPtr<IDirect3D9> api;api.Attach(bf2142::CreateGpuTransferFactory(D3D_SDK_VERSION));Require(bool(api),"9on12 factory");
    D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.hDeviceWindow=window;p.BackBufferWidth=w;p.BackBufferHeight=h;p.BackBufferFormat=D3DFMT_X8R8G8B8;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.MultiSampleType=D3DMULTISAMPLE_8_SAMPLES;p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    ComPtr<IDirect3DDevice9> d;ComPtr<IDirect3DDevice9Ex> deviceEx;LUID adapterLuid{};
    if(nativeEx){
        ComPtr<IDirect3D9Ex> apiEx;Check(api.As(&apiEx),"Ex factory");
        Check(apiEx->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,nullptr,&deviceEx),"native Ex device");d=deviceEx;
        Check(apiEx->GetAdapterLUID(0,&adapterLuid),"native adapter");
        Require(MH_Initialize()==MH_OK&&bf2142::InstallNativeExResources(d.Get(),nullptr),"managed compatibility");
    }else{
        Check(api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,&d),"device");
        ComPtr<IDirect3DDevice9On12> interop;Check(d.As(&interop),"interop");
        ComPtr<ID3D12Device> d12;Check(interop->GetD3D12Device(IID_PPV_ARGS(&d12)),"underlying device");adapterLuid=d12->GetAdapterLuid();
    }
    shared::SharedTextureRequirements requirements{};
    requirements.adapterLuid=adapterLuid;requirements.format=DXGI_FORMAT_B8G8R8A8_UNORM;
    requirements.leftWorldWidth=requirements.rightWorldWidth=requirements.uiWidth=w;
    requirements.leftWorldHeight=requirements.rightWorldHeight=requirements.uiHeight=h;
    shared::SharedTextureProducer producer;
    const auto name=L"Local\\BF2142GpuPublication-"+std::to_wstring(GetCurrentProcessId());
    Require(producer.Initialize(name.c_str(),requirements,Log,nullptr),"producer initialization");
    ComPtr<IDXGIDevice> dxgi;Check(producer.Device()->QueryInterface(IID_PPV_ARGS(&dxgi)),"producer adapter query");
    ComPtr<IDXGIAdapter> adapter;Check(dxgi->GetAdapter(&adapter),"adapter");
    ComPtr<ID3D11Device> receiver;ComPtr<ID3D11DeviceContext> context;
    Check(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&receiver,nullptr,&context),"independent receiver");
    ComPtr<ID3D11Device1> receiver1;Check(receiver.As(&receiver1),"receiver1");
    std::array<shared::SharedTextureDescription,3> descriptions;producer.CopyDescriptions(descriptions.data(),descriptions.size());
    std::array<ComPtr<ID3D11Texture2D>,3> opened;
    std::array<ComPtr<IDXGIKeyedMutex>,3> mutex;
    for(unsigned i=0;i<3;++i){
        Check(receiver1->OpenSharedResourceByName(descriptions[i].name,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,IID_PPV_ARGS(&opened[i])),"receiver open");
        Check(opened[i].As(&mutex[i]),"ownership mutex");
    }
    D3D11_TEXTURE2D_DESC desc{};opened[0]->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    ComPtr<ID3D11Texture2D> staging;Check(receiver->CreateTexture2D(&desc,nullptr,&staging),"verification staging");
    ComPtr<IDirect3DTexture9> managed;Check(d->CreateTexture(8,8,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&managed,nullptr),"managed resource");
    D3DLOCKED_RECT lock{};Check(managed->LockRect(0,&lock,nullptr,0),"managed lock");*static_cast<DWORD*>(lock.pBits)=0xabcdef01;Check(managed->UnlockRect(0),"managed unlock");
    bf2142::GpuFrameTransfer gpu;
    for(unsigned reset=0;reset<2;++reset){
        Check(gpu.Initialize(d.Get(),w,h,producer.Device()),"bridge initialization");
        Require(FAILED(gpu.Export()),"incomplete frame rejected");
        Require(FAILED(gpu.Stage(3,nullptr)),"invalid slot rejected");
        Require(!producer.PublishGpuFrame({}),"invalid publication rejected without taking ownership");
        ComPtr<IDirect3DSurface9> back,hud;Check(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back),"back");
        Check(d->CreateRenderTarget(w,h,D3DFMT_A8R8G8B8,p.MultiSampleType,0,FALSE,&hud,nullptr),"alpha HUD");
        for(unsigned frame=0;frame<6;++frame){
            std::array<DWORD,3> color{};
            for(unsigned i=0;i<3;++i){
                color[i]=(i==2?0x80000000:0xff000000)|((reset*6+frame+1)<<16)|((i+1)<<8)|0x56;
                auto* source=i==2?hud.Get():back.Get();Check(d->SetRenderTarget(0,source),"target");
                Check(d->Clear(0,nullptr,D3DCLEAR_TARGET,color[i],1,0),"clear");
                // Missing HUD must clear stale alpha from the preceding frame.
                if(frame==4&&i==2){color[i]=0;Check(gpu.Stage(i,nullptr),"absent UI");}
                else Check(gpu.Stage(i,source),"stage");
            }
            if(frame==2){
                // Existing CPU publications and new GPU publications alternate
                // on the same keyed ownership protocol without stale frames.
                std::array<std::vector<DWORD>,3> image;
                std::array<shared::SharedTexturePixels,3> pixels;
                for(unsigned i=0;i<3;++i){image[i].assign(size_t(w)*h,color[i]);pixels[i]={image[i].data(),w*4,w,h,requirements.format};}
                Require(producer.PublishFrame(pixels),"CPU fallback publication");
            }else {Check(gpu.Export(),"GPU export");Require(producer.PublishGpuFrame(gpu.Exported()),"GPU publication");}
            for(unsigned i=0;i<3;++i){
                Require(mutex[i]->AcquireSync(1,1000)==S_OK,"receiver ownership");
                context->CopyResource(staging.Get(),opened[i].Get());D3D11_MAPPED_SUBRESOURCE map{};
                Check(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map),"receiver completion");
                for(UINT y=0;y<h;++y){const auto* row=reinterpret_cast<DWORD*>(static_cast<BYTE*>(map.pData)+size_t(y)*map.RowPitch);
                    for(UINT x=0;x<w;++x)Require(row[x]==color[i],"current-frame exact pixels including alpha");}
                context->Unmap(staging.Get(),0);Check(mutex[i]->ReleaseSync(0),"release receiver");
            }
        }
        Check(d->SetRenderTarget(0,back.Get()),"restore backbuffer");hud.Reset();back.Reset();gpu.Reset();
        Check(deviceEx?deviceEx->ResetEx(&p,nullptr):d->Reset(&p),"D3D9 reset after shared resources released");
        Check(managed->LockRect(0,&lock,nullptr,D3DLOCK_READONLY),"managed lock after reset");
        Require(*static_cast<DWORD*>(lock.pBits)==0xabcdef01,"managed content survived reset");Check(managed->UnlockRect(0),"unlock");
    }
    printf("GPU publication %ux%u: separate-device consumer, alpha, CPU/GPU alternation, stale UI clearing, reset and managed lifetime passed.\n",w,h);
    DestroyWindow(window);return 0;
}
