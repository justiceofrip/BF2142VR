#pragma once
#include "HandPoseMath.h"
#include <cstdint>
namespace bfvr::bf2142 {
// Settle both authored attachments after equip. Never permanently capture
// the first draw frame, and never recapture settled grips during reload/ADS.
struct HandBindingCache {
    std::optional<HandBindings> value;
    bool supportReady=false,rightReady=false;
    void Update(const HandBones& native,std::uint64_t now,bool allowSettlement=true) noexcept;
private:
    stereo::Matrix4 candidate{},rightCandidate{};
    std::uint64_t started=0,stableSince=0,lastSeen=0,rightStableSince=0;
    bool candidateValid=false,rightCandidateValid=false;
};
}
