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
int main(){
 std::vector<BYTE> gameMemory(0x5a0000),renderMemory(0x230000);game=gameMemory.data();render=renderMemory.data();
 std::array<BYTE,0x400> manager{},player{},soldier{},remote{};
 std::array<void*,2> weak{nullptr,soldier.data()};
 Put<void*>(render,0x221a58,manager.data());Put<void*>(manager.data(),0,game+0x5292a0);Put<void*>(manager.data(),0x6c,player.data());
 Put<void*>(player.data(),0,game+0x566cd8);Put<void*>(player.data(),0xcc,weak.data());Put<void*>(soldier.data(),0,game+0x566150);
 installed=true;CHECK(NativeSnapTurnAvailable());
 CHECK(RequestNativeSnapTurn(30,100));CHECK(!RequestNativeSnapTurn(30,100));
 // Other callers / remote soldiers retain exact native recoil and cannot steal a local pulse.
 CHECK(LocalLookInput(soldier.data(),game+0x18acff,.25f)==.25f);
 CHECK(LocalLookInput(remote.data(),game+0x18acfe,.25f)==.25f);
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,.25f)==30.25f);
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,-.1f)==-.1f);
 CHECK(std::abs(*yaw.Heading(30.15f,0)-30)<.0001f); // snap not subtracted as recoil
 CHECK(RequestNativeSnapTurn(-45,101));focused=false;
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,0)==0);focused=true;
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,0)==0); // no deferred turn on refocus
 CHECK(RequestNativeSnapTurn(30,102));ClearNativeSnapTurn();CHECK(LocalLookInput(soldier.data(),game+0x18acfe,0)==0);
 CHECK(RequestNativeSnapTurn(30,103));weak[1]=remote.data();Put<void*>(remote.data(),0,game+0x566150);
 CHECK(LocalLookInput(remote.data(),game+0x18acfe,0)==0);weak[1]=soldier.data();
 CHECK(LocalLookInput(soldier.data(),game+0x18acfe,0)==0);
 for(size_t at:{size_t(0x14),size_t(0x34),size_t(0x28c),size_t(0x2f0)}){
  Put<DWORD>(soldier.data(),at,at==0x14?0x20u:1u);CHECK(!NativeSnapTurnAvailable());CHECK(!RequestNativeSnapTurn(30,104));Put<DWORD>(soldier.data(),at,0);
 }
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
 installed=false;CHECK(!NativeSnapTurnAvailable());CHECK(!RequestNativeSnapTurn(30,105));
 puts("Native turn: exact local caller, one pulse, recoil separation, focus/owner/menu/death/vehicle guards passed.");
}

namespace bfvr::bf2142 {void PublishNetworkSnap(void*,float){}}
