#pragma once
#include "StereoCamera.h"
#include <d3d9.h>
#include <wrl/client.h>
#include <string>
#include <vector>
namespace bfvr::bf2142 {
class LobbyScene {
public:
 bool Load(const std::wstring&) noexcept;
 bool Ready() const noexcept {return !draws.empty();}
 void ResetDevice(){for(auto& draw:draws)draw.texture.Reset();device=nullptr;}
 bool Draw(IDirect3DDevice9*,const EyeCamera&);
private:
 struct Vertex {float x,y,z,nx,ny,nz,u,v;};
 struct DrawData {std::vector<Vertex> vertices;std::vector<WORD> indices;std::vector<DWORD> pixels;UINT width=0,height=0;Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;};
 std::vector<DrawData> draws;IDirect3DDevice9* device=nullptr;
};
}
