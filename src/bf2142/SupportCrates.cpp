#include "SupportCrates.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
using V=stereo::Vec3;
float Distance(V a,V b){const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z;return std::sqrt(x*x+y*y+z*z);}
std::string_view Name(const std::array<std::array<char,49>,10>& n,int i){return i>0&&i<10?std::string_view(n[i].data(),std::find(n[i].begin(),n[i].end(),char(0))-n[i].begin()):std::string_view{};}
}
SupportFrame SupportCrates::Update(const SupportObservation& s){
    SupportFrame out;out.heldPose=s.left;
    const auto hand=s.leftTracked?stereo::MakeRelativePose(s.anchor,s.left):std::nullopt;
    const bool leftAvailable=hand.has_value();
    const bool gap=last && (s.time<last||s.time-last>250000000);
    if(s.rightGrab||!s.active||!leftAvailable||!s.owner||s.equipped<=0||s.equipped>=10||gap||owner!=s.owner){
        Reset();owner=s.owner;last=s.time;pressed=s.leftGrip;leftObserved=leftAvailable;return out;
    }
    const bool rising=leftAvailable&&leftObserved&&s.leftGrip&&!pressed,falling=leftAvailable&&leftObserved&&!s.leftGrip&&pressed;
    pressed=s.leftGrip;leftObserved=leftAvailable;const bool advance=s.time!=last;last=s.time;
    // A small history window smooths the release estimate without depending on refresh rate.
    const unsigned k=samples%6;if(advance&&leftAvailable){history[k]=s.left.position;times[k]=s.time;++samples;}
    V measured{};unsigned count=0;
    for(unsigned i=0;i<std::min(samples,6u);++i){const auto dt=s.time-times[i];if(dt<18000000||dt>110000000)continue;
        const float seconds=float(dt)*1.e-9f;measured.x+=(s.left.position.x-history[i].x)/seconds;measured.y+=(s.left.position.y-history[i].y)/seconds;measured.z+=(s.left.position.z-history[i].z)/seconds;++count;}
    if(count){measured.x/=count;measured.y/=count;measured.z/=count;
        if(Distance(measured,{})<=14.f)velocity=measured;else {velocity={};samples=0;}}
    else velocity={};
    // Native ammo readiness, not an invented cooldown timer, drives holster visibility.
    if(s.leftCrates)for(int i=1;i<10;++i)if(SupportCrateWeapon(Name(s.names,i)))out.unavailable[i]=!s.ammo[i].valid||(s.ammo[i].deployable>=0?s.ammo[i].deployable==0:s.ammo[i].rounds<1);
    if(!s.leftCrates){crate=0;phase=0;grabPressUntil=0;return out;}
    // Allow an intentional squeeze just before reaching a crate. Do not arm a
    // long-held grip or carry an intent across release/tracking/owner changes.
    if(!s.leftGrip)grabPressUntil=0;
    if(rising&&!crate)grabPressUntil=s.time+250000000;
    if(!crate){
        float nearest=1;
        for(const auto& slot:BodySlots())if(SupportCrateWeapon(Name(s.names,slot.item))&&!out.unavailable[slot.item]){
            const float d=Distance(hand->position,slot.offset)/(slot.radius+.035f);
            if(d<nearest){nearest=d;out.hovered=int(slot.item);}
        }
        if(s.leftGrip&&grabPressUntil&&s.time<=grabPressUntil&&out.hovered>0){grabPressUntil=0;crate=out.hovered;grabTime=s.time;released=false;restoreItem=s.keepWeapon?s.equipped:0;phase=restoreItem?5:1;deadline=s.time+1800000000;}
    }
    if(crate){
        out.busy=out.consumeLeft=true;out.crateItem=crate;
        if(falling&&!released){released=true;releasePose=s.left;if(phase==2)deadline=s.time+1200000000;throwVelocity=velocity;
            const float speed=Distance(throwVelocity,{});
            if(!std::isfinite(speed)||speed<.25f){
                // A relaxed release is a gentle toss along the left hand, not a failed gesture.
                const auto q=s.left.orientation;const V forward{-2*(q.x*q.z+q.w*q.y),2*(q.w*q.x-q.y*q.z),2*(q.x*q.x+q.y*q.y)-1};
                throwVelocity={forward.x*2.f,forward.y*2.f+.8f,forward.z*2.f};
            }else if(speed>9.f){throwVelocity.x*=9.f/speed;throwVelocity.y*=9.f/speed;throwVelocity.z*=9.f/speed;}
        }
        if(phase==5){
            if(s.equipped!=restoreItem||out.unavailable[crate]){crate=phase=restoreItem=0;out.busy=out.consumeLeft=false;return out;}
            out.preview=true;
            if(released){phase=1;grabTime=s.time;deadline=s.time+1800000000;}
        }
        out.blockFire=phase!=5;
        if(phase==1){
            out.preview=restoreItem&&s.equipped!=crate;
            if(s.equipped==crate){phase=2;deadline=s.time+(released?1200000000LL:30000000000LL);}
            else if(s.time<deadline){out.select=crate;out.selectionTime=grabTime;}
            else {out.holster=released&&!restoreItem;out.busy=out.consumeLeft=out.blockFire=out.preview=false;crate=phase=restoreItem=0;return out;}
        }
        if(phase==2){
            if(s.equipped!=crate||!s.ammo[crate].valid){out.holster=released&&!restoreItem;out.busy=out.consumeLeft=out.blockFire=out.preview=false;crate=phase=restoreItem=0;return out;}
            out.leftCrate=true;
            if(released&&s.ammo[crate].rounds>0&&s.throwReady){
                out.throwNow=true;phase=3;pulseUntil=s.time+120000000;deadline=s.time+900000000;
            }else if(s.time>deadline){phase=4;}

        }
        if(phase==3){
            out.leftCrate=true;out.throwVelocity=throwVelocity;out.fire=s.time<(restoreItem?deadline:pulseUntil);out.throwPose=releasePose;
            if(s.equipped!=crate||s.ammo[crate].rounds<1||s.time>deadline){phase=4;}
        }
        if(phase==4){
            // Completion is a one-shot holster, not a timed owner of all gun input.
            out.preview=out.leftCrate=out.fire=out.throwNow=false;
            if(restoreItem){phase=6;grabTime=s.time;deadline=s.time+1200000000;}
            else {out.busy=out.consumeLeft=out.blockFire=false;out.holster=true;crate=0;phase=0;}
        }
        if(phase==6){
            out.consumeLeft=false;
            if(s.equipped==restoreItem||s.time>=deadline||Name(s.names,restoreItem).empty()){
                out.holster=s.equipped==crate;out.busy=out.blockFire=false;crate=phase=restoreItem=0;
            }else {out.select=restoreItem;out.selectionTime=grabTime;}
        }
    }
    return out;
}
}
