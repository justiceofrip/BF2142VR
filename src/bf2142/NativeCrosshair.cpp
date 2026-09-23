#include "NativeCrosshair.h"
#include <windows.h>
#include <cstring>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
bool hidden=true;
template<class T>T Read(const void* p,size_t off=0){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+off,sizeof(v));return v;}
float* Resolve(const BYTE* game){
 __try {
  const BYTE lookup[]={0x55,0x8b,0xec,0x8b,0x45,8,0x56,0x8b,0xf1,0x50,0x8d,0x4d,8,0x51,0x8d,0x4e,0x5c};
  const BYTE setter[]={0x8b,0x45,0x24,0x89,6};
  if(!game||std::memcmp(game+0x4e360,lookup,sizeof(lookup))||std::memcmp(game+0x39a44e,setter,sizeof(setter))||
      Read<const BYTE*>(game,0x5221e0+0x30)!=game+0x4e360||Read<const BYTE*>(game,0x5221e0+0x2c)!=game+0x4e500)return nullptr;
  const auto manager=Read<const BYTE*>(game,0x60f79c);if(!manager||Read<const BYTE*>(manager)!=game+0x5221e0)return nullptr;
  // Read-only traversal of the same two maps used by the verified native
  // name lookup. No native string construction, ABI call or cache lifetime.
  const auto sentinel=Read<const BYTE*>(manager,0x60);if(!sentinel)return nullptr;
  auto node=Read<const BYTE*>(sentinel,4);int id=-1;
  for(unsigned n=0;node && node!=sentinel && n<64;++n){
   const unsigned size=Read<unsigned>(node,0x20),capacity=Read<unsigned>(node,0x24);
   if(size>128||capacity<size||capacity>4096)return nullptr;
   const char* name=capacity<16?reinterpret_cast<const char*>(node+0x10):Read<const char*>(node,0x10);
   if(!name||name[size])return nullptr;const int order=std::strcmp("CrossHairColorAlpha",name);
   if(!order){id=Read<int>(node,0x28);break;}node=Read<const BYTE*>(node,order<0?0:8);
  }
  if(id<0||id>100000)return nullptr;
  const auto floats=Read<const BYTE*>(manager,0x30);if(!floats)return nullptr;node=Read<const BYTE*>(floats,4);
  for(unsigned n=0;node && node!=floats && n<64;++n){
   const int key=Read<int>(node,0xc);if(key==id){auto p=Read<float*>(node,0x10);if(!p)return nullptr;
    const float value=*p;return std::isfinite(value)&&value>=0&&value<=1?p:nullptr;}
   node=Read<const BYTE*>(node,id<key?0:8);
  }
 }__except(EXCEPTION_EXECUTE_HANDLER){}return nullptr;
}
}
void SetCrosshairHidden(bool value){hidden=value;}
CrosshairScope::CrosshairScope(){if(hidden){alpha=Resolve(reinterpret_cast<const BYTE*>(GetModuleHandleW(nullptr)));if(alpha){saved=*alpha;*alpha=0;}}}
CrosshairScope::~CrosshairScope(){if(alpha)*alpha=saved;}
}
