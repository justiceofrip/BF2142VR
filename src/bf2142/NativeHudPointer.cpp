#include "NativeHudPointer.h"
#include "NativeHudState.h"
#include <MinHook.h>
#include <algorithm>
#include <cmath>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
using Move=bool(__thiscall*)(void*,void*,float,float);
using Update=bool(__thiscall*)(void*,float);
using Down=bool(__thiscall*)(void*,int,int,int);
using Up=bool(__thiscall*)(void*,int,int);
Move nativeMove=nullptr;Update nativeUpdate=nullptr;Down down=nullptr;Up up=nullptr;
BYTE* game=nullptr;bool installed=false;LogFunction logLine=nullptr;
struct Request {bool valid=false,held=false;float x=0,y=0;ULONGLONG time=0;unsigned serial=0;};
SRWLOCK lock=SRWLOCK_INIT;Request request{};
void* lastRoot=nullptr;void* lastPointer=nullptr;bool held=false;unsigned consumed=0;
bool Sample(Request& out){
    AcquireSRWLockShared(&lock);out=request;ReleaseSRWLockShared(&lock);
    DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    return out.valid && GetTickCount64()-out.time<=150 && pid==GetCurrentProcessId();
}
bool __fastcall MoveHook(void* pointer,void*,void* root,float x,float y){
    Request sample;HudTarget target;
    if(Sample(sample)&&NativeHudMenuActive(&target)&&target.root==root&&target.pointer==pointer){x=target.left+sample.x*target.width;y=target.top+sample.y*target.height;}
    return nativeMove(pointer,root,x,y);
}
bool __fastcall UpdateHook(void* root,void*,float dt){
    HudTarget target;Request sample;
    const bool active=NativeHudMenuActive(&target)&&target.root==root;
    if(active){
        if(root!=lastRoot || target.pointer!=lastPointer){held=false;consumed=0;lastRoot=root;lastPointer=target.pointer;}
        const bool fresh=Sample(sample);
        // Run input once per new runtime request, even if the GUI is replayed
        // for both eyes. A stale/lost ray releases its existing UI press.
        if(fresh && consumed!=sample.serial){
            nativeMove(target.pointer,root,target.left+sample.x*target.width,target.top+sample.y*target.height);
            if(sample.held!=held){if(sample.held)down(root,1,1,1);else up(root,1,1);held=sample.held;}
            consumed=sample.serial;
        }else if(!fresh && held){up(root,1,1);held=false;}
    }else if(root==lastRoot){
        HudTarget current;
        if(held && NativeHudTarget(&current) && current.root==root && current.pointer==lastPointer)up(root,1,1);
        held=false;consumed=0;lastRoot=lastPointer=nullptr;
    }
    return nativeUpdate(root,dt);
}
bool Profile(){
    __try {
        const BYTE update[]={0x55,0x8b,0xec,0xd9,0x45,8,0xd8,0x1d};
        const BYTE press[]={0x55,0x8b,0xec,0x8b,0x51,0x20,0x5d,0xe9};
        return !std::memcmp(game+0x3c9160,update,sizeof(update)) &&
            !std::memcmp(game+0x46e2a0,press,sizeof(press)) && !std::memcmp(game+0x46e2b0,press,sizeof(press)) &&
            *reinterpret_cast<BYTE**>(game+0x5cb870+0x4c)==game+0x46dd20 &&
            *reinterpret_cast<BYTE**>(game+0x5cb870+0x50)==game+0x46dd50;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
void PublishNativeHudPointer(bool valid,float x,float y,bool pressed){
    valid=valid&&std::isfinite(x)&&std::isfinite(y)&&x>=0&&x<=1&&y>=0&&y<=1;
    AcquireSRWLockExclusive(&lock);request={valid,valid&&pressed,x,y,GetTickCount64(),request.serial+1};ReleaseSRWLockExclusive(&lock);
}
bool InstallNativeHudPointer(LogFunction logger){
    if(installed)return true;game=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));logLine=logger;
    if(!Profile())return false;
    // Cache the untouched profile before MinHook changes Move's prologue.
    (void)NativeHudMenuActive();
    if(MH_CreateHook(game+0x46dcd0,MoveHook,reinterpret_cast<void**>(&nativeMove))!=MH_OK)return false;
    if(MH_CreateHook(game+0x3c9160,UpdateHook,reinterpret_cast<void**>(&nativeUpdate))!=MH_OK){MH_RemoveHook(game+0x46dcd0);return false;}
    down=reinterpret_cast<Down>(game+0x46e2a0);up=reinterpret_cast<Up>(game+0x46e2b0);
    if(MH_EnableHook(game+0x46dcd0)!=MH_OK || MH_EnableHook(game+0x3c9160)!=MH_OK){
        for(auto off:{0x46dcd0,0x3c9160}){MH_DisableHook(game+off);MH_RemoveHook(game+off);}return false;
    }
    installed=true;
    logger("Deployment HUD pointer connected: native absolute GUI coordinates and select/release edges.");return true;
}
}
