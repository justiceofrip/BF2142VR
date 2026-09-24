#include <windows.h>
#include <array>
#include <vector>
#include <cstdio>
#include <cstring>
HWND TestForeground(){return reinterpret_cast<HWND>(1);}
DWORD TestWindowProcess(HWND,LPDWORD id){*id=GetCurrentProcessId();return 1;}
#define GetForegroundWindow TestForeground
#define GetWindowThreadProcessId TestWindowProcess
#include "NativeHands.cpp"
#undef GetForegroundWindow
#undef GetWindowThreadProcessId
namespace bfvr::bf2142 {
bool NativeWeaponAds(void*){return false;}
void InstallNetworkClient(LogFunction){}
void TickNetworkClient(){}
void ApplyRemoteNetworkPose(void*){}
void PublishNetworkPose(void*,void*,const net::Matrix&,const net::Matrix&,const net::Matrix&,const net::Matrix&,const net::Matrix&,const net::Matrix&,bool,bool,const std::array<std::array<float,5>,2>&,bool){}
void PublishNetworkCrateThrow(void*,void*,const net::Matrix&,stereo::Vec3){}
bool canopyTest=false;void* canopySoldier=nullptr;
bool ReadNativeTraversal(TraversalSample* s){if(!canopyTest)return false;s->mode=TraversalMode::Parachute;s->owner=reinterpret_cast<uintptr_t>(canopySoldier);return true;}
bool ReadNativeComfortCamera(const stereo::Matrix4&,stereo::Matrix4*,const void*){return false;}
}
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Native motion failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
template<class T> void Put(void* p,size_t at,T value){std::memcpy(static_cast<BYTE*>(p)+at,&value,sizeof(value));}
M Identity(){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;return m;}
M gotLaunch{},gotParent{};stereo::Vec3 gotVelocity{};unsigned calls=0;
void* __fastcall CaptureFire(void*,void*,const M* launch,const M* parent,const stereo::Vec3* velocity){
    ++calls;gotLaunch=*launch;gotParent=*parent;gotVelocity=*velocity;return reinterpret_cast<void*>(1);
}
M nativeSource{};
const M* __fastcall CaptureLaunch(void*,void*){return &nativeSource;}
void Log(const char*,...){}
}
int main(){
    auto* allocated=static_cast<BYTE*>(VirtualAlloc(nullptr,0x5a0000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));CHECK(allocated);
    game=allocated;std::vector<BYTE> renderMemory(0x230000);render=renderMemory.data();
    std::array<BYTE,0x400> soldier{},weapon{},definition{},manager{},player{},skel{},component{};
    std::array<void*,2> weak{nullptr,soldier.data()},inventory{nullptr,weapon.data()};std::array<BYTE,70*0x24> meta{};HandBones bones{};for(auto& m:bones)m=Identity();
    Put<void*>(render,0x221a58,manager.data());Put<void*>(manager.data(),0,game+0x5292a0);Put<void*>(manager.data(),0x6c,player.data());
    Put<void*>(player.data(),0,game+0x566cd8);Put<void*>(player.data(),0xcc,weak.data());Put<void*>(soldier.data(),0,game+0x566150);
    Put<int>(soldier.data(),0x218,1);Put<void*>(soldier.data(),0x234,inventory.data());Put<void*>(soldier.data(),0x238,inventory.data()+2);
    Put<void*>(weapon.data(),0,game+0x56cd88);Put<void*>(weapon.data(),0x34,soldier.data());Put<void*>(weapon.data(),0x24,definition.data());
    Put<void*>(definition.data(),0,game+0x56c758);Put<unsigned>(definition.data(),0x24,16);
    const char fragName[]="unl_grenade_frag";Put<const char*>(definition.data(),0x10,fragName);Put<unsigned>(definition.data(),0x20,16);
    Put<void*>(soldier.data(),0x2c0,skel.data());Put<int>(skel.data(),0x14,70);Put<void*>(skel.data(),8,meta.data());Put<void*>(skel.data(),0x10,bones.data());
    const int ids[]={0,1,2,3,4,5,6,7,28,29,30,31,32,33,54,69};const short parents[]={-1,-1,1,2,3,4,5,6,1,28,29,30,31,32,1,1};
    for(unsigned i=0;i<std::size(ids);++i){Put<int>(meta.data(),ids[i]*0x24,ids[i]);Put<short>(meta.data(),ids[i]*0x24+6,parents[i]);}
    Put<void*>(component.data(),0xc,weapon.data());Put<void*>(component.data(),0x10,game+0x5703c8);Put<void*>(weapon.data(),0x1b4,component.data()+0x10);
    // Exact profiled accessor; the optional native velocity component is absent.
    const BYTE speedCode[]={0x56,0x8b,0xf1,0x83,0x7e,0x2c,0,0x74,0x1e,0x8b,0x4e,0x2c,0x8b,1,0xff,0x90,0xc0,0,0,0,0xd9,5,0,0x94,0x92,0,0xd9,0xc1,0xda,0xe9,0xdf,0xe0,0xf6,0xc4,0x44,0x7a,0x0e,0xdd,0xd8,0x8b,0x8e,0x80,0,0,0,0xd9,0x81,0xf0,2,0,0,0x5e,0xc3};
    std::memcpy(game+0x1c1ac0,speedCode,sizeof(speedCode));FlushInstructionCache(GetCurrentProcess(),game+0x1c1ac0,sizeof(speedCode));
    Put<void*>(weapon.data(),0x1a0,game+0x56cb58);Put<void*>(game+0x56cb58,0x1b0,game+0x1c1ac0);Put<void*>(weapon.data(),0x220,definition.data());Put<float>(definition.data(),0x2f0,20);
    firePose={soldier.data(),weapon.data(),Identity(),Identity(),Identity(),123,GetTickCount64()};current.enabled=true;current.input.predictedDisplayTime=123;
    logMessage=Log;originalFire=reinterpret_cast<Fire>(&CaptureFire);motionEnabled=true;
    CHECK(Kind(weapon.data())==MotionKind::Frag);CHECK(OwnedFireInterface(firePose));CHECK(FireOwner(component.data(),firePose));float speed=0;CHECK(NativeSpeed(weapon.data(),&speed)&&speed==20);
    Gesture g{};g.owner=firePose;g.launch=Identity();g.launch.values[3]={42,7,-9,1};g.kind=MotionKind::Frag;g.velocity={1,4,-2};g.tick=GetTickCount64();g.serial=8;
    gesture=g;launchGesture=8;trackedLaunch=g.launch;const stereo::Vec3 nativeVelocity{3,0,22};
    CHECK(FireHook(component.data(),nullptr,&trackedLaunch,&firePose.world,&nativeVelocity));
    CHECK(calls==1&&gotVelocity.x==3&&gotVelocity.y==0&&gotVelocity.z==22&&gotParent.values[3][0]==0);
    gesture=g;gesture.tick=GetTickCount64()-1801;FireHook(component.data(),nullptr,&trackedLaunch,&firePose.world,&nativeVelocity);CHECK(gotVelocity.z==22&&gotParent.values[3][0]==0);
    gesture=g;gesture.owner.weapon=nullptr;FireHook(component.data(),nullptr,&trackedLaunch,&firePose.world,&nativeVelocity);CHECK(gotVelocity.z==22);
    current.enabled=false;gesture=g;FireHook(component.data(),nullptr,&trackedLaunch,&firePose.world,&nativeVelocity);CHECK(gotVelocity.z==22);current.enabled=true;
    GrenadeTrajectory arc;CHECK(ReadNativeGrenadeTrajectory(&arc));CHECK(std::abs(arc.start.z-.18f)<.001f&&arc.velocity.z==20);
    originalLaunch=reinterpret_cast<LaunchMatrix>(&CaptureLaunch);nativeSource=Identity();
    nativeSource.values[3]={1,2,3,1};
    firePose.world=Identity();firePose.world.values[0]={0,0,-1,0};firePose.world.values[2]={1,0,0,0};firePose.world.values[3]={4,2,6,1};
    const auto actual=*LaunchHook(component.data()+0x10,nullptr);
    CHECK(ReadNativeGrenadeTrajectory(&arc));
    CHECK(std::abs(actual.values[3][0]-arc.start.x)<.00001f&&std::abs(actual.values[3][2]-arc.start.z)<.00001f);
    CHECK(arc.velocity.x==20&&arc.velocity.z==0&&actual.values[2][0]==1&&actual.values[2][2]==0);
    std::array<BYTE,0x100> physics{};Put<void*>(soldier.data(),0x50,physics.data());Put<void*>(physics.data(),0,game+0x593aa0);
    Put<void*>(game+0x593aa0,0x64,game+0x2c5560);const BYTE getter[]={0x8d,0x41,0x78,0xc3};std::memcpy(game+0x2c5560,getter,4);
    Put<stereo::Vec3>(physics.data(),0x78,{3,1,-2});CHECK(ReadNativeGrenadeTrajectory(&arc));CHECK(arc.velocity.x==23&&arc.velocity.y==1&&arc.velocity.z==-2);
    current.input.predictedDisplayTime=124;CHECK(!ReadNativeGrenadeTrajectory(&arc));current.input.predictedDisplayTime=123;
    Put<void*>(physics.data(),0,nullptr);CHECK(!ReadNativeGrenadeTrajectory(&arc));Put<void*>(soldier.data(),0x50,nullptr);firePose.world=Identity();
    SetNativeWeaponHeld(false);CHECK(!ReadNativeGrenadeTrajectory(&arc));const auto count=calls;
    CHECK(!FireHook(component.data(),nullptr,&trackedLaunch,&firePose.world,&nativeVelocity)&&calls==count);
    SetNativeWeaponHeld(true);CHECK(!NativeWeaponInputReady());
    CHECK(!FireHook(component.data(),nullptr,&trackedLaunch,&firePose.world,&nativeVelocity)&&calls==count);
    InterlockedExchange(&gripTransition,0); // fresh post-grab native attachment acknowledged
    CHECK(NativeWeaponInputReady());
    // Knife retains native speed and inherited movement with the captured strike frame.
    Put<unsigned>(definition.data(),0x20,5);Put<unsigned>(definition.data(),0x24,15);std::memcpy(definition.data()+0x10,"knife",6);
    g.kind=MotionKind::Knife;g.tick=GetTickCount64();gesture=g;launchGesture=g.serial;FireHook(component.data(),nullptr,&trackedLaunch,&firePose.world,&nativeVelocity);
    CHECK(gotVelocity.z==22&&gotLaunch.values[3][0]==42&&!gesture.serial);
    // Supported knife input remains consumed while the controller has lost tracking.
    bindingSoldier=soldier.data();shared::SharedControllerSample lost{};ControllerCommand command{};command.buttons[0]=command.buttons[1]=0x80;
    CHECK(!UpdateNativeMotion(&lost,{},command));CHECK(command.buttons[0]==0&&command.buttons[1]==0);
    // Only the proven local canopy may use tracked 1P arms while mounted.
    Target mounted;Put<void*>(soldier.data(),0x28c,reinterpret_cast<void*>(1));CHECK(!FindTarget(soldier.data(),&mounted));
    canopyTest=true;canopySoldier=soldier.data();CHECK(FindTarget(soldier.data(),&mounted));
    canopySoldier=player.data();CHECK(!FindTarget(soldier.data(),&mounted));canopyTest=false;
    Put<void*>(soldier.data(),0x28c,nullptr);
    Put<void*>(component.data(),0xc,nullptr);CHECK(!FireOwner(component.data(),firePose));
    VirtualFree(allocated,0,MEM_RELEASE);puts("Native gesture bridge: native grenade velocity, empty-hand shot suppression, knife speed, owner/stale/focus guards passed.");return 0;
}
