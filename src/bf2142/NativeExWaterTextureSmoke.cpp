// Hidden GPU regression for the cube reflections / volume normals used by water.
#include "NativeExResources.h"
#include <MinHook.h>
#include <wrl/client.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstring>
#include <algorithm>
using Microsoft::WRL::ComPtr;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static DWORD Color(unsigned face,unsigned mip,unsigned slice=0){return 0xff000000|((30+face*31)<<16)|((40+mip*23)<<8)|(50+slice*19);}
int main(){
 setvbuf(stdout,nullptr,_IONBF,0);SetEnvironmentVariableW(L"BF2142VR_EX_MANAGED_UPLOAD",L"1");
 HWND window=CreateWindowW(L"STATIC",L"Hidden water texture compatibility",WS_OVERLAPPED,0,0,64,64,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(window);
 ComPtr<IDirect3D9Ex> api;CHECK(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&api)));
 D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.hDeviceWindow=window;p.BackBufferWidth=64;p.BackBufferHeight=64;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.SwapEffect=D3DSWAPEFFECT_DISCARD;
 ComPtr<IDirect3DDevice9Ex> d;CHECK(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,nullptr,&d)));
 CHECK(MH_Initialize()==MH_OK);CHECK(bfvr::bf2142::InstallNativeExResources(d.Get(),nullptr));
 ComPtr<IDirect3DCubeTexture9> cube,referenceCube;
 CHECK(SUCCEEDED(d->CreateCubeTexture(16,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&cube,nullptr)));
 CHECK(SUCCEEDED(d->CreateCubeTexture(16,0,0,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&referenceCube,nullptr)));
 for(unsigned face=0;face<6;++face)for(unsigned mip=0;mip<cube->GetLevelCount();++mip){
  auto side=static_cast<D3DCUBEMAP_FACES>(face);D3DLOCKED_RECT a{},b{};
  CHECK(SUCCEEDED(referenceCube->LockRect(side,mip,&a,nullptr,0)));CHECK(SUCCEEDED(cube->LockRect(side,mip,&b,nullptr,0)));
  printf("cube face%u mip%u native-row=%d translated-row=%d\n",face,mip,a.Pitch,b.Pitch);CHECK(a.Pitch==b.Pitch);
  unsigned n=std::max(1u,16u>>mip);for(unsigned y=0;y<n;++y)for(unsigned x=0;x<n;++x)reinterpret_cast<DWORD*>(static_cast<BYTE*>(b.pBits)+y*a.Pitch)[x]=Color(face,mip);
  CHECK(SUCCEEDED(referenceCube->UnlockRect(side,mip)));CHECK(SUCCEEDED(cube->UnlockRect(side,mip)));
 }
 ComPtr<IDirect3DVolumeTexture9> volume,referenceVolume;
 CHECK(SUCCEEDED(d->CreateVolumeTexture(16,8,4,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&volume,nullptr)));
 CHECK(SUCCEEDED(d->CreateVolumeTexture(16,8,4,0,0,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&referenceVolume,nullptr)));
 for(unsigned mip=0;mip<volume->GetLevelCount();++mip){
  D3DLOCKED_BOX a{},b{};CHECK(SUCCEEDED(referenceVolume->LockBox(mip,&a,nullptr,0)));CHECK(SUCCEEDED(volume->LockBox(mip,&b,nullptr,0)));
  printf("volume mip%u native-row/slice=%d/%d translated=%d/%d\n",mip,a.RowPitch,a.SlicePitch,b.RowPitch,b.SlicePitch);CHECK(a.RowPitch==b.RowPitch&&a.SlicePitch==b.SlicePitch);
  for(unsigned z=0;z<std::max(1u,4u>>mip);++z)for(unsigned y=0;y<std::max(1u,8u>>mip);++y)for(unsigned x=0;x<std::max(1u,16u>>mip);++x)
   *reinterpret_cast<DWORD*>(static_cast<BYTE*>(b.pBits)+z*a.SlicePitch+y*a.RowPitch+x*4)=Color(0,mip,z);
  CHECK(SUCCEEDED(referenceVolume->UnlockBox(mip)));CHECK(SUCCEEDED(volume->UnlockBox(mip)));
 }
 // Stock watervolume.dds uses R5G6B5; cover that format's smaller row/slice sizes.
 ComPtr<IDirect3DVolumeTexture9> water,referenceWater;
 CHECK(SUCCEEDED(d->CreateVolumeTexture(16,8,4,0,0,D3DFMT_R5G6B5,D3DPOOL_MANAGED,&water,nullptr)));
 CHECK(SUCCEEDED(d->CreateVolumeTexture(16,8,4,0,0,D3DFMT_R5G6B5,D3DPOOL_SYSTEMMEM,&referenceWater,nullptr)));
 for(unsigned mip=0;mip<water->GetLevelCount();++mip){
  D3DLOCKED_BOX a{},b{};CHECK(SUCCEEDED(referenceWater->LockBox(mip,&a,nullptr,0)));CHECK(SUCCEEDED(water->LockBox(mip,&b,nullptr,0)));
  CHECK(a.RowPitch==b.RowPitch&&a.SlicePitch==b.SlicePitch);
  for(unsigned z=0;z<std::max(1u,4u>>mip);++z)for(unsigned y=0;y<std::max(1u,8u>>mip);++y)for(unsigned x=0;x<std::max(1u,16u>>mip);++x)
   *reinterpret_cast<WORD*>(static_cast<BYTE*>(b.pBits)+z*a.SlicePitch+y*a.RowPitch+x*2)=z%2?0xf800:0x07e0;
  CHECK(SUCCEEDED(referenceWater->UnlockBox(mip)));CHECK(SUCCEEDED(water->UnlockBox(mip)));
 }
 ComPtr<IDirect3DCubeTexture9> compressed,referenceCompressed;
 CHECK(SUCCEEDED(d->CreateCubeTexture(16,0,0,D3DFMT_DXT1,D3DPOOL_MANAGED,&compressed,nullptr)));
 CHECK(SUCCEEDED(d->CreateCubeTexture(16,0,0,D3DFMT_DXT1,D3DPOOL_SYSTEMMEM,&referenceCompressed,nullptr)));
 for(unsigned side=0;side<6;++side)for(unsigned mip=0;mip<compressed->GetLevelCount();++mip){
  auto f=static_cast<D3DCUBEMAP_FACES>(side);D3DLOCKED_RECT a{},b{};CHECK(SUCCEEDED(referenceCompressed->LockRect(f,mip,&a,nullptr,0)));CHECK(SUCCEEDED(compressed->LockRect(f,mip,&b,nullptr,0)));CHECK(a.Pitch==b.Pitch);
  unsigned blocks=std::max(1u,(16u>>mip)/4);for(unsigned y=0;y<blocks;++y)for(unsigned x=0;x<blocks;++x){auto* block=static_cast<BYTE*>(b.pBits)+y*a.Pitch+x*8;const WORD color=side%2?0xf800:0x07e0;memcpy(block,&color,2);memset(block+2,0,6);}
  CHECK(SUCCEEDED(referenceCompressed->UnlockRect(f,mip)));CHECK(SUCCEEDED(compressed->UnlockRect(f,mip)));
 }
 // A subresource write must upload its parent, including nonzero mip levels.
 ComPtr<IDirect3DSurface9> face;CHECK(SUCCEEDED(cube->GetCubeMapSurface(D3DCUBEMAP_FACE_NEGATIVE_Z,2,&face)));
 D3DLOCKED_RECT r{};RECT region{1,1,3,3};CHECK(SUCCEEDED(face->LockRect(&r,&region,0)));
 for(int y=0;y<2;++y)for(int x=0;x<2;++x)reinterpret_cast<DWORD*>(static_cast<BYTE*>(r.pBits)+y*r.Pitch)[x]=0xffabcdef;
 CHECK(SUCCEEDED(face->UnlockRect()));
 ComPtr<IDirect3DVolume9> layer;CHECK(SUCCEEDED(volume->GetVolumeLevel(1,&layer)));
 D3DLOCKED_BOX box{};D3DBOX part{1,1,3,3,0,1};CHECK(SUCCEEDED(layer->LockBox(&box,&part,0)));
 for(int y=0;y<2;++y)for(int x=0;x<2;++x)reinterpret_cast<DWORD*>(static_cast<BYTE*>(box.pBits)+y*box.RowPitch)[x]=0xfffedcba;
 CHECK(SUCCEEDED(layer->UnlockBox()));
 // Sample the actual GPU resources, not their lock shadows, before/after ResetEx.
 const char* code="samplerCUBE C:register(s0);sampler3D V:register(s1);float4 q:register(c0);float4 mode:register(c1);float4 main():COLOR{return mode.x<.5?texCUBElod(C,q):tex3Dlod(V,q);}";
 ComPtr<ID3DBlob> blob,errors;auto compiled=D3DCompile(code,strlen(code),nullptr,nullptr,nullptr,"main","ps_3_0",0,0,&blob,&errors);if(errors)printf("%s\n",static_cast<const char*>(errors->GetBufferPointer()));CHECK(SUCCEEDED(compiled));
 ComPtr<IDirect3DPixelShader9> ps;CHECK(SUCCEEDED(d->CreatePixelShader(static_cast<const DWORD*>(blob->GetBufferPointer()),&ps)));
 for(unsigned pass=0;pass<2;++pass){
  if(pass)CHECK(SUCCEEDED(d->ResetEx(&p,nullptr)));
  ComPtr<IDirect3DSurface9> back,read;CHECK(SUCCEEDED(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)));CHECK(SUCCEEDED(d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr)));
  CHECK(SUCCEEDED(d->SetPixelShader(ps.Get())));CHECK(SUCCEEDED(d->SetVertexShader(nullptr)));CHECK(SUCCEEDED(d->SetFVF(D3DFVF_XYZRHW)));
  for(auto rs:{D3DRS_ZENABLE,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_LIGHTING})CHECK(SUCCEEDED(d->SetRenderState(rs,FALSE)));
  CHECK(SUCCEEDED(d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE)));CHECK(SUCCEEDED(d->SetTexture(0,cube.Get())));CHECK(SUCCEEDED(d->SetTexture(1,volume.Get())));
  for(unsigned sampler=0;sampler<2;++sampler)for(auto state:{D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER,D3DSAMP_MIPFILTER})CHECK(SUCCEEDED(d->SetSamplerState(sampler,state,D3DTEXF_POINT)));
  const float vertices[][4]={{-.5f,-.5f,0,1},{63.5f,-.5f,0,1},{-.5f,63.5f,0,1},{63.5f,63.5f,0,1}};
  auto sample=[&](bool v,float x,float y,float z,unsigned mip,DWORD expected){
   float q[]={x,y,z,float(mip)},mode[]={v?1.f:0.f,0,0,0};
   if(FAILED(d->SetPixelShaderConstantF(0,q,1))||FAILED(d->SetPixelShaderConstantF(1,mode,1))||FAILED(d->BeginScene()))return false;
   HRESULT draw=d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(vertices[0]));if(FAILED(d->EndScene())||FAILED(draw)||FAILED(d->GetRenderTargetData(back.Get(),read.Get())))return false;
   D3DLOCKED_RECT lock{};if(FAILED(read->LockRect(&lock,nullptr,D3DLOCK_READONLY)))return false;
   DWORD actual=reinterpret_cast<DWORD*>(static_cast<BYTE*>(lock.pBits)+32*lock.Pitch)[32];read->UnlockRect();
   if(actual!=expected)printf("GPU mismatch volume=%d mip=%u actual=%08lx expected=%08lx\n",v,mip,actual,expected);return actual==expected;
  };
  for(unsigned side=0;side<6;++side)for(unsigned mip=0;mip<cube->GetLevelCount();++mip){float q[3]={};q[side/2]=side%2?-1.f:1.f;CHECK(sample(false,q[0],q[1],q[2],mip,side==5&&mip==2?0xffabcdef:Color(side,mip)));}
  for(unsigned mip=0;mip<volume->GetLevelCount();++mip)for(unsigned z=0;z<std::max(1u,4u>>mip);++z)
   CHECK(sample(true,.8f,.8f,(z+.5f)/std::max(1u,4u>>mip),mip,Color(0,mip,z)));
  CHECK(sample(true,1.5f/8,1.5f/4,.25f,1,0xfffedcba));
  CHECK(SUCCEEDED(d->SetTexture(1,water.Get())));
  for(unsigned mip=0;mip<water->GetLevelCount();++mip)for(unsigned z=0;z<std::max(1u,4u>>mip);++z)
   CHECK(sample(true,.8f,.8f,(z+.5f)/std::max(1u,4u>>mip),mip,z%2?0xffff0000:0xff00ff00));
  CHECK(SUCCEEDED(d->SetTexture(0,compressed.Get())));
  for(unsigned side=0;side<6;++side)for(unsigned mip=0;mip<compressed->GetLevelCount();++mip){float q[3]={};q[side/2]=side%2?-1.f:1.f;CHECK(sample(false,q[0],q[1],q[2],mip,side%2?0xffff0000:0xff00ff00));}
 }
 puts("PASS cube/volume native row/slice layouts, all faces/mips, subresources, R5G6B5 water, DXT1 cube and GPU samples across reset.");return 0;
}
