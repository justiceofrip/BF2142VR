#pragma once
#include <cstdint>
namespace bfvr::bf2142 {
// A distinct equipment cue, with bounded repeats so one missed/too-brief hover
// event does not make a behind-shoulder holster silent for the rest of a reach.
class EquipmentHaptics {
 std::int64_t last=0,pulse=0;int previous=-1;
public:
 bool Update(bool active,int target,bool grabbed,std::int64_t now) noexcept {
  if(!active||now<=0){*this={};return false;}
  if(last&&(now<last||now-last>250000000)){previous=-1;pulse=0;}
  last=now;
  const bool cue=grabbed||(target>=0&&(target!=previous||!pulse||now-pulse>=400000000));
  previous=target;
  if(!cue||(pulse&&now-pulse<80000000&&!grabbed))return false;
  pulse=now;return true;
 }
 void Reset() noexcept {*this={};}
};
}
