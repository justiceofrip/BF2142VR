#include "NativeParticleReplay.cpp"
#include <array>
#include <vector>
#include <cstdio>
namespace bfvr::bf2142 {
bool rightEye=false,scopeEye=false;
bool IsSecondStereoEye(){return rightEye;}bool IsScopeRender(){return scopeEye;}
}
using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Particle replay failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
template<class T>void Put(BYTE* p,size_t o,T v){std::memcpy(p+o,&v,sizeof(v));}
unsigned updates=0,cameras=0;float advanced=0;void* currentCamera=nullptr;
void __fastcall Update(void*,void*,float dt){++updates;advanced+=dt;}
void __fastcall Camera(void*,void*,void* camera){if(camera==currentCamera)++cameras;}
void* __fastcall Getter(void*,void*){return currentCamera;}
int wmain(int argc,wchar_t** argv){
 std::vector<BYTE> image(0x230000);auto b=image.data();
 const BYTE update[]={0x55,0x8b,0xec,0x53,0x56,0x57,0x6a,0,0x8b,0xf1,0xe8};
 const BYTE camera[]={0x55,0x8b,0xec,0x83,0xec,0x10,0x53,0x56,0x8b,0x75,8,0x8b,6,0x57,0x8b,0xf9};
 const BYTE call[]={0x8b,0x11,0x51,0xd9,0x1c,0x24,0xff,0x52,0x2c};
 const BYTE getter[]={0x8b,0x81,0xf8,0,0,0,0xc3};
 std::memcpy(b+0x9a4c0,update,sizeof(update));std::memcpy(b+0x98bf0,camera,sizeof(camera));
 std::memcpy(b+0x2d5a0,call,sizeof(call));std::memcpy(b+0x6930,getter,sizeof(getter));
 Put(b,0x1d3c78+0x2c,b+0x9a4c0);Put(b,0x1c1868+0x5c,b+0x6930);Put<int>(b,0x9a4ec,0x98bf0-0x9a4f0);b[0x9a6bc]=0xc2;b[0x9a6bd]=4;
 CHECK(ParticleReplayProfile(b));
 for(size_t offset:{size_t(0x9a4c0),size_t(0x98bf0),size_t(0x2d5a0),size_t(0x9a4ec),size_t(0x6930),size_t(0x9a6bd)}){
  b[offset]^=1;CHECK(!ParticleReplayProfile(b));b[offset]^=1;
 }
 std::array<BYTE,32> manager{},renderer{},view{};Put(manager.data(),0,b+0x1d3c78);Put(renderer.data(),0,b+0x1c1868);Put(view.data(),0,b+0x1c6610);
 Put(b,0x1f8e58,renderer.data());particleImage=b;currentCamera=view.data();
 updateParticles=reinterpret_cast<UpdateParticles>(Update);updateCamera=reinterpret_cast<UpdateCamera>(Camera);getCamera=reinterpret_cast<GetCamera>(Getter);
 ParticleUpdateHook(manager.data(),nullptr,.02f);CHECK(updates==1&&cameras==0);
 rightEye=true;ParticleUpdateHook(manager.data(),nullptr,0);CHECK(updates==1&&cameras==1);
 rightEye=false;scopeEye=true;ParticleUpdateHook(manager.data(),nullptr,0);CHECK(updates==1&&cameras==2);
 scopeEye=false;ParticleUpdateHook(manager.data(),nullptr,.03f);CHECK(updates==2&&advanced>.049f&&advanced<.051f);
 // Flat and unknown owner/profile paths stay native; a stale camera cannot be used.
 rightEye=true;Put(view.data(),0,b+1);ParticleUpdateHook(manager.data(),nullptr,0);CHECK(updates==3&&cameras==2);
 Put(view.data(),0,b+0x1c6610);Put(manager.data(),0,b+1);ParticleUpdateHook(manager.data(),nullptr,0);CHECK(updates==4&&cameras==2);
 if(argc>1){
  // Optional local-owned renderer validation: map/relocate code without running
  // its DllMain or importing game state, then install/remove the real trampoline.
  auto module=LoadLibraryExW(argv[1],nullptr,DONT_RESOLVE_DLL_REFERENCES);CHECK(module);
  particleImage=nullptr;CHECK(MH_Initialize()==MH_OK);
  CHECK(InstallNativeParticleReplay(reinterpret_cast<BYTE*>(module),nullptr));
  CHECK(MH_DisableHook(MH_ALL_HOOKS)==MH_OK);CHECK(MH_Uninitialize()==MH_OK);
  particleImage=nullptr;FreeLibrary(module);puts("Owned renderer profile and real hook install/remove passed; no game process started.");
 }
 puts("Native particle signature, lifecycle-once, right-eye/scope camera refresh and stale-owner fallback checks passed.");
}
