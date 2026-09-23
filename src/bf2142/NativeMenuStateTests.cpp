#include "NativeMenuState.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <vector>
using namespace bfvr::bf2142;
namespace {
template<class T>void Put(BYTE* p,size_t offset,T value){std::memcpy(p+offset,&value,sizeof(value));}
}
int main(){
    static_assert(sizeof(void*)==4,"Native BF2142 layout is Win32 only");
    std::vector<BYTE> image(0x610000);
    auto* game=image.data();
    const BYTE move[]={0x55,0x8b,0xec,0x8b,0x45,8,0x8b,0x55,0x0c,0x89,0x41,0x3c,0x89,0x51,0x40,0x5d,0xc2,8,0};
    const BYTE release[]={0xc6,0x41,0x38,0,0xc3},activate[]={0xc6,0x46,8,1},receiver[]={0x8b,0x45,0x0c,0x8b,0x0d};
    std::memcpy(game+0x874d0,move,sizeof(move));std::memcpy(game+0x87520,release,sizeof(release));
    std::memcpy(game+0x870ed,activate,sizeof(activate));std::memcpy(game+0x18f1,receiver,sizeof(receiver));
    Put(game,0x18f6,game+0x60f7a8);
    Put(game,0x525350+5*4,game+0x87020);Put(game,0x525350+19*4,game+0x874d0);
    Put(game,0x525350+21*4,game+0x874f0);Put(game,0x525350+22*4,game+0x87520);
    NativeMenuState menus;
    if(menus.Active() || menus.Connect(nullptr) || !menus.Connect(game) || menus.Active())return 1;
    std::array<BYTE,16> first{},second{};
    Put(first.data(),0,game+0x525350);Put(second.data(),0,game+0x525350);
    Put(game,0x60f7a8,first.data());
    if(menus.Active())return 2; // Frontend not initialized yet.
    first[8]=1;if(!menus.Active())return 3; // Menu present without consulting any cursor API.
    Put(game,0x60f7a8,second.data());if(menus.Active())return 4; // Never cache the last object.
    second[8]=1;if(!menus.Active())return 5;
    Put(second.data(),0,game+0x525354);if(menus.Active())return 6; // Unknown frontend remains native.
    Put(game,0x60f7a8,static_cast<BYTE*>(nullptr));if(menus.Active())return 7; // Spawn closes menu.
    Put(game,0x60f7a8,reinterpret_cast<BYTE*>(1));if(menus.Active())return 8; // Stale transition pointer.
    Put(game,0x60f7a8,first.data());
    const size_t guarded[]={0x874d0,0x87520,0x870ed,0x18f1,0x18f6,0x525350+5*4,
        0x525350+19*4,0x525350+21*4,0x525350+22*4};
    for(auto offset:guarded){
        game[offset]^=1;
        if(menus.Connect(game) || menus.Active())return 9;
        game[offset]^=1;
        if(!menus.Connect(game) || !menus.Active())return 10;
    }
    if(menus.Connect(reinterpret_cast<BYTE*>(1)) || menus.Active())return 11;
    puts("Native menu profile, deployment/spawn transitions, object replacement and invalid layout guards passed.");
}
