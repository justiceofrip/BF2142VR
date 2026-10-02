#include "NativeCrosshair.cpp"
#include <vector>
#include <array>
#include <limits>
#include <cstdio>
using namespace bfvr::bf2142;
template<class T>void Put(BYTE* p,size_t off,T v){std::memcpy(p+off,&v,sizeof(v));}
unsigned draws=0;void* expectedArgument=reinterpret_cast<void*>(0x1234);
bool argumentsOk=true;
void __fastcall DrawCachedWidget(void* self,void*,void* a,void* b,void* c,void* d,void* e){
 argumentsOk=argumentsOk&&a==expectedArgument&&b==a&&c==a&&d==a&&e==a;
 // Reproduce native cached visibility: drawing never reads GuiIndex again.
 draws+=Read<float>(self,0x14)>0;
}
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
 Put(name.data(),0x28,841);
 std::array<BYTE,64> guiName{},intRoot{},intEntry{};int index=84;
 Put(name.data(),8,guiName.data());Put(guiName.data(),0,nameRoot.data());Put(guiName.data(),8,nameRoot.data());
 std::memcpy(guiName.data()+0x10,"GuiIndex",9);Put(guiName.data(),0x20,8u);Put(guiName.data(),0x24,15u);Put(guiName.data(),0x28,842);
 Put(manager.data(),0x20,intRoot.data());Put(intRoot.data(),4,intEntry.data());Put(intEntry.data(),0,intRoot.data());Put(intEntry.data(),8,intRoot.data());Put(intEntry.data(),0xc,842);Put(intEntry.data(),0x10,&index);
 const BYTE intMap[]={0x55,0x8b,0xec,0x83,0xec,8,0x56,0x8d,0x71,0x1c};
 std::memcpy(g+0x4e490,intMap,sizeof(intMap));const BYTE reg[]={0xff,0x52,0x1c};std::memcpy(g+0x39f84f,reg,sizeof(reg));
 Put(g,0x39f82f,g+0x5b4238);std::memcpy(g+0x5b4238,"GuiIndex",9);Put(g,0x5221e0+0x20,g+0x4e490);Put(g,0x5221e0+0x24,g+0x4e320);
 for(int zoom:{59,63,78,80,81,82,84,88,90,92,93}){
  index=zoom;{CrosshairScope scope(g);if(index!=1024||alpha!=0)return 5;
   {CrosshairScope nested(g);if(index!=1024||alpha!=0)return 6;}
   if(index!=1024||alpha!=0)return 7;
  }if(index!=zoom||alpha!=.8f)return 8;
  {CrosshairScope optic(g,true);if(!optic.Optic()||index!=zoom||alpha!=1)return 15;}if(alpha!=.8f)return 16;
 }
 for(int other:{0,24,25,52,55,79,89,777}){index=other;{CrosshairScope scope(g);if(index!=other||alpha!=0)return 9;}if(alpha!=.8f)return 10;}
 index=84;SetCrosshairHidden(false);{CrosshairScope scope(g);if(index!=1024||alpha!=.8f)return 11;}SetCrosshairHidden(true);
 index=84;{CrosshairScope optic(g,true);if(index!=84||alpha!=1)return 17;
  {CrosshairScope flash(g);if(index!=84||alpha!=1)return 18;}
  if(index!=84||alpha!=1)return 19;
 }if(index!=84||alpha!=.8f)return 20;
 // Missing crosshair-alpha lookup must not let fixed-opacity ADS widgets leak.
 Put(entry.data(),0x10,static_cast<float*>(nullptr));
 {CrosshairScope flash(g);if(index!=1024)return 21;}if(index!=84)return 22;
 Put(entry.data(),0x10,&alpha);
 g[0x4e490]=0;{CrosshairScope scope(g);if(index!=84||alpha!=0)return 12;}if(alpha!=.8f)return 13;
 g[0x4e490]=intMap[0];Put(intEntry.data(),0x10,reinterpret_cast<int*>(1));if(ResolveGui(g))return 14;
 // Cached node still draws even after changing the source GUI variable.
 // Exercise the actual detour with opaque arguments, no game binary needed.
 Put(intEntry.data(),0x10,&index);widgetImage=g;
 nativeCullDraw=reinterpret_cast<CullDraw>(&DrawCachedWidget);
 std::array<BYTE,64> node{};Put(node.data(),0,g+0x5bb4f8);
 Put(node.data(),4,reinterpret_cast<void*>(0x5678));Put(node.data(),0x14,1.f);
 const auto draw=[&](){CullDrawHook(node.data(),nullptr,expectedArgument,expectedArgument,expectedArgument,expectedArgument,expectedArgument);};
 for(const char* name:{"CarbineZoomCullNode","PacAssaultZoomCullNode","EuAssaultZoomCullNode",
                       "EuMachineZoomCullNode","EuSniperHudCullNode","PacSniperHudCullNode",
                       "EuSmgZoomCullNode","PacZoomCullNode","EuAntiVehicleCullNode","PacAntiVehicleCullNode"}){
  Put(node.data(),0x18,name);index=84;draws=0;
  {CrosshairScope main(g);draw();if(draws||index!=1024)return 31;}
  if(index!=84||Read<float>(node.data(),0x14)!=1)return 32;
  {CrosshairScope optic(g,true);if(optic.DrewArtwork())return 33;
   {CrosshairScope flash(g);draw();}if(draws!=1||!optic.DrewArtwork())return 34;
  }
  draw();if(draws!=2)return 35; // Outside VR HUD drawing: native behavior.
 }
 for(const char* name:{"HealthCullNode","EuSoldierAACullNode","PacSoldierAACullNode","EuSmgCrossStandardCullNode","PacSmgCrossStandardCullNode","EuAssaultCrossStandardCullNode","EuSniperHudCullNodeCustom","TankCrosshairZoomCullNode"}){
  Put(node.data(),0x18,name);draws=0;
  {CrosshairScope main(g);draw();}if(draws!=1)return 36;
  {CrosshairScope optic(g,true);draw();if(optic.DrewArtwork())return 37;}
 }
 Put(node.data(),0x18,"EuSniperHudCullNode");Put(node.data(),0x14,0.f);
 {CrosshairScope optic(g,true);draw();if(optic.DrewArtwork())return 38;}
 Put(node.data(),0x14,1.f);Put(node.data(),4,static_cast<void*>(nullptr));
 {CrosshairScope optic(g,true);draw();if(optic.DrewArtwork())return 39;}
 Put(node.data(),0x18,reinterpret_cast<const char*>(1));if(ScopeNode(node.data()))return 40;
 if(hudDrawDepth||opticReplayDepth||!argumentsOk)return 41;
 puts("Named crosshair lookup, cached scope-node suppression, replay visibility, empty-capture fallback and unrelated HUD preservation passed.");return 0;
}
