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
    SupportFrame out;
    const auto hand=s.leftTracked?stereo::MakeRelativePose(s.anchor,s.left):std::nullopt;
    const bool leftAvailable=hand.has_value();
    const bool gap=last && (s.time<last||s.time-last>250000000);
    if(!s.active||!leftAvailable||!s.owner||s.equipped<=0||s.equipped>=10||gap||owner!=s.owner){
        Reset();owner=s.owner;last=s.time;pressed=s.leftGrip;leftObserved=leftAvailable;return out;
    }
    const bool rising=leftAvailable&&leftObserved&&s.leftGrip&&!pressed,falling=leftAvailable&&leftObserved&&!s.leftGrip&&pressed;
    pressed=s.leftGrip;leftObserved=leftAvailable;const bool advance=s.time!=last;last=s.time;
    // A small history window smooths the release estimate without depending on refresh rate.
    const unsigned k=samples%6;if(advance&&leftAvailable){history[k]=s.left.position;times[k]=s.time;++samples;}
    V measured{};unsigned count=0;
    for(unsigned i=0;i<std::min(samples,6u);++i){const auto dt=s.time-times[i];if(dt<18000000||dt>110000000)continue;
        const float seconds=float(dt)*1.e-9f;measured.x+=(s.left.position.x-history[i].x)/seconds;measured.y+=(s.left.position.y-history[i].y)/seconds;measured.z+=(s.left.position.z-history[i].z)/seconds;++count;}
    if(count){measured.x/=count;measured.y/=count;measured.z/=count;velocity=measured;}
    // Native ammo readiness, not an invented cooldown timer, drives holster visibility.
    if(s.leftCrates)for(int i=1;i<10;++i)if(SupportCrateWeapon(Name(s.names,i)))out.unavailable[i]=!s.ammo[i].valid||(s.ammo[i].deployable>=0?s.ammo[i].deployable==0:s.ammo[i].rounds<1);
    if(!s.leftCrates){crate=0;phase=0;return out;}
    if(!crate){
        float nearest=1;
        for(const auto& slot:BodySlots())if(SupportCrateWeapon(Name(s.names,slot.item))&&!out.unavailable[slot.item]){
            const float d=Distance(hand->position,slot.offset)/slot.radius;
            if(d<nearest){nearest=d;out.hovered=int(slot.item);}
        }
        if(rising&&out.hovered>0){crate=out.hovered;restore=s.equipped;phase=1;deadline=s.time+1800000000;}
    }
    if(crate){
        out.busy=out.consumeLeft=true;out.crateItem=crate;
        if(phase==1){
            if(s.equipped==crate){phase=2;deadline=s.time+30000000000LL;}
            else if(s.time<deadline&&s.leftGrip)out.select=crate;
            else {crate=0;phase=0;return out;}
        }
        if(phase==2){
            if(s.equipped!=crate||!s.ammo[crate].valid){crate=0;phase=0;return out;}
            out.leftCrate=true;
            if(falling||s.time>deadline){
                const float speed=Distance(velocity,{});
                if(s.ammo[crate].rounds>0 && falling && std::isfinite(speed)&&speed>.65f){
                    const float scale=std::min(1.f,9.f/speed);throwVelocity={velocity.x*scale,velocity.y*scale,velocity.z*scale};
                    out.throwNow=true;phase=3;pulseUntil=s.time+120000000;deadline=s.time+900000000;
                }else {phase=4;deadline=s.time+1200000000;}
            }
        }
        if(phase==3){
            out.leftCrate=true;out.throwVelocity=throwVelocity;out.fire=s.time<pulseUntil;
            if(s.ammo[crate].rounds<1||s.time>deadline){phase=4;deadline=s.time+1200000000;}
        }
        if(phase==4){
            if(s.equipped==restore||s.time>deadline){crate=0;phase=0;}
            else out.select=restore;
        }
    }
    return out;
}
}
