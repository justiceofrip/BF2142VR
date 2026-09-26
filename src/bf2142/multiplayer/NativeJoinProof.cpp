#include "NativeJoinProof.h"
#include "NativeRoster.h"
#include <cstring>
namespace bfvr::bf2142::net {
bool DecodeJoinChallenge(const void* p,std::size_t n,const Secret& secret,JoinChallenge& out) noexcept {
 if(!p||n!=sizeof(JoinChallenge))return false;JoinChallenge c;std::memcpy(&c,p,n);
 if(c.magic!=JoinProofMagic||c.version!=1||c.player>255||c.reserved||!c.session||c.secret!=secret)return false;
 bool any=false;for(auto b:c.nonce)any|=b!=0;if(!any)return false;out=c;return true;
}
bool JoinProofPolicy::Accept(const JoinChallenge& c,unsigned player,std::uint64_t current,std::uint64_t now) noexcept {
 if(player>255||c.player!=player||!current||c.session!=current)return false;
 if(current!=session){last=0;previous={};session=current;}
 if(last&&(now<last||now-last<750))return false;previous=c.nonce;last=now;return true;
}
bool SendNativeJoinProof(void* module,void* localPlayer,const Secret& nonce) noexcept {
 __try {
  const auto game=static_cast<BYTE*>(module);if(!game||!localPlayer)return false;
  const BYTE prologue[]={0x55,0x8b,0xec,0x8b,0x0d};
  const BYTE dispatch[]={0x8b,0x11,0x50,0x6a,0xff,0xff,0x92,0x90,0,0,0,0xb0,1,0x5e,0x5d,0xc2,0x14,0};
  const BYTE constructor[]={0x55,0x8b,0xec,0x56,0x8b,0xf1,0xe8};
  if(std::memcmp(game+0xcc5b0,prologue,sizeof(prologue))||Read<void*>(game,0xcc5b5)!=game+0x685228||
   std::memcmp(game+0xcc609,dispatch,sizeof(dispatch))||std::memcmp(game+0x111ef0,constructor,sizeof(constructor)))return false;
  const auto pm=Read<void*>(game,0x685228);const auto client=Read<void*>(game,0x61c170);
  if(!pm||Read<void*>(pm,0)!=game+ClientProfile.managerVt||Read<void*>(pm,0x6c)!=localPlayer||
   Read<void*>(localPlayer,0)!=game+ClientProfile.playerVt||!client||Read<void*>(client,0)!=game+0x529840||
   Read<void*>(game+0x529840,0x12c)!=game+0xcc5b0)return false;
  std::array<std::int32_t,4> words;std::memcpy(words.data(),nonce.data(),16);
  using Send=bool(__thiscall*)(void*,int,int,int,int,int);
  return reinterpret_cast<Send>(game+0xcc5b0)(client,int(JoinProofCommand),words[0],words[1],words[2],words[3]);
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
