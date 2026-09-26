#include "NativeRemoteWeapon.h"
#include "NativeRoster.h"
#include <MinHook.h>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
BYTE* weaponRenderImage=nullptr;
using MeshDraw=void(__thiscall*)(void*,DWORD,DWORD,DWORD);
using MeshShadow=void(__thiscall*)(void*,DWORD,DWORD,DWORD,DWORD,DWORD,DWORD);
MeshDraw originalWeaponDraw=nullptr;MeshShadow originalWeaponShadow=nullptr;
bool weaponRenderInstalled=false;
bool WeaponRenderProfile(BYTE* image){
 __try {
  const auto dos=reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
  if(!dos||dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>4096)return false;
  const auto pe=reinterpret_cast<const IMAGE_NT_HEADERS*>(image+dos->e_lfanew);
  if(pe->Signature!=IMAGE_NT_SIGNATURE||pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_I386||pe->OptionalHeader.SizeOfImage<0x1d5500)return false;
  const BYTE draw[]={0x55,0x8b,0xec,0x81,0xec,0x8c,1,0,0,0x53,0x8b,0xd9};
  const BYTE shadow[]={0x55,0x8b,0xec,0x81,0xec,0x8c,0,0,0,0x56,0x8b,0xf1,0x8b,0x86,0x3c,2,0,0};
  const BYTE owner[]={0x55,0x8b,0xec,0x8b,0x45,8,0x89,0x81,0x90,2,0,0,0x5d,0xc2,4,0};
  return !memcmp(image+0xc5030,draw,sizeof(draw))&&!memcmp(image+0xc67e0,shadow,sizeof(shadow))&&
   !memcmp(image+0xc2a10,owner,sizeof(owner))&&net::Read<void*>(image+0x1d5400,0x18)==image+0xc5030&&
   net::Read<void*>(image+0x1d5400,0x20)==image+0xc67e0&&net::Read<void*>(image+0x1d5400,0x9c)==image+0xc2a10&&
   image[0xc63d8]==0xc2&&image[0xc63d9]==12&&image[0xc63da]==0&&
   image[0xc6acd]==0xc2&&image[0xc6ace]==24&&image[0xc6acf]==0;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool SuppressWeaponDraw(void* geometry){
 __try {return geometry&&net::Read<void*>(geometry,0)==weaponRenderImage+0x1d5400&&HideRemoteWeaponGeometry(geometry);}
 __except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
// The verified callbacks return void and pop three/six 32-bit arguments.
// Forward their opaque bits unchanged. No mesh flags, palette, inventory,
// transforms or shared materials are mutated, so fallback needs no restoration.
void __fastcall WeaponDrawHook(void* self,void*,DWORD a,DWORD b,DWORD c){
 if(!SuppressWeaponDraw(self))originalWeaponDraw(self,a,b,c);
}
void __fastcall WeaponShadowHook(void* self,void*,DWORD a,DWORD b,DWORD c,DWORD d,DWORD e,DWORD f){
 if(!SuppressWeaponDraw(self))originalWeaponShadow(self,a,b,c,d,e,f);
}
}
bool InstallRemoteWeaponVisibility(BYTE* renderer,LogFunction log){
 if(weaponRenderInstalled)return renderer==weaponRenderImage;
 if(!WeaponRenderProfile(renderer))return false;
 weaponRenderImage=renderer;const auto draw=renderer+0xc5030,shadow=renderer+0xc67e0;
 if(MH_CreateHook(draw,WeaponDrawHook,reinterpret_cast<void**>(&originalWeaponDraw))!=MH_OK)return false;
 if(MH_CreateHook(shadow,WeaponShadowHook,reinterpret_cast<void**>(&originalWeaponShadow))!=MH_OK){MH_RemoveHook(draw);return false;}
 if(MH_EnableHook(draw)!=MH_OK||MH_EnableHook(shadow)!=MH_OK){
  MH_DisableHook(draw);MH_DisableHook(shadow);MH_RemoveHook(draw);MH_RemoveHook(shadow);return false;
 }
 weaponRenderInstalled=true;if(log)log("Remote weapon visibility connected: fresh empty-hand poses suppress only the owned mesh and shadow.");return true;
}
}
