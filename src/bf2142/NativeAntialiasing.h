#pragma once
#include <d3d9.h>
#include <initializer_list>
namespace bfvr::bf2142 {
// Use the API's native swapchain/depth allocation and its resolve path. Never
// resize the window or replace the engine's offscreen targets or state cache.
inline bool SelectWorldSamples(IDirect3D9* api,UINT adapter,D3DDEVTYPE type,
    unsigned requested,D3DPRESENT_PARAMETERS& p) {
    if(!api||!requested||!p.EnableAutoDepthStencil||p.SwapEffect!=D3DSWAPEFFECT_DISCARD||
        (p.Flags&D3DPRESENTFLAG_LOCKABLE_BACKBUFFER))return false;
    D3DFORMAT color=p.BackBufferFormat;
    if(color==D3DFMT_UNKNOWN){D3DDISPLAYMODE mode{};if(FAILED(api->GetAdapterDisplayMode(adapter,&mode)))return false;color=mode.Format;}
    for(unsigned n:{8u,4u,2u}){
        if(n>requested||n<=unsigned(p.MultiSampleType))continue;
        DWORD colorLevels=0,depthLevels=1;const auto sample=D3DMULTISAMPLE_TYPE(n);
        if(FAILED(api->CheckDeviceMultiSampleType(adapter,type,color,p.Windowed,sample,&colorLevels))||!colorLevels)continue;
        if(p.EnableAutoDepthStencil&&(FAILED(api->CheckDeviceMultiSampleType(adapter,type,p.AutoDepthStencilFormat,p.Windowed,sample,&depthLevels))||!depthLevels))continue;
        p.MultiSampleType=sample;p.MultiSampleQuality=0;return true;
    }
    return false;
}
}
