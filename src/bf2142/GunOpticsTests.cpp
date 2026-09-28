#include "GunOptics.h"
#include "AutoAdsPolicy.h"
#include <cstdio>
#include <cmath>
#include <limits>
using namespace bfvr;
namespace {
stereo::Matrix4 Identity(){stereo::Matrix4 m{};for(int i=0;i<4;++i)m.values[i][i]=1;return m;}
bool Close(float a,float b){return std::abs(a-b)<.00001f;}
}
int main(){
    using namespace bf2142;
    AutoAdsPolicy ads;
    if(ads.Update(true,1,false,1000)||ads.Update(true,1,false,1100)||!ads.Update(true,1,false,1180))return 80;
    if(!ads.Update(true,.25f,false,1230)||!ads.Update(true,0,false,1250)||!ads.Update(true,0,false,1450)||ads.Update(true,0,false,1550))return 81;
    // No button can force alignment. Lowering the sight releases it; native
    // cancellation requires physically lowering before acquiring again.
    if(ads.Update(true,0,false,1560)||ads.Update(true,1,true,1570)||ads.Update(true,1,false,1600))return 82;
    if(ads.Update(true,0,false,1650)||ads.Update(true,1,false,1670)||!ads.Update(true,1,false,1850))return 83;
    if(ads.Update(false,1,false,1860)||ads.active)return 84;
    if(ads.Update(true,1,false,1900)||ads.Update(true,1,false,2300)||ads.active)return 85;
    if(ads.Update(true,std::numeric_limits<float>::quiet_NaN(),false,2310))return 86;

    const auto* d=FindGunOptic("eu_ar_rifle");
    if(!d || FindGunOptic("unknown") || FindGunOptic("eu_ar_rifle_mod"))return 1;
    GunOptic gun{d,Identity(),2};
    CameraInput source{Identity(),.01f,1000};source.world.values[3]={d->center.x,d->center.y,d->center.z-.2f,1};
    auto eye=MakeEyeCamera(source,{}, {},{-1,1,1,-1});if(!eye)return 2;
    std::array<EyeCamera,2> eyes{*eye,*eye};eyes[1].world.values[3][0]+=.064f;
    const auto view=MakeOpticView(gun,eyes);if(!view || view->visibility[0]<.99f || view->visibility[1]!=0 || !Close(view->relief,.2f))return 3;
    if(!Close(view->fov.right,d->halfWidth/.4f)||!Close(view->fov.up,d->halfHeight/.4f))return 4;
    // 4x halves the visible angular field; the HMD eye projections never change.
    gun.magnification=4;const auto zoom=MakeOpticView(gun,eyes);if(!zoom||!Close(zoom->fov.right*2,view->fov.right))return 5;
    gun.magnification=2;
    const auto original=eyes;
    std::vector<DWORD> pixels(512*512,0xff112233),scope(512*512,0xff336699);
    const auto count=CompositeGunOptic(pixels,scope,512,512,gun,eyes[0],*view,0,false);
    if(count<200 || count>4000 || pixels.front()!=0xff112233 || pixels.back()!=0xff112233)return 6;
    bool changed=false,reticle=false;for(auto c:pixels){changed|=c==0xff336699;reticle|=c==0xffffdc70;}if(!changed||!reticle)return 7;
    auto absent=std::vector<DWORD>(512*512,0xff112233);
    if(CompositeGunOptic(absent,scope,512,512,gun,eyes[1],*view,1,false))return 8;
    for(auto c:absent)if(c!=0xff112233)return 9;
    pixels.assign(pixels.size(),0xff112233);CompositeGunOptic(pixels,scope,512,512,gun,eyes[0],*view,0,true);
    reticle=false;for(auto c:pixels)reticle|=c==0xff70dcff;if(!reticle)return 10;
    // Off-axis/behind-glass/too-far eyes never request another scene render.
    for(auto& e:eyes)e.world.values[3][0]+=.2f;if(MakeOpticView(gun,eyes))return 11;
    eyes=original;for(auto& e:eyes)e.world.values[3][2]=d->center.z+.1f;if(MakeOpticView(gun,eyes))return 12;
    eyes=original;for(auto& e:eyes)e.world.values[3][2]-=1;if(MakeOpticView(gun,eyes))return 13;
    gun.magnification=std::numeric_limits<float>::quiet_NaN();if(MakeOpticView(gun,original))return 14;
    gun.magnification=2;gun.gun.values[0][0]=0;if(MakeOpticView(gun,original))return 15;
    gun.gun=Identity();
    // Common rigid world rotation/translation keeps relief, magnification and
    // aperture visibility identical, including scope roll.
    auto world=Identity();world.values[0]={0,1,0,0};world.values[1]={-1,0,0,0};world.values[3]={20,30,40,1};
    gun.gun=world;eyes=original;for(auto& e:eyes)e.world=Multiply(e.world,world);
    const auto moved=MakeOpticView(gun,eyes);if(!moved||!Close(moved->relief,view->relief)||!Close(moved->fov.right,view->fov.right))return 16;
    // Each measured stock profile has a usable aperture and exact name match.
    for(const char* name:{"as_ar_rifle","eu_sni","as_sni","unl_adv_sni"}){
        const auto* p=FindGunOptic(name);if(!p)return 17;
        gun={p,Identity(),p->magnification};eyes=original;for(auto& e:eyes)e.world.values[3]={p->center.x,p->center.y,p->center.z-.2f,1};
        const auto calibrated=MakeOpticView(gun,eyes);if(!calibrated)return 18;
        const auto& m=calibrated->world.values;const float toZero=(100-m[3][2])/m[2][2];
        if(!Close(m[3][0]+toZero*m[2][0],0)||!Close(m[3][1]+toZero*m[2][1],0))return 19;
    }
    // The LMG's native glass is a 1x reflex sight: no scope image required,
    // and every changed pixel belongs to the tiny reticle within its aperture.
    const auto* mg=FindGunOptic("eu_mg");if(!mg||mg->nativeFactor!=.59f||mg->magnification!=1)return 20;
    gun={mg,Identity(),1};eyes=original;
    for(auto& e:eyes)e.world.values[3]={mg->center.x,mg->center.y,mg->center.z-.2f,1};
    const auto reflex=MakeOpticView(gun,eyes);if(!reflex)return 21;
    pixels.assign(pixels.size(),0xff112233);
    const auto dots=CompositeGunOptic(pixels,{},512,512,gun,eyes[0],*reflex,0,false);
    if(dots==0||dots>40)return 22;
    size_t colored=0;for(auto p:pixels)colored+=p!=0xff112233;
    if(colored!=dots)return 23;
    // No effect off-axis and no new rectangular magnification around the gun.
    eyes[0].world.values[3][0]+=.1f;
    const auto offAxis=MakeOpticView(gun,eyes);if(!offAxis||offAxis->visibility[0]!=0)return 24;
    if(CompositeGunOptic(pixels,{},512,512,gun,eyes[0],*offAxis,0,false))return 25;
    std::vector<DWORD> baseline(512*512),hud(512*512);baseline[5]=hud[5]=0xffaabbcc;hud[512*300+256]=0x80800000;
    IsolateOpticHud(hud,baseline,512,512);if(hud.empty()||hud[5]||hud[512*300+256]!=0x80800000)return 26;
    gun={d,Identity(),2};pixels.assign(pixels.size(),0xff112233);std::fill(hud.begin(),hud.end(),0xff123456);
    if(!CompositeGunOptic(pixels,scope,512,512,gun,original[0],*view,0,false,hud)||pixels.front()!=0xff112233)return 27;
    bool native=false;for(auto c:pixels){native|=c==0xff123456;if(c==0xffffdc70)return 28;}if(!native)return 29;
    hud=baseline;IsolateOpticHud(hud,baseline,512,512);if(!hud.empty())return 30;
    puts("Weapon-specific optics, eye relief, monocular alignment, magnification, aperture clipping, reticle colors and rigid-frame invariance passed.");
}
