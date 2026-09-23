#pragma once
#include <cstdint>
#include <cmath>
namespace bfvr::bf2142 {
// Hysteresis in both alignment and time; a native cancellation (reload/sprint)
// requires lowering the sight before automatic ADS may acquire it again.
struct AutoAdsPolicy {
    bool active=false,blocked=false;std::uint64_t since=0,last=0;bool timing=false;
    bool Update(bool valid,float alignment,bool nativeCancelled,std::uint64_t now) noexcept {
        if(!valid || !std::isfinite(alignment) || (last && (now<last || now-last>250))){*this={};return false;}
        last=now;
        if(nativeCancelled){active=false;blocked=true;timing=false;}
        if(blocked){if(alignment<.15f)blocked=false;else return false;}
        const bool change=active?alignment<.15f:alignment>.55f;
        if(!change){timing=false;return active;}
        if(!timing){timing=true;since=now;}
        if(now>=since && now-since>=(active?300u:180u)){active=!active;timing=false;}
        return active;
    }
};
}
