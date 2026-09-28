#pragma once
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
// Native renderer effects use elapsed seconds, even when the outer simulation
// is faster than the XR consumer. Advance once per stereo pair, never per eye.
class RenderTimeBudget {
public:
 void Skip(double delta) noexcept {if(std::isfinite(delta)&&delta>=0&&delta<=.25)pending=std::min(.25,pending+delta);else Reset();}
 double Take(double delta) noexcept {const double result=std::isfinite(delta)&&delta>=0&&delta<=.25?std::min(.25,delta+pending):delta;Reset();return result;}
 void Reset() noexcept {pending=0;}
private: double pending=0;
};
}
