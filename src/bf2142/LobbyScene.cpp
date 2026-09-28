#include "LobbyScene.h"
#include "TrackingMath.h"
#include <filesystem>
#include <fstream>
#include <cmath>
#include <cstring>
namespace bfvr::bf2142 {
bool LobbyScene::Load(const std::wstring& path) noexcept {
 draws.clear();device=nullptr;
 try{
  if(path.empty()||std::filesystem::file_size(path)>64*1024*1024)return false;
  std::ifstream f(std::filesystem::path(path),std::ios::binary);char magic[8]{};f.read(magic,8);
  if(std::memcmp(magic,"BFLS0001",8))return false;
  const auto word=[&](){unsigned n=0;f.read(reinterpret_cast<char*>(&n),4);if(!f)throw 1;return n;};
  const unsigned count=word();if(!count||count>32)return false;std::vector<DrawData> result;
  for(unsigned i=0;i<count;++i){DrawData draw;const auto nv=word(),ni=word();draw.width=word();draw.height=word();
   if(!nv||nv>65535||!ni||ni>300000||ni%3||draw.width<4||draw.width>2048||draw.height<4||draw.height>2048)return false;
   draw.vertices.resize(nv);draw.indices.resize(ni);draw.pixels.resize(size_t(draw.width)*draw.height);
   f.read(reinterpret_cast<char*>(draw.vertices.data()),nv*sizeof(Vertex));f.read(reinterpret_cast<char*>(draw.indices.data()),ni*2);f.read(reinterpret_cast<char*>(draw.pixels.data()),draw.pixels.size()*4);if(!f)return false;
   for(const auto& v:draw.vertices){const float values[]={v.x,v.y,v.z,v.nx,v.ny,v.nz,v.u,v.v};for(float value:values)if(!std::isfinite(value)||std::abs(value)>128)return false;
    const float normal=v.nx*v.nx+v.ny*v.ny+v.nz*v.nz;if(normal<.5f||normal>1.5f)return false;}
   for(auto index:draw.indices)if(index>=nv)return false;
   result.push_back(std::move(draw));
  }
  if(f.peek()!=std::char_traits<char>::eof())return false;draws=std::move(result);return true;
 }catch(...){draws.clear();return false;}
}
bool LobbyScene::Draw(IDirect3DDevice9* d,const EyeCamera& camera){
 if(!d||draws.empty())return false;const auto view=InverseRigid(camera.world);if(!view)return false;
 if(device!=d){ResetDevice();device=d;}
 for(auto& draw:draws)if(!draw.texture){
  if(FAILED(d->CreateTexture(draw.width,draw.height,0,D3DUSAGE_AUTOGENMIPMAP,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&draw.texture,nullptr))&&
     FAILED(d->CreateTexture(draw.width,draw.height,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&draw.texture,nullptr)))return false;
  D3DLOCKED_RECT locked{};if(FAILED(draw.texture->LockRect(0,&locked,nullptr,0)))return false;
  for(UINT y=0;y<draw.height;++y)std::memcpy(static_cast<BYTE*>(locked.pBits)+size_t(y)*locked.Pitch,draw.pixels.data()+size_t(y)*draw.width,size_t(draw.width)*4);
  draw.texture->UnlockRect(0);draw.texture->SetAutoGenFilterType(D3DTEXF_LINEAR);draw.texture->GenerateMipSubLevels();
 }
 D3DMATRIX world{};for(int i=0;i<4;++i)world.m[i][i]=1;
 d->SetTransform(D3DTS_WORLD,&world);d->SetTransform(D3DTS_VIEW,reinterpret_cast<const D3DMATRIX*>(&*view));d->SetTransform(D3DTS_PROJECTION,reinterpret_cast<const D3DMATRIX*>(&camera.projection));
 d->SetStreamSourceFreq(0,1);d->SetStreamSourceFreq(1,1);d->SetRenderState(D3DRS_VERTEXBLEND,D3DVBF_DISABLE);d->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE,FALSE);
 d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetFVF(D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_TEX1);
 d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);
 d->SetRenderState(D3DRS_LIGHTING,TRUE);d->SetRenderState(D3DRS_NORMALIZENORMALS,TRUE);d->SetRenderState(D3DRS_SPECULARENABLE,FALSE);d->SetRenderState(D3DRS_AMBIENT,D3DCOLOR_XRGB(88,103,119));
 d->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,D3DMCS_MATERIAL);d->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,D3DMCS_MATERIAL);
 D3DMATERIAL9 material{};material.Diffuse=material.Ambient={1,1,1,1};d->SetMaterial(&material);
 D3DLIGHT9 light{};light.Type=D3DLIGHT_DIRECTIONAL;light.Diffuse={.95f,.83f,.63f,1};light.Direction={.3f,-.8f,.45f};d->SetLight(0,&light);d->LightEnable(0,TRUE);
 light.Diffuse={.15f,.26f,.42f,1};light.Direction={-.8f,-.3f,-.5f};d->SetLight(1,&light);d->LightEnable(1,TRUE);for(DWORD i=2;i<8;++i)d->LightEnable(i,FALSE);
 d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
 d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);d->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
 d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
 d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);
 d->SetSamplerState(0,D3DSAMP_MAXMIPLEVEL,0);d->SetSamplerState(0,D3DSAMP_MIPMAPLODBIAS,0);
 d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_LINEAR);d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);
 for(const auto& draw:draws){d->SetTexture(0,draw.texture.Get());if(FAILED(d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,UINT(draw.vertices.size()),UINT(draw.indices.size()/3),draw.indices.data(),D3DFMT_INDEX16,draw.vertices.data(),sizeof(Vertex))))return false;}
 return true;
}
}
