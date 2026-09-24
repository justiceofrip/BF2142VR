#pragma once
#include <cstdint>
namespace bfvr::bf2142 {
// Native alternate-fire is a toggle. Send one bounded press through ordinary
// input, then wait for native acknowledgement. Never spam zoom against a
// reload/sprint cancellation or server correction.
struct AutoAdsInput {
    bool owned=false,pending=false,target=false,acknowledged=false;
    std::uint64_t started=0;
    bool Observe(bool zoomed,std::uint64_t now) noexcept {
        bool cancelled=false;
        if(pending){
            if(zoomed==target){acknowledged=true;owned=target;}
            if(now<started || now-started>=750){
                cancelled=target&&!acknowledged;pending=false;
            }else if(now-started>=120 && acknowledged)pending=false;
        }
        if(!pending && owned && !zoomed){owned=false;cancelled=true;}
        return cancelled;
    }
    void Suspend(bool zoomed,std::uint64_t now) noexcept {
        Observe(zoomed,now);
        // A dropped controller/menu must not replay a not-yet-consumed press
        // to enter ADS. Retain acknowledged ownership only for a later exit.
        if(pending&&target)pending=false;
    }
    void Request(bool desired,bool zoomed,std::uint64_t now) noexcept {
        if(pending)return;
        if((desired&&!zoomed)||(!desired&&owned&&zoomed)){
            pending=true;target=desired;acknowledged=false;started=now;
        }
    }
    bool Pressed(std::uint64_t now) const noexcept {
        return pending&&now>=started&&now-started<120;
    }
};
}
