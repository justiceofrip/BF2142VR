#include "NativeParticleReplay.h"
#include <MinHook.h>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
BYTE* particleImage=nullptr;
using UpdateParticles=void(__thiscall*)(void*,float);
using UpdateCamera=void(__thiscall*)(void*,void*);
using GetCamera=void*(__thiscall*)(void*);
UpdateParticles updateParticles=nullptr;UpdateCamera updateCamera=nullptr;GetCamera getCamera=nullptr;
template<class T>T ParticleRead(const void* p,size_t offset=0){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+offset,sizeof(v));return v;}
bool ParticleReplayProfile(BYTE* image){
 __try {
  if(!image)return false;
  const BYTE update[]={0x55,0x8b,0xec,0x53,0x56,0x57,0x6a,0,0x8b,0xf1,0xe8};
  const BYTE camera[]={0x55,0x8b,0xec,0x83,0xec,0x10,0x53,0x56,0x8b,0x75,8,0x8b,6,0x57,0x8b,0xf9};
  const BYTE renderCall[]={0x8b,0x11,0x51,0xd9,0x1c,0x24,0xff,0x52,0x2c};
  const auto getter=ParticleRead<BYTE*>(image,0x1c1868+0x5c);
  const BYTE getterCode[]={0x8b,0x81,0xf8,0,0,0,0xc3};
  return ParticleRead<BYTE*>(image,0x1d3c78+0x2c)==image+0x9a4c0 &&
   !std::memcmp(image+0x9a4c0,update,sizeof(update))&&!std::memcmp(image+0x98bf0,camera,sizeof(camera))&&
   !std::memcmp(image+0x2d5a0,renderCall,sizeof(renderCall))&&
   image+0x9a4f0+ParticleRead<int>(image,0x9a4ec)==image+0x98bf0&&
   image[0x9a6bc]==0xc2&&image[0x9a6bd]==4&&getter>=image&&getter<image+0x1b0000&&!std::memcmp(getter,getterCode,sizeof(getterCode));
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool RefreshParticleCamera(void* manager){
 __try {
  if(!manager||ParticleRead<BYTE*>(manager)!=particleImage+0x1d3c78)return false;
  const auto renderer=ParticleRead<void*>(particleImage,0x1f8e58);
  if(!renderer||ParticleRead<BYTE*>(renderer)!=particleImage+0x1c1868)return false;
  const auto camera=getCamera(renderer);
  if(!camera||ParticleRead<BYTE*>(camera)!=particleImage+0x1c6610)return false;
  updateCamera(manager,camera);return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void __fastcall ParticleUpdateHook(void* manager,void*,float delta){
 // The stock zero-delta update still admits/removes emitters and runs every
 // particle's update callback. Keep one effect simulation per stereo frame;
 // recompute camera-dependent culling/LOD for the right eye and scope instead.
 if((IsSecondStereoEye()||IsScopeRender())&&RefreshParticleCamera(manager))return;
 updateParticles(manager,delta);
}
}
bool InstallNativeParticleReplay(BYTE* renderer,LogFunction log){
 if(particleImage)return particleImage==renderer;
 if(!ParticleReplayProfile(renderer))return false;
 updateCamera=reinterpret_cast<UpdateCamera>(renderer+0x98bf0);
 getCamera=reinterpret_cast<GetCamera>(ParticleRead<BYTE*>(renderer,0x1c1868+0x5c));
 const auto target=renderer+0x9a4c0;
 if(MH_CreateHook(target,ParticleUpdateHook,reinterpret_cast<void**>(&updateParticles))!=MH_OK)return false;
 particleImage=renderer;
 if(MH_EnableHook(target)!=MH_OK){MH_RemoveHook(target);particleImage=nullptr;return false;}
 if(log)log("Stereo particles: one lifecycle update, per-eye camera refresh, including optic replay.");
 return true;
}
}
