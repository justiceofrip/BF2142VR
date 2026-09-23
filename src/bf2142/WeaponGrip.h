#pragma once
#include <cstdint>
namespace bfvr::bf2142 {
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
    void Reset() noexcept {*this={};}
private:
    bool held=true,previousPressed=false,observed=false;
    std::uint64_t owner=0;
    std::int64_t previousTime=0;
    int equipped=0;
};
}
