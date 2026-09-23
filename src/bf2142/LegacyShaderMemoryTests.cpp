#include "LegacyShaderMemory.cpp"
#include <cstdio>
#include <thread>
#include <atomic>
#include <vector>
using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Shader memory failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    auto* image=static_cast<BYTE*>(VirtualAlloc(nullptr,0x251000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));CHECK(image);
    auto* dos=reinterpret_cast<IMAGE_DOS_HEADER*>(image);dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
    auto* pe=reinterpret_cast<IMAGE_NT_HEADERS*>(image+0x80);pe->Signature=IMAGE_NT_SIGNATURE;pe->FileHeader.Machine=IMAGE_FILE_MACHINE_I386;pe->OptionalHeader.SizeOfImage=0x251000;
    const BYTE tag[]={0xf7,0xd0,0x80,0x78,0x14,0x03,0x75,0x38,0x8b,0x50,0x18};
    const BYTE reserve[]={0x68,0x00,0x20,0x00,0x00,0xff,0x75,0x0c,0x6a,0x00,0xff,0x15};
    const BYTE commit[]={0x85,0xc0,0x75,0x11,0x68,0x00,0x80,0x00,0x00,0x50,0xff,0x75,0xfc,0xff,0x15};
    std::memcpy(image+0xfb178,tag,sizeof(tag));std::memcpy(image+0x208e1d,reserve,sizeof(reserve));std::memcpy(image+0x208e55,commit,sizeof(commit));
    *reinterpret_cast<DWORD*>(image+0x208e29)=reinterpret_cast<DWORD>(image+0x11a0);
    *reinterpret_cast<void**>(image+0x11a0)=reinterpret_cast<void*>(VirtualAlloc);
    *reinterpret_cast<void**>(image+0x119c)=reinterpret_cast<void*>(VirtualFree);
    *reinterpret_cast<void**>(image+0x1140)=reinterpret_cast<void*>(HeapAlloc);
    *reinterpret_cast<void**>(image+0x1148)=reinterpret_cast<void*>(HeapFree);
    auto crt=LoadLibraryW(L"msvcrt.dll");CHECK(crt);
    const size_t offsets[]={0x1240,0x1244,0x1248,0x1254,0x1258,0x12a4,0x12a8};
    const char* names[]={"calloc","realloc","malloc","_strdup","free","??2@YAPAXI@Z","??3@YAXPAX@Z"};
    for(size_t i=0;i<7;++i)*reinterpret_cast<void**>(image+offsets[i])=reinterpret_cast<void*>(GetProcAddress(crt,names[i]));
    image[0xfb178]^=1;CHECK(!InstallLegacyShaderMemory(reinterpret_cast<HMODULE>(image),nullptr));
    CHECK(*reinterpret_cast<void**>(image+0x1248)==reinterpret_cast<void*>(GetProcAddress(crt,"malloc")));image[0xfb178]^=1;
    CHECK(InstallLegacyShaderMemory(reinterpret_cast<HMODULE>(image),nullptr));
    CHECK(InstallLegacyShaderMemory(reinterpret_cast<HMODULE>(image),nullptr));
    CHECK(!InstallLegacyShaderMemory(nullptr,nullptr));
    auto low=reinterpret_cast<Malloc>(*reinterpret_cast<void**>(image+0x1248));
    for(size_t size:{size_t(0),size_t(1),size_t(4096),size_t(0x70000),size_t(2*1024*1024)}){
        void* p=low(size);CHECK(p && reinterpret_cast<uintptr_t>(p)<0x80000000);
        std::memset(p,0x5a,size);void* q=LowRealloc(p,size+256);CHECK(q);
        if(size)CHECK(static_cast<BYTE*>(q)[size-1]==0x5a);LowFree(q);
    }
    CHECK(!LowCalloc(SIZE_MAX,2));CHECK(!LowMalloc(SIZE_MAX));
    auto* zero=static_cast<BYTE*>(LowCalloc(20,16));CHECK(zero && zero[319]==0);LowFree(zero);
    auto* dup=LowStrdup("shader");CHECK(dup && !std::strcmp(dup,"shader"));LowFree(dup);
    auto crtAlloc=reinterpret_cast<Malloc>(GetProcAddress(crt,"malloc"));
    auto* old=static_cast<char*>(crtAlloc(32));std::memcpy(old,"preinstall",11);
    auto* migrated=static_cast<char*>(LowRealloc(old,64));CHECK(migrated && !std::strcmp(migrated,"preinstall"));LowFree(migrated);
    LowFree(crtAlloc(64)); // original CRT ownership
    void* hp=LowHeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,32);CHECK(hp && static_cast<BYTE*>(hp)[31]==0);CHECK(LowHeapFree(GetProcessHeap(),0,hp));
    void* ordinary=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(ordinary && Release(ordinary,0,MEM_RELEASE));
    auto* a=static_cast<BYTE*>(Allocate(nullptr,granule,MEM_RESERVE,PAGE_READWRITE));
    auto* b=static_cast<BYTE*>(Allocate(nullptr,granule,MEM_COMMIT,PAGE_READWRITE));CHECK(a && b);
    CHECK(Allocate(a,4096,MEM_COMMIT,PAGE_READWRITE)==a);a[0]=42;b[0]=99;
    CHECK(!Release(a+1,0,MEM_RELEASE));CHECK(!Release(a,1,MEM_RELEASE));
    CHECK(Release(a,0,MEM_DECOMMIT));CHECK(b[0]==99);CHECK(Allocate(a,4096,MEM_COMMIT,PAGE_READWRITE)==a);CHECK(a[0]==0);
    CHECK(Release(a,0,MEM_RELEASE));CHECK(!Release(a,0,MEM_RELEASE));CHECK(Release(b,0,MEM_RELEASE));
    void* full=Allocate(nullptr,capacity,MEM_RESERVE,PAGE_NOACCESS);CHECK(full);
    CHECK(!Allocate(nullptr,1,MEM_RESERVE,PAGE_NOACCESS));CHECK(Release(full,0,MEM_RELEASE));
    std::atomic<bool> ok=true;std::vector<std::thread> workers;
    for(int t=0;t<8;++t)workers.emplace_back([&]{for(int i=0;i<400;++i){auto* p=static_cast<BYTE*>(LowMalloc(700000));if(!p){ok=false;break;}p[699999]=17;LowFree(p);}});
    for(auto& t:workers)t.join();CHECK(ok.load());CHECK(GetLegacyShaderMemoryStats().active==0);
    // Fixed heap exhaustion fails without spilling tagged pointers into high VA.
    std::vector<void*> held;for(int i=0;i<400;++i){void* p=LowMalloc(400000);if(!p)break;CHECK(OwnsHeap(p));held.push_back(p);}CHECK(!held.empty() && held.size()<400);
    for(void* p:held)LowFree(p);void* recovered=LowMalloc(400000);CHECK(recovered);LowFree(recovered);
    puts("Legacy shader memory ownership, pressure, reuse and concurrency passed.");return 0;
}
