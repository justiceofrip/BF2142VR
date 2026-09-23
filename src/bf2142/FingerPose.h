#pragma once
#include "presenter/SharedPresentationProtocol.h"
#include <array>
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
// Native chain order: thumb, pointing/index finger, ring, middle, little.
inline std::array<float,5> ControllerFingerCurls(const shared::SharedControllerHandSample& h) noexcept {
    const auto analog=[&](DWORD flag,float value){return (h.flags&flag)&&std::isfinite(value)?std::clamp(value,0.f,1.f):0.f;};
    const float grip=analog(shared::kControllerHandFlagSqueezeActive,h.squeezeValue);
    float index=analog(shared::kControllerHandFlagTriggerActive,h.triggerValue);
    if((h.flags&shared::kControllerHandFlagTriggerTouchActive) && (h.flags&shared::kControllerHandFlagTriggerTouched))index=std::max(index,.55f);
    float thumb=std::max(grip,index)*.7f;
    if(h.flags&shared::kControllerHandFlagThumbTouchActive)thumb=(h.flags&shared::kControllerHandFlagThumbTouched)?1.f:0.f;
    return {thumb,index,grip,grip,grip};
}
}
