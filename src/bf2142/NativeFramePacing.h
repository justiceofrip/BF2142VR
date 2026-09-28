#pragma once
#include <windows.h>
#include <cstring>
namespace bfvr::bf2142 {
// BF2142's own game.lockFps console getter/setter, not the BF1942 limiter.
// Use its setter so the game's network restrictions remain authoritative.
inline void* ResolveFrameLimiter(const BYTE* g) noexcept {
 __try {
  if(!g)return nullptr;
  const BYTE getter[]={0x8b,0x81,0x9c,0,0,0,0xc3};
  const BYTE setter[]={0x55,0x8b,0xec,0x8b,0x45,8,0x85,0xc0,0x56,0x8b,0xf1,0x7e,0x0b,0x89,0x86,0x9c,0,0,0,0x5e,0x5d,0xc2,4,0,0xe8};
  const BYTE suffix[]={0x84,0xc0,0x75,0x0a,0xc7,0x86,0x9c,0,0,0,0,0,0,0,0x5e,0x5d,0xc2,4,0};
  if(std::memcmp(g+0x1afc00,getter,sizeof(getter))||std::memcmp(g+0x3030,setter,sizeof(setter))||std::memcmp(g+0x304d,suffix,sizeof(suffix))||std::memcmp(g+0x519464,"lockFps",8))return nullptr;
  const auto u32=[](const BYTE* p){DWORD v;std::memcpy(&v,p,4);return v;};
  if(INT_PTR(g+0x304d)+static_cast<LONG>(u32(g+0x3049))!=INT_PTR(g+0x2b800)||u32(g+0x243c2)!=DWORD(g+0x519464)||u32(g+0x5193e0+0x5c)!=DWORD(g+0x2447b)||
     u32(g+0x24492)!=DWORD(g+0x60f3b0)||u32(g+0x244b4)!=DWORD(g+0x60f3b0)||g[0x244b9]!=0xe8||
     INT_PTR(g+0x244be)+static_cast<LONG>(u32(g+0x244ba))!=INT_PTR(g+0x3030))return nullptr;
  auto object=reinterpret_cast<BYTE*>(u32(g+0x60f3b0));if(!object)return nullptr;
  const int fps=int(u32(object+0x9c));return fps>=0&&fps<=1000?object:nullptr;
 }__except(EXCEPTION_EXECUTE_HANDLER){return nullptr;}
}
inline int SetNativeFrameRate(int value,int* previous=nullptr,bool onlyIfUnlocked=false) noexcept {
 __try {
  const auto g=reinterpret_cast<const BYTE*>(GetModuleHandleW(nullptr));void* object=ResolveFrameLimiter(g);if(!object)return -1;
  int before=0;std::memcpy(&before,static_cast<BYTE*>(object)+0x9c,4);if(previous)*previous=before;
  if(value<0||value>1000||(onlyIfUnlocked&&before!=0))return before;
  using Set=void(__thiscall*)(void*,int);reinterpret_cast<Set>(const_cast<BYTE*>(g)+0x3030)(object,value);
  int result=0;std::memcpy(&result,static_cast<BYTE*>(object)+0x9c,4);return result;
 }__except(EXCEPTION_EXECUTE_HANDLER){return -1;}
}
}
