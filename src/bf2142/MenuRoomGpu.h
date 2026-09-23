#pragma once
#include "MenuRoom.h"
#include "FrameCapture.h"
#include "LobbyScene.h"
#include <d3d9.h>
#include <wrl/client.h>
namespace bfvr::bf2142 {
class MenuRoomGpu {
public:
 bool Draw(IDirect3DDevice9*,std::vector<DWORD>& left,std::vector<DWORD>& right,
     UINT width,UINT height,DXGI_FORMAT,const shared::SharedRenderRequest&,const stereo::Pose& anchor);
 bool LoadScene(const std::wstring& path){Reset();return scene.Load(path);}
 void Reset(){surface.Reset();depthSurface.Reset();scene.ResetDevice();shader.Reset();capture.Reset();device=nullptr;width=height=0;failed=false;}
private:
 bool failed=false;IDirect3DDevice9* device=nullptr;UINT width=0,height=0;
 LobbyScene scene;
 Microsoft::WRL::ComPtr<IDirect3DSurface9> surface,depthSurface;
 Microsoft::WRL::ComPtr<IDirect3DPixelShader9> shader;
 FrameCapture capture;std::array<std::vector<DWORD>,2> completed;
};
}
