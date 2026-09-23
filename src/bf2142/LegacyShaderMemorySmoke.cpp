#include "LegacyShaderMemory.h"
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <cstdint>
#include <cstdarg>
struct Buffer : IUnknown { virtual void* WINAPI GetBufferPointer()=0; virtual DWORD WINAPI GetBufferSize()=0; };
using CreateEffect=HRESULT(WINAPI*)(IDirect3DDevice9*,const void*,UINT,const void*,void*,DWORD,void*,IUnknown**,Buffer**);
void Log(const char* fmt,...) {va_list ap;va_start(ap,fmt);vprintf(fmt,ap);va_end(ap);puts("");fflush(stdout);}
HRESULT Invoke(CreateEffect create,IDirect3DDevice9* device,const char* text,UINT bytes,IUnknown** effect,Buffer** errors) {
    __try { return create(device,text,bytes,nullptr,nullptr,0x100,nullptr,effect,errors); }
    __except(EXCEPTION_EXECUTE_HANDLER) {return HRESULT_FROM_WIN32(ERROR_NOACCESS);}
}
int wmain(int argc,wchar_t** argv) {
    if(argc<3)return 2;
    const bool fix=wcscmp(argv[2],L"fixed")==0;
    auto dll=LoadLibraryW(argv[1]);if(!dll)return 3;
    if(fix && !bfvr::bf2142::InstallLegacyShaderMemory(dll,Log))return 4;
    auto create=reinterpret_cast<CreateEffect>(GetProcAddress(dll,"D3DXCreateEffect"));if(!create)return 5;
    auto window=CreateWindowW(L"STATIC",L"Shader memory stress",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    auto d3d=Direct3DCreate9(D3D_SDK_VERSION);IDirect3DDevice9* device=nullptr;
    D3DPRESENT_PARAMETERS pp{};pp.Windowed=TRUE;pp.hDeviceWindow=window;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.BackBufferWidth=64;pp.BackBufferHeight=64;
    if(!d3d || FAILED(d3d->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&device)))return 6;
    const char* source="float4 tint=float4(0.3,0.7,0.4,1); float4 ps():COLOR0{return tint;} technique main {pass p {PixelShader=compile ps_2_0 ps();}}";
    IUnknown* effect=nullptr;Buffer* errors=nullptr;
    HRESULT hr=Invoke(create,device,source,static_cast<UINT>(strlen(source)),&effect,&errors);
    Log("baseline hr=%08lX effect=%p",hr,effect);if(FAILED(hr))return 7;
    effect->Release();if(errors)errors->Release();effect=nullptr;errors=nullptr;
    std::vector<std::string> inputs;
    bool batch=argc>3 && std::filesystem::is_directory(argv[3]);
    const auto read=[&](const std::filesystem::path& path){std::ifstream file(path,std::ios::binary);inputs.emplace_back(std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>());return !inputs.back().empty();};
    if(batch){for(const auto& entry:std::filesystem::directory_iterator(argv[3]))if(entry.path().extension()==L".fxo" && !read(entry.path()))return 8;}
    else if(argc>3){if(!read(argv[3]))return 8;}
    else inputs.emplace_back(source);
    if(inputs.empty())return 8;
    std::vector<IUnknown*> held;held.reserve(inputs.size());
    // Exhaust free low virtual ranges, not physical RAM. Heap and driver may
    // still use high memory exactly as in the LAA BF2142 crash dumps.
    std::vector<void*> reservations;reservations.reserve(32768);
    size_t bytes=0;uintptr_t address=0x10000;
    while(address<0x80000000) {
        MEMORY_BASIC_INFORMATION mbi{};
        if(!VirtualQuery(reinterpret_cast<void*>(address),&mbi,sizeof(mbi)))break;
        uintptr_t end=(std::min<uintptr_t>)(uintptr_t(0x80000000),reinterpret_cast<uintptr_t>(mbi.BaseAddress)+mbi.RegionSize);
        if(mbi.State==MEM_FREE) {
            uintptr_t start=(address+65535)&~uintptr_t(65535);
            if(end>start){void* p=VirtualAlloc(reinterpret_cast<void*>(start),end-start,MEM_RESERVE,PAGE_NOACCESS);if(p){reservations.push_back(p);bytes+=end-start;}}
        }
        if(end<=address)break;address=end;
    }
    Log("pressure reserved %zu MiB below 2 GiB",bytes/(1024*1024));
    auto crt=GetModuleHandleW(L"msvcrt.dll");
    auto alloc=reinterpret_cast<void*(__cdecl*)(size_t)>(GetProcAddress(crt,"malloc"));
    auto release=reinterpret_cast<void(__cdecl*)(void*)>(GetProcAddress(crt,"free"));
    std::vector<void*> heapPressure;heapPressure.reserve(100000);
    bool high=false;
    for(int i=0;i<100000;++i){void* p=alloc(256);if(!p)break;heapPressure.push_back(p);if(reinterpret_cast<uintptr_t>(p)>=0x80000000){high=true;if(i>4096)break;}}
    Log("CRT heap pressure blocks=%zu high=%d",heapPressure.size(),high);
    bool ok=true;
    for(int round=0;round<(batch?2:20) && ok;++round) {
        for(size_t i=0;i<inputs.size();++i){
            effect=nullptr;errors=nullptr;const auto& input=inputs[i];
            hr=Invoke(create,device,input.data(),static_cast<UINT>(input.size()),&effect,&errors);
            if(!batch || FAILED(hr))Log("compile %d/%zu hr=%08lX effect=%p",round,i,hr,effect);
            if(errors){Log("errors: %s",static_cast<const char*>(errors->GetBufferPointer()));errors->Release();}
            if(effect)held.push_back(effect);
            if(FAILED(hr)){ok=false;break;}
        }
        Log("round %d held %zu effects",round,held.size());
        for(auto* e:held)e->Release();held.clear();
    }
    auto stats=bfvr::bf2142::GetLegacyShaderMemoryStats();
    Log("arena active=%zu peak=%zu allocations=%zu failures=%zu",stats.active,stats.peak,stats.allocations,stats.failures);
    for(void* p:heapPressure)release(p);
    for(void* p:reservations)VirtualFree(p,0,MEM_RELEASE);
    device->Release();d3d->Release();DestroyWindow(window);
    return ok?0:1;
}
