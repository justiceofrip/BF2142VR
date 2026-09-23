#pragma once
#include "TrackingMath.h"
#include <algorithm>
#include <vector>
#include <windows.h>
#include <cmath>
namespace bfvr::bf2142 {
namespace face {
using V=stereo::Vec3;
inline V Sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V Add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V Mul(V a,float t){return {a.x*t,a.y*t,a.z*t};}
inline float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float SegmentDistance2(V p,V a,V b){
    const auto d=Sub(b,a);const float n=Dot(d,d);
    const float t=n>1.e-10f?std::clamp(Dot(Sub(p,a),d)/n,0.f,1.f):0;
    const auto offset=Sub(p,Add(a,Mul(d,t)));return Dot(offset,offset);
}
inline float TriangleDistance2(V p,V a,V b,V c){
    const auto ab=Sub(b,a),ac=Sub(c,a),n=Cross(ab,ac);const float n2=Dot(n,n);
    if(n2>1.e-12f){const float plane=Dot(Sub(p,a),n);const auto q=Sub(p,Mul(n,plane/n2));
        if(Dot(Cross(ab,Sub(q,a)),n)>=0 && Dot(Cross(Sub(c,b),Sub(q,b)),n)>=0 && Dot(Cross(Sub(a,c),Sub(q,c)),n)>=0)return plane*plane/n2;}
    return std::min({SegmentDistance2(p,a,b),SegmentDistance2(p,b,c),SegmentDistance2(p,c,a)});
}
inline float RayHit(V p,V a,V b,V c){
    const V d{.936329f,.267523f,.225131f};const auto ab=Sub(b,a),ac=Sub(c,a),h=Cross(d,ac);const float det=Dot(ab,h);
    if(std::abs(det)<1.e-8f)return -1;const auto s=Sub(p,a);const float u=Dot(s,h)/det;if(u<0||u>1)return -1;
    const auto q=Cross(s,ab);const float v=Dot(d,q)/det;if(v<0||u+v>1)return -1;
    const float t=Dot(ac,q)/det;return t>1.e-5f?t:-1;
}
inline float Opacity(float distance,bool inside){
    if(!std::isfinite(distance)||distance<0)return 1;
    if(inside)return 0;const float t=std::clamp((distance-.025f)/.045f,0.f,1.f);return t*t*(3-2*t);
}
}
inline bool CompositeWeaponFade(std::vector<DWORD>& normal,const std::vector<DWORD>& clean,float opacity){
    if(normal.size()!=clean.size()||!std::isfinite(opacity))return false;
    const unsigned a=unsigned(std::clamp(opacity,0.f,1.f)*256+.5f);
    for(size_t i=0;i<normal.size();++i){DWORD out=0xff000000;for(unsigned s:{0u,8u,16u})out|=((((normal[i]>>s)&255)*a+((clean[i]>>s)&255)*(256-a)+128)>>8)<<s;normal[i]=out;}
    return true;
}
}
