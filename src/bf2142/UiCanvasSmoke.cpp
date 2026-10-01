#include "presenter/D3D11TextureScaler.h"
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <cstdio>
#include <cmath>
using Microsoft::WRL::ComPtr;
int main(){
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    if(FAILED(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context)))return 1;
    D3D11_TEXTURE2D_DESC d{};d.Width=252;d.Height=270;d.MipLevels=d.ArraySize=1;d.Format=DXGI_FORMAT_R8G8B8A8_UNORM;d.SampleDesc.Count=1;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
    std::vector<unsigned> pixels(size_t(d.Width)*d.Height,0xffffffff);D3D11_SUBRESOURCE_DATA data{pixels.data(),d.Width*4,0};
    ComPtr<ID3D11Texture2D> source,target,staging;ComPtr<ID3D11ShaderResourceView> view;ComPtr<ID3D11RenderTargetView> output;
    if(FAILED(device->CreateTexture2D(&d,&data,&source))||FAILED(device->CreateShaderResourceView(source.Get(),nullptr,&view)))return 2;
    d.Width=d.Height=256;d.BindFlags=D3D11_BIND_RENDER_TARGET;
    if(FAILED(device->CreateTexture2D(&d,nullptr,&target))||FAILED(device->CreateRenderTargetView(target.Get(),nullptr,&output)))return 3;
    d.BindFlags=0;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    if(FAILED(device->CreateTexture2D(&d,nullptr,&staging)))return 4;
    bfvr::shared::D3D11TextureScaler scaler;
    if(!scaler.Initialize(device.Get(),context.Get(),nullptr,nullptr,false,false,false,false,false))return 5;
    for(float aspect:{0.f,16.f/9}){
        if(!scaler.ScaleAspectFit(view.Get(),252,270,output.Get(),256,256,true,true,false,0,nullptr,0,nullptr,0,0,nullptr,0,false,0,0,{},false,aspect))return 6;
        D3D11_VIEWPORT viewport{};UINT count=1;context->RSGetViewports(&count,&viewport);
        if(aspect && (std::abs(viewport.Width-256)>0.01||std::abs(viewport.Height-144)>0.01||std::abs(viewport.TopLeftY-56)>0.01))return 7;
        context->CopyResource(staging.Get(),target.Get());D3D11_MAPPED_SUBRESOURCE map{};
        if(FAILED(context->Map(staging.Get(),0,D3D11_MAP_READ,0,&map)))return 8;
        const auto at=[&](unsigned x,unsigned y){return reinterpret_cast<const unsigned*>(static_cast<const BYTE*>(map.pData)+y*map.RowPitch)[x];};
        const auto top=at(128,2),center=at(128,128),edge=at(2,128);context->Unmap(staging.Get(),0);
        if((center>>24)!=255||(aspect?top!=0||(edge>>24)!=255:(top>>24)!=255||edge!=0))return 9;
    }
    puts("GPU UI layout passed: real portrait source retained, widescreen menu rectangle and transparent padding verified; legacy aspect unchanged.");
    return 0;
}
