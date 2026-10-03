#include "NativeCrosshair.h"
#include <windows.h>
#include <cstring>
#include <cmath>
#include <initializer_list>
#include <MinHook.h>
namespace bfvr::bf2142 {
namespace {
bool hidden=true;thread_local bool adsTicketsHidden=false;
thread_local unsigned opticReplayDepth=0,hudDrawDepth=0;
thread_local bool replayArtwork=false;
BYTE* widgetImage=nullptr;
using CullDraw=void(__thiscall*)(void*,void*,void*,void*,void*,void*);
CullDraw nativeCullDraw=nullptr;
template<class T>T Read(const void* p,size_t off=0){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+off,sizeof(v));return v;}
// Stock ADS HUD selectors. Disable the entire weapon widget root on the main
// panel, including fixed-opacity rangefinders and scope stabilizers. The optic
// layer replays the unchanged native selector, then isolates its pixels.
bool OpticHud(int index){switch(index){case 24:case 25:case 51:case 55:case 59:case 63:case 78:case 80:case 81:case 82:case 84:case 88:case 90:case 92:case 93:return true;default:return false;}}
// Show-variable results and alpha are cached during HUD update, before Draw.
// Changing GuiIndex during Draw cannot hide these nodes. Match only stock
// scope roots, not HP/ammo, menus, hip crosshairs or vehicle artwork.
bool ScopeWidget(const char* name){
 if(!name)return false;
 for(const char* known:{"CarbineZoomCullNode","PacAssaultZoomCullNode",
     "EuAssaultZoomCullNode","EuMachineZoomCullNode","EuSniperHudCullNode",
     "PacSniperHudCullNode","EuSmgZoomCullNode","PacZoomCullNode",
     "EuAntiVehicleCullNode","PacAntiVehicleCullNode","PacMachineZoomCullNode",
     "EuRocketZoomCullNode","PacRocketZoomCullNode",
     // The AA zoom root carries its reticle and native lock information.
     // Individual static picture nodes need not have their own cull wrapper.
     "EuSoldierAACullNode","PacSoldierAACullNode"})if(!std::strcmp(name,known))return true;
 return false;
}
enum class HudNode { Other, Scope, Tickets };
HudNode ClassifyNode(void* node){
 __try {
  if(!widgetImage||Read<const BYTE*>(node)!=widgetImage+0x5bb4f8)return HudNode::Other;
  const char* name=Read<const char*>(node,0x18);if(!name)return HudNode::Other;
  char bounded[64]{};unsigned i=0;
  for(;i<sizeof(bounded)-1&&name[i];++i)bounded[i]=name[i];
  if(name[i])return HudNode::Other;
  if(ScopeWidget(bounded))return HudNode::Scope;
  // Hide only the stock top conquest/coop ticket bar. Its parent also owns
  // map UI; Titan health and commander widgets have separate native roots.
  if(!std::strcmp(bounded,"TicketInfoConquestCullNode"))return HudNode::Tickets;
  return HudNode::Other;
 }__except(EXCEPTION_EXECUTE_HANDLER){return HudNode::Other;}
}
void __fastcall CullDrawHook(void* self,void*,void* a,void* b,void* c,void* d,void* e){
 const auto kind=hudDrawDepth?ClassifyNode(self):HudNode::Other;
 // Apply identically to the base HUD and scope replay, so subtraction cannot
 // turn a hidden ticket bar into spurious scope artwork.
 if(kind==HudNode::Tickets&&adsTicketsHidden)return;
 if(kind==HudNode::Scope){
  if(!opticReplayDepth)return;
  // An allocated capture alone is not evidence of scope artwork. A hidden
  // or unknown node keeps the existing fallback aiming mark on the GPU path.
  __try {if(Read<void*>(self,4)&&Read<float>(self,0x14)>0)replayArtwork=true;}
  __except(EXCEPTION_EXECUTE_HANDLER){}
 }
 nativeCullDraw(self,a,b,c,d,e);
}
void* NamedValue(const BYTE* manager,const char* key,size_t mapOffset){
 const auto sentinel=Read<const BYTE*>(manager,0x60);if(!sentinel)return nullptr;
 auto node=Read<const BYTE*>(sentinel,4);int id=-1;
 for(unsigned n=0;node&&node!=sentinel&&n<64;++n){
  const unsigned size=Read<unsigned>(node,0x20),capacity=Read<unsigned>(node,0x24);
  if(size>128||capacity<size||capacity>4096)return nullptr;
  const char* name=capacity<16?reinterpret_cast<const char*>(node+0x10):Read<const char*>(node,0x10);
  if(!name||name[size])return nullptr;const int order=std::strcmp(key,name);
  if(!order){id=Read<int>(node,0x28);break;}node=Read<const BYTE*>(node,order<0?0:8);
 }
 if(id<0||id>100000)return nullptr;
 const auto values=Read<const BYTE*>(manager,mapOffset);if(!values)return nullptr;node=Read<const BYTE*>(values,4);
 for(unsigned n=0;node&&node!=values&&n<64;++n){
  const int k=Read<int>(node,0xc);if(k==id)return Read<void*>(node,0x10);
  node=Read<const BYTE*>(node,id<k?0:8);
 }
 return nullptr;
}
float* Resolve(const BYTE* game){
 __try {
  const BYTE lookup[]={0x55,0x8b,0xec,0x8b,0x45,8,0x56,0x8b,0xf1,0x50,0x8d,0x4d,8,0x51,0x8d,0x4e,0x5c};
  const BYTE setter[]={0x8b,0x45,0x24,0x89,6};
  if(!game||std::memcmp(game+0x4e360,lookup,sizeof(lookup))||std::memcmp(game+0x39a44e,setter,sizeof(setter))||
      Read<const BYTE*>(game,0x5221e0+0x30)!=game+0x4e360||Read<const BYTE*>(game,0x5221e0+0x2c)!=game+0x4e500)return nullptr;
  const auto manager=Read<const BYTE*>(game,0x60f79c);if(!manager||Read<const BYTE*>(manager)!=game+0x5221e0)return nullptr;
  auto p=static_cast<float*>(NamedValue(manager,"CrossHairColorAlpha",0x30));
  return p&&std::isfinite(*p)&&*p>=0&&*p<=1?p:nullptr;
 }__except(EXCEPTION_EXECUTE_HANDLER){}return nullptr;
}
int* ResolveGui(const BYTE* game){
 __try {
  // GuiIndex registration uses integer-register vslot 0x1c; integer by-ID
  // lookup vslot 0x20 traverses manager+0x1c (sentinel +0x20).
  const BYTE integerMap[]={0x55,0x8b,0xec,0x83,0xec,8,0x56,0x8d,0x71,0x1c};
  const BYTE registration[]={0xff,0x52,0x1c};
  if(!game||std::memcmp(game+0x4e490,integerMap,sizeof(integerMap))||
     std::memcmp(game+0x39f84f,registration,sizeof(registration))||
     Read<const BYTE*>(game,0x39f82f)!=game+0x5b4238||std::memcmp(game+0x5b4238,"GuiIndex",9)||
     Read<const BYTE*>(game,0x5221e0+0x20)!=game+0x4e490||Read<const BYTE*>(game,0x5221e0+0x24)!=game+0x4e320)return nullptr;
  const auto manager=Read<const BYTE*>(game,0x60f79c);if(!manager||Read<const BYTE*>(manager)!=game+0x5221e0)return nullptr;
  auto p=static_cast<int*>(NamedValue(manager,"GuiIndex",0x20));return p&&*p>=0&&*p<=1024?p:nullptr;
 }__except(EXCEPTION_EXECUTE_HANDLER){}return nullptr;
}
}
bool InstallNativeOpticHud(){
 if(nativeCullDraw)return true;
 auto* game=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
 __try {
  const BYTE prefix[]={0x55,0x8b,0xec,0x83,0xec,0x1c,0x56,0x8b,0xf1,0x83,0x7e,4,0};
  if(Read<const BYTE*>(game,0x5bb4f8+0x54)!=game+0x47aca0||
     std::memcmp(game+0x47aca0,prefix,sizeof(prefix))||
     Read<unsigned short>(game,0x47ace3)!=0x14c2)return false;
  if(MH_CreateHook(game+0x47aca0,CullDrawHook,reinterpret_cast<void**>(&nativeCullDraw))!=MH_OK)return false;
  if(MH_EnableHook(game+0x47aca0)!=MH_OK){MH_RemoveHook(game+0x47aca0);nativeCullDraw=nullptr;return false;}
  widgetImage=game;return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void SetCrosshairHidden(bool value){hidden=value;}
void SetAdsTicketsHidden(bool value){adsTicketsHidden=value;}
CrosshairScope::CrosshairScope():CrosshairScope(nullptr){}
CrosshairScope::CrosshairScope(const void* module,bool optic){
 // Native Flash can execute outside HudHook, and a replay can nest Flash.
 // Suppress ADS widgets independently of the user's ordinary crosshair toggle.
 if(!optic&&opticReplayDepth)return;
 drawing=true;++hudDrawDepth;
 const auto game=module?static_cast<const BYTE*>(module):reinterpret_cast<const BYTE*>(GetModuleHandleW(nullptr));
 __try {
  gui=ResolveGui(game);if(gui)savedGui=*gui;
  opticReady=gui&&OpticHud(savedGui);
  alpha=Resolve(game);if(alpha)saved=*alpha;
  if(optic){replay=true;if(!opticReplayDepth)replayArtwork=false;++opticReplayDepth;if(alpha&&opticReady)*alpha=1;}
  else {if(alpha&&hidden)*alpha=0;if(opticReady)*gui=1024;}
 }__except(EXCEPTION_EXECUTE_HANDLER){}
}
bool CrosshairScope::DrewArtwork() const{return replay&&replayArtwork;}
CrosshairScope::~CrosshairScope(){if(drawing&&hudDrawDepth)--hudDrawDepth;if(replay&&opticReplayDepth)--opticReplayDepth;__try{if(gui)*gui=savedGui;if(alpha)*alpha=saved;}__except(EXCEPTION_EXECUTE_HANDLER){}}
}
