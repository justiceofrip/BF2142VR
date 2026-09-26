#include "ComfortControls.h"
#include <cmath>
namespace bfvr::bf2142 {
float SnapTurn::Update(bool enabled,float axis,std::int64_t time,float degrees) noexcept {
 const bool continuous=previous>0 && time>previous && time-previous<=250000000;
 previous=time;
 if(!enabled || !std::isfinite(axis)||!std::isfinite(degrees)||degrees<15||degrees>90){armed=false;return 0;}
 if(!continuous){armed=std::abs(axis)<.25f;return 0;}
 if(std::abs(axis)<.25f)armed=true;
 if(armed && std::abs(axis)>.7f){armed=false;return std::copysign(degrees,axis);}return 0;
}
bool NativeTurnPulse::Queue(std::uint64_t identity,float turn,std::int64_t sample,std::uint64_t now) noexcept {
 if(!identity || !std::isfinite(turn) || std::abs(turn)<15 || std::abs(turn)>90 || sample<=0)return false;
 if(owner!=identity){*this={};owner=identity;}
 if(sample<=lastSample)return false;
 lastSample=sample;degrees=turn;queuedAt=now;return true;
}
float NativeTurnPulse::Consume(std::uint64_t identity,bool focused,std::uint64_t now) noexcept {
 const float turn=degrees;degrees=0;
 return identity==owner && identity && focused && now>=queuedAt && now-queuedAt<=150 ? turn:0.f;
}
void StandingHeightReference::Ensure(float current,float configured) noexcept {
 if(!ready)Recenter(std::isfinite(configured)&&configured!=0?configured:current);
}
bool StandingHeightReference::Recenter(float current) noexcept {
 if(!std::isfinite(current))return false;
 height=current;ready=true;return true;
}
PhysicalPosture PhysicalStance::Update(bool active,float drop,int native,std::int64_t time) noexcept {
 if(!active||!std::isfinite(drop)||drop<-.8f||drop>2.5f||native<0||native>2){Reset();return {};}
 const bool gap=last==0||time<=last||time-last>250000000;last=time;
 int wanted=0;
 if(drop>(target==2?.72f:.88f))wanted=2;
 else if(drop>(target==1?.20f:.30f))wanted=1;
 if(gap){candidate=wanted;since=time;pulseUntil=0;retryAfter=time+200000000;}
 if(wanted!=candidate){candidate=wanted;since=time;}
 if(time-since>=180000000)target=candidate;
 // Native prone is a toggle, crouch is held. Observe actual native stance so
 // a rejected transition or respawn never leaves the toggle inverted.
 if(time>=retryAfter && ((target==2)!=(native==2))){pulseUntil=time+120000000;retryAfter=time+800000000;}
 return {target,target==1,time<pulseUntil};
}
}
