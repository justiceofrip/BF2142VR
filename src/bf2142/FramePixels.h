#pragma once
#include <windows.h>
#include <emmintrin.h>
#include <algorithm>
#include <cstring>
namespace bfvr::bf2142 {
// Byte-identical conversion, with one decision per row instead of per pixel.
// Unaligned source/destination and a short final row tail are supported.
inline void CopyFramePixels(DWORD* output,const DWORD* input,size_t count,bool swapRedBlue,bool opaque){
    if(!swapRedBlue&&!opaque){std::memcpy(output,input,count*sizeof(DWORD));return;}
    const auto alpha=_mm_set1_epi32(int(0xff000000)),rb=_mm_set1_epi32(0x00ff00ff),ga=_mm_set1_epi32(int(0xff00ff00));
    size_t i=0;
    for(;i+4<=count;i+=4){
        auto p=_mm_loadu_si128(reinterpret_cast<const __m128i*>(input+i));
        if(swapRedBlue){const auto colors=_mm_and_si128(p,rb);p=_mm_or_si128(_mm_and_si128(p,ga),_mm_or_si128(_mm_slli_epi32(colors,16),_mm_srli_epi32(colors,16)));}
        if(opaque)p=_mm_or_si128(p,alpha);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(output+i),p);
    }
    for(;i<count;++i){DWORD p=input[i];if(swapRedBlue)p=(p&0xff00ff00)|((p&255)<<16)|((p>>16)&255);output[i]=opaque?p|0xff000000:p;}
}
inline void BlendDesktopHud(DWORD* output,const DWORD* hud,size_t count){
    for(size_t i=0;i<count;++i){
        const DWORD ui=hud[i];const unsigned alpha=ui>>24;
        if(!ui){output[i]|=0xff000000;continue;}
        if(alpha==255){output[i]=ui;continue;}
        DWORD color=0xff000000;
        for(unsigned shift:{0u,8u,16u})color|=std::min<DWORD>(255u,((ui>>shift)&255u)+(((output[i]>>shift)&255u)*(255-alpha)+127)/255)<<shift;
        output[i]=color;
    }
}
}
