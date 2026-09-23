#include "NativeHudState.h"
#include <cstring>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
template<class T>T Get(const void* p,size_t off=0){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+off,sizeof(v));return v;}
bool Profile(const BYTE* g,const BYTE* r){
    __try {
        const BYTE mode[]={0x8b,0x81,0x70,2,0,0,0xc3};
        const BYTE query[]={0x56,0x8b,0xf1,0x8b,6,0xff,0x90,0xe4,0,0,0,0x84,0xc0,0x75,0x1d};
        const BYTE excluded[]={0x85,0xc0,0x74,0x13,0x83,0xf8,2,0x74,0x0e,0x83,0xf8,0x0a,0x74,9,0x83,0xf8,0x0b,0x74,4};
        const BYTE move[]={0x55,0x8b,0xec,0x83,0xec,8,0x80,0x79,0x10,0};
        return g && r && Get<const BYTE*>(g,0x5a6b50+0x30)==g+0x3515c0 &&
            Get<const BYTE*>(g,0x5a6b50+0x228)==g+0xc2db0 &&
            Get<const BYTE*>(g,0x5b8528+0x4c)==g+0x3c9160 &&
            Get<const BYTE*>(g,0x5cb870+0x48)==g+0x46dcd0 &&
            !std::memcmp(g+0xc2db0,mode,sizeof(mode)) && !std::memcmp(g+0x3515c0,query,sizeof(query)) &&
            !std::memcmp(g+0x3515d9,excluded,sizeof(excluded)) && !std::memcmp(g+0x46dcd0,move,sizeof(move)) &&
            Get<const BYTE*>(r,0x1c1868+10*4)==r+0x2cef0;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
bool InteractiveHudMode(int mode) noexcept {return mode>=0 && mode<=27 && mode!=0 && mode!=2 && mode!=10 && mode!=11;}
bool NativeHudState::Connect(const BYTE* g,const BYTE* r){game=renderer=nullptr;if(!Profile(g,r))return false;game=g;renderer=r;return true;}
bool NativeHudState::Read(HudTarget* out,bool interactive) const {
    if(!game||!renderer)return false;
    __try {
        // An active Flash frontend owns input. Its object may remain allocated
        // while inactive; allocation alone must not hide the deployment laser.
        const auto flash=Get<const BYTE*>(game,0x60f7a8);
        if(flash && (Get<const BYTE*>(flash)!=game+0x525350 || Get<BYTE>(flash,8)!=0))return false;
        const auto render=Get<const BYTE*>(renderer,0x1f8e58);
        if(!render || Get<const BYTE*>(render)!=renderer+0x1c1868)return false;
        const auto hud=Get<const BYTE*>(render,0x5cc);
        if(!hud || Get<const BYTE*>(hud)!=game+0x5a6b50 || Get<BYTE>(hud,0x268)!=1)return false;
        // Do not infer deployment from death, missing soldiers, or cursor handles.
        if(interactive && !InteractiveHudMode(Get<int>(hud,0x270)))return false;
        const auto root=Get<const BYTE*>(hud,0x84);
        if(!root || Get<const BYTE*>(root)!=game+0x5b8528)return false;
        const auto pointer=Get<const BYTE*>(root,0x20);
        if(!pointer || Get<const BYTE*>(pointer)!=game+0x5cb870 || Get<BYTE>(pointer,0x10)!=1)return false;
        const float w=Get<float>(root,0xc),h=Get<float>(root,0x10);
        const float left=Get<float>(root,4),top=Get<float>(root,8);
        if(!std::isfinite(w)||!std::isfinite(h)||w<100||h<100||w>8192||h>8192)return false;
        // The native root constructor (EXE+0x46e020) initializes a centered
        // rectangle: left=-width/2, top=-height/2. Pointer Move takes canvas
        // coordinates in that rectangle, NOT a zero-origin pixel position.
        if(!std::isfinite(left)||!std::isfinite(top)||std::abs(left)>8192||std::abs(top)>8192)return false;
        if(out)*out={const_cast<BYTE*>(hud),const_cast<BYTE*>(root),const_cast<BYTE*>(pointer),w,h,left,top};return true;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
namespace {
NativeHudState& State(){
    static NativeHudState state;static bool attempted=false;
    if(!attempted){
        auto r=reinterpret_cast<const BYTE*>(GetModuleHandleW(L"RendDX9_ori.dll"));
        if(!r)r=reinterpret_cast<const BYTE*>(GetModuleHandleW(L"RendDX9.dll"));
        if(!r)return state;attempted=true;state.Connect(reinterpret_cast<const BYTE*>(GetModuleHandleW(nullptr)),r);
    }
    return state;
}
}
bool NativeHudMenuActive(HudTarget* target){return State().Read(target);}
bool NativeHudTarget(HudTarget* target){return State().Read(target,false);}
}
