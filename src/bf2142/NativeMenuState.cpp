#include "NativeMenuState.h"
#include <cstring>
namespace bfvr::bf2142 {
namespace {
template<class T>T Read(const void* p,size_t offset=0){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+offset,sizeof(v));return v;}
bool Profile(const BYTE* image){
    __try {
        // The current frontend is the same object receiving absolute mouse
        // coordinates and button edges in the native window listener.
        const BYTE move[]={0x55,0x8b,0xec,0x8b,0x45,8,0x8b,0x55,0x0c,
            0x89,0x41,0x3c,0x89,0x51,0x40,0x5d,0xc2,8,0};
        const BYTE release[]={0xc6,0x41,0x38,0,0xc3};
        const BYTE activate[]={0xc6,0x46,8,1};
        const BYTE receiver[]={0x8b,0x45,0x0c,0x8b,0x0d};
        return image &&
            Read<const BYTE*>(image,0x525350+5*4)==image+0x87020 &&
            Read<const BYTE*>(image,0x525350+19*4)==image+0x874d0 &&
            Read<const BYTE*>(image,0x525350+21*4)==image+0x874f0 &&
            Read<const BYTE*>(image,0x525350+22*4)==image+0x87520 &&
            std::memcmp(image+0x874d0,move,sizeof(move))==0 &&
            std::memcmp(image+0x87520,release,sizeof(release))==0 &&
            std::memcmp(image+0x870ed,activate,sizeof(activate))==0 &&
            std::memcmp(image+0x18f1,receiver,sizeof(receiver))==0 &&
            Read<const BYTE*>(image,0x18f6)==image+0x60f7a8;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
bool NativeMenuState::Connect(const BYTE* image){game=Profile(image)?image:nullptr;return game!=nullptr;}
bool NativeMenuState::Active() const{
    if(!game)return false;
    __try {
        const auto state=Read<const BYTE*>(game,0x60f7a8);
        // Read anew every frame; never retain an object across spawn, pause or
        // map transitions. Do not treat a missing soldier as proof of a menu.
        return state && Read<const BYTE*>(state)==game+0x525350 && Read<BYTE>(state,8)==1;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool NativeInGameMenuActive(){
    static const auto state=[] {NativeMenuState s;s.Connect(reinterpret_cast<const BYTE*>(GetModuleHandleW(nullptr)));return s;}();
    return state.Active();
}
}
