#include <windows.h>
#include <array>
#include <vector>
#include <cstdio>
#include <cstring>
#include <limits>
bool focused=true;
HWND TestForeground(){return reinterpret_cast<HWND>(1);}
DWORD TestWindowProcess(HWND,LPDWORD id){*id=focused?GetCurrentProcessId():0;return 1;}
#define GetForegroundWindow TestForeground
#define GetWindowThreadProcessId TestWindowProcess
#include "NativeComfort.cpp"
#undef GetForegroundWindow
#undef GetWindowThreadProcessId
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Native comfort failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
template<class T>void Put(void* p,size_t at,T value){std::memcpy(static_cast<BYTE*>(p)+at,&value,sizeof(value));}
float __fastcall TestScale(void*,void*){return 1.f;}
void __fastcall TestBatch(void* input,void*,int count,float){
 if(count<=0||count>16)return;
 const int start=Read<int>(input,0x24);auto* ring=Read<BYTE*>(input,0x34);
 for(int i=0;i<count;++i){auto* a=ring+size_t((start+i)%16)*0x118;std::memset(a,0,0x118);Put<float>(a,0x10,.5f);Put<DWORD>(a,0x100,0x31);Put<float>(a,0,1.f);}
 Put<int>(input,0x24,start+count);
}
int main(){
 std::vector<BYTE> gameMemory(0x700000),renderMemory(0x230000);game=gameMemory.data();render=renderMemory.data();
 std::array<BYTE,0x400> manager{},player{},soldier{},remote{};
 std::array<void*,2> weak{nullptr,soldier.data()};
 Put<void*>(render,0x221a58,manager.data());Put<void*>(manager.data(),0,game+0x5292a0);Put<void*>(manager.data(),0x6c,player.data());
 Put<void*>(player.data(),0,game+0x566cd8);Put<void*>(player.data(),0xcc,weak.data());Put<void*>(soldier.data(),0,game+0x566150);
 installed=true;stockTurnInstalled=true;CHECK(NativeSnapTurnAvailable());
 std::array<BYTE,0x118> action{};const auto cleanAction=action;
 CHECK(RequestNativeSnapTurn(30,100));CHECK(!RequestNativeSnapTurn(30,100));
 // Other callers / remote soldiers retain exact native recoil and cannot steal a local pulse.
 CHECK(LocalLookInput(soldier.data(),game+0x18acff,.25f)==.25f);
 CHECK(LocalLookInput(remote.data(),game+0x18acfe,.25f)==.25f);
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,.25f)==.25f);
 CHECK(ApplyStockTurn(action.data(),5));CHECK(Read<float>(action.data(),0x10)==6.f);
 CHECK(Read<DWORD>(action.data(),0x100)==0x10);CHECK(!ApplyStockTurn(action.data(),5));
 // Every other action field (movement, firing, posture, weapon selection) is unchanged.
 auto restored=action;Put<float>(restored.data(),0x10,0);Put<DWORD>(restored.data(),0x100,0);CHECK(restored==cleanAction);
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,-.1f)==-.1f);
 CHECK(std::abs(*yaw.Heading(30.15f,0)-30)<.0001f); // snap not subtracted as recoil
 CHECK(RequestNativeSnapTurn(-45,101));focused=false;CHECK(!ApplyStockTurn(action.data(),5));
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,0)==0);focused=true;
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,0)==0); // no deferred turn on refocus
 CHECK(!ApplyStockTurn(action.data(),5));
 CHECK(RequestNativeSnapTurn(30,102));ClearNativeSnapTurn();CHECK(LocalLookInput(soldier.data(),game+0x18acfe,0)==0);
 CHECK(RequestNativeSnapTurn(30,103));weak[1]=remote.data();Put<void*>(remote.data(),0,game+0x566150);
 CHECK(!ApplyStockTurn(action.data(),5));CHECK(LocalLookInput(remote.data(),game+0x18acfe,0)==0);weak[1]=soldier.data();
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,0)==0);
 for(size_t at:{size_t(0x14),size_t(0x34),size_t(0x28c),size_t(0x2f0)}){
  Put<DWORD>(soldier.data(),at,at==0x14?0x20u:1u);CHECK(!NativeSnapTurnAvailable());CHECK(!RequestNativeSnapTurn(30,104));Put<DWORD>(soldier.data(),at,0);
 }
 // Exercise the production batch hook across ring wrap with real owner guards.
 std::array<BYTE,0x40> input{};std::array<BYTE,16*0x118> ring{};float nativeYawFactor=5.f;
 Put<void*>(input.data(),0,game+0x58de40);Put<int>(input.data(),0x24,15);Put<int>(input.data(),0x2c,16);Put<BYTE*>(input.data(),0x34,ring.data());
 Put<float*>(game,0x668e4c,&nativeYawFactor);nativeLookScale=reinterpret_cast<LookScale>(TestScale);nativeInputBatch=reinterpret_cast<InputBatch>(TestBatch);
 CHECK(RequestNativeSnapTurn(30,200));InputBatchHook(input.data(),nullptr,3,.016f);
 CHECK(Read<float>(ring.data()+15*0x118,0x10)==6.5f);CHECK(Read<float>(ring.data(),0x10)==.5f);CHECK(Read<float>(ring.data()+0x118,0x10)==.5f);
 CHECK(Read<float>(ring.data()+15*0x118,0)==1.f);CHECK(Read<DWORD>(ring.data()+15*0x118,0x100)==0x31);
 InputBatchHook(input.data(),nullptr,1,.016f);CHECK(Read<float>(ring.data()+2*0x118,0x10)==.5f);
 CHECK(RequestNativeSnapTurn(-45,201));focused=false;InputBatchHook(input.data(),nullptr,1,.016f);focused=true;
 InputBatchHook(input.data(),nullptr,1,.016f);CHECK(Read<float>(ring.data()+4*0x118,0x10)==.5f);
 // Exact controlled object/template classification excludes arbitrary vehicles
 // and deployment pods even though they share the parachute class.
 std::array<BYTE,0x500> mountObject{},mountDefinition{};std::array<void*,2> control{nullptr,soldier.data()};
 Put<void*>(player.data(),0x80,control.data());stereo::Matrix4 world{};for(int i=0;i<4;++i)world.values[i][i]=1;
 Put<stereo::Matrix4>(soldier.data(),0xa0,world);TraversalSample traversal;
 CHECK(ReadNativeTraversal(&traversal)&&traversal.mode==TraversalMode::Foot);
 traversalProfile=true;control[1]=mountObject.data();Put<void*>(mountObject.data(),0x24,mountDefinition.data());Put<stereo::Matrix4>(mountObject.data(),0xa0,world);
 Put<void*>(mountObject.data(),0,game+0x5673d8);Put<void*>(mountDefinition.data(),0,game+0x567790);
 CHECK(ReadNativeTraversal(&traversal)&&traversal.mode==TraversalMode::Ladder);CHECK(!NativeSnapTurnAvailable());
 Put<void*>(mountObject.data(),0,game+0x56e818);Put<void*>(mountDefinition.data(),0,game+0x56e518);
 Put<unsigned>(mountDefinition.data(),0x20,9);Put<unsigned>(mountDefinition.data(),0x24,15);std::memcpy(mountDefinition.data()+0x10,"parachute",10);
 CHECK(ReadNativeTraversal(&traversal)&&traversal.mode==TraversalMode::Parachute);
 std::memcpy(mountDefinition.data()+0x10,"eu_pod_cq",10);CHECK(!ReadNativeTraversal(&traversal));
 Put<void*>(mountObject.data(),0,nullptr);CHECK(!ReadNativeTraversal(&traversal));control[1]=soldier.data();
 // Movement basis follows the comfort heading for keyboard and controller
 // forward alike; only the proven input caller can obtain this temporary copy.
 movementInstalled=true;Put<stereo::Matrix4>(soldier.data(),0xa0,world);
 yaw.Bind(0);yaw.Bind(reinterpret_cast<std::uintptr_t>(weak.data()));yaw.AddRecoil(37);
 CHECK(ConfigureNativeMovement(true,.5235987756f));float offset=0;
 CHECK(ReadNativeMovementYaw(soldier.data(),&offset)&&std::abs(offset+7)<.001f);
 const auto original=world;const auto corrected=LocalMovementCamera(soldier.data(),game+0x18b033,&world);
 CHECK(corrected!=&world&&std::abs(corrected->values[2][0]-std::sin(-7*.01745329252f))<.0001f);
 CHECK(std::memcmp(&world,&original,sizeof(world))==0);
 CHECK(LocalMovementCamera(soldier.data(),game+0x18b034,&world)==&world);
 CHECK(LocalMovementCamera(remote.data(),game+0x18b033,&world)==&world);
 focused=false;CHECK(LocalMovementCamera(soldier.data(),game+0x18b033,&world)==&world);focused=true;
 movement.tick=GetTickCount64()-151;CHECK(LocalMovementCamera(soldier.data(),game+0x18b033,&world)==&world);
 CHECK(ConfigureNativeMovement(true,0));weak[1]=remote.data();CHECK(!ReadNativeMovementYaw(remote.data(),&offset));weak[1]=soldier.data();
 CHECK(ConfigureNativeMovement(true,0));Put<void*>(soldier.data(),0x34,mountObject.data());CHECK(!ReadNativeMovementYaw(soldier.data(),&offset));Put<void*>(soldier.data(),0x34,nullptr);
 CHECK(!ConfigureNativeMovement(false,0));CHECK(LocalMovementCamera(soldier.data(),game+0x18b033,&world)==&world);
 CHECK(!ConfigureNativeMovement(true,std::numeric_limits<float>::quiet_NaN()));
 // Hidden native pitch must not drift downward and block ladder entry while
 // the rendered headset remains level. Exercise the actual batch writer.
 stockPitchInstalled=true;stereo::Pose head{};head.orientation.w=1;
 CHECK(NativeHeadPitch(head)&&std::abs(*NativeHeadPitch(head)+5)<.0001f);
 for(float degrees:{-60.f,-30.f,0.f,30.f,60.f}){
  const float half=degrees*.00872664626f;head.orientation={std::sin(half),0,0,std::cos(half)};
  CHECK(std::abs(*NativeHeadPitch(head)-(-degrees-5.f))<.001f);
 }
 head.orientation={0,0,0,1};Put<float>(soldier.data(),0x270,81.5f);
 CHECK(ConfigureNativeLookPitch(true,head));
 ring.fill(0);for(int i=0;i<16;++i){auto* a=ring.data()+i*0x118;Put<float>(a,0x10,6.f);Put<float>(a,0x14,19.f);Put<DWORD>(a,0x100,0x11);Put<float>(a,0,1.f);}
 const auto beforePitch=ring;
 CHECK(ApplyStockPitch(ring.data(),15,16,3,5.f));
 CHECK(Read<float>(ring.data()+15*0x118,0x14)==-6.f);
 CHECK(Read<float>(ring.data(),0x14)==0&&Read<float>(ring.data()+0x118,0x14)==0);
 // Yaw/snap turn, weapon actions, movement and all actions outside this batch
 // must be byte-identical. Neither body nor rendered camera is directly written.
 auto restoredPitch=ring;
 for(int i:{15,0,1}){auto* a=restoredPitch.data()+i*0x118;Put<float>(a,0x14,19.f);Put<DWORD>(a,0x100,0x11);}
 CHECK(restoredPitch==beforePitch);CHECK(Read<float>(soldier.data(),0x270)==81.5f);
 auto unchangedPitch=ring;
 focused=false;CHECK(!ApplyStockPitch(ring.data(),15,16,3,5.f));focused=true;CHECK(ring==unchangedPitch);
 lookPitch.tick=GetTickCount64()-151;CHECK(!ApplyStockPitch(ring.data(),15,16,3,5.f));CHECK(ring==unchangedPitch);
 CHECK(ConfigureNativeLookPitch(true,head));weak[1]=remote.data();CHECK(!ApplyStockPitch(ring.data(),15,16,3,5.f));weak[1]=soldier.data();
 control[1]=mountObject.data();Put<void*>(mountObject.data(),0,game+0x5673d8);Put<void*>(mountDefinition.data(),0,game+0x567790);
 CHECK(!ApplyStockPitch(ring.data(),15,16,3,5.f));CHECK(!ConfigureNativeLookPitch(true,head));control[1]=soldier.data();
 CHECK(ConfigureNativeLookPitch(true,head));Put<DWORD>(soldier.data(),0x14,0x20);CHECK(!ApplyStockPitch(ring.data(),15,16,3,5.f));Put<DWORD>(soldier.data(),0x14,0);
 CHECK(!ConfigureNativeLookPitch(false,head));CHECK(!ApplyStockPitch(ring.data(),15,16,3,5.f));CHECK(ring==unchangedPitch);
 head.orientation.w=0;CHECK(!NativeHeadPitch(head));CHECK(!ConfigureNativeLookPitch(true,head));
 head.orientation.w=std::numeric_limits<float>::quiet_NaN();CHECK(!NativeHeadPitch(head));head.orientation.w=1;
 CHECK(ConfigureNativeLookPitch(true,head));CHECK(!ApplyStockPitch(ring.data(),15,16,3,0));CHECK(ring==unchangedPitch);
 // The stock server and local prediction receive the same bounded pitch
 // correction, including inverted native pitch scaling and backlog catch-up.
 for(float factor:{-.5f,.001f,.05f,.5f,1.f,5.f,100.f}){
  float pitch=81.5f;
  for(int i=0;i<400;++i){const auto axis=NativePitchAxis(pitch,-5.f,factor);CHECK(axis);const float decoded=short(std::lround(*axis*100.f))*.01f;
   const float next=pitch+decoded*factor;CHECK(std::abs(next+5)<=std::abs(pitch+5)+.001f);pitch=next;}
  CHECK(std::abs(pitch+5)<=std::abs(factor)*.0051f+.001f);
 }
 CHECK(!NativePitchAxis(0,0,0));CHECK(!NativePitchAxis(181,0,1));CHECK(!NativePitchAxis(0,81,1));
 CHECK(!NativePitchAxis(0,0,std::numeric_limits<float>::quiet_NaN()));
 // Integration with the existing stock turn batch hook, including ring wrap.
 float pitchFactor=5;Put<float*>(game,0x668f24,&pitchFactor);Put<int>(input.data(),0x24,15);ring.fill(0);
 CHECK(ConfigureNativeLookPitch(true,head));CHECK(RequestNativeSnapTurn(30,204));InputBatchHook(input.data(),nullptr,3,.016f);
 CHECK(Read<float>(ring.data()+15*0x118,0x10)==6.5f);CHECK(Read<float>(ring.data()+15*0x118,0x14)==-6.f);
 CHECK(Read<float>(ring.data(),0x10)==.5f&&Read<float>(ring.data(),0x14)==0.f);
 CHECK(!ConfigureNativeLookPitch(false,head));
 installed=false;CHECK(!NativeSnapTurnAvailable());CHECK(!RequestNativeSnapTurn(30,205));
 // Stock codec round trip: both server and local native input integrate the
 // same yaw; the existing recoil-free frame and IK receive that single yaw.
 for(float factor:{.05f,.5f,1.f,3.125f,5.f,10.f})for(float degrees:{-90.f,-45.f,-30.f,15.f,30.f,90.f}){
  const auto value=StockTurnAxis(0,degrees,factor);
  if(std::abs(degrees/factor)>327.67f){CHECK(!value);continue;}
  CHECK(value);const float decoded=float(short(std::lround(*value*100.f)))*.01f;
  CHECK(std::abs(decoded*factor-degrees)<=factor*.0051f);
 }
 CHECK(!StockTurnAxis(0,30,0));CHECK(!StockTurnAxis(0,30,std::numeric_limits<float>::quiet_NaN()));
 CHECK(!StockTurnAxis(327,30,5));
 puts("Native turn: exact local caller, one pulse, recoil separation, focus/owner/menu/death/vehicle guards passed.");
}
