#include "GpuFrameTransfer.h"
#include <dxgi1_2.h>
namespace bfvr::bf2142 {
using Microsoft::WRL::ComPtr;
namespace {
void Transition(ID3D12GraphicsCommandList* commands,ID3D12Resource* resource,
    D3D12_RESOURCE_STATES from,D3D12_RESOURCE_STATES to){
    D3D12_RESOURCE_BARRIER barrier{};barrier.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition={resource,D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,from,to};
    commands->ResourceBarrier(1,&barrier);
}
}
namespace {GpuDiagnosticLog gpuLog=nullptr;}
void ConfigureGpuDiagnostics(GpuDiagnosticLog log){
    wchar_t value[8]{};gpuLog=GetEnvironmentVariableW(L"BF2142VR_GPU_DEBUG",value,8)==1&&value[0]==L'1'?log:nullptr;
}
bool DiagnosticCpuTransfer(){
    // Opt-in, process-scoped diagnostic: compare transfer paths without reloading a map.
    struct Control {
        HANDLE event=nullptr;
        Control(){
            wchar_t value[8]{};
            if(GetEnvironmentVariableW(L"BF2142VR_GPU_DEBUG",value,8)!=1||value[0]!=L'1')return;
            wchar_t name[96]{};swprintf_s(name,L"Local\\BF2142VR.CpuTransfer.%lu",GetCurrentProcessId());
            event=CreateEventW(nullptr,TRUE,FALSE,name);
        }
        ~Control(){if(event)CloseHandle(event);}
    };
    static Control control;
    return control.event&&WaitForSingleObject(control.event,0)==WAIT_OBJECT_0;
}
void ReportGpuDrawFailure(IDirect3DDevice9* device){
    if(!gpuLog||!device)return;
    DWORD passes=0,fvf=0;const auto validation=device->ValidateDevice(&passes);device->GetFVF(&fvf);
    gpuLog("GPU draw failure state: validation=0x%08lX passes=%lu fvf=%lu.",validation,passes,fvf);
    for(unsigned i=0;i<4;++i){
        ComPtr<IDirect3DSurface9> target;D3DSURFACE_DESC d{};const auto hr=device->GetRenderTarget(i,&target);
        if(target)target->GetDesc(&d);
        gpuLog("GPU target %u: hr=0x%08lX %ux%u format=%u samples=%u.",i,hr,d.Width,d.Height,unsigned(d.Format),unsigned(d.MultiSampleType));
    }
    for(unsigned i=0;i<8;++i){
        DWORD color=0,alpha=0,index=0,transform=0;UINT frequency=0;
        device->GetTextureStageState(i,D3DTSS_COLOROP,&color);device->GetTextureStageState(i,D3DTSS_ALPHAOP,&alpha);
        device->GetTextureStageState(i,D3DTSS_TEXCOORDINDEX,&index);device->GetTextureStageState(i,D3DTSS_TEXTURETRANSFORMFLAGS,&transform);
        device->GetStreamSourceFreq(i,&frequency);
        gpuLog("GPU stage %u: color=%lu alpha=%lu coords=%lu transform=%lu stream-frequency=%u.",i,color,alpha,index,transform,frequency);
    }
    for(auto type:{D3DRS_FILLMODE,D3DRS_CLIPPING,D3DRS_CLIPPLANEENABLE,D3DRS_VERTEXBLEND,D3DRS_INDEXEDVERTEXBLENDENABLE,D3DRS_POINTSPRITEENABLE,D3DRS_MULTISAMPLEANTIALIAS,D3DRS_MULTISAMPLEMASK}){
        DWORD value=0;const auto hr=device->GetRenderState(type,&value);gpuLog("GPU state %u: hr=0x%08lX value=0x%08lX.",unsigned(type),hr,value);
    }
}
bool NativeExTransferRequested(){
    wchar_t value[16]{};
    return GetEnvironmentVariableW(L"BF2142VR_GPU_TRANSFER",value,16)==5&&wcscmp(value,L"dx9ex")==0;
}
bool GpuTransferRequested(){
    wchar_t value[8]{};
    return NativeExTransferRequested()||(GetEnvironmentVariableW(L"BF2142VR_GPU_TRANSFER",value,8)==1&&value[0]==L'1');
}
IDirect3D9* CreateGpuTransferFactory(UINT version){
    // Do not load a replacement from the game directory or change system policy.
    static const HMODULE runtime=LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(NativeExTransferRequested()){
        using CreateEx=HRESULT(WINAPI*)(UINT,IDirect3D9Ex**);
        const auto createEx=runtime?reinterpret_cast<CreateEx>(GetProcAddress(runtime,"Direct3DCreate9Ex")):nullptr;
        IDirect3D9Ex* factory=nullptr;
        return createEx&&SUCCEEDED(createEx(version,&factory))?factory:nullptr;
    }
    const auto create=runtime?reinterpret_cast<PFN_Direct3DCreate9On12>(GetProcAddress(runtime,"Direct3DCreate9On12")):nullptr;
    if(!create)return nullptr;
    D3D9ON12_ARGS options{};options.Enable9On12=TRUE;
    return create(version,&options,1);
}
HRESULT GpuFrameTransfer::Initialize(IDirect3DDevice9* source,UINT width,UINT height,ID3D11Device* destination){
    Reset();
    if(!source||!width||!height||width>8192||height>8192)return E_INVALIDARG;
    if(SUCCEEDED(source->QueryInterface(IID_PPV_ARGS(&nativeEx))))return InitializeNativeEx(source,width,height,destination);
    auto hr=source->QueryInterface(IID_PPV_ARGS(&interop));
    if(FAILED(hr))return hr;
    device=source;
    for(unsigned i=0;i<3;++i){
        hr=source->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&textures[i],nullptr);
        if(SUCCEEDED(hr))hr=textures[i]->GetSurfaceLevel(0,&surfaces[i]);
        if(FAILED(hr)){Reset();return hr;}
    }
    // Desktop checks can display the D3D9 snapshots directly without export.
    if(!destination)return S_OK;
    hr=interop->GetD3D12Device(IID_PPV_ARGS(&device12));
    ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;DXGI_ADAPTER_DESC adapterDesc{};
    if(SUCCEEDED(hr))hr=destination->QueryInterface(IID_PPV_ARGS(&dxgi));
    if(SUCCEEDED(hr))hr=dxgi->GetAdapter(&adapter);
    if(SUCCEEDED(hr))hr=adapter->GetDesc(&adapterDesc);
    if(SUCCEEDED(hr)){
        const auto luid=device12->GetAdapterLuid();
        if(luid.LowPart!=adapterDesc.AdapterLuid.LowPart||luid.HighPart!=adapterDesc.AdapterLuid.HighPart)hr=DXGI_ERROR_UNSUPPORTED;
    }
    if(FAILED(hr)){Reset();return hr;}
    D3D12_COMMAND_QUEUE_DESC q{};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    hr=device12->CreateCommandQueue(&q,IID_PPV_ARGS(&queue));
    if(SUCCEEDED(hr))hr=device12->CreateCommandAllocator(q.Type,IID_PPV_ARGS(&allocator));
    if(SUCCEEDED(hr))hr=device12->CreateCommandList(0,q.Type,allocator.Get(),nullptr,IID_PPV_ARGS(&commands));
    if(SUCCEEDED(hr))hr=commands->Close();
    if(SUCCEEDED(hr))hr=device12->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence));
    if(SUCCEEDED(hr)){event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event)hr=HRESULT_FROM_WIN32(GetLastError());}
    ComPtr<ID3D11Device1> receiver;
    if(SUCCEEDED(hr))hr=destination->QueryInterface(IID_PPV_ARGS(&receiver));
    for(unsigned i=0;SUCCEEDED(hr)&&i<3;++i){
        D3D12_HEAP_PROPERTIES heap{};heap.Type=D3D12_HEAP_TYPE_DEFAULT;heap.CreationNodeMask=heap.VisibleNodeMask=1;
        D3D12_RESOURCE_DESC desc{};desc.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.Width=width;desc.Height=height;desc.DepthOrArraySize=desc.MipLevels=1;
        desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;desc.SampleDesc.Count=1;
        desc.Flags=D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS|D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        hr=device12->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_SHARED,&desc,D3D12_RESOURCE_STATE_COMMON,nullptr,IID_PPV_ARGS(&shared[i]));
        HANDLE handle=nullptr;
        if(SUCCEEDED(hr))hr=device12->CreateSharedHandle(shared[i].Get(),nullptr,GENERIC_ALL,nullptr,&handle);
        if(SUCCEEDED(hr))hr=receiver->OpenSharedResource1(handle,IID_PPV_ARGS(&opened[i]));
        if(handle)CloseHandle(handle);
    }
    if(FAILED(hr))Reset();
    return hr;
}
HRESULT GpuFrameTransfer::InitializeNativeEx(IDirect3DDevice9* source,UINT width,UINT height,ID3D11Device* destination){
    device=source;HRESULT hr=S_OK;
    if(destination){
        ComPtr<IDirect3D9> api;ComPtr<IDirect3D9Ex> apiEx;D3DDEVICE_CREATION_PARAMETERS creation{};LUID luid{};
        ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;DXGI_ADAPTER_DESC desc{};
        hr=source->GetDirect3D(&api);
        if(SUCCEEDED(hr))hr=api.As(&apiEx);
        if(SUCCEEDED(hr))hr=source->GetCreationParameters(&creation);
        if(SUCCEEDED(hr))hr=apiEx->GetAdapterLUID(creation.AdapterOrdinal,&luid);
        if(SUCCEEDED(hr))hr=destination->QueryInterface(IID_PPV_ARGS(&dxgi));
        if(SUCCEEDED(hr))hr=dxgi->GetAdapter(&adapter);
        if(SUCCEEDED(hr))hr=adapter->GetDesc(&desc);
        if(SUCCEEDED(hr)&&(luid.LowPart!=desc.AdapterLuid.LowPart||luid.HighPart!=desc.AdapterLuid.HighPart))hr=DXGI_ERROR_UNSUPPORTED;
    }
    for(unsigned i=0;SUCCEEDED(hr)&&i<3;++i){
        HANDLE handle=nullptr;
        hr=source->CreateTexture(width,height,1,D3DUSAGE_RENDERTARGET,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&textures[i],destination?&handle:nullptr);
        if(SUCCEEDED(hr))hr=textures[i]->GetSurfaceLevel(0,&surfaces[i]);
        if(SUCCEEDED(hr)&&destination)hr=destination->OpenSharedResource(handle,IID_PPV_ARGS(&opened[i]));
        // D3D9 shared handles are non-NT handles owned by the resource; do not CloseHandle.
    }
    if(SUCCEEDED(hr)&&destination)hr=source->CreateQuery(D3DQUERYTYPE_EVENT,&completion9);
    if(FAILED(hr))Reset();return hr;
}
HRESULT GpuFrameTransfer::Stage(unsigned slot,IDirect3DSurface9* source){
    if(!device||slot>=3||!surfaces[slot])return E_INVALIDARG;
    // Starting an eye pair cannot accidentally reuse a prior HUD/eye snapshot.
    if(slot==0)staged=0;
    HRESULT hr=S_OK;
    if(source){
        D3DSURFACE_DESC desc{},expected{};
        hr=source->GetDesc(&desc);
        if(SUCCEEDED(hr))hr=surfaces[slot]->GetDesc(&expected);
        if(SUCCEEDED(hr)&&(desc.Width!=expected.Width||desc.Height!=expected.Height||
            (desc.Format!=D3DFMT_A8R8G8B8&&desc.Format!=D3DFMT_X8R8G8B8)))hr=E_INVALIDARG;
        if(SUCCEEDED(hr))hr=device->StretchRect(source,nullptr,surfaces[slot].Get(),nullptr,D3DTEXF_NONE);
    }else hr=device->ColorFill(surfaces[slot].Get(),nullptr,slot==2?0:0xff000000);
    if(SUCCEEDED(hr))staged|=1u<<slot;
    return hr;
}
HRESULT GpuFrameTransfer::StageBackbuffer(unsigned slot){
    if(!device)return E_UNEXPECTED;
    ComPtr<IDirect3DSurface9> back;
    auto hr=device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back);
    return SUCCEEDED(hr)?Stage(slot,back.Get()):hr;
}
HRESULT GpuFrameTransfer::Wait(){
    if(!fence||!sequence)return S_OK;
    const auto complete=fence->GetCompletedValue();
    if(complete==UINT64_MAX)return DXGI_ERROR_DEVICE_REMOVED;
    if(complete>=sequence)return S_OK;
    auto hr=fence->SetEventOnCompletion(sequence,event);
    if(SUCCEEDED(hr)&&WaitForSingleObject(event,1000)!=WAIT_OBJECT_0)hr=HRESULT_FROM_WIN32(ERROR_TIMEOUT);
    return hr;
}
HRESULT GpuFrameTransfer::Export(){
    if(staged!=7)return E_UNEXPECTED;
    if(nativeEx){
        if(!completion9)return E_UNEXPECTED;
        auto hr=completion9->Issue(D3DISSUE_END);if(FAILED(hr))return hr;
        const auto started=GetTickCount64();
        while((hr=completion9->GetData(nullptr,0,D3DGETDATA_FLUSH))==S_FALSE){
            if(GetTickCount64()-started>=1000)return HRESULT_FROM_WIN32(ERROR_TIMEOUT);
            SwitchToThread();
        }
        return hr;
    }
    if(!commands||!interop)return E_UNEXPECTED;
    auto hr=Wait();
    if(SUCCEEDED(hr))hr=allocator->Reset();
    if(SUCCEEDED(hr))hr=commands->Reset(allocator.Get(),nullptr);
    if(FAILED(hr))return hr;
    std::array<ComPtr<ID3D12Resource>,3> source;
    unsigned count=0;
    for(;count<3;++count){
        hr=interop->UnwrapUnderlyingResource(surfaces[count].Get(),queue.Get(),IID_PPV_ARGS(&source[count]));
        if(FAILED(hr))break;
        Transition(commands.Get(),source[count].Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_SOURCE);
        Transition(commands.Get(),shared[count].Get(),D3D12_RESOURCE_STATE_COMMON,D3D12_RESOURCE_STATE_COPY_DEST);
        commands->CopyResource(shared[count].Get(),source[count].Get());
        Transition(commands.Get(),source[count].Get(),D3D12_RESOURCE_STATE_COPY_SOURCE,D3D12_RESOURCE_STATE_COMMON);
        Transition(commands.Get(),shared[count].Get(),D3D12_RESOURCE_STATE_COPY_DEST,D3D12_RESOURCE_STATE_COMMON);
    }
    const auto close=commands->Close();if(SUCCEEDED(hr))hr=close;
    bool submitted=false;
    if(SUCCEEDED(hr)){
        ID3D12CommandList* lists[]={commands.Get()};queue->ExecuteCommandLists(1,lists);
        submitted=true;++sequence;hr=queue->Signal(fence.Get(),sequence);
    }
    for(unsigned i=0;i<count;++i){
        ID3D12Fence* completion=fence.Get();
        const auto returned=interop->ReturnUnderlyingResource(surfaces[i].Get(),submitted?1:0,
            submitted?&sequence:nullptr,submitted?&completion:nullptr);
        if(SUCCEEDED(hr))hr=returned;
    }
    if(SUCCEEDED(hr))hr=Wait();
    return hr;
}
void GpuFrameTransfer::Reset(){
    (void)Wait();
    opened={};shared={};commands.Reset();allocator.Reset();queue.Reset();fence.Reset();device12.Reset();
    if(event)CloseHandle(event);event=nullptr;sequence=0;
    surfaces={};textures={};completion9.Reset();nativeEx.Reset();interop.Reset();device=nullptr;staged=0;
}
}
