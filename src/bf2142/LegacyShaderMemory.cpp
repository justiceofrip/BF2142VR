#include "LegacyShaderMemory.h"
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace bfvr::bf2142 {
namespace {
constexpr size_t granule=64*1024, capacity=64*1024*1024, slots=capacity/granule;
BYTE* arena=nullptr;
// 0 free, block length at start, 0xffff continuation. No heap allocation in hooks.
unsigned short blocks[slots]{};
SRWLOCK lock=SRWLOCK_INIT;
size_t active=0, peak=0, allocations=0, failures=0;
HMODULE installed=nullptr;
HANDLE shaderHeap=nullptr;uintptr_t heapBegin=0,heapEnd=0;
constexpr size_t heapCapacity=128*1024*1024;
using Malloc=void*(__cdecl*)(size_t);using CrtFree=void(__cdecl*)(void*);
using Size=size_t(__cdecl*)(void*);
CrtFree crtFree=nullptr;Size crtSize=nullptr;
bool OwnsHeap(void* p){auto a=reinterpret_cast<uintptr_t>(p);return a>=heapBegin && a<heapEnd;}

using Alloc=LPVOID(WINAPI*)(LPVOID,SIZE_T,DWORD,DWORD);
using Free=BOOL(WINAPI*)(LPVOID,SIZE_T,DWORD);
Alloc originalAlloc=VirtualAlloc; Free originalFree=VirtualFree;
bool Owns(const void* p) {
    const auto a=reinterpret_cast<uintptr_t>(p), b=reinterpret_cast<uintptr_t>(arena);
    return arena && a>=b && a-b<capacity;
}
void* WINAPI Allocate(void* address,SIZE_T size,DWORD flags,DWORD protect) {
    // Explicit commits to our reservation retain ordinary page semantics.
    if(address || !(flags&(MEM_RESERVE|MEM_COMMIT)) ||
        (flags&~(MEM_RESERVE|MEM_COMMIT|MEM_TOP_DOWN)))
        return originalAlloc(address,size,flags,protect);
    if(!size || size>capacity) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return nullptr; }
    const size_t needed=(size+granule-1)/granule;
    AcquireSRWLockExclusive(&lock);
    size_t run=0,start=0;
    for(size_t i=0;i<slots;++i) {
        if(blocks[i]) {run=0;continue;}
        if(!run)start=i;
        if(++run!=needed)continue;
        BYTE* result=arena+start*granule;
        if((flags&MEM_COMMIT) && !originalAlloc(result,size,MEM_COMMIT,protect))break;
        blocks[start]=static_cast<unsigned short>(needed);
        for(size_t j=1;j<needed;++j)blocks[start+j]=0xffff;
        active+=needed*granule;peak=(std::max)(peak,active);++allocations;
        ReleaseSRWLockExclusive(&lock);return result;
    }
    ++failures;ReleaseSRWLockExclusive(&lock);
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);return nullptr;
}
BOOL WINAPI Release(void* address,SIZE_T size,DWORD flags) {
    if(!Owns(address))return originalFree(address,size,flags);
    const size_t offset=static_cast<BYTE*>(address)-arena;
    AcquireSRWLockExclusive(&lock);
    size_t index=offset/granule;
    while(index && blocks[index]==0xffff)--index;
    const size_t count=blocks[index],start=index*granule,end=start+count*granule;
    BOOL ok=FALSE;
    if(!count || count==0xffff || (flags!=MEM_RELEASE && flags!=MEM_DECOMMIT)) {
        SetLastError(ERROR_INVALID_ADDRESS);
    } else if(flags==MEM_DECOMMIT) {
        // Zero-size decommit means this logical reservation, never the entire
        // backing arena (which may contain another live shader's pages).
        if(!size && offset==start)size=count*granule;
        if(size && size<=end-offset)ok=originalFree(address,size,MEM_DECOMMIT);
        else SetLastError(ERROR_INVALID_PARAMETER);
    } else if(size || offset!=start) {
        SetLastError(ERROR_INVALID_PARAMETER);
    } else {
        ok=originalFree(address,count*granule,MEM_DECOMMIT);
        if(ok){for(size_t j=0;j<count;++j)blocks[index+j]=0;active-=count*granule;}
    }
    ReleaseSRWLockExclusive(&lock);return ok;
}
void* __cdecl LowMalloc(size_t bytes) {
    if(bytes<0x70000)return HeapAlloc(shaderHeap,0,bytes?bytes:1);
    // Fixed Win32 heaps cannot allocate individual blocks >= 512 KiB.
    // Large D3DX buffers use our page arena with a size header.
    if(bytes>capacity-16){SetLastError(ERROR_NOT_ENOUGH_MEMORY);return nullptr;}
    auto* p=static_cast<BYTE*>(Allocate(nullptr,bytes+16,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    if(p){std::memcpy(p,&bytes,sizeof(bytes));return p+16;}return nullptr;
}
void __cdecl LowFree(void* p) {
    if(!p)return;
    if(OwnsHeap(p)){HeapFree(shaderHeap,0,p);return;}
    if(Owns(p)){Release(static_cast<BYTE*>(p)-16,0,MEM_RELEASE);return;}
    crtFree(p); // Objects allocated before installation keep their original owner.
}
void* __cdecl LowCalloc(size_t count,size_t bytes) {
    if(bytes && count>SIZE_MAX/bytes)return nullptr;
    size_t size=count*bytes;auto* p=LowMalloc(size);if(p)std::memset(p,0,size);return p;
}
void* __cdecl LowRealloc(void* p,size_t bytes) {
    if(!p)return LowMalloc(bytes);
    if(!bytes){LowFree(p);return nullptr;}
    size_t oldSize=0;
    if(OwnsHeap(p))oldSize=HeapSize(shaderHeap,0,p);
    else if(Owns(p))std::memcpy(&oldSize,static_cast<BYTE*>(p)-16,sizeof(oldSize));
    else oldSize=crtSize(p);
    if(oldSize==SIZE_MAX)return nullptr;
    auto* result=LowMalloc(bytes);if(result){std::memcpy(result,p,(std::min)(bytes,oldSize));LowFree(p);}return result;
}
char* __cdecl LowStrdup(const char* p) {
    if(!p)return nullptr;size_t size=std::strlen(p)+1;
    auto* result=static_cast<char*>(LowMalloc(size));if(result)std::memcpy(result,p,size);return result;
}
void* WINAPI LowHeapAlloc(HANDLE heap,DWORD flags,SIZE_T bytes) {
    if(heap!=GetProcessHeap() || flags&~HEAP_ZERO_MEMORY)return HeapAlloc(heap,flags,bytes);
    void* p=LowMalloc(bytes);if(p && (flags&HEAP_ZERO_MEMORY))std::memset(p,0,bytes);return p;
}
BOOL WINAPI LowHeapFree(HANDLE heap,DWORD flags,void* p) {
    if(OwnsHeap(p) || Owns(p)){LowFree(p);return TRUE;}return HeapFree(heap,flags,p);
}
bool Profile(BYTE* image) {
    __try {
        auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>4096)return false;
        auto* pe=reinterpret_cast<IMAGE_NT_HEADERS*>(image+dos->e_lfanew);
        if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 ||
           pe->OptionalHeader.SizeOfImage<0x241000)return false;
        // Verified compiler tagged-pointer decode and arena reserve/commit calls.
        const BYTE tag[]={0xf7,0xd0,0x80,0x78,0x14,0x03,0x75,0x38,0x8b,0x50,0x18};
        const BYTE reserve[]={0x68,0x00,0x20,0x00,0x00,0xff,0x75,0x0c,0x6a,0x00,0xff,0x15};
        const BYTE commit[]={0x85,0xc0,0x75,0x11,0x68,0x00,0x80,0x00,0x00,0x50,0xff,0x75,0xfc,0xff,0x15};
        DWORD slot=0;std::memcpy(&slot,image+0x208e29,4);
        return !std::memcmp(image+0xfb178,tag,sizeof(tag)) &&
            !std::memcmp(image+0x208e1d,reserve,sizeof(reserve)) &&
            !std::memcmp(image+0x208e55,commit,sizeof(commit)) &&
            slot==reinterpret_cast<DWORD>(image+0x11a0);
    } __except(EXCEPTION_EXECUTE_HANDLER) { return false; }
}
}
bool InstallLegacyShaderMemory(HMODULE module,ShaderMemoryLog log) {
    if(installed)return installed==module;
    auto* image=reinterpret_cast<BYTE*>(module);
    if(!module || !Profile(image)) { if(log)log("Legacy shader memory profile unavailable; runtime unchanged.");return false; }
    auto** allocSlot=reinterpret_cast<void**>(image+0x11a0);
    auto** freeSlot=reinterpret_cast<void**>(image+0x119c);
    if(*allocSlot!=reinterpret_cast<void*>(VirtualAlloc) || *freeSlot!=reinterpret_cast<void*>(VirtualFree))return false;
    // Reserve high in the LOWER half, before the level loader fills it. No
    // physical RAM is committed until D3DX requests pages. Other modules retain
    // their original allocators and can use the host's full address space.
    for(uintptr_t end=0x70000000;end>=capacity+0x10000000;end-=granule) {
        auto* p=VirtualAlloc(reinterpret_cast<void*>(end-capacity),capacity,MEM_RESERVE,PAGE_NOACCESS);
        if(p){arena=static_cast<BYTE*>(p);break;}
    }
    if(!arena){if(log)log("Legacy shader low-address reservation failed.");return false;}
    shaderHeap=HeapCreate(0,1024*1024,heapCapacity);
    MEMORY_BASIC_INFORMATION heapInfo{};
    if(shaderHeap)VirtualQuery(shaderHeap,&heapInfo,sizeof(heapInfo));
    heapBegin=reinterpret_cast<uintptr_t>(heapInfo.AllocationBase);heapEnd=heapBegin;
    while(heapBegin) {
        MEMORY_BASIC_INFORMATION region{};
        if(!VirtualQuery(reinterpret_cast<void*>(heapEnd),&region,sizeof(region)) ||
           reinterpret_cast<uintptr_t>(region.AllocationBase)!=heapBegin)break;
        heapEnd=reinterpret_cast<uintptr_t>(region.BaseAddress)+region.RegionSize;
    }
    if(!shaderHeap || !heapBegin || heapEnd-heapBegin<heapCapacity || heapEnd>0x80000000) {
        if(shaderHeap)HeapDestroy(shaderHeap);shaderHeap=nullptr;heapBegin=heapEnd=0;
        VirtualFree(arena,0,MEM_RELEASE);arena=nullptr;return false;
    }
    const auto crt=GetModuleHandleW(L"msvcrt.dll");
    crtFree=reinterpret_cast<CrtFree>(GetProcAddress(crt,"free"));
    crtSize=reinterpret_cast<Size>(GetProcAddress(crt,"_msize"));
    struct Import {size_t offset;const char* name;void* replacement;};
    const Import imports[]={
        {0x1240,"calloc",reinterpret_cast<void*>(LowCalloc)},
        {0x1244,"realloc",reinterpret_cast<void*>(LowRealloc)},
        {0x1248,"malloc",reinterpret_cast<void*>(LowMalloc)},
        {0x1254,"_strdup",reinterpret_cast<void*>(LowStrdup)},
        {0x1258,"free",reinterpret_cast<void*>(LowFree)},
        {0x12a4,"??2@YAPAXI@Z",reinterpret_cast<void*>(LowMalloc)},
        {0x12a8,"??3@YAXPAX@Z",reinterpret_cast<void*>(LowFree)}};
    bool valid=crtFree && crtSize;
    for(const auto& entry:imports)valid=valid && *reinterpret_cast<void**>(image+entry.offset)==reinterpret_cast<void*>(GetProcAddress(crt,entry.name));
    valid=valid && *reinterpret_cast<void**>(image+0x1140)==reinterpret_cast<void*>(HeapAlloc) &&
        *reinterpret_cast<void**>(image+0x1148)==reinterpret_cast<void*>(HeapFree);
    if(!valid){HeapDestroy(shaderHeap);shaderHeap=nullptr;heapBegin=heapEnd=0;VirtualFree(arena,0,MEM_RELEASE);arena=nullptr;return false;}
    DWORD oldProtect=0;
    if(!VirtualProtect(image+0x1000,0x1000,PAGE_READWRITE,&oldProtect)) {
        HeapDestroy(shaderHeap);shaderHeap=nullptr;heapBegin=heapEnd=0;VirtualFree(arena,0,MEM_RELEASE);arena=nullptr;return false;
    }
    for(const auto& entry:imports)*reinterpret_cast<void**>(image+entry.offset)=entry.replacement;
    *reinterpret_cast<void**>(image+0x1140)=reinterpret_cast<void*>(LowHeapAlloc);
    *reinterpret_cast<void**>(image+0x1148)=reinterpret_cast<void*>(LowHeapFree);
    *allocSlot=reinterpret_cast<void*>(&Allocate);
    *freeSlot=reinterpret_cast<void*>(&Release);
    DWORD ignored=0;VirtualProtect(image+0x1000,0x1000,oldProtect,&ignored);
    installed=module;
    if(log)log("Legacy shader private heap: %zu MiB below 2 GiB at %p.",heapCapacity/(1024*1024),shaderHeap);
    if(log)log("Legacy shader compiler arena: %zu MiB reserved below 2 GiB at %p; pages committed on demand.",capacity/(1024*1024),arena);
    return true;
}
ShaderMemoryStats GetLegacyShaderMemoryStats() {
    AcquireSRWLockShared(&lock);
    ShaderMemoryStats stats{arena?capacity:0,active,peak,allocations,failures};
    ReleaseSRWLockShared(&lock);return stats;
}
}
