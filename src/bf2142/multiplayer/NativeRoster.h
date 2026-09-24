#pragma once
#include "PoseProtocol.h"
#include <windows.h>
#include <cstring>
namespace bfvr::bf2142::net {
struct Profile {std::size_t managerVt,playerVt,soldierVt,weaponVt,weaponTemplateVt,fireVt;};
constexpr Profile ClientProfile{0x5292a0,0x566cd8,0x566150,0x56cd88,0x56c758,0x5703c8};
constexpr Profile ServerProfile{0x394338,0x3cead0,0x3cdbc8,0x3d40d8,0x3d3aa8,0x3d7fd0};
template<class T>T Read(const void* p,std::size_t offset){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+offset,sizeof(v));return v;}
struct Actor {void* player=nullptr;void* weak=nullptr;void* soldier=nullptr;void* weapon=nullptr;Matrix body{};std::array<char,48> name{};bool ai=false;};
inline bool WeaponName(void* weapon,BYTE* game,const Profile& profile,std::array<char,48>* out){
 __try {
  if(!weapon||Read<void*>(weapon,0)!=game+profile.weaponVt)return false;
  const auto t=Read<BYTE*>(weapon,0x24);if(!t||Read<void*>(t,0)!=game+profile.weaponTemplateVt)return false;
  const auto n=Read<unsigned>(t,0x20),cap=Read<unsigned>(t,0x24);if(!n||n>=out->size()||cap<n||cap>4096)return false;
  const char* text=cap<16?reinterpret_cast<char*>(t+0x10):Read<char*>(t,0x10);if(!text||text[n])return false;
  *out={};for(unsigned i=0;i<n;++i){char c=text[i];if(c>='A'&&c<='Z')c+=32;if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return false;(*out)[i]=c;}return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
inline bool ReadRoster(void* manager,BYTE* game,const Profile& profile,std::array<Actor,256>* out){
 __try {
  *out={};if(!manager||Read<void*>(manager,0)!=game+profile.managerVt)return false;
  const auto count=Read<unsigned>(manager,0x5c);if(count>256)return false;
  const auto sentinel=Read<void*>(manager,0x58);if(!sentinel)return false;
  void* todo[256]{};void* seen[256]{};unsigned pending=1,visited=0;todo[0]=Read<void*>(sentinel,4);
  while(pending){
   void* node=todo[--pending];if(node==sentinel)continue;if(!node||visited>=256)return false;
   for(unsigned i=0;i<visited;++i)if(seen[i]==node)return false;seen[visited++]=node;
   const auto id=Read<unsigned>(node,12);const auto p=Read<void*>(node,16);if(id>255||!p||(*out)[id].player||Read<void*>(p,0)!=game+profile.playerVt)return false;
   auto& a=(*out)[id];a.player=p;a.ai=Read<BYTE>(p,0xd4)!=0; // Player::isAI (vtable slot 26).
   a.weak=Read<void*>(p,0xcc);a.soldier=a.weak?Read<void*>(a.weak,4):nullptr;
   if(a.soldier && (Read<void*>(a.soldier,0)!=game+profile.soldierVt||(Read<DWORD>(a.soldier,0x14)&0x20)))a.soldier=nullptr;
   if(a.soldier){
    a.body=Read<Matrix>(a.soldier,0xa0);
    const int index=Read<int>(a.soldier,0x218);auto begin=Read<BYTE*>(a.soldier,0x234),end=Read<BYTE*>(a.soldier,0x238);
    // Infantry only. Vehicles retain their existing native look/input path.
    if(!Read<void*>(a.soldier,0x34)&&!Read<void*>(a.soldier,0x28c)&&index>0&&index<32&&begin&&end>=begin&&std::size_t(end-begin)<=128&&std::size_t(index*4)<std::size_t(end-begin)){
     void* w=Read<void*>(begin,index*4);
     if(w&&Read<void*>(w,0x34)==a.soldier&&WeaponName(w,game,profile,&a.name))a.weapon=w;
    }
   }
   for(unsigned offset:{0u,8u}){void* child=Read<void*>(node,offset);if(child!=sentinel){if(!child||pending>=256||Read<void*>(child,4)!=node)return false;todo[pending++]=child;}}
  }
  return visited==count;
 }__except(EXCEPTION_EXECUTE_HANDLER){*out={};return false;}
}
}
