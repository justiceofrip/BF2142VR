#pragma once
#include <cstdint>
#include <optional>
namespace bfvr::bf2142 {
// A missing controller packet is unknown, not evidence of lost HMD focus.
inline std::optional<bool> ObserveRuntimeFocus(bool coherent,std::int64_t sampleTime,
    std::int64_t frameTime,bool focused) noexcept {
    if(!coherent || sampleTime<=0 || frameTime<=0)return {};
    const auto age=sampleTime>frameTime?sampleTime-frameTime:frameTime-sampleTime;
    if(age>150000000)return {};
    return focused;
}
// A brief bad sample is not a headset removal. Recenter once after a sustained
// loss of runtime focus/tracking, and never continuously while wearing it.
struct RecenterPolicy {
    bool seenTracked=false,lost=false;std::uint64_t lostAt=0;
    bool Update(bool tracked,std::uint64_t now) noexcept {
        if(!tracked){if(seenTracked && !lost){lost=true;lostAt=now;}return false;}
        const bool reset=lost && now>=lostAt && now-lostAt>=750;
        seenTracked=true;lost=false;return reset;
    }
};
}
