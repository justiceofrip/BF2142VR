#include "NativeQueryGuard.h"
#include <MinHook.h>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
BYTE* queryImage=nullptr;LogFunction queryLog=nullptr;
using QueryDecision=bool(__thiscall*)(void*);
using FindQuery=void*(__thiscall*)(void*,void**,const int*);
QueryDecision originalDecision=nullptr;FindQuery findQuery=nullptr;
bool queryInstalled=false;volatile LONG missingReported=0;
template<class T>T ReadQuery(const void* p,size_t offset){T value{};std::memcpy(&value,static_cast<const BYTE*>(p)+offset,sizeof(value));return value;}
bool QueryProfile(BYTE* image){
    __try {
        const auto* dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0 || dos->e_lfanew>4096)return false;
        const auto* pe=reinterpret_cast<const IMAGE_NT_HEADERS*>(image+dos->e_lfanew);
        if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 || pe->OptionalHeader.SizeOfImage<0x220000)return false;
        const BYTE decision[]={0x55,0x8b,0xec,0x83,0xec,0x28,0x56,0x57,0x8b,0xf9};
        const BYTE lookup[]={0x55,0x8b,0xec,0x51,0x8b,0x51,4,0x8b,0x42,4,0x80,0x78,0x15,0};
        const BYTE update[]={0x55,0x8b,0xec,0x51,0x8d,0x45,8,0x50,0x8d,0x55,0xfc,0x52,0x83,0xc1,0x10};
        return !std::memcmp(image+0xbc4c0,decision,sizeof(decision)) && !std::memcmp(image+0x5570,lookup,sizeof(lookup)) &&
            !std::memcmp(image+0x4c40,update,sizeof(update)) && image[0x55bb]==0xc2 && image[0x55bc]==8 &&
            image[0xbc5b3]==0xe8 && image+0xbc5b8+ReadQuery<int>(image,0xbc5b4)==image+0x4c40 &&
            ReadQuery<BYTE*>(image,0xbc5af)==image+0x1f8e74;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool QueryAvailable(void* object){
    __try {
        if(!object)return false;
        const int id=ReadQuery<int>(object,0x350);
        const auto manager=ReadQuery<BYTE*>(queryImage,0x1f8e74);if(!manager)return false;
        const auto head=ReadQuery<void*>(manager,0x14);if(!head)return false;
        void* node=nullptr;findQuery(manager+0x10,&node,&id);
        // Native find returns the end sentinel for an absent id. The old path
        // reads its null value and calls +0x1260 with ECX=0 (v12 crash dump).
        // An id of zero is not rejected if it actually owns a valid record.
        return node && node!=head && !ReadQuery<BYTE>(node,0x15) && ReadQuery<void*>(node,0x10);
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool __fastcall QueryHook(void* object,void*){
    if(!QueryAvailable(object)){
        if(InterlockedCompareExchange(&missingReported,1,0)==0 && queryLog)
            queryLog("Native renderer query record unavailable; using the existing ordinary draw branch.");
        return false;
    }
    return originalDecision(object);
}
}
bool InstallNativeQueryGuard(BYTE* renderer,LogFunction logger){
    if(queryInstalled)return renderer==queryImage;
    if(!renderer || !QueryProfile(renderer))return false;
    queryImage=renderer;queryLog=logger;findQuery=reinterpret_cast<FindQuery>(renderer+0x5570);
    const auto target=renderer+0xbc4c0;
    if(MH_CreateHook(target,QueryHook,reinterpret_cast<void**>(&originalDecision))!=MH_OK)return false;
    if(MH_EnableHook(target)!=MH_OK){MH_RemoveHook(target);return false;}
    queryInstalled=true;if(logger)logger("Native renderer query guard connected; missing records use ordinary drawing.");return true;
}
}
