#pragma once
#include "HandPoseMath.h"
#include <cstdint>
namespace bfvr::bf2142 {
// Experimental v17 helper; disconnected from the v18 client after a visual regression.
// Cache hand/arm shape in weapon coordinates, never the camera or current gun.
// Weapon parts retain native firing/reload animation. The pose is reset on an
// owner change, and only replaces arms during ADS and its short exit blend.
struct AdsHandPose {
    void Apply(HandBones& native,bool ads,bool settled,std::uint64_t now) noexcept;
private:
    HandBones neutral{};bool ready=false,wasAds=false;std::uint64_t exited=0;
};
}
