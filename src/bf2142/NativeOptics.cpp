#include "NativeOptics.h"
#include "NativeHands.h"
#include "AutoAdsPolicy.h"
#include "AutoAdsInput.h"
#include "multiplayer/NativeNetwork.h"
#include <MinHook.h>
#include <cmath>
#include <cstring>
#include <algorithm>
namespace bfvr::bf2142 {
namespace {
BYTE* game=nullptr;bool installed=false,enabled=false,reported=false;LogFunction logLine=nullptr;
using SetZoomLod=void(__thiscall*)(void*,int);
SetZoomLod nativeLod=nullptr;
using SetZoom=void(__thiscall*)(void*,int,bool);
SetZoom setZoom=nullptr;
bool autoEnabled=false,autoGameplay=false,owned=false;
void* autoWeapon=nullptr;AutoAdsPolicy autoPolicy;AutoAdsInput networkAds;
ULONGLONG autoRequestTime=0;

template<class T>T Read(const void* p,size_t offset=0){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+offset,sizeof(v));return v;}
bool Profile(){
    __try {
        const BYTE setter[]={0x55,0x8b,0xec,0x53,0x8b,0x5d,8,0x85,0xdb,0x56,0x57,0x8b,0xf9};
        const BYTE name[]={0x8d,0x41,0x0c,0xc3};
        const BYTE current[]={0x8b,0x41,0x0c,0xc3};
        const BYTE zoom[]={0x55,0x8b,0xec,0x8b,0x45,8,0x6a,1,0x50,0x83,0xc1,0xf0,0xe8};
        const BYTE zoomEntry[]={0x55,0x8b,0xec,0x51,0x56,0x8b,0xf1,0x8b,0x46,0x0c,0x8d,0x88,0xa0,1,0,0};
        const BYTE factor[]={0x55,0x8b,0xec,0x57,0x8b,0x7d,8,0x85,0xff};
        return game && Read<void*>(game,0x571360+39*4)==game+0x1da610 &&
            !std::memcmp(game+0x1da610,zoom,sizeof(zoom)) &&
            !std::memcmp(game+0x1da3f0,zoomEntry,sizeof(zoomEntry)) &&
            Read<int>(game,0x1da61d)==-0x231 &&
            game[0x1da52c]==0xc2 && game[0x1da52d]==8 && game[0x1da52e]==0 &&
            Read<void*>(game,0x571438+17*4)==game+0x1da770 &&
            Read<void*>(game,0x571360+40*4)==game+0x190530 &&
            Read<void*>(game,0x571360+49*4)==game+0x1da820 &&
            Read<void*>(game,0x56c758+10*4)==game+0xb7510 &&
            std::memcmp(game+0x1da770,setter,sizeof(setter))==0 &&
            std::memcmp(game+0xb7510,name,sizeof(name))==0 &&
            std::memcmp(game+0x190530,current,sizeof(current))==0 &&
            std::memcmp(game+0x1da820,factor,sizeof(factor))==0 &&
            game[0x1da7e1]==0xc2 && game[0x1da7e2]==4 && game[0x1da7e3]==0;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
struct Weapon {const OpticDefinition* definition=nullptr;void* zoomBase=nullptr;int step=0;float factor=0;};
bool ReadWeapon(void* weapon,Weapon* result){
    __try {
        if(!weapon||Read<void*>(weapon)!=game+0x56cd88)return false;
        const auto t=Read<const BYTE*>(weapon,0x24);
        if(!t||Read<void*>(t)!=game+0x56c758)return false;
        // Native string at template +0xc: allocator, 16-byte buffer/pointer,
        // length and capacity. Read only bounded ASCII identifiers.
        const auto length=Read<unsigned>(t,0x20),capacity=Read<unsigned>(t,0x24);
        if(length<3||length>48||capacity<length||capacity>4096)return false;
        const char* text=capacity<16?reinterpret_cast<const char*>(t+0x10):Read<const char*>(t,0x10);
        if(!text)return false;char name[49]{};
        for(unsigned i=0;i<length;++i){char c=text[i];if(c>='A'&&c<='Z')c=char(c-'A'+'a');if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return false;name[i]=c;}
        if(text[length]!=0)return false;
        const auto definition=FindGunOptic(std::string_view(name,length));if(!definition)return false;
        // GenericFireArm's bounded component fields; match the exact zoom
        // interface, its owning weapon and template, not a coincidental pointer.
        for(size_t offset=0x1b4;offset<=0x1e8;offset+=4){
            const auto component=Read<const BYTE*>(weapon,offset);
            if(!component || Read<void*>(component)!=game+0x571360)continue;
            const auto base=component-0x10;
            if(Read<void*>(base)!=game+0x571438 || Read<void*>(base,0xc)!=weapon)return false;
            const auto zoom=Read<const BYTE*>(component,0x20);
            if(!zoom||Read<void*>(zoom)!=game+0x571640||Read<int>(zoom,0x2c)!=1)return false;
            const auto begin=Read<const float*>(zoom,0x18),end=Read<const float*>(zoom,0x1c);
            if(!begin||end<begin||size_t(end-begin)<2||size_t(end-begin)>16)return false;
            if(!std::isfinite(begin[0])||std::abs(begin[0])>.00001f||!std::isfinite(begin[1])||std::abs(begin[1]-definition->nativeFactor)>.0001f)return false;
            const int step=Read<int>(component,0xc);
            if(step<0||size_t(step)>=size_t(end-begin))return false;
            const float fine=Read<float>(component,0x24);
            if(!std::isfinite(fine)||fine<0||fine>.4f)return false;
            const float factor=begin[step]-fine;
            if(step && (!std::isfinite(factor)||factor<.04f||factor>.85f))return false;
            *result={definition,const_cast<BYTE*>(base),step,factor};return true;
        }
        return false;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void __fastcall LodHook(void* self,void*,int lod){
    TrackedWeaponFrame frame;Weapon weapon;
    // Only the local supported optic's visual LOD changes. Native zoom state,
    // spread, recoil, fine tuning, cadence and every other owner pass through.
    if(enabled && lod==1 && ReadTrackedWeaponFrame(&frame,true) && ReadWeapon(frame.weapon,&weapon) && weapon.zoomBase==self)lod=0;
    nativeLod(self,lod);
}
}
bool InstallNativeOptics(LogFunction logger){
    if(installed)return enabled;game=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));logLine=logger;
    if(!Profile())return false;auto entry=game+0x1da770;
    if(MH_CreateHook(entry,LodHook,reinterpret_cast<void**>(&nativeLod))!=MH_OK)return false;
    if(MH_EnableHook(entry)!=MH_OK){MH_RemoveHook(entry);return false;}
    setZoom=reinterpret_cast<SetZoom>(game+0x1da3f0);installed=enabled=true;logger("VR optics connected: verified local zoom LOD; native ADS gameplay retained; complete stock optical-zoom roster; physical alignment uses no grip binding.");return true;
}
bool ReadNativeOptic(GunOptic* optic){
    if(!enabled||!optic)return false;TrackedWeaponFrame frame;Weapon weapon;
    if(!ReadTrackedWeaponFrame(&frame)||!ReadWeapon(frame.weapon,&weapon)||!weapon.step)return false;
    const float magnification=weapon.definition->magnification<=1.f?1.f:std::clamp(weapon.definition->magnification*weapon.definition->nativeFactor/weapon.factor,1.f,16.f);
    *optic={weapon.definition,frame.world,magnification};
    if(!reported && logLine){reported=true;logLine("Native VR optic active: %s, zoom step=%d, magnification=%.2f; gameplay zoom retained.",weapon.definition->name,weapon.step,magnification);}
    return true;
}
void DisableNativeOptics(){ResetAutomaticAds();if(enabled && logLine)logLine("VR optic rendering disabled after resource failure; native zoom presentation restored.");enabled=false;}
}

namespace bfvr::bf2142 {
bool NativeWeaponAds(void* object){Weapon w;return enabled && ReadWeapon(object,&w) && w.step>0;}
void ConfigureAutomaticAds(bool value){autoEnabled=value;}
bool HandlesAutomaticAds(){TrackedWeaponFrame f;Weapon w;return autoEnabled&&enabled&&ReadTrackedWeaponFrame(&f,true)&&ReadWeapon(f.weapon,&w);}
void ResetAutomaticAds(){
    // Resolve the current local inventory again before releasing a zoom we own.
    // Never retain or call an old component after death/equip/map transitions.
    Weapon w;
    if(NetworkClientActive()){
        // Suspend presses immediately. Keep only a still-local owner so the
        // normal input path can release its zoom on return from a menu or a
        // tracking interruption. Never call the local-only setter in this mode.
        if(autoWeapon && IsLocalTrackedWeapon(autoWeapon) && ReadWeapon(autoWeapon,&w))
            networkAds.Suspend(w.step>0,GetTickCount64());
        else {autoWeapon=nullptr;networkAds={};}
        owned=false;autoPolicy={};autoGameplay=false;return;
    }
    if(owned && setZoom && IsLocalTrackedWeapon(autoWeapon) && ReadWeapon(autoWeapon,&w) && w.step>0)setZoom(w.zoomBase,0,false);
    autoWeapon=nullptr;owned=false;autoPolicy={};autoGameplay=false;
}
void RequestAutomaticAds(bool gameplay){
    if(!gameplay || !autoEnabled || !enabled){ResetAutomaticAds();return;}
    autoGameplay=true;autoRequestTime=GetTickCount64();
}
void UpdateAutomaticAds(const std::array<EyeCamera,2>& eyes){
    TrackedWeaponFrame f;Weapon w;
    if(!autoEnabled || !enabled || !autoGameplay || GetTickCount64()-autoRequestTime>150 || !ReadTrackedWeaponFrame(&f) || !ReadWeapon(f.weapon,&w)){ResetAutomaticAds();return;}
    if(autoWeapon!=f.weapon){ResetAutomaticAds();networkAds={};autoWeapon=f.weapon;autoGameplay=true;}
    GunOptic candidate{w.definition,f.world,w.definition->magnification};
    const auto view=MakeOpticView(candidate,eyes);
    const float alignment=view?std::max(view->visibility[0],view->visibility[1]):0;
    const bool network=NetworkClientActive();
    const bool cancelled=network?networkAds.Observe(w.step>0,GetTickCount64()):owned && w.step==0;
    if(cancelled)owned=false;
    const bool desired=autoPolicy.Update(true,alignment,cancelled,GetTickCount64());
    if(network){networkAds.Request(desired,w.step>0,GetTickCount64());return;}
    // Manual keyboard zoom and native fine tuning remain native. Only release
    // the zoom this adapter engaged; reload/sprint cancellation is respected.
    if(desired && !w.step){setZoom(w.zoomBase,1,false);Weapon after;if(ReadWeapon(f.weapon,&after)&&after.step>0)owned=true;}
    else if(!desired && owned){if(w.step)setZoom(w.zoomBase,0,false);owned=false;}
}
}

namespace bfvr::bf2142 {
bool AutomaticAdsButton(bool allowInput){
    if(!NetworkClientActive())return false;
    Weapon w;
    if(!autoWeapon || !IsLocalTrackedWeapon(autoWeapon) || !ReadWeapon(autoWeapon,&w)){
        networkAds={};autoWeapon=nullptr;return false;
    }
    if(!allowInput)return false;
    const auto now=GetTickCount64();
    if(!autoGameplay){
        networkAds.Observe(w.step>0,now);
        networkAds.Request(false,w.step>0,now);
    }else if(now<autoRequestTime || now-autoRequestTime>150)return false;
    return networkAds.Pressed(now);
}
bool ReadNativeSightAlignmentOffset(stereo::Vec3* offset){
    TrackedWeaponFrame f;Weapon w;stereo::Vec3 grip;
    if(!offset||!enabled||!ReadTrackedWeaponFrame(&f,true)||!ReadWeapon(f.weapon,&w)||!ReadNativeRightGripOffset(&grip))return false;
    const auto c=w.definition->center;
    // Runtime coordinates: +X right, +Y up, -Z forward. Align to the left
    // desktop eye, using the same measured sight and accepted wrist binding.
    *offset={-.032f-c.x+grip.x,-c.y+grip.y,-(.35f-c.z+grip.z)};return true;
}
}
