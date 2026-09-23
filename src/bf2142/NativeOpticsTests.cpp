#include <Windows.h>
static ULONGLONG testTime=1000;
static ULONGLONG TestTime(){return testTime;}
#define GetTickCount64 TestTime
#include "NativeOptics.cpp"
#undef GetTickCount64
#include <array>
#include <cstdio>
using namespace bfvr::bf2142;
namespace {int zoomChanges=0;bool immediateSeen=false;
void __fastcall ZoomSpy(void* base,void*,int step,bool immediate){++zoomChanges;immediateSeen|=immediate;std::memcpy(static_cast<BYTE*>(base)+0x1c,&step,sizeof(step));}
TrackedWeaponFrame tracked;bool tracking=true;int forwarded=0,lastLod=-9;void* lastSelf=nullptr;
void __fastcall Original(void* self,void*,int lod){++forwarded;lastSelf=self;lastLod=lod;}
template<class T>void Set(BYTE* p,size_t off,T value){std::memcpy(p+off,&value,sizeof(value));}
}
namespace bfvr::bf2142 {bool ReadTrackedWeaponFrame(TrackedWeaponFrame* out,bool){if(!tracking)return false;*out=tracked;return true;}bool IsLocalTrackedWeapon(void* w){return tracking && w==tracked.weapon;}}
int main(){
 game=static_cast<BYTE*>(VirtualAlloc(nullptr,0x660000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));if(!game)return 1;
 std::array<BYTE,0x268> w{};std::array<BYTE,0x370> t{};std::array<BYTE,0x38> z{};std::array<BYTE,0x4c> zt{};
 std::array<float,3> factors{0,.484f,.106f};
 Set(w.data(),0,game+0x56cd88);Set(w.data(),0x24,t.data());Set(w.data(),0x1c4,z.data()+0x10);
 Set(t.data(),0,game+0x56c758);const char* name="eu_ar_rifle";std::memcpy(t.data()+0x10,name,std::strlen(name)+1);Set(t.data(),0x20,unsigned(std::strlen(name)));Set(t.data(),0x24,15u);
 Set(z.data(),0,game+0x571438);Set(z.data(),0xc,w.data());Set(z.data(),0x10,game+0x571360);Set(z.data(),0x1c,1);Set(z.data(),0x30,zt.data());
 Set(zt.data(),0,game+0x571640);Set(zt.data(),0x18,factors.data());Set(zt.data(),0x1c,factors.data()+2);Set(zt.data(),0x2c,1);
 tracked.weapon=w.data();for(int i=0;i<4;++i)tracked.world.values[i][i]=1;
 enabled=true;nativeLod=reinterpret_cast<SetZoomLod>(Original);
 GunOptic optic{};bool ok=ReadNativeOptic(&optic)&&std::abs(optic.magnification-2.f)<.001f;
 const auto before=z;
 LodHook(z.data(),nullptr,1);ok=ok && lastLod==0 && lastSelf==z.data() && before==z;
 LodHook(z.data(),nullptr,0);ok=ok && lastLod==0;
 LodHook(w.data(),nullptr,1);ok=ok && lastLod==1;
 tracking=false;LodHook(z.data(),nullptr,1);ok=ok && lastLod==1 && !ReadNativeOptic(&optic);tracking=true;
 Set(z.data(),0x1c,0);ok=ok && !ReadNativeOptic(&optic);Set(z.data(),0x1c,1);
 Set(z.data(),0x34,.1f);ok=ok && ReadNativeOptic(&optic) && optic.magnification>2.5f;Set(z.data(),0x34,0.f);
 factors[1]=.5f;LodHook(z.data(),nullptr,1);ok=ok && lastLod==1 && !ReadNativeOptic(&optic);factors[1]=.484f;
 Set(z.data(),0xc,static_cast<void*>(nullptr));ok=ok && !ReadNativeOptic(&optic);Set(z.data(),0xc,w.data());
 Set(w.data(),0x24,reinterpret_cast<void*>(1));ok=ok && !ReadNativeOptic(&optic);Set(w.data(),0x24,t.data());
 t[0x10]='x';LodHook(z.data(),nullptr,1);ok=ok && lastLod==1 && !ReadNativeOptic(&optic);t[0x10]='e';
 // Automatic ADS uses native transitions, not a visual-only accuracy shortcut.
 setZoom=reinterpret_cast<SetZoom>(ZoomSpy);Set(z.data(),0x1c,0);ConfigureAutomaticAds(true);
 std::array<EyeCamera,2> eyes{};for(auto& e:eyes){e.world=tracked.world;e.world.values[3]={0,.095f,-.281f,1};}
 RequestAutomaticAds(true);UpdateAutomaticAds(eyes);ok=ok && zoomChanges==0;
 testTime=1100;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);ok=ok && zoomChanges==0;
 testTime=1180;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);ok=ok && zoomChanges==1 && owned && Read<int>(z.data(),0x1c)==1;
 for(auto& e:eyes)e.world.values[3][0]=1;
 for(auto stamp:{1200u,1400u,1500u}){testTime=stamp;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);}
 ok=ok && zoomChanges==2 && !owned && Read<int>(z.data(),0x1c)==0;
 // Reacquire by alignment alone. A native reload/sprint cancellation does
 // not repeatedly force zoom back on while the sight remains raised.
 for(auto& e:eyes)e.world.values[3][0]=0;
 for(auto stamp:{1510u,1690u}){testTime=stamp;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);}
 ok=ok && zoomChanges==3 && owned;
 Set(z.data(),0x1c,0);testTime=1700;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);
 testTime=1800;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);ok=ok && zoomChanges==3 && !owned;
 for(auto& e:eyes)e.world.values[3][0]=1;
 testTime=1810;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);
 for(auto& e:eyes)e.world.values[3][0]=0;
 for(auto stamp:{1820u,2000u}){testTime=stamp;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);}
 ok=ok && zoomChanges==4 && owned;
 RequestAutomaticAds(false);ok=ok && zoomChanges==5 && !owned && !immediateSeen;
 // Do not undo keyboard-owned native zoom when auto alignment disengages.
 Set(z.data(),0x1c,1);testTime=2010;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);ResetAutomaticAds();ok=ok && zoomChanges==5 && Read<int>(z.data(),0x1c)==1;
 // The EU LMG now keeps its normal, completed first-person mesh on zoom.
 std::memset(t.data()+0x10,0,16);std::memcpy(t.data()+0x10,"eu_mg",6);Set(t.data(),0x20,5u);factors[1]=.59f;
 Set(z.data(),0x1c,1);LodHook(z.data(),nullptr,1);
 ok=ok && lastLod==0 && ReadNativeOptic(&optic) && optic.magnification==1.f;
 Set(z.data(),0x34,.1f);ok=ok && ReadNativeOptic(&optic) && optic.magnification==1.f;
 Set(z.data(),0x34,0.f);
 DisableNativeOptics();LodHook(z.data(),nullptr,1);ok=ok && lastLod==1 && !ReadNativeOptic(&optic);
 printf("Native optic/auto-ADS adapter %s: x86 LOD forwarding=%d, local ownership, stale/fault/unknown fallback, fine zoom, native ADS transitions and cancellation.\n",ok?"PASS":"FAIL",forwarded);
 VirtualFree(game,0,MEM_RELEASE);return ok?0:2;
}

namespace bfvr::bf2142 {bool ReadNativeRightGripOffset(stereo::Vec3*){return false;}}
