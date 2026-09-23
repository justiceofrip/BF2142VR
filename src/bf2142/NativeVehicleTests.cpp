#include <windows.h>
#include <array>
#include <vector>
#include <cstdio>
#include <cstring>
#include "NativeVehicle.cpp"
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Native vehicle failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
template<class T>void Put(void* p,size_t at,T value){std::memcpy(static_cast<BYTE*>(p)+at,&value,sizeof(value));}
int main(){
 std::vector<BYTE> gameMemory(0x660000),renderMemory(0x230000);game=gameMemory.data();render=renderMemory.data();installed=true;
 std::array<BYTE,0x500> manager{},player{},soldier{},seat{},root{},definition{};
 std::array<void*,2> soldierWeak{nullptr,soldier.data()},seatWeak{nullptr,seat.data()};
 Put<void*>(render,0x221a58,manager.data());Put<void*>(manager.data(),0,game+0x5292a0);Put<void*>(manager.data(),0x6c,player.data());
 Put<void*>(player.data(),0,game+0x566cd8);Put<void*>(player.data(),0xcc,soldierWeak.data());Put<void*>(player.data(),0x80,seatWeak.data());Put<void*>(soldier.data(),0,game+0x566150);
 Put<void*>(seat.data(),0,game+0x56de60);Put<void*>(seat.data(),0x24,definition.data());Put<void*>(seat.data(),0x34,root.data());
 Put<void*>(root.data(),0,game+0x56de60);Put<void*>(game+0x56de60,0x78,game+0x2f6e20);
 Put<void*>(definition.data(),0,game+0x56e058);Put<unsigned>(definition.data(),0x20,6);Put<unsigned>(definition.data(),0x24,15);std::memcpy(definition.data()+0x10,"eu_fav",7);
 stereo::Matrix4 world{};for(int i=0;i<4;++i)world.values[i][i]=1;world.values[3]={7,8,9,1};Put(root.data(),0xa0,world);Put(seat.data(),0xa0,world);
 VehicleSample sample;CHECK(ReadNativeVehicle(&sample)&&sample.role==VehicleRole::Driver&&sample.root==uintptr_t(root.data())&&sample.seat==uintptr_t(seat.data())&&sample.rootWorld.values[3][0]==7);
 CHECK(NearSeat(sample.seat,world));auto detachedCamera=world;detachedCamera.values[3][0]+=21;CHECK(!NearSeat(sample.seat,detachedCamera));
 // Heap-backed mixed-case stock names are normalized; unknown/modded objects
 // never activate an offset-based native input adapter.
 const char longName[]="EU_FAV_SecondPosition";Put<const char*>(definition.data(),0x10,longName);Put<unsigned>(definition.data(),0x20,sizeof(longName)-1);Put<unsigned>(definition.data(),0x24,sizeof(longName)-1);
 CHECK(ReadNativeVehicle(&sample)&&sample.role==VehicleRole::Aimed);
 Put<unsigned>(definition.data(),0x24,4097);CHECK(!ReadNativeVehicle(&sample));Put<unsigned>(definition.data(),0x24,sizeof(longName)-1);
 const char unknown[]="not_a_vehicle_profile";Put<const char*>(definition.data(),0x10,unknown);Put<unsigned>(definition.data(),0x20,sizeof(unknown)-1);Put<unsigned>(definition.data(),0x24,sizeof(unknown)-1);CHECK(!ReadNativeVehicle(&sample));
 Put<const char*>(definition.data(),0x10,longName);Put<unsigned>(definition.data(),0x20,sizeof(longName)-1);Put<unsigned>(definition.data(),0x24,sizeof(longName)-1);
 Put<void*>(root.data(),0x34,seat.data());CHECK(!ReadNativeVehicle(&sample));Put<void*>(root.data(),0x34,nullptr);
 // Articulated walker ancestry has its own verified lazy world getter.
 Put<void*>(root.data(),0,game+0x5626d8);Put<void*>(game+0x5626d8,0x78,game+0x1636f0);
 const BYTE rotationGetter[]={0x55,0x8b,0xec,0x83,0xec,0x0c,0x56,0x8b,0xf1,0x8b,0x46,0x14};std::memcpy(game+0x1636f0,rotationGetter,sizeof(rotationGetter));CHECK(ReadNativeVehicle(&sample));
 game[0x1636f0]=0;CHECK(!ReadNativeVehicle(&sample));game[0x1636f0]=0x55;
 Put<void*>(root.data(),0,reinterpret_cast<void*>(1));CHECK(!ReadNativeVehicle(&sample));Put<void*>(root.data(),0,game+0x56de60);
 for(void* object:{static_cast<void*>(soldier.data()),static_cast<void*>(seat.data()),static_cast<void*>(root.data())}){Put<DWORD>(object,0x14,0x20);CHECK(!ReadNativeVehicle(&sample));Put<DWORD>(object,0x14,0);}
 Put<void*>(definition.data(),0,game+0x56e518);CHECK(!ReadNativeVehicle(&sample));Put<void*>(definition.data(),0,game+0x56e058);
 auto broken=world;broken.values[0][0]=0;Put(root.data(),0xa0,broken);CHECK(!ReadNativeVehicle(&sample));Put(root.data(),0xa0,world);
 seatWeak[1]=soldier.data();CHECK(!ReadNativeVehicle(&sample));seatWeak[1]=seat.data();CHECK(ReadNativeVehicle(&sample));
 installed=false;CHECK(!ReadNativeVehicle(&sample));
 puts("Native vehicle local ownership, exact profile, names, ancestor cycles, transforms and death guards passed.");
}

