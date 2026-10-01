#include "NativeExResources.h"
#include <MinHook.h>
#include <wrl/client.h>
#include <array>
#include <atomic>
namespace bfvr::bf2142 {
namespace {
using Microsoft::WRL::ComPtr;
IDirect3DDevice9* owner=nullptr;ExResourceLog logLine=nullptr;
constexpr GUID managedTag={0xc99af705,0x3d84,0x4aad,{0xad,0xd4,0xe0,0x55,0xa1,0xc1,0x90,0x8d}};
struct Metadata{DWORD magic=0x3945584d,usage=0;};
struct HookEntry{void* address;void* detour;void* original;};
std::array<HookEntry,24> hooks{};size_t hookCount=0;
SRWLOCK hookLock=SRWLOCK_INIT;
struct HookGuard{HookGuard(){AcquireSRWLockExclusive(&hookLock);}~HookGuard(){ReleaseSRWLockExclusive(&hookLock);}};
bool debugResources=false;
bool Hook(void* address,void* detour,void** original){
 HookGuard guard;
 for(size_t i=0;i<hookCount;++i){const auto& h=hooks[i];if(h.address==address){if(h.detour!=detour)return false;*original=h.original;return true;}}
 if(hookCount==hooks.size())return false;
 if(MH_CreateHook(address,detour,original)!=MH_OK)return false;
 if(MH_EnableHook(address)!=MH_OK){MH_RemoveHook(address);return false;}
 hooks[hookCount++]={address,detour,*original};return true;
}
template<class T> bool ReadTag(T* resource,Metadata& value){DWORD bytes=sizeof(value);return resource&&SUCCEEDED(resource->GetPrivateData(managedTag,&value,&bytes))&&bytes==sizeof(value)&&value.magic==Metadata{}.magic;}
template<class T> HRESULT Tag(T* resource,DWORD usage){Metadata value;value.usage=usage;return resource->SetPrivateData(managedTag,&value,sizeof(value),0);}
template<class D> void Correct(D* desc,const Metadata& m){desc->Usage=m.usage;desc->Pool=D3DPOOL_MANAGED;}
using CreateTexture=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,UINT,UINT,UINT,DWORD,D3DFORMAT,D3DPOOL,IDirect3DTexture9**,HANDLE*);
using CreateVolume=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,UINT,UINT,UINT,UINT,DWORD,D3DFORMAT,D3DPOOL,IDirect3DVolumeTexture9**,HANDLE*);
using CreateCube=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,UINT,UINT,DWORD,D3DFORMAT,D3DPOOL,IDirect3DCubeTexture9**,HANDLE*);
using CreateVB=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,UINT,DWORD,DWORD,D3DPOOL,IDirect3DVertexBuffer9**,HANDLE*);
using CreateIB=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,UINT,DWORD,D3DFORMAT,D3DPOOL,IDirect3DIndexBuffer9**,HANDLE*);
CreateTexture createTexture=nullptr;CreateVolume createVolume=nullptr;CreateCube createCube=nullptr;CreateVB createVB=nullptr;CreateIB createIB=nullptr;
using LevelDesc=HRESULT(STDMETHODCALLTYPE*)(IDirect3DBaseTexture9*,UINT,D3DSURFACE_DESC*);
LevelDesc textureDesc=nullptr,cubeDesc=nullptr;
using VolumeLevelDesc=HRESULT(STDMETHODCALLTYPE*)(IDirect3DVolumeTexture9*,UINT,D3DVOLUME_DESC*);VolumeLevelDesc volumeLevelDesc=nullptr;
using SurfaceDesc=HRESULT(STDMETHODCALLTYPE*)(IDirect3DSurface9*,D3DSURFACE_DESC*);SurfaceDesc surfaceDesc=nullptr;
using VolumeDesc=HRESULT(STDMETHODCALLTYPE*)(IDirect3DVolume9*,D3DVOLUME_DESC*);VolumeDesc volumeDesc=nullptr;
using BufferDesc=HRESULT(STDMETHODCALLTYPE*)(IDirect3DResource9*,void*);BufferDesc vbDesc=nullptr,ibDesc=nullptr;
HRESULT STDMETHODCALLTYPE DescribeLevel(IDirect3DBaseTexture9* t,UINT level,D3DSURFACE_DESC* d){
 const auto original=t->GetType()==D3DRTYPE_CUBETEXTURE?cubeDesc:textureDesc;
 const auto hr=original(t,level,d);Metadata m;if(SUCCEEDED(hr)&&d&&ReadTag(t,m))Correct(d,m);return hr;
}
HRESULT STDMETHODCALLTYPE DescribeVolumeLevel(IDirect3DVolumeTexture9* t,UINT level,D3DVOLUME_DESC* d){auto hr=volumeLevelDesc(t,level,d);Metadata m;if(SUCCEEDED(hr)&&d&&ReadTag(t,m))Correct(d,m);return hr;}
HRESULT STDMETHODCALLTYPE DescribeSurface(IDirect3DSurface9* s,D3DSURFACE_DESC* d){
 auto hr=surfaceDesc(s,d);ComPtr<IDirect3DResource9> container;Metadata m;
 if(SUCCEEDED(hr)&&d&&SUCCEEDED(s->GetContainer(IID_PPV_ARGS(&container)))&&ReadTag(container.Get(),m))Correct(d,m);return hr;
}
HRESULT STDMETHODCALLTYPE DescribeVolume(IDirect3DVolume9* s,D3DVOLUME_DESC* d){
 auto hr=volumeDesc(s,d);ComPtr<IDirect3DResource9> container;Metadata m;
 if(SUCCEEDED(hr)&&d&&SUCCEEDED(s->GetContainer(IID_PPV_ARGS(&container)))&&ReadTag(container.Get(),m))Correct(d,m);return hr;
}
HRESULT STDMETHODCALLTYPE DescribeBuffer(IDirect3DResource9* b,void* d){
 const bool vertex=b->GetType()==D3DRTYPE_VERTEXBUFFER;const auto hr=(vertex?vbDesc:ibDesc)(b,d);Metadata m;
 if(SUCCEEDED(hr)&&d&&ReadTag(b,m)){if(vertex)Correct(static_cast<D3DVERTEXBUFFER_DESC*>(d),m);else Correct(static_cast<D3DINDEXBUFFER_DESC*>(d),m);}return hr;
}
template<class T,class F> bool Connect(T* object,unsigned slot,F detour,F* original){return Hook((*reinterpret_cast<void***>(object))[slot],reinterpret_cast<void*>(detour),reinterpret_cast<void**>(original));}
using TextureLock=HRESULT(STDMETHODCALLTYPE*)(IDirect3DTexture9*,UINT,D3DLOCKED_RECT*,const RECT*,DWORD);
using BufferLock=HRESULT(STDMETHODCALLTYPE*)(IDirect3DResource9*,UINT,UINT,void**,DWORD);
using TextureLod=DWORD(STDMETHODCALLTYPE*)(IDirect3DBaseTexture9*,DWORD);
TextureLock textureLock=nullptr;BufferLock vertexLock=nullptr,indexLock=nullptr;TextureLod textureLod=nullptr;
std::atomic<unsigned> textureWarnings{0},bufferWarnings{0},lodWarnings{0};
HRESULT STDMETHODCALLTYPE LockTexture(IDirect3DTexture9* t,UINT level,D3DLOCKED_RECT* result,const RECT* rect,DWORD flags){
 const auto hr=textureLock(t,level,result,rect,flags);Metadata m;
 if((FAILED(hr)||(flags&D3DLOCK_DISCARD))&&ReadTag(t,m)&&textureWarnings.fetch_add(1)<24&&logLine)
  logLine("NativeEx texture lock: level=%u flags=0x%08lX subrect=%d hr=0x%08lX.",level,flags,rect!=nullptr,hr);
 return hr;
}
HRESULT STDMETHODCALLTYPE LockBuffer(IDirect3DResource9* b,UINT offset,UINT bytes,void** out,DWORD flags){
 const auto hr=(b->GetType()==D3DRTYPE_VERTEXBUFFER?vertexLock:indexLock)(b,offset,bytes,out,flags);Metadata m;
 if((FAILED(hr)||(flags&D3DLOCK_DISCARD))&&ReadTag(b,m)&&bufferWarnings.fetch_add(1)<24&&logLine)
  logLine("NativeEx buffer lock: type=%u offset=%u bytes=%u flags=0x%08lX hr=0x%08lX.",unsigned(b->GetType()),offset,bytes,flags,hr);
 return hr;
}
DWORD STDMETHODCALLTYPE SetTextureLod(IDirect3DBaseTexture9* t,DWORD lod){
 const auto result=textureLod(t,lod);Metadata m;
 if(lod&&ReadTag(t,m)&&lodWarnings.fetch_add(1)<24&&logLine)logLine("NativeEx managed texture SetLOD: requested=%lu returned=%lu.",lod,result);
 return result;
}
bool Track(IDirect3DTexture9* t,DWORD usage){
 if(debugResources&&(!Connect(t,19,LockTexture,&textureLock)||!Connect<IDirect3DTexture9,TextureLod>(t,11,SetTextureLod,&textureLod)))return false;
 ComPtr<IDirect3DSurface9> s;return SUCCEEDED(Tag(t,usage))&&Connect<IDirect3DTexture9,LevelDesc>(t,17,DescribeLevel,&textureDesc)&&
 SUCCEEDED(t->GetSurfaceLevel(0,&s))&&Connect(s.Get(),12,DescribeSurface,&surfaceDesc);
}
bool Track(IDirect3DCubeTexture9* t,DWORD usage){
 ComPtr<IDirect3DSurface9> s;return SUCCEEDED(Tag(t,usage))&&Connect<IDirect3DCubeTexture9,LevelDesc>(t,17,DescribeLevel,&cubeDesc)&&
 SUCCEEDED(t->GetCubeMapSurface(D3DCUBEMAP_FACE_POSITIVE_X,0,&s))&&Connect(s.Get(),12,DescribeSurface,&surfaceDesc);
}
bool Track(IDirect3DVolumeTexture9* t,DWORD usage){
 ComPtr<IDirect3DVolume9> v;return SUCCEEDED(Tag(t,usage))&&Connect(t,17,DescribeVolumeLevel,&volumeLevelDesc)&&
 SUCCEEDED(t->GetVolumeLevel(0,&v))&&Connect(v.Get(),8,DescribeVolume,&volumeDesc);
}
bool Track(IDirect3DVertexBuffer9* b,DWORD usage){if(debugResources&&!Connect<IDirect3DVertexBuffer9,BufferLock>(b,11,LockBuffer,&vertexLock))return false;return SUCCEEDED(Tag(b,usage))&&Connect<IDirect3DVertexBuffer9,BufferDesc>(b,13,DescribeBuffer,&vbDesc);}
bool Track(IDirect3DIndexBuffer9* b,DWORD usage){if(debugResources&&!Connect<IDirect3DIndexBuffer9,BufferLock>(b,11,LockBuffer,&indexLock))return false;return SUCCEEDED(Tag(b,usage))&&Connect<IDirect3DIndexBuffer9,BufferDesc>(b,13,DescribeBuffer,&ibDesc);}
template<class T> HRESULT Finish(HRESULT hr,T** out,DWORD usage,bool translated){
 if(SUCCEEDED(hr)&&translated&&out&&*out&&!Track(*out,usage)){(*out)->Release();*out=nullptr;hr=E_FAIL;}
 if(FAILED(hr)&&translated&&logLine)logLine("Native D3D9Ex managed resource failed: hr=0x%08lX usage=0x%08lX.",hr,usage);return hr;
}
HRESULT STDMETHODCALLTYPE Texture(IDirect3DDevice9* d,UINT w,UINT h,UINT levels,DWORD usage,D3DFORMAT f,D3DPOOL p,IDirect3DTexture9** out,HANDLE* handle){
 const bool convert=d==owner&&p==D3DPOOL_MANAGED;return Finish(createTexture(d,w,h,levels,convert?usage|D3DUSAGE_DYNAMIC:usage,f,convert?D3DPOOL_DEFAULT:p,out,handle),out,usage,convert);
}
HRESULT STDMETHODCALLTYPE Volume(IDirect3DDevice9* d,UINT w,UINT h,UINT depth,UINT levels,DWORD usage,D3DFORMAT f,D3DPOOL p,IDirect3DVolumeTexture9** out,HANDLE* handle){
 const bool convert=d==owner&&p==D3DPOOL_MANAGED;return Finish(createVolume(d,w,h,depth,levels,convert?usage|D3DUSAGE_DYNAMIC:usage,f,convert?D3DPOOL_DEFAULT:p,out,handle),out,usage,convert);
}
HRESULT STDMETHODCALLTYPE Cube(IDirect3DDevice9* d,UINT w,UINT levels,DWORD usage,D3DFORMAT f,D3DPOOL p,IDirect3DCubeTexture9** out,HANDLE* handle){
 const bool convert=d==owner&&p==D3DPOOL_MANAGED;return Finish(createCube(d,w,levels,convert?usage|D3DUSAGE_DYNAMIC:usage,f,convert?D3DPOOL_DEFAULT:p,out,handle),out,usage,convert);
}
HRESULT STDMETHODCALLTYPE VB(IDirect3DDevice9* d,UINT bytes,DWORD usage,DWORD f,D3DPOOL p,IDirect3DVertexBuffer9** out,HANDLE* handle){
 const bool convert=d==owner&&p==D3DPOOL_MANAGED;return Finish(createVB(d,bytes,convert?usage|D3DUSAGE_DYNAMIC:usage,f,convert?D3DPOOL_DEFAULT:p,out,handle),out,usage,convert);
}
HRESULT STDMETHODCALLTYPE IB(IDirect3DDevice9* d,UINT bytes,DWORD usage,D3DFORMAT f,D3DPOOL p,IDirect3DIndexBuffer9** out,HANDLE* handle){
 const bool convert=d==owner&&p==D3DPOOL_MANAGED;return Finish(createIB(d,bytes,convert?usage|D3DUSAGE_DYNAMIC:usage,f,convert?D3DPOOL_DEFAULT:p,out,handle),out,usage,convert);
}
}
bool InstallNativeExResources(IDirect3DDevice9* d,ExResourceLog log){
 if(owner)return owner==d;ComPtr<IDirect3DDevice9Ex> ex;if(!d||FAILED(d->QueryInterface(IID_PPV_ARGS(&ex))))return false;
 logLine=log;wchar_t flag[8]{};debugResources=GetEnvironmentVariableW(L"BF2142VR_GPU_DEBUG",flag,8)==1&&flag[0]==L'1';
 if(!Connect(d,23,Texture,&createTexture)||!Connect(d,24,Volume,&createVolume)||!Connect(d,25,Cube,&createCube)||!Connect(d,26,VB,&createVB)||!Connect(d,27,IB,&createIB))return false;
 owner=d;if(logLine)logLine("Native D3D9Ex managed resource compatibility installed (experimental).");return true;
}
}
