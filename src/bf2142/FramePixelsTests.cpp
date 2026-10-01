#include "FramePixels.h"
#include <vector>
#include <cstdio>
#include <random>
using namespace bfvr::bf2142;
int main(){
    std::mt19937 rng(2142);
    for(unsigned count=1;count<=137;++count)for(unsigned offset=0;offset<4;++offset)for(bool swap:{false,true})for(bool opaque:{false,true}){
        std::vector<DWORD> src(count+8),dst(count+8,0x12345678);for(auto& p:src)p=rng();
        CopyFramePixels(dst.data()+offset,src.data()+offset,count,swap,opaque);
        for(unsigned i=0;i<dst.size();++i){
            DWORD expected=0x12345678;
            if(i>=offset&&i<offset+count){expected=src[i];if(swap)expected=(expected&0xff00ff00)|((expected&255)<<16)|((expected>>16)&255);if(opaque)expected|=0xff000000;}
            if(dst[i]!=expected)return 1;
        }
    }
    // Every alpha, arbitrary RGB (including additive alpha-zero pixels),
    // transparent/opaque fast paths and untouched guard pixels.
    for(unsigned alpha=0;alpha<256;++alpha)for(unsigned sample=0;sample<512;++sample){
        DWORD original=rng(),ui=(rng()&0xffffff)|(alpha<<24);if(!sample)ui=alpha<<24;
        DWORD actual[3]={0x12345678,original,0x87654321};BlendDesktopHud(actual+1,&ui,1);
        DWORD expected=0xff000000;
        for(unsigned shift:{0u,8u,16u})expected|=std::min<DWORD>(255,((ui>>shift)&255)+(((original>>shift)&255)*(255-alpha)+127)/255)<<shift;
        if(actual[1]!=expected||actual[0]!=0x12345678||actual[2]!=0x87654321)return 2;
    }
    puts("Frame copies and HUD blend match scalar reference for formats, alpha, row tails and alignment.");return 0;
}
