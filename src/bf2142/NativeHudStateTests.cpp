#include "NativeHudState.h"
#include <array>
#include <vector>
#include <cstring>
#include <cstdio>
#include <limits>
using namespace bfvr::bf2142;
template<class T>void Put(BYTE* p,size_t n,T value){std::memcpy(p+n,&value,sizeof(value));}
int main(){
    std::vector<BYTE> g(0x620000),r(0x230000);auto game=g.data(),render=r.data();
    const BYTE modeCode[]={0x8b,0x81,0x70,2,0,0,0xc3},query[]={0x56,0x8b,0xf1,0x8b,6,0xff,0x90,0xe4,0,0,0,0x84,0xc0,0x75,0x1d};
    const BYTE excluded[]={0x85,0xc0,0x74,0x13,0x83,0xf8,2,0x74,0x0e,0x83,0xf8,0x0a,0x74,9,0x83,0xf8,0x0b,0x74,4};
    const BYTE move[]={0x55,0x8b,0xec,0x83,0xec,8,0x80,0x79,0x10,0};
    std::memcpy(game+0xc2db0,modeCode,sizeof(modeCode));std::memcpy(game+0x3515c0,query,sizeof(query));std::memcpy(game+0x3515d9,excluded,sizeof(excluded));std::memcpy(game+0x46dcd0,move,sizeof(move));
    Put(game,0x5a6b50+0x30,game+0x3515c0);Put(game,0x5a6b50+0x228,game+0xc2db0);Put(game,0x5b8528+0x4c,game+0x3c9160);Put(game,0x5cb870+0x48,game+0x46dcd0);Put(render,0x1c1868+40,render+0x2cef0);
    NativeHudState state;HudTarget target;if(!state.Connect(game,render)||state.Read())return 1;
    std::array<BYTE,0x600> renderer{};std::array<BYTE,0x280> hud{};std::array<BYTE,0x700> root{};std::array<BYTE,0x20> pointer{};
    Put(render,0x1f8e58,renderer.data());Put(renderer.data(),0,render+0x1c1868);Put(renderer.data(),0x5cc,hud.data());Put(hud.data(),0,game+0x5a6b50);Put(hud.data(),0x84,root.data());hud[0x268]=1;Put(hud.data(),0x270,1);
    Put(root.data(),0,game+0x5b8528);Put(root.data(),0x20,pointer.data());Put(root.data(),0xc,800.f);Put(root.data(),0x10,600.f);Put(root.data(),4,-400.f);Put(root.data(),8,-300.f);Put(pointer.data(),0,game+0x5cb870);pointer[0x10]=1;
    if(!state.Read(&target)||target.root!=root.data()||target.width!=800||target.height!=600||target.left!=-400||target.top!=-300)return 2;
    for(int mode:{0,2,10,11,-1,28,500}){Put(hud.data(),0x270,mode);if(state.Read())return 3;}
    for(int mode:{1,3,4,16,27}){Put(hud.data(),0x270,mode);if(!state.Read())return 4;}
    Put(game,0x60f7a8,root.data());if(state.Read())return 5;Put(game,0x60f7a8,static_cast<BYTE*>(nullptr));
    std::array<BYTE,0x50> flash{};Put(flash.data(),0,game+0x525350);Put(game,0x60f7a8,flash.data());
    if(!state.Read())return 13;flash[8]=1;if(state.Read())return 14;
    flash[8]=0;if(!state.Read())return 15;Put(game,0x60f7a8,static_cast<BYTE*>(nullptr));
    Put(root.data(),4,std::numeric_limits<float>::quiet_NaN());if(state.Read())return 16;Put(root.data(),4,-400.f);
    hud[0x268]=0;if(state.Read())return 6;hud[0x268]=1;
    pointer[0x10]=0;if(state.Read())return 7;pointer[0x10]=1;
    Put(root.data(),0xc,std::numeric_limits<float>::quiet_NaN());if(state.Read())return 8;Put(root.data(),0xc,800.f);
    Put(renderer.data(),0x5cc,reinterpret_cast<BYTE*>(1));if(state.Read())return 9;Put(renderer.data(),0x5cc,hud.data());
    Put(pointer.data(),0,game+0x5cb874);if(state.Read())return 10;Put(pointer.data(),0,game+0x5cb870);
    for(size_t off:{size_t(0xc2db0),size_t(0x3515c0),size_t(0x3515d9),size_t(0x46dcd0),size_t(0x5a6b50+0x30),size_t(0x5a6b50+0x228),size_t(0x5b8528+0x4c),size_t(0x5cb870+0x48)}){
        game[off]^=1;if(state.Connect(game,render)||state.Read())return 11;game[off]^=1;if(!state.Connect(game,render)||!state.Read())return 12;
    }
    puts("HUD deployment/spawn/Flash transitions, hidden cursor, native GUI dimensions and stale/unknown owner guards passed.");
}
