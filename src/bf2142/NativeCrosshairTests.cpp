#include "NativeCrosshair.cpp"
#include <vector>
#include <array>
#include <limits>
#include <cstdio>
using namespace bfvr::bf2142;
template<class T>void Put(BYTE* p,size_t off,T v){std::memcpy(p+off,&v,sizeof(v));}
int main(){
 std::vector<BYTE> game(0x610000);std::array<BYTE,128> manager{};std::array<BYTE,64> nameRoot{},name{},floatRoot{},entry{};float alpha=.8f;
 auto g=game.data();const BYTE lookup[]={0x55,0x8b,0xec,0x8b,0x45,8,0x56,0x8b,0xf1,0x50,0x8d,0x4d,8,0x51,0x8d,0x4e,0x5c};
 const BYTE setter[]={0x8b,0x45,0x24,0x89,6};std::memcpy(g+0x4e360,lookup,sizeof(lookup));std::memcpy(g+0x39a44e,setter,sizeof(setter));
 Put(g,0x5221e0+0x30,g+0x4e360);Put(g,0x5221e0+0x2c,g+0x4e500);Put(g,0x60f79c,manager.data());Put(manager.data(),0,g+0x5221e0);
 Put(manager.data(),0x60,nameRoot.data());Put(nameRoot.data(),4,name.data());Put(name.data(),0,nameRoot.data());Put(name.data(),8,nameRoot.data());
 const char* key="CrossHairColorAlpha";Put(name.data(),0x10,key);Put(name.data(),0x20,19u);Put(name.data(),0x24,19u);Put(name.data(),0x28,841);
 Put(manager.data(),0x30,floatRoot.data());Put(floatRoot.data(),4,entry.data());Put(entry.data(),0,floatRoot.data());Put(entry.data(),8,floatRoot.data());Put(entry.data(),0xc,841);Put(entry.data(),0x10,&alpha);
 if(Resolve(g)!=&alpha||alpha!=.8f)return 1;
 alpha=std::numeric_limits<float>::quiet_NaN();if(Resolve(g))return 2;alpha=.8f;
 g[0x4e360]=0;if(Resolve(g))return 3;g[0x4e360]=lookup[0];
 Put(name.data(),0x28,842);if(Resolve(g))return 4;
 puts("Signature-backed named crosshair alpha lookup passed; unrelated HUD state remains untouched.");return 0;
}
