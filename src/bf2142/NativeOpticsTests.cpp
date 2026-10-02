#include <Windows.h>
static ULONGLONG testTime=1000;
static ULONGLONG TestTime(){return testTime;}
#define GetTickCount64 TestTime
#include "NativeOptics.cpp"
#undef GetTickCount64
#include <array>
#include <cstdio>
using namespace bfvr::bf2142;
namespace {bool networkTest=false;int zoomChanges=0;bool immediateSeen=false;
void __fastcall ZoomSpy(void* base,void*,int step,bool immediate){++zoomChanges;immediateSeen|=immediate;std::memcpy(static_cast<BYTE*>(base)+0x1c,&step,sizeof(step));}
TrackedWeaponFrame tracked;bool tracking=true;int forwarded=0,lastLod=-9;void* lastSelf=nullptr;
void __fastcall Original(void* self,void*,int lod){++forwarded;lastSelf=self;lastLod=lod;}
template<class T>void Set(BYTE* p,size_t off,T value){std::memcpy(p+off,&value,sizeof(value));}
}
namespace bfvr::bf2142 {bool NetworkClientActive(){return networkTest;}bool ReadTrackedWeaponFrame(TrackedWeaponFrame* out,bool){if(!tracking)return false;*out=tracked;return true;}bool IsLocalTrackedWeapon(void* w){return tracking && w==tracked.weapon;}}
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
 // New exact stock names/factors use the same validated firearm component.
 // Launcher .85 must be accepted; altered templates still fail closed.
 for(const char* profile:{"eu_smg","as_smg","eu_av","as_av","unl_av_rifle"}){
  const auto* def=FindGunOptic(profile);
  std::memset(t.data()+0x10,0,16);std::memcpy(t.data()+0x10,profile,std::strlen(profile));Set(t.data(),0x20,unsigned(std::strlen(profile)));
  factors[1]=def->nativeFactor;const auto unchanged=z;
  LodHook(z.data(),nullptr,1);
  if(!ReadNativeOptic(&optic)||optic.definition!=def||std::abs(optic.magnification-def->magnification)>.0001f||lastLod!=0||z!=unchanged){printf("Stock optic profile failed: %s\n",profile);return 4;}
  factors[1]+=.01f;ok=ok&&!ReadNativeOptic(&optic);
 }
 std::memset(t.data()+0x10,0,16);std::memcpy(t.data()+0x10,"eu_ar_rifle",12);Set(t.data(),0x20,11u);factors[1]=.484f;
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
 // Dedicated mode uses the normal input stream; local setters cannot own
 // authoritative zoom. Model delayed acknowledgement and server correction.
 networkTest=true;enabled=true;Set(z.data(),0x1c,0);Set(z.data(),0x34,0.f);
 ResetAutomaticAds();networkAds={};autoWeapon=nullptr;
 // EU LMG alignment has a different measured sight center.
 std::memset(t.data()+0x10,0,16);std::memcpy(t.data()+0x10,"eu_ar_rifle",12);Set(t.data(),0x20,11u);factors[1]=.484f;
 for(auto& e:eyes)e.world.values[3]={0,.095f,-.281f,1};
 const int prior=zoomChanges;
 #define CHECK(x) do{if(!(x)){printf("Network ADS failed line %d: %s\n",__LINE__,#x);return 3;}}while(0)
 const auto tick=[&](ULONGLONG stamp){testTime=stamp;RequestAutomaticAds(true);UpdateAutomaticAds(eyes);};
 tick(3000);tick(3180);CHECK(AutomaticAdsButton(true)&&!AutomaticAdsButton(false)&&zoomChanges==prior);
 tick(3220);CHECK(AutomaticAdsButton(true)&&!autoPolicy.blocked); // no premature cancellation awaiting native input
 Set(z.data(),0x1c,1);tick(3260);CHECK(networkAds.owned&&AutomaticAdsButton(true));
 tick(3300);CHECK(!AutomaticAdsButton(true));
 for(unsigned i=1;i<=60;++i){tick(3300+i*50);CHECK(networkAds.owned&&!AutomaticAdsButton(true)&&Read<int>(z.data(),0x1c)==1);}
 // Lowering emits one native toggle, rather than clearing the local field.
 for(auto& e:eyes)e.world.values[3][0]=1;
 tick(6350);tick(6550);tick(6650);CHECK(AutomaticAdsButton(true)&&Read<int>(z.data(),0x1c)==1);
 Set(z.data(),0x1c,0);tick(6700);tick(6770);CHECK(!networkAds.owned&&!AutomaticAdsButton(true));
 for(auto& e:eyes)e.world.values[3][0]=0;
 tick(6800);tick(6980);CHECK(AutomaticAdsButton(true));Set(z.data(),0x1c,1);tick(7020);tick(7100);
 Set(z.data(),0x1c,0);tick(7150);CHECK(autoPolicy.blocked&&!networkAds.owned&&!AutomaticAdsButton(true));
 tick(7200);CHECK(!AutomaticAdsButton(true)); // genuine native cancellation is respected
 for(auto& e:eyes)e.world.values[3][0]=1;tick(7250);
 for(auto& e:eyes)e.world.values[3][0]=0;tick(7300);tick(7480);Set(z.data(),0x1c,1);tick(7520);tick(7600);
 RequestAutomaticAds(false);CHECK(!AutomaticAdsButton(false)&&Read<int>(z.data(),0x1c)==1);
 testTime=7700;CHECK(AutomaticAdsButton(true)); // release on safe gameplay return
 Set(z.data(),0x1c,0);testTime=7820;CHECK(!AutomaticAdsButton(true)&&!networkAds.owned);
 // Losing tracking before native input is consumed cannot replay an entry.
 for(auto& e:eyes)e.world.values[3][0]=0;
 tick(7850);tick(8030);CHECK(AutomaticAdsButton(true));RequestAutomaticAds(false);
 CHECK(!AutomaticAdsButton(true)&&!networkAds.pending&&!networkAds.owned);
 // A dead/stale/different local weapon must never receive the queued toggle.
 networkAds={true};autoWeapon=w.data();tracking=false;CHECK(!AutomaticAdsButton(true)&&!autoWeapon);tracking=true;
 // Native/manual zoom is never claimed or released by the automatic adapter.
 networkAds={};Set(z.data(),0x1c,1);tick(8100);tick(8280);RequestAutomaticAds(false);CHECK(!AutomaticAdsButton(true)&&!networkAds.owned);
 // Unacknowledged press expires once and blocks reacquisition until lowered.
 Set(z.data(),0x1c,0);tick(8300);tick(8480);CHECK(AutomaticAdsButton(true));
 for(auto stamp:{8600u,8800u,9000u,9250u})tick(stamp);
 CHECK(autoPolicy.blocked&&!AutomaticAdsButton(true)&&zoomChanges==prior);
 #undef CHECK
 printf("Native optic/auto-ADS adapter %s: x86 LOD forwarding=%d, local ownership, stale/fault/unknown fallback, fine zoom, native ADS transitions and cancellation.\n",ok?"PASS":"FAIL",forwarded);
 VirtualFree(game,0,MEM_RELEASE);return ok?0:2;
}

namespace bfvr::bf2142 {bool ReadNativeRightGripOffset(stereo::Vec3*){return false;}}
