#pragma once
#include <algorithm>
#include <cmath>
#include <optional>
namespace bfvr::bf2142 {
// Stock BF2142 stores axis 4 as a signed short at 1/100 precision, then
// multiplies it by the infantry yaw factor and the current weapon look scale.
// Alter the generated input once, before BOTH prediction and serialization.
inline std::optional<float> StockTurnAxis(float native,float degrees,float factor) noexcept {
 if(!std::isfinite(native)||!std::isfinite(degrees)||!std::isfinite(factor)||
    factor<.01f||factor>100.f||std::abs(degrees)>90.f)return {};
 const float added=std::round(degrees/factor*100.f)*.01f;
 const float result=native+added;
 if(!std::isfinite(result)||std::abs(result)>327.67f)return {};
 return result;
}
}
