#include "presenter/SharedTextureProducer.h"
#include <wrl/client.h>
namespace bfvr::shared {
bool SharedTextureProducer::PublishGpuFrame(const std::array<ID3D11Texture2D*,kTextureCount>& frame){
    if(!context_||!device_)return false;
    for(size_t i=0;i<frame.size();++i){
        if(!frame[i])return false;
        D3D11_TEXTURE2D_DESC desc{};frame[i]->GetDesc(&desc);
        const auto& expected=textures_[i].description;
        Microsoft::WRL::ComPtr<ID3D11Device> owner;frame[i]->GetDevice(&owner);
        if(owner.Get()!=device_||desc.Width!=expected.width||desc.Height!=expected.height||
            DWORD(desc.Format)!=expected.format||desc.SampleDesc.Count!=1||desc.ArraySize!=1||desc.MipLevels!=1)return false;
    }
    if(!gpuCompletion_){
        D3D11_QUERY_DESC desc{D3D11_QUERY_EVENT,0};
        if(FAILED(device_->CreateQuery(&desc,&gpuCompletion_)))return false;
    }
    size_t acquired=0;
    for(;acquired<textures_.size();++acquired){
        const auto hr=textures_[acquired].keyedMutex->AcquireSync(0,500);
        if(hr!=S_OK){
            for(size_t i=0;i<acquired;++i)textures_[i].keyedMutex->ReleaseSync(0);
            WriteLog(L"GPU frame ownership unavailable at slot %zu: 0x%08lX",acquired,hr);return false;
        }
    }
    for(size_t i=0;i<frame.size();++i)context_->CopyResource(textures_[i].resource,frame[i]);
    context_->End(gpuCompletion_);context_->Flush();
    // A flush only submits work. Finish the copy before the caller can reuse
    // the D3D12 export surfaces. Keyed mutexes protect the published textures.
    const auto deadline=GetTickCount64()+1000;
    HRESULT complete=S_FALSE;
    do{
        complete=context_->GetData(gpuCompletion_,nullptr,0,D3D11_ASYNC_GETDATA_DONOTFLUSH);
        if(complete!=S_FALSE)break;
        SwitchToThread();
    }while(GetTickCount64()<deadline);
    bool ok=complete==S_OK;
    for(auto& texture:textures_)ok=SUCCEEDED(texture.keyedMutex->ReleaseSync(complete==S_OK?1:0))&&ok;
    if(!ok)WriteLog(L"GPU frame completion failed: 0x%08lX",complete);
    return ok;
}
}
