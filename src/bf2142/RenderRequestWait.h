#pragma once
#include <cstdint>
namespace bfvr::bf2142 {
// Keep the current engine frame alive while the XR consumer owns its textures
// or is preparing a pose. Repeatedly returning to the outer loop advances the
// game clock without evaluating native draw/reload animations at that cadence.
// Elapsed-time catch-up alone cannot repair those render-dependent transitions.
// A disconnected/suspended presenter must still yield the game thread promptly.
template<class TryRequest,class Running,class Clock,class Wait>
bool AwaitRenderRequest(TryRequest attempt,Running running,Clock clock,Wait wait) {
    const auto start=clock();
    for(;;){
        if(attempt())return true;
        const auto now=clock();
        if(!running() || now<start || now-start>=100)return false;
        wait();
    }
}
}
