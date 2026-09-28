#include "MotionActions.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
using V=stereo::Vec3;
V Sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V Scale(V a,float b){return {a.x*b,a.y*b,a.z*b};}
V Mix(V a,V b,float t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t};}
float Length(V a){return std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);}
bool Finite(V a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
}
stereo::Vec3 TransformMotionVector(V v,const stereo::Matrix4& m) noexcept {
    return {v.x*m.values[0][0]+v.y*m.values[1][0]+v.z*m.values[2][0],v.x*m.values[0][1]+v.y*m.values[1][1]+v.z*m.values[2][1],v.x*m.values[0][2]+v.y*m.values[1][2]+v.z*m.values[2][2]};
}
MotionResult MotionActions::Update(const MotionInput& in) noexcept {
    MotionResult out;
    if(!in.active||!in.owner||in.time<=0||in.kind!=MotionKind::Knife||!Finite(in.hand)||!Finite(in.head)){Reset();return out;}
    const auto elapsed=in.time-previous.time;
    if(in.owner!=previous.owner||in.kind!=previous.kind||elapsed<0||elapsed>100000000||!previous.active){
        Reset();previous=in;started=in.time;restSince=in.time;return out;
    }
    if(elapsed==0){out.primary=in.time<pressedUntil;out.primed=primed;return out;}
    const float dt=float(elapsed)*1.e-9f;
    const V step=Sub(in.hand,previous.hand),headStep=Sub(in.head,previous.head);
    // Reject tracking jumps, including a recenter/return that looks like a swing.
    if(Length(step)>.45f||Length(step)/dt>14.f||Length(headStep)>.30f){Reset();previous=in;started=in.time;restSince=in.time;return out;}
    const float blend=1-std::exp(-dt/0.035f);
    velocity=Mix(velocity,Scale(step,1/dt),blend);
    relativeVelocity=Mix(relativeVelocity,Scale(Sub(step,headStep),1/dt),blend);
    const float speed=Length(relativeVelocity);
    const bool ready=in.time-started>=450000000;
    if(speed<.4f){
        if(!restSince)restSince=in.time;
        if(in.time-restSince>=90000000){rested=true;travel=0;swingSince=0;}
    }else restSince=0;
    if(in.kind==MotionKind::Knife){
        if(rested && speed>.4f){
            if(!swingSince)swingSince=in.time;
            travel+=Length(Sub(step,headStep));
            if(in.time-swingSince>350000000){rested=false;travel=0;}
        }
        if(ready && rested && speed>.65f && travel>.045f && in.time-lastStrike>550000000 && Length(Sub(in.hand,in.head))>.20f){
            out.action=MotionKind::Knife;out.position=in.hand;out.velocity=relativeVelocity;
            pressedUntil=in.time+140000000;lastStrike=in.time;rested=false;travel=0;swingSince=0;
        }
    }
    out.primary=in.time<pressedUntil;out.primed=primed;previous=in;return out;
}
}
