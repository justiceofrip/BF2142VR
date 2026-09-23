#include "AdsHandPose.h"
namespace bfvr::bf2142 {
void AdsHandPose::Apply(HandBones& native,bool ads,bool settled,std::uint64_t now) noexcept {
    const auto inverse=InverseRigid(native[54]);if(!inverse)return;
    if(!ads && wasAds)exited=now;wasAds=ads;
    const bool blending=exited && now>=exited && now-exited<350;
    if(!ads && !blending && settled){
        for(int i=2;i<54;++i)if(!InverseRigid(native[i]))return;
        for(int i=2;i<54;++i)neutral[i]=Multiply(native[i],*inverse);
        ready=true;return;
    }
    if(ready && (ads || blending))for(int i=2;i<54;++i)native[i]=Multiply(neutral[i],native[54]);
}
}
