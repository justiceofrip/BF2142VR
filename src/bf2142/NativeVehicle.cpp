#include "NativeVehicle.h"
#include "TrackingMath.h"
#include <cstring>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
BYTE* game=nullptr;BYTE* render=nullptr;bool installed=false;LogFunction logger=nullptr;
SRWLOCK lock=SRWLOCK_INIT;VehicleView view;VehicleControls controls;
stereo::Matrix4 stableCamera{};VehicleSample cached{};ULONGLONG cameraTime=0;
template<class T>T Read(const void* p,size_t o){T v{};std::memcpy(&v,static_cast<const BYTE*>(p)+o,sizeof(v));return v;}
bool Object(void* p){
 if(!p)return false;auto* vt=Read<BYTE*>(p,0);
 if(vt<game+0x500000||vt>=game+0x660000||(Read<DWORD>(p,0x14)&0x20))return false;
 const auto getter=Read<void*>(vt,0x78);
 if(getter==game+0x2f6e20)return true;
 // The walker gunner is parented through a rotational bundle whose lazy
 // matrix getter overrides the base object's getter. Its +0x34 parent and
 // cached world matrix have the same layout, verified on the native chain.
 const BYTE rotationGetter[]={0x55,0x8b,0xec,0x83,0xec,0x0c,0x56,0x8b,0xf1,0x8b,0x46,0x14};
 return vt==game+0x5626d8&&getter==game+0x1636f0&&!std::memcmp(getter,rotationGetter,sizeof(rotationGetter));
}
bool Name(void* definition,char (&name)[80]){
 if(!definition||Read<void*>(definition,0)!=game+0x56e058)return false;
 const auto n=Read<unsigned>(definition,0x20),cap=Read<unsigned>(definition,0x24);
 if(!n||n>=sizeof(name)||cap<n||cap>4096)return false;
 const auto* text=cap<16?static_cast<const char*>(definition)+0x10:Read<const char*>(definition,0x10);
 if(!text||text[n])return false;
 for(unsigned i=0;i<n;++i){char c=text[i];if(c>='A'&&c<='Z')c+=32;if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return false;name[i]=c;}name[n]=0;return true;
}
bool NearSeat(std::uint64_t seat,const stereo::Matrix4& native){
 __try{const auto world=Read<stereo::Matrix4>(reinterpret_cast<void*>(uintptr_t(seat)),0xa0);float distance=0;
  for(int i=0;i<3;++i){const float d=world.values[3][i]-native.values[3][i];distance+=d*d;}
  return std::isfinite(distance)&&distance<400;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool NativeCamera(stereo::Matrix4* out){
 __try{
  const auto renderer=Read<void*>(render,0x1f8e58);if(!renderer||Read<void*>(renderer,0)!=render+0x1c1868)return false;
  const auto camera=Read<void*>(renderer,0xf8);if(!camera||Read<void*>(camera,0)!=render+0x1c6610||Read<DWORD>(camera,0x18)!=0)return false;
  *out=Read<stereo::Matrix4>(camera,0x40);return bool(InverseRigid(*out));
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
bool InstallNativeVehicle(LogFunction log){
 if(installed)return true;logger=log;game=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));render=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9_ori.dll"));if(!render)render=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9.dll"));
 __try{
  if(!game||!render)return false;
  const BYTE ctor[]={0x55,0x8b,0xec,0x53,0x56,0x57,0x8b,0x7d,8,0x57,0x8b,0xf1};
  const BYTE getter[]={0x8b,0x41,0x6c,0xc3};
  if(std::memcmp(game+0x1cd180,ctor,sizeof(ctor))||Read<void*>(game,0x1cd1a7)!=game+0x56de60||Read<void*>(game,0x1cd9f6)!=game+0x56e058||
   Read<void*>(game+0x56de60,0x78)!=game+0x2f6e20||Read<void*>(game+0x5292a0,0x30)!=game+0xb7530||std::memcmp(game+0xb7530,getter,sizeof(getter)))return false;
  installed=true;if(log)log("Vehicle control profile: stock local seats, chassis camera, head-directed movable guns and seat shortcuts.");return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
bool ReadNativeVehicle(VehicleSample* out){
 if(!installed||!out)return false;*out={};
 __try{
  const auto pm=Read<void*>(render,0x221a58);if(!pm||Read<void*>(pm,0)!=game+0x5292a0)return false;
  const auto player=Read<void*>(pm,0x6c);if(!player||Read<void*>(player,0)!=game+0x566cd8)return false;
  const auto weak=Read<void*>(player,0xcc);const auto soldier=weak?Read<void*>(weak,4):nullptr;
  if(!soldier||Read<void*>(soldier,0)!=game+0x566150||(Read<DWORD>(soldier,0x14)&0x20))return false;
  const auto control=Read<void*>(player,0x80);const auto seat=control?Read<void*>(control,4):nullptr;
  if(!Object(seat)||seat==soldier||Read<void*>(seat,0)!=game+0x56de60)return false;
  char name[80]{};if(!Name(Read<void*>(seat,0x24),name))return false;
  const auto role=StockVehicleRole(name);if(role==VehicleRole::Unknown)return false;
  void* root=seat;void* ancestors[24]{};size_t count=0;
  for(;;){
   if(count==24)return false;for(size_t i=0;i<count;++i)if(ancestors[i]==root)return false;ancestors[count++]=root;
   void* parent=Read<void*>(root,0x34);if(!parent)break;if(!Object(parent))return false;root=parent;
  }
  const auto world=Read<stereo::Matrix4>(root,0xa0);if(!InverseRigid(world))return false;
  *out={(std::uint64_t(reinterpret_cast<uintptr_t>(weak))<<32)|reinterpret_cast<uintptr_t>(soldier),reinterpret_cast<uintptr_t>(seat),reinterpret_cast<uintptr_t>(root),role,world};return true;
 }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
void UpdateNativeVehicle(const VehicleSample* s,const shared::SharedControllerSample* input,const stereo::Pose& reference,
 const stereo::Pose& head,float scale,float height,ControllerCommand& command){
 AcquireSRWLockExclusive(&lock);
 if(!s||!input){controls.Reset();cameraTime=0;VehicleSample occupied;
  if(!ReadNativeVehicle(&occupied))view.Reset();
  ReleaseSRWLockExclusive(&lock);return;}
 stereo::Matrix4 native{};std::optional<stereo::Matrix4> stable,target;
 if(NativeCamera(&native)){
  // Ignore detached third-person/death cameras. The saved native eye must
  // remain reasonably near the controlled object's transform.
  if(NearSeat(s->seat,native))stable=view.Update(*s,native);
  if(stable){target=stereo::ComposeRuntimeHeadWithD3D8Camera(*stable,reference,head,scale);if(target)target->values[3][1]+=height;stableCamera=*stable;cached=*s;cameraTime=GetTickCount64();}
 }
 controls.Update(*s,*input,stable?&native:nullptr,target?&*target:nullptr,command);
 ReleaseSRWLockExclusive(&lock);
}
bool ReadNativeVehicleCamera(const stereo::Matrix4& native,stereo::Matrix4* stable){
 if(!stable)return false;VehicleSample s;if(!ReadNativeVehicle(&s))return false;
 AcquireSRWLockShared(&lock);const bool valid=cameraTime&&GetTickCount64()-cameraTime<=150&&s.owner==cached.owner&&s.seat==cached.seat&&s.root==cached.root&&InverseRigid(native).has_value();
 if(valid)*stable=stableCamera;ReleaseSRWLockShared(&lock);return valid;
}
void RecenterNativeVehicle(){AcquireSRWLockExclusive(&lock);view.Reset();controls.Reset();cameraTime=0;ReleaseSRWLockExclusive(&lock);}
}
