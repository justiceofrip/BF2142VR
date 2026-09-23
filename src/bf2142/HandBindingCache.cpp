#include "HandBindingCache.h"
#include <cmath>
namespace bfvr::bf2142 {
namespace {
using M=stereo::Matrix4;
bool Foregrip(const HandBindings& b){
    const auto& l=b.leftFromWeapon.values[3];const auto& r=b.rightFromWeapon.values[3];
    const float x=l[0]-r[0],y=l[1]-r[1],z=l[2]-r[2];
    const float distance=x*x+y*y+z*z;
    // A supported rifle has its off-hand forward of its firing hand. Reject
    // the rearward/sideways reach of draw and reload poses; do not invent a
    // new grip location for a particular gun or alter its right-hand aim.
    return z>.04f && distance>=.12f*.12f && distance<.80f*.80f && std::abs(x)<.30f && std::abs(y)<.30f;
}
bool Stable(const M& a,const M& b){
    float distance=0;for(int i=0;i<3;++i){const float d=a.values[3][i]-b.values[3][i];distance+=d*d;}
    if(distance>.012f*.012f)return false;
    for(int i=0;i<3;++i){float dot=0;for(int j=0;j<3;++j)dot+=a.values[i][j]*b.values[i][j];if(dot<.998f)return false;}
    return true;
}
}
void HandBindingCache::Update(const HandBones& native,std::uint64_t now,bool allowSettlement) noexcept {
    const auto observed=CaptureHandBindings(native);
    if(!observed){candidateValid=rightCandidateValid=false;return;}
    if(!value){value=observed;started=now;}
    if(supportReady && rightReady)return;
    const bool gap=now<lastSeen || (lastSeen && now-lastSeen>150);lastSeen=now;
    // The provisional firing grip follows untouched animation until it settles.
    // Freezing the first equip/reach frame leaves the gun beside the closed fist.
    if(!rightReady){
        value->rightFromWeapon=observed->rightFromWeapon;
        if(!allowSettlement){rightCandidateValid=false;}
        else {
            if(gap || !rightCandidateValid || !Stable(rightCandidate,observed->rightFromWeapon)){
                rightCandidate=observed->rightFromWeapon;rightStableSince=now;rightCandidateValid=true;
            }
            if(now>=started && now-started>=750 && now>=rightStableSince && now-rightStableSince>=250)rightReady=true;
        }
    }
    if(supportReady)return;
    if(!allowSettlement || !Foregrip(*observed)){candidateValid=false;return;}
    if(gap || !candidateValid || !Stable(candidate,observed->leftFromWeapon)){
        candidate=observed->leftFromWeapon;stableSince=now;candidateValid=true;
    }
    if(rightReady && now>=started && now-started>=750 && now>=stableSince && now-stableSince>=250){
        value->leftFromWeapon=observed->leftFromWeapon;supportReady=true;
    }
}
}
