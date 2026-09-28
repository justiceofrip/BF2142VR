#pragma once
#include <cstdint>
namespace bfvr::bf2142 {
struct SupportFrame;
struct WeaponGripInput {
    bool active=false,pressed=false,slotGrab=false;
    std::uint64_t owner=0;
    std::int64_t time=0;
    int equipped=0;
};
class WeaponGrip {
public:
    bool Update(const WeaponGripInput&) noexcept;
    bool Held() const noexcept {return held;}
    bool ResolveSupport(const SupportFrame&,std::int64_t time) noexcept;
    void Reset() noexcept {*this={};}
    void Holster(std::int64_t time) noexcept {held=false;autoEquipUntil=time+1500000000;}
private:
    bool held=true,previousPressed=false,observed=false;
    std::uint64_t owner=0;
    std::int64_t previousTime=0,autoEquipUntil=0;
    int equipped=0;
};
}
