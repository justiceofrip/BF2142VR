#include "StereoSession.h"
#include <MinHook.h>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
// Swiff renderer BeginDisplay/EndDisplay, verified against their vtable and
// native stack cleanup. Arguments are forwarded bit-for-bit (nine stack words).
using BeginDisplay=void(__thiscall*)(void*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
using EndDisplay=void(__thiscall*)(void*);
BeginDisplay beginDisplay=nullptr;
EndDisplay endDisplay=nullptr;
thread_local unsigned depth=0;
thread_local bool captured=false;
void __fastcall BeginHook(void* self,void*,DWORD a,DWORD b,DWORD c,DWORD d,DWORD e,DWORD f,DWORD g,DWORD h,DWORD i) {
    if(depth++==0) captured=StereoHudBegin(true);
    beginDisplay(self,a,b,c,d,e,f,g,h,i);
}
void __fastcall EndHook(void* self,void*) {
    endDisplay(self);
    if(depth && --depth==0 && captured) {captured=false;StereoHudEnd();}
}
bool Profile(BYTE* image) {
    __try {
        const BYTE begin[]={0x55,0x8b,0xec,0x81,0xec,0xc4,0,0,0,0x53,0x8b,0xd9,0x80,0x7b,0x10,0};
        const BYTE end[]={0x56,0x8b,0xf1,0x80,0x7e,0x10,0,0x0f,0x85,0xbf,0,0,0};
        auto** table=reinterpret_cast<void**>(image+0x1d7ad8);
        return std::memcmp(image+0x117a20,begin,sizeof(begin))==0 &&
            std::memcmp(image+0x118af0,end,sizeof(end))==0 &&
            table[13]==image+0x117a20 && table[14]==image+0x118af0 &&
            image[0x117f65]==0xc2 && image[0x117f66]==0x24 && image[0x117f67]==0;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
}
bool InstallNativeMenus(BYTE* image,LogFunction logger) {
    if(!Profile(image))return false;
    void* a=image+0x117a20;void* b=image+0x118af0;
    if(MH_CreateHook(a,reinterpret_cast<void*>(&BeginHook),reinterpret_cast<void**>(&beginDisplay))!=MH_OK)return false;
    if(MH_CreateHook(b,reinterpret_cast<void*>(&EndHook),reinterpret_cast<void**>(&endDisplay))!=MH_OK) {MH_RemoveHook(a);return false;}
    if(MH_EnableHook(a)!=MH_OK || MH_EnableHook(b)!=MH_OK) {
        MH_DisableHook(a);MH_DisableHook(b);MH_RemoveHook(a);MH_RemoveHook(b);return false;
    }
    logger("Native Flash menu display boundaries connected.");return true;
}
}
