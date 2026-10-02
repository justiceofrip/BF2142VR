#include "NativeExResources.h"
#include "NativeExReset.h"
#include <MinHook.h>
#include <wrl/client.h>
#include <cstdio>
#include <cstring>
#include <vector>
using Microsoft::WRL::ComPtr;
#define CHECK(x) do{if(!(x)){printf("FAIL line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 SetEnvironmentVariableW(L"BF2142VR_GPU_DEBUG",L"1");
 HWND window=CreateWindowExW(0,L"STATIC",L"Hidden native D3D9Ex compatibility",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);CHECK(window);
 ComPtr<IDirect3D9Ex> api;CHECK(SUCCEEDED(Direct3DCreate9Ex(D3D_SDK_VERSION,&api)));
 D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.hDeviceWindow=window;p.BackBufferWidth=320;p.BackBufferHeight=240;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.MultiSampleType=D3DMULTISAMPLE_8_SAMPLES;p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 ComPtr<IDirect3DDevice9Ex> d;CHECK(SUCCEEDED(api->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_PUREDEVICE,&p,nullptr,&d)));
 // This overlay exists before resource hooks are registered for native assets.
 ComPtr<IDirect3DTexture9> upload;
 CHECK(SUCCEEDED(d->CreateTexture(320,240,1,D3DUSAGE_DYNAMIC,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&upload,nullptr)));
 bfvr::bf2142::NativeExReset reset;CHECK(SUCCEEDED(reset.Initialize(d.Get())));
 ComPtr<IDirect3DTexture9> tex;CHECK(FAILED(d->CreateTexture(16,16,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&tex,nullptr)));
 CHECK(MH_Initialize()==MH_OK);CHECK(bfvr::bf2142::InstallNativeExResources(d.Get(),nullptr));
 CHECK(SUCCEEDED(d->CreateTexture(16,16,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&tex,nullptr)));
 D3DSURFACE_DESC td{};CHECK(SUCCEEDED(tex->GetLevelDesc(0,&td))&&td.Pool==D3DPOOL_MANAGED&&td.Usage==0);
 const auto levels=tex->GetLevelCount();CHECK(levels==5);
 for(UINT level=0;level<levels;++level){D3DLOCKED_RECT lock{};CHECK(SUCCEEDED(tex->LockRect(level,&lock,nullptr,0)));for(UINT y=0;y<(16u>>level);++y)for(UINT x=0;x<(16u>>level);++x)reinterpret_cast<DWORD*>(static_cast<BYTE*>(lock.pBits)+y*lock.Pitch)[x]=0xff123400|level;CHECK(SUCCEEDED(tex->UnlockRect(level)));}
 ComPtr<IDirect3DSurface9> surface;CHECK(SUCCEEDED(tex->GetSurfaceLevel(1,&surface)));CHECK(SUCCEEDED(surface->GetDesc(&td))&&td.Pool==D3DPOOL_MANAGED&&td.Usage==0);
 RECT region{2,2,4,4};D3DLOCKED_RECT lock{};CHECK(SUCCEEDED(tex->LockRect(0,&lock,&region,0)));*static_cast<DWORD*>(lock.pBits)=0xffabcdee;CHECK(SUCCEEDED(tex->UnlockRect(0)));
 ComPtr<IDirect3DCubeTexture9> cube;CHECK(SUCCEEDED(d->CreateCubeTexture(8,0,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&cube,nullptr)));
 CHECK(SUCCEEDED(cube->GetLevelDesc(0,&td))&&td.Pool==D3DPOOL_MANAGED&&td.Usage==0);
 CHECK(SUCCEEDED(cube->LockRect(D3DCUBEMAP_FACE_POSITIVE_X,0,&lock,nullptr,0)));*static_cast<DWORD*>(lock.pBits)=0xff567890;CHECK(SUCCEEDED(cube->UnlockRect(D3DCUBEMAP_FACE_POSITIVE_X,0)));
 ComPtr<IDirect3DSurface9> face;CHECK(SUCCEEDED(cube->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_X,0,&face)));CHECK(SUCCEEDED(face->GetDesc(&td))&&td.Pool==D3DPOOL_MANAGED);
 ComPtr<IDirect3DVolumeTexture9> volume;CHECK(SUCCEEDED(d->CreateVolumeTexture(8,8,8,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&volume,nullptr)));
 D3DVOLUME_DESC vd{};CHECK(SUCCEEDED(volume->GetLevelDesc(0,&vd))&&vd.Pool==D3DPOOL_MANAGED&&vd.Usage==0);
 D3DLOCKED_BOX box{};CHECK(SUCCEEDED(volume->LockBox(0,&box,nullptr,0)));*static_cast<DWORD*>(box.pBits)=0xff987654;CHECK(SUCCEEDED(volume->UnlockBox(0)));
 ComPtr<IDirect3DVolume9> layer;CHECK(SUCCEEDED(volume->GetVolumeLevel(0,&layer)));CHECK(SUCCEEDED(layer->GetDesc(&vd))&&vd.Pool==D3DPOOL_MANAGED);
 ComPtr<IDirect3DVertexBuffer9> vb;ComPtr<IDirect3DIndexBuffer9> ib;CHECK(SUCCEEDED(d->CreateVertexBuffer(256,0,D3DFVF_XYZ,D3DPOOL_MANAGED,&vb,nullptr)));CHECK(SUCCEEDED(d->CreateIndexBuffer(128,0,D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr)));
 D3DVERTEXBUFFER_DESC vbd{};D3DINDEXBUFFER_DESC ibd{};CHECK(SUCCEEDED(vb->GetDesc(&vbd))&&vbd.Pool==D3DPOOL_MANAGED&&vbd.Usage==0);CHECK(SUCCEEDED(ib->GetDesc(&ibd))&&ibd.Pool==D3DPOOL_MANAGED&&ibd.Usage==0);
 void* data=nullptr;CHECK(SUCCEEDED(vb->Lock(0,0,&data,0)));std::memset(data,0x46,256);CHECK(SUCCEEDED(vb->Unlock()));CHECK(SUCCEEDED(ib->Lock(0,0,&data,0)));std::memset(data,0x35,128);CHECK(SUCCEEDED(ib->Unlock()));
 // Managed compressed textures are used throughout the stock game.
 for(auto format:{D3DFMT_DXT1,D3DFMT_DXT3,D3DFMT_DXT5}){ComPtr<IDirect3DTexture9> compressed;CHECK(SUCCEEDED(d->CreateTexture(16,16,0,0,format,D3DPOOL_MANAGED,&compressed,nullptr)));for(UINT level=0;level<compressed->GetLevelCount();++level){CHECK(SUCCEEDED(compressed->LockRect(level,&lock,nullptr,0)));std::memset(lock.pBits,0,format==D3DFMT_DXT1?8:16);CHECK(SUCCEEDED(compressed->UnlockRect(level)));}}
 // Native font-like alpha texture: half is transparent, half is opaque.
 ComPtr<IDirect3DTexture9> font;CHECK(SUCCEEDED(d->CreateTexture(16,16,1,0,D3DFMT_A8,D3DPOOL_MANAGED,&font,nullptr)));
 CHECK(SUCCEEDED(font->LockRect(0,&lock,nullptr,0)));for(int y=0;y<16;++y)for(int x=0;x<16;++x)static_cast<BYTE*>(lock.pBits)[y*lock.Pitch+x]=x<8?0:255;CHECK(SUCCEEDED(font->UnlockRect(0)));
 // Mixed resource classes must not overwrite an older object's trampoline.
 D3DSURFACE_DESC overlayDesc{};CHECK(SUCCEEDED(upload->GetLevelDesc(0,&overlayDesc))&&overlayDesc.Pool==D3DPOOL_DEFAULT&&(overlayDesc.Usage&D3DUSAGE_DYNAMIC));
 CHECK(SUCCEEDED(upload->LockRect(0,&lock,nullptr,D3DLOCK_DISCARD)));*static_cast<DWORD*>(lock.pBits)=0xff2468ac;CHECK(SUCCEEDED(upload->UnlockRect(0)));
 for(unsigned pass=0;pass<2;++pass){
  if(pass){
   // Reproduce legacy menu state surviving an Ex reset. Verify both that the
   // mismatch exists and that the adapter clears it without losing resources.
   CHECK(SUCCEEDED(d->SetRenderState(D3DRS_SCISSORTESTENABLE,TRUE)));
   CHECK(SUCCEEDED(d->SetRenderState(D3DRS_DEPTHBIAS,0x3f000000)));
   CHECK(SUCCEEDED(d->SetRenderState(D3DRS_FOGENABLE,TRUE)));
   CHECK(SUCCEEDED(d->SetStreamSource(0,vb.Get(),0,12)));CHECK(SUCCEEDED(d->SetIndices(ib.Get())));
   p.BackBufferWidth=400;p.BackBufferHeight=300;
   CHECK(SUCCEEDED(d->ResetEx(&p,nullptr)));
   DWORD state=0;CHECK(SUCCEEDED(d->GetRenderState(D3DRS_SCISSORTESTENABLE,&state))&&state==TRUE);
   CHECK(SUCCEEDED(reset.Restore(d.Get(),false)));
   for(auto rs:{D3DRS_SCISSORTESTENABLE,D3DRS_DEPTHBIAS,D3DRS_FOGENABLE,D3DRS_ALPHABLENDENABLE})CHECK(SUCCEEDED(d->GetRenderState(rs,&state))&&state==0);
   ComPtr<IDirect3DBaseTexture9> bound;CHECK(SUCCEEDED(d->GetTexture(0,&bound))&&!bound);
   ComPtr<IDirect3DVertexBuffer9> stream;UINT offset=0,stride=0;CHECK(SUCCEEDED(d->GetStreamSource(0,&stream,&offset,&stride))&&!stream);
   ComPtr<IDirect3DIndexBuffer9> indices;CHECK(SUCCEEDED(d->GetIndices(&indices))&&!indices);
   D3DVIEWPORT9 viewport{};RECT scissor{};CHECK(SUCCEEDED(d->GetViewport(&viewport))&&viewport.Width==400&&viewport.Height==300);
   CHECK(SUCCEEDED(d->GetScissorRect(&scissor))&&scissor.left==0&&scissor.top==0&&scissor.right==400&&scissor.bottom==300);
   p.BackBufferWidth=320;p.BackBufferHeight=240;CHECK(SUCCEEDED(d->ResetEx(&p,nullptr)));CHECK(SUCCEEDED(reset.Restore(d.Get(),false)));
  }
  CHECK(SUCCEEDED(tex->LockRect(0,&lock,nullptr,D3DLOCK_READONLY)));CHECK(*static_cast<DWORD*>(lock.pBits)==0xff123400);CHECK(reinterpret_cast<DWORD*>(static_cast<BYTE*>(lock.pBits)+2*lock.Pitch)[2]==0xffabcdee);CHECK(SUCCEEDED(tex->UnlockRect(0)));
  CHECK(SUCCEEDED(cube->LockRect(D3DCUBEMAP_FACE_POSITIVE_X,0,&lock,nullptr,D3DLOCK_READONLY)));CHECK(*static_cast<DWORD*>(lock.pBits)==0xff567890);CHECK(SUCCEEDED(cube->UnlockRect(D3DCUBEMAP_FACE_POSITIVE_X,0)));
  CHECK(SUCCEEDED(volume->LockBox(0,&box,nullptr,D3DLOCK_READONLY)));CHECK(*static_cast<DWORD*>(box.pBits)==0xff987654);CHECK(SUCCEEDED(volume->UnlockBox(0)));
  CHECK(SUCCEEDED(vb->Lock(0,0,&data,D3DLOCK_READONLY)));CHECK(*static_cast<DWORD*>(data)==0x46464646);CHECK(SUCCEEDED(vb->Unlock()));CHECK(SUCCEEDED(ib->Lock(0,0,&data,D3DLOCK_READONLY)));CHECK(*static_cast<DWORD*>(data)==0x35353535);CHECK(SUCCEEDED(ib->Unlock()));
  CHECK(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff123456,1,0)));CHECK(SUCCEEDED(d->BeginScene()));
  d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);d->SetTexture(0,font.Get());d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_LIGHTING,FALSE);
  d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);d->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
  struct V{float x,y,z,w;DWORD color;float u,v;};const V vertices[]={{-.5f,-.5f,0,1,0xffffffff,0,0},{319.5f,-.5f,0,1,0xffffffff,1,0},{-.5f,239.5f,0,1,0xffffffff,0,1},{319.5f,239.5f,0,1,0xffffffff,1,1}};
  CHECK(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V))));CHECK(SUCCEEDED(d->EndScene()));
  ComPtr<IDirect3DSurface9> back,resolved,staging;CHECK(SUCCEEDED(d->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)));CHECK(SUCCEEDED(d->CreateRenderTarget(320,240,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_NONE,0,FALSE,&resolved,nullptr)));CHECK(SUCCEEDED(d->CreateOffscreenPlainSurface(320,240,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&staging,nullptr)));CHECK(SUCCEEDED(d->StretchRect(back.Get(),nullptr,resolved.Get(),nullptr,D3DTEXF_NONE)));CHECK(SUCCEEDED(d->GetRenderTargetData(resolved.Get(),staging.Get())));CHECK(SUCCEEDED(staging->LockRect(&lock,nullptr,D3DLOCK_READONLY)));auto* row=reinterpret_cast<DWORD*>(static_cast<BYTE*>(lock.pBits)+120*lock.Pitch);CHECK((row[40]&0xffffff)==0x123456);CHECK((row[240]&0xffffff)==0xffffff);CHECK(SUCCEEDED(staging->UnlockRect()));
 }
 puts("Native D3D9Ex: managed 2D/mips/subrect, cube, volume, VB/IB, DXT1/3/5, font alpha and retained resources across ResetEx passed.");return 0;
}
