#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
namespace bfvr::bf2142 {
struct CanvasSize { unsigned width=0,height=0; };
inline bool ValidCanvas(CanvasSize s) {
    return s.width>=640 && s.height>=640 && s.width<=3072 && s.height<=3072 &&
        uint64_t(s.width)*s.height<=8388608;
}
inline CanvasSize ParseCanvas(std::wstring_view text) {
    CanvasSize s{};const auto split=text.find(L'x');
    if(split==text.npos)return {};
    const auto number=[](std::wstring_view value,unsigned& out){
        if(value.empty()||value.size()>4)return false;
        for(wchar_t c:value){if(c<L'0'||c>L'9')return false;out=out*10+unsigned(c-L'0');}return true;
    };
    if(!number(text.substr(0,split),s.width)||!number(text.substr(split+1),s.height)||!ValidCanvas(s))return {};
    return s;
}
inline CanvasSize RecommendCanvas(CanvasSize recommended,CanvasSize maximum) {
    if(recommended.width<640||recommended.height<640||recommended.width>16384||recommended.height>16384||
       maximum.width<640||maximum.height<640||maximum.width>16384||maximum.height>16384)return {};
    const double scale=std::min({1.,double(std::min(3072u,maximum.width))/recommended.width,
        double(std::min(3072u,maximum.height))/recommended.height,
        std::sqrt(8388608./(double(recommended.width)*recommended.height))});
    CanvasSize result{unsigned(std::floor(recommended.width*scale)),unsigned(std::floor(recommended.height*scale))};
    return ValidCanvas(result)?result:CanvasSize{};
}
// Mouse messages use signed 16-bit client coordinates. Preserve out-of-window
// drag positions; wheel/non-client coordinates must never enter this mapping.
inline int CanvasCoordinate(int pixel,unsigned client,unsigned canvas) {
    if(client<2||canvas<2)return pixel;
    return std::clamp(int(std::lround(double(pixel)*(canvas-1)/(client-1))),-32768,32767);
}
}
