// Unreleased standalone GPU transfer experiment. Not linked into the player runtime.
// The final staging read verifies pixels outside the timing interval.
#include <windows.h>
#include <d3d9on12.h>
#include <d3d11_4.h>
#include <dxgi1_4.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <cstring>
#include <array>
using Microsoft::WRL::ComPtr;
void Check(HRESULT hr,const char* step){if(FAILED(hr)){printf("FAIL %s=%08lx\n",step,hr);exit(2);}}
double Now(){return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();}
void Wait(ID3D12Fence* f,UINT64 value,HANDLE event){
 if(f->GetCompletedValue()<value){Check(f->SetEventOnCompletion(value,event),"wait event");if(WaitForSingleObject(event,3000)!=WAIT_OBJECT_0){puts("FAIL fence timeout");exit(3);}}
}
void Barrier(ID3D12GraphicsCommandList* list,ID3D12Resource* resource,D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
 D3D12_RESOURCE_BARRIER b{};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};list->ResourceBarrier(1,&b);
}
int main(){
 setvbuf(stdout,nullptr,_IONBF,0);
 WNDCLASSW cls{};cls.lpfnWndProc=DefWindowProcW;cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName=L"On12SharingProbe";RegisterClassW(&cls);
 HWND window=CreateWindowW(cls.lpszClassName,L"Hidden 9on12 transfer probe",WS_OVERLAPPEDWINDOW,0,0,800,450,nullptr,nullptr,cls.hInstance,nullptr);
 auto factory=reinterpret_cast<PFN_Direct3DCreate9On12>(GetProcAddress(LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32),"Direct3DCreate9On12"));if(!factory)return 1;
 D3D9ON12_ARGS args{};args.Enable9On12=TRUE;
 ComPtr<IDirect3D9> api;api.Attach(factory(D3D_SDK_VERSION,&args,1));if(!api)return 1;
 for(bool big:{false,true}){
  unsigned w=big?2528:1600,h=big?2704:900;
  D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.hDeviceWindow=window;p.BackBufferWidth=w;p.BackBufferHeight=h;p.BackBufferFormat=D3DFMT_X8R8G8B8;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.MultiSampleType=D3DMULTISAMPLE_8_SAMPLES;p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
  ComPtr<IDirect3DDevice9> d;Check(api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,&d),"9on12 device");
  // Ordinary D3D9 resource lifetime/locking must work. Do not silently use Ex.
  ComPtr<IDirect3DTexture9> managed;ComPtr<IDirect3DVertexBuffer9> vb;ComPtr<IDirect3DIndexBuffer9> ib;
  Check(d->CreateTexture(32,32,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&managed,nullptr),"managed texture");
  D3DLOCKED_RECT lock{};Check(managed->LockRect(0,&lock,nullptr,0),"managed texture lock");memset(lock.pBits,0x57,lock.Pitch*32);Check(managed->UnlockRect(0),"texture unlock");
  Check(d->CreateVertexBuffer(128,0,0,D3DPOOL_MANAGED,&vb,nullptr),"managed vertices");
  Check(d->CreateIndexBuffer(12,0,D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr),"managed indices");
  void* data=nullptr;Check(vb->Lock(0,128,&data,0),"vertex lock");memset(data,0,128);Check(vb->Unlock(),"vertex unlock");
  Check(ib->Lock(0,12,&data,0),"index lock");memset(data,0,12);Check(ib->Unlock(),"index unlock");
  ComPtr<IDirect3DDevice9On12> interop;Check(d.As(&interop),"9on12 interface");
  ComPtr<ID3D12Device> d12;Check(interop->GetD3D12Device(IID_PPV_ARGS(&d12)),"12 device");
  ComPtr<ID3D12CommandQueue> queue;D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;Check(d12->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"queue");
  ComPtr<ID3D12CommandAllocator> allocator;Check(d12->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&allocator)),"allocator");
  ComPtr<ID3D12GraphicsCommandList> list;Check(d12->CreateCommandList(0,q.Type,allocator.Get(),nullptr,IID_PPV_ARGS(&list)),"list");Check(list->Close(),"close");
  ComPtr<ID3D12Fence> fence;Check(d12->CreateFence(0,D3D12_FENCE_FLAG_SHARED,IID_PPV_ARGS(&fence)),"fence");
  HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
  ComPtr<IDXGIFactory4> dxgi;Check(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi)),"factory");
  ComPtr<IDXGIAdapter1> adapter;Check(dxgi->EnumAdapterByLuid(d12->GetAdapterLuid(),IID_PPV_ARGS(&adapter)),"matching adapter");
  ComPtr<ID3D11Device> d11;ComPtr<ID3D11DeviceContext> context;
  Check(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&d11,nullptr,&context),"11 device");
  ComPtr<ID3D11Device5> d115;ComPtr<ID3D11DeviceContext4> context4;Check(d11.As(&d115),"device5");Check(context.As(&context4),"context4");
  HANDLE fenceHandle=nullptr;Check(d12->CreateSharedHandle(fence.Get(),nullptr,GENERIC_ALL,nullptr,&fenceHandle),"shared fence handle");
  ComPtr<ID3D11Fence> fence11;Check(d115->OpenSharedFence(fenceHandle,IID_PPV_ARGS(&fence11)),"11 fence");CloseHandle(fenceHandle);
  ComPtr<IDirect3DSurface9> back,hud;Check(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back),"backbuffer");
  Check(d->CreateRenderTarget(w,h,D3DFMT_A8R8G8B8,p.MultiSampleType,0,FALSE,&hud,nullptr),"hud");
  std::array<ComPtr<IDirect3DSurface9>,3> resolved;
  std::array<ComPtr<ID3D12Resource>,3> shared;
  std::array<ComPtr<ID3D11Texture2D>,3> opened;
  for(unsigned i=0;i<3;++i){
   Check(d->CreateRenderTarget(w,h,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&resolved[i],nullptr),"resolve target");
   D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
   D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;desc.Width=w;desc.Height=h;desc.DepthOrArraySize=desc.MipLevels=1;desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS|D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
   Check(d12->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_SHARED,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&shared[i])),"shared12 texture");
   HANDLE handle=nullptr;Check(d12->CreateSharedHandle(shared[i].Get(),nullptr,GENERIC_ALL,nullptr,&handle),"texture handle");
   Check(d115->OpenSharedResource1(handle,IID_PPV_ARGS(&opened[i])),"11 open texture");CloseHandle(handle);
  }
  ComPtr<ID3D11Texture2D> verify;D3D11_TEXTURE2D_DESC desc{};opened[0]->GetDesc(&desc);desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
  Check(d11->CreateTexture2D(&desc,nullptr,&verify),"verification staging");
  UINT64 value=0;double sum=0;
  for(unsigned frame=0;frame<30;++frame){
   const double start=Now();
   for(unsigned i=0;i<3;++i){
    auto* source=i==2?hud.Get():back.Get();Check(d->SetRenderTarget(0,source),"target");
    Check(d->Clear(0,nullptr,D3DCLEAR_TARGET,(i==2?0x80000000:0xff000000)|((frame+1)<<16)|((i+1)<<8)|0x56,1,0),"clear");
    Check(d->StretchRect(source,nullptr,resolved[i].Get(),nullptr,D3DTEXF_NONE),"resolve");
   }
   Check(allocator->Reset(),"reset allocator");Check(list->Reset(allocator.Get(),nullptr),"reset list");
   std::array<ComPtr<ID3D12Resource>,3> source;
   for(unsigned i=0;i<3;++i){
    Check(interop->UnwrapUnderlyingResource(resolved[i].Get(),queue.Get(),IID_PPV_ARGS(&source[i])),"unwrap");
    Barrier(list.Get(),source[i].Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
    Barrier(list.Get(),shared[i].Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
    list->CopyResource(shared[i].Get(),source[i].Get());
    Barrier(list.Get(),source[i].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
    Barrier(list.Get(),shared[i].Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COMMON);
   }
   Check(list->Close(),"close copy list");ID3D12CommandList* lists[]={list.Get()};queue->ExecuteCommandLists(1,lists);
   ++value;Check(queue->Signal(fence.Get(),value),"signal12");
   ID3D12Fence* f=fence.Get();for(unsigned i=0;i<3;++i)Check(interop->ReturnUnderlyingResource(resolved[i].Get(),1,&value,&f),"return");
   Wait(fence.Get(),value,event);const double end=Now();if(frame>=6)sum+=end-start;
   // Read ONLY to verify the probe; deliberately outside measured GPU transport.
   for(unsigned i=0;i<3;++i){
    context->CopyResource(verify.Get(),opened[i].Get());D3D11_MAPPED_SUBRESOURCE mapped{};Check(context->Map(verify.Get(),0,D3D11_MAP_READ,0,&mapped),"verification map");
    const DWORD expected=(i==2?0x80000000:0xff000000)|((frame+1)<<16)|((i+1)<<8)|0x56;
    for(unsigned y=0;y<h;++y){const auto* row=reinterpret_cast<const DWORD*>(static_cast<const BYTE*>(mapped.pData)+size_t(y)*mapped.RowPitch);for(unsigned x=0;x<w;++x)if(row[x]!=expected){printf("FAIL pixel %08lx != %08lx at %u,%u frame%u slot%u\n",row[x],expected,x,y,frame,i);return 4;}}
    context->Unmap(verify.Get(),0);
   }
  }
  printf("%ux%u 8xMSAA: three-image GPU transfer plus completion wait %.3f ms; managed resource locks and all current-frame pixels pass\n",w,h,sum/24);
  CloseHandle(event);
 }
 DestroyWindow(window);UnregisterClassW(cls.lpszClassName,cls.hInstance);
}
