#pragma once
#include <cstdint>
namespace bfvr::bf2142 {
class SnapTurn {
public: float Update(bool enabled,float axis,std::int64_t time,float degrees) noexcept;
private: bool armed=false;std::int64_t previous=0;
};
class NativeTurnPulse {
public:
 bool Queue(std::uint64_t owner,float degrees,std::int64_t sample,std::uint64_t now) noexcept;
 float Consume(std::uint64_t owner,bool focused,std::uint64_t now) noexcept;
 void Cancel() noexcept {degrees=0;}
private:
 std::uint64_t owner=0,queuedAt=0;std::int64_t lastSample=0;float degrees=0;
};
struct PhysicalPosture {int stance=0;bool crouch=false,proneKey=false;};
class PhysicalStance {
public:
 PhysicalPosture Update(bool active,float drop,int nativeStance,std::int64_t time) noexcept;
 void Reset() noexcept {*this={};}
private: int target=0,candidate=0;std::int64_t since=0,last=0,pulseUntil=0,retryAfter=0;
};
}
