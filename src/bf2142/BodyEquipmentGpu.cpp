#include "BodyEquipment.h"
#include "BodyEquipmentPlacement.h"
#include "TrackingMath.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
stereo::Vec3 Transform(const stereo::Pose& p,stereo::Vec3 v){const auto q=p.orientation;const stereo::Vec3 t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
 return {p.position.x+v.x+q.w*t.x+q.y*t.z-q.z*t.y,p.position.y+v.y+q.w*t.y+q.z*t.x-q.x*t.z,p.position.z+v.z+q.w*t.z+q.x*t.y-q.y*t.x};}
}
bool BodyEquipment::DrawGpu(IDirect3DDevice9* d,const shared::SharedPresentationView& eye,const stereo::Matrix4& projection,
 float scale,const BodyInventoryResult& body,const InventoryNames& names,int equipped){
 if(!d||!body.anchorValid||models.empty()||!std::isfinite(scale)||scale<=0)return false;
 const auto& p=eye.pose;const auto a=stereo::MakeRelativePose({{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}},body.anchor);
 if(!a)return false;for(const auto& row:projection.values)for(float v:row)if(!std::isfinite(v))return false;
 if(device!=d){ResetGpu();device=d;}
 Microsoft::WRL::ComPtr<IDirect3DStateBlock9> state;if(FAILED(d->CreateStateBlock(D3DSBT_ALL,&state)))return false;
 D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;
 bool ok=true;const auto set=[&](HRESULT hr){ok=SUCCEEDED(hr)&&ok;};
 set(d->SetVertexShader(nullptr));set(d->SetPixelShader(nullptr));set(d->SetFVF(D3DFVF_XYZ|D3DFVF_TEX1));
 set(d->SetTransform(D3DTS_WORLD,&identity));set(d->SetTransform(D3DTS_VIEW,&identity));set(d->SetTransform(D3DTS_PROJECTION,reinterpret_cast<const D3DMATRIX*>(&projection)));
 set(d->SetStreamSourceFreq(0,1));set(d->SetStreamSourceFreq(1,1));set(d->SetRenderState(D3DRS_VERTEXBLEND,D3DVBF_DISABLE));set(d->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE,FALSE));
 for(auto s:{D3DRS_LIGHTING,D3DRS_FOGENABLE,D3DRS_ALPHATESTENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_STENCILENABLE,D3DRS_SCISSORTESTENABLE,D3DRS_CLIPPLANEENABLE,D3DRS_SRGBWRITEENABLE})set(d->SetRenderState(s,FALSE));
 set(d->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS,TRUE));set(d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE));set(d->SetRenderState(D3DRS_COLORWRITEENABLE,15));set(d->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID));
 set(d->SetRenderState(D3DRS_ZENABLE,TRUE));set(d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE));set(d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL));
 set(d->SetRenderState(D3DRS_DEPTHBIAS,0));set(d->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS,0));
 set(d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE));set(d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE));set(d->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_TFACTOR));
 set(d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1));set(d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TFACTOR));
 set(d->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0));set(d->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE));set(d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE));
 set(d->SetSamplerState(0,D3DSAMP_MAXMIPLEVEL,0));set(d->SetSamplerState(0,D3DSAMP_MIPMAPLODBIAS,0));
 set(d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR));set(d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR));set(d->SetSamplerState(0,D3DSAMP_MIPFILTER,D3DTEXF_LINEAR));
 set(d->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP));set(d->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP));set(d->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE));
 for(unsigned slotIndex=0;slotIndex<BodySlots().size()&&ok;++slotIndex){
  const auto item=BodySlots()[slotIndex].item;if(int(item)==equipped||!names[item][0])continue;
  const std::string name(names[item].begin(),std::find(names[item].begin(),names[item].end(),char(0)));
  Model* model=nullptr;for(auto& m:models)if(m.name==name||m.name==name+"_rifle"){model=&m;break;}if(!model)continue;
  const auto placement=PlaceBodyEquipment(slotIndex,model->low,model->high);if(placement.scale<=0)continue;
  set(d->SetRenderState(D3DRS_TEXTUREFACTOR,int(slotIndex)==body.hovered?0xff80d8ff:0xffffffff));
  for(auto& m:model->materials){
   if(!m.gpu){
    if(FAILED(d->CreateTexture(m.width,m.height,0,D3DUSAGE_AUTOGENMIPMAP,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&m.gpu,nullptr))&&
       FAILED(d->CreateTexture(m.width,m.height,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&m.gpu,nullptr))){ok=false;break;}
    D3DLOCKED_RECT locked{};if(FAILED(m.gpu->LockRect(0,&locked,nullptr,0))){m.gpu.Reset();ok=false;break;}
    for(UINT y=0;y<m.height;++y)std::memcpy(static_cast<BYTE*>(locked.pBits)+size_t(y)*locked.Pitch,m.texture.data()+size_t(y)*m.width,size_t(m.width)*4);
    set(m.gpu->UnlockRect(0));m.gpu->SetAutoGenFilterType(D3DTEXF_LINEAR);m.gpu->GenerateMipSubLevels();
   }
   std::vector<Vertex> vertices;vertices.reserve(m.vertices.size());
   for(const auto& v:m.vertices){auto t=Transform(*a,placement.Transform({v.x,v.y,v.z}));vertices.push_back({t.x*scale,t.y*scale,-t.z*scale,v.u,v.v});}
   set(d->SetTexture(0,m.gpu.Get()));if(ok)set(d->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,UINT(vertices.size()),UINT(m.indices.size()/3),m.indices.data(),D3DFMT_INDEX16,vertices.data(),sizeof(Vertex)));
  }
 }
 const bool restored=SUCCEEDED(state->Apply());return ok&&restored;
}
}
