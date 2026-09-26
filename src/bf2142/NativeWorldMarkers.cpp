#include "NativeWorldMarkers.h"
#include "WorldMarker.h"
#include <MinHook.h>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
struct NativeMarkerPoint {float x,y,z,distance;};
using Project=NativeMarkerPoint* (__thiscall*)(void*,NativeMarkerPoint*,stereo::Vec3,void*);
Project originalProject=nullptr;
bool installed=false;
template<class T>T Read(const void* p,size_t offset){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+offset,sizeof(v));return v;}
bool GluedWorldMarker(const void* tag){
 __try{return tag && Read<int>(tag,0xc)==3 && (Read<unsigned>(tag,0xb8)&0x20)!=0;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool Store(void* tag,NativeMarkerPoint* out,const WorldMarkerPoint& p){
 __try{*out={p.x,p.y,p.z,p.distance};static_cast<BYTE*>(tag)[0xc0]=p.edge?1:0;return true;}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
NativeMarkerPoint* __fastcall ProjectHook(void* self,void*,NativeMarkerPoint* out,stereo::Vec3 world,void* tag){
 // Preserve native filtering/selection and the ABI. Only type-3 glued
 // 3D-map items are affected; player names and the separate HUD stay native.
 auto* result=originalProject(self,out,world,tag);
 EyeCamera head{},eye{};
 if(result==out && out && GluedWorldMarker(tag)){
  // The native manager writes this sentinel for culled items at +0xf38cd
  // and +0xf3a16. Both primary/secondary projection calls use this hook.
  if(HideStereoWorldMarkers())Store(tag,out,{0,0,-1,0,false});
  else if(ReadStereoMarkerFrame(&head,&eye))
   if(const auto point=ProjectWorldMarker(world,head,eye))Store(tag,out,*point);
 }
 return result;
}
bool Profile(const BYTE* r){
 __try{
  const BYTE entry[]={0x55,0x8b,0xec,0x83,0xec,0x28,0x8b,0x0d};
  const BYTE marker[]={0x8b,0x8f,0xb8,0,0,0,0xc1,0xe9,5};
  const BYTE tail[]={0x5e,0x8b,0xe5,0x5d,0xc2,0x14,0};
  const BYTE caller[]={0xe8,0x8f,0xb8,0xff,0xff};
  const BYTE hidden[]={0xb9,0,0,0x80,0xbf,0x89,0x18,0x89,0x58,4,0x89,0x48,8};
  return r && !std::memcmp(r+0xef210,entry,sizeof(entry)) &&
   !std::memcmp(r+0xef2ef,marker,sizeof(marker)) && !std::memcmp(r+0xef4b9,tail,sizeof(tail)) &&
   !std::memcmp(r+0xf397c,caller,sizeof(caller)) && !std::memcmp(r+0xf38cd,hidden,sizeof(hidden)) &&
   Read<const BYTE*>(r,0x1d6c80+5*4)==r+0xf3490 &&
   Read<const BYTE*>(r,0x1c6610+28*4)==r+0x670c0;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
bool InstallNativeWorldMarkers(BYTE* renderer,LogFunction logger){
 if(installed)return true;if(!Profile(renderer))return false;
 void* target=renderer+0xef210;
 if(MH_CreateHook(target,reinterpret_cast<void*>(&ProjectHook),reinterpret_cast<void**>(&originalProject))!=MH_OK)return false;
 if(MH_EnableHook(target)!=MH_OK){MH_RemoveHook(target);return false;}
 installed=true;logger("Native 3D-map marker visibility connected; optional marker projection uses a shared stereo edge.");return true;
}
}
