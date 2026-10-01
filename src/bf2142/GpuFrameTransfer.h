#pragma once
#include <d3d9on12.h>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <array>

namespace bfvr::bf2142 {
bool GpuTransferRequested();
bool NativeExTransferRequested();
using GpuDiagnosticLog=void(*)(const char*,...);
void ConfigureGpuDiagnostics(GpuDiagnosticLog);
void ReportGpuDrawFailure(IDirect3DDevice9*);
bool DiagnosticCpuTransfer();
IDirect3D9* CreateGpuTransferFactory(UINT version);

// Captures stay on the GPU. The caller owns each complete frame until its
// D3D11 consumer has finished copying it; Export never grants concurrent reuse.
class GpuFrameTransfer {
public:
    ~GpuFrameTransfer(){Reset();}
    HRESULT Initialize(IDirect3DDevice9*,UINT width,UINT height,ID3D11Device* destination=nullptr);
    HRESULT Stage(unsigned slot,IDirect3DSurface9* source); // null clears this slot
    HRESULT StageBackbuffer(unsigned slot);
    HRESULT Export(); // returns only after this queue has completed its copies
    void Reset();
    IDirect3DTexture9* Texture(unsigned slot) const{return slot<3?textures[slot].Get():nullptr;}
    IDirect3DSurface9* Surface(unsigned slot) const{return slot<3?surfaces[slot].Get():nullptr;}
    std::array<ID3D11Texture2D*,3> Exported() const{return {opened[0].Get(),opened[1].Get(),opened[2].Get()};}
private:
    HRESULT Wait();
    HRESULT InitializeNativeEx(IDirect3DDevice9*,UINT,UINT,ID3D11Device*);
    Microsoft::WRL::ComPtr<IDirect3DDevice9Ex> nativeEx;
    Microsoft::WRL::ComPtr<IDirect3DQuery9> completion9;
    IDirect3DDevice9* device=nullptr; // borrowed, released by owner after Reset
    std::array<Microsoft::WRL::ComPtr<IDirect3DTexture9>,3> textures;
    std::array<Microsoft::WRL::ComPtr<IDirect3DSurface9>,3> surfaces;
    Microsoft::WRL::ComPtr<IDirect3DDevice9On12> interop;
    Microsoft::WRL::ComPtr<ID3D12Device> device12;
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> allocator;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commands;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>,3> shared;
    std::array<Microsoft::WRL::ComPtr<ID3D11Texture2D>,3> opened;
    HANDLE event=nullptr;
    UINT64 sequence=0;
    unsigned staged=0;
};
}
