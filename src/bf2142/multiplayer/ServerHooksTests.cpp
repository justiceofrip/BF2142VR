#include "NativeServer.cpp"
#include <vector>
#include <cmath>
using namespace bfvr;using namespace bfvr::bf2142;using namespace bfvr::bf2142::net;
#define CHECK(x) do{if(!(x)){printf("Server hook test failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
template<class T>void Put(void* p,unsigned at,T value){memcpy(static_cast<BYTE*>(p)+at,&value,sizeof(value));}
Matrix At(float x=0,float y=0,float z=0){Matrix m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
Matrix source{},gotParent{},gotLaunch{};stereo::Vec3 gotVelocity{};unsigned calls=0;
const Matrix* __fastcall LaunchStub(void*,void*){return &source;}
void* __fastcall FireStub(void*,void*,const Matrix* m,const Matrix* parent,const stereo::Vec3* velocity){gotLaunch=*m;gotParent=*parent;gotVelocity=*velocity;++calls;return reinterpret_cast<void*>(1);}
void* __fastcall LookupStub(void*,void*,int id){return reinterpret_cast<void*>(id+1);}
void* __fastcall ListStub(void* self,void*){return static_cast<BYTE*>(self)+12;}
bool Near(const Matrix& a,const Matrix& b){for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(std::abs(a.values[i][j]-b.values[i][j])>.001f)return false;return true;}
}
int main(){
 std::vector<BYTE> executable(0x400000);game=executable.data();ownerThread=GetCurrentThreadId();
 std::array<BYTE,0x400> pm{},player{},soldier{},weapon{},definition{},receiver{};
 std::array<BYTE,20> sentinel{},node{};std::array<void*,2> weak{nullptr,soldier.data()},inventory{nullptr,weapon.data()};
 manager=pm.data();Put<void*>(pm.data(),0,game+ServerProfile.managerVt);Put<void*>(pm.data(),0x58,sentinel.data());Put<unsigned>(pm.data(),0x5c,1);
 Put<void*>(sentinel.data(),4,node.data());Put<void*>(node.data(),0,sentinel.data());Put<void*>(node.data(),4,sentinel.data());Put<void*>(node.data(),8,sentinel.data());Put<unsigned>(node.data(),12,7);Put<void*>(node.data(),16,player.data());
 Put<void*>(player.data(),0,game+ServerProfile.playerVt);Put<void*>(player.data(),0xcc,weak.data());Put<void*>(soldier.data(),0,game+ServerProfile.soldierVt);Put<Matrix>(soldier.data(),0xa0,At(50,3,80));Put<int>(soldier.data(),0x218,1);Put<void*>(soldier.data(),0x234,inventory.data());Put<void*>(soldier.data(),0x238,inventory.data()+2);
 Put<void*>(weapon.data(),0,game+ServerProfile.weaponVt);Put<void*>(weapon.data(),0x34,soldier.data());Put<void*>(weapon.data(),0x24,definition.data());Put<void*>(weapon.data(),0x1b4,receiver.data()+0x10);Put<void*>(receiver.data(),0xc,weapon.data());Put<void*>(receiver.data(),0x10,game+ServerProfile.fireVt);
 Put<void*>(definition.data(),0,game+ServerProfile.weaponTemplateVt);memcpy(definition.data()+0x10,"eu_ar_scar11",12);Put<unsigned>(definition.data(),0x20,12);Put<unsigned>(definition.data(),0x24,15);
 CHECK(ReadRoster(manager,game,ServerProfile,&actors));CHECK(actors[7].weapon==weapon.data());CHECK(Owner(receiver.data(),actors[7]));
 Put<BYTE>(player.data(),0xd4,1);CHECK(ReadRoster(manager,game,ServerProfile,&actors)&&actors[7].ai);Put<BYTE>(player.data(),0xd4,0);Put<BYTE>(player.data(),0x3d1,1);CHECK(ReadRoster(manager,game,ServerProfile,&actors)&&!actors[7].ai);
 // Malformed native tree must fail closed, including cycles and duplicate IDs.
 Put<void*>(node.data(),0,node.data());CHECK(!ReadRoster(manager,game,ServerProfile,&actors));Put<void*>(node.data(),0,sentinel.data());CHECK(ReadRoster(manager,game,ServerProfile,&actors));
 settings.enabled=true;settings.secret[0]=17;CHECK(transport.Open(false,1));Transport client;CHECK(client.Open(false,1));
 Packet p;p.secret=settings.secret;p.session=123;p.sequence=1;p.player=7;p.flags=LeftValid|RightValid|WeaponHeld;p.weaponName=actors[7].name;p.body=actors[7].body;p.camera=At(0,1.5f,0);p.head=p.camera;p.left=At(-.2f,1.3f,.3f);p.right=At(.2f,1.3f,.3f);p.weapon=At(.2f,1.4f,.3f);p.weapon.values[0]={0,0,-1,0};p.weapon.values[2]={1,0,0,0};
 CHECK(Validate(p,settings.secret));CHECK(client.Send(p,transport.LocalPort()));for(int i=0;i<100&&!peers[7].pose.Read(GetTickCount64());++i){lastPump=0;Pump();Sleep(1);}CHECK(peers[7].pose.Read(GetTickCount64()));
 originalLookup=reinterpret_cast<Lookup>(&LookupStub);originalList=reinterpret_cast<List>(&ListStub);CHECK(LookupHook(manager,nullptr,7)==reinterpret_cast<void*>(8));CHECK(ListHook(manager,nullptr)==static_cast<BYTE*>(manager)+12);
 originalLaunch=reinterpret_cast<Launch>(&LaunchStub);originalFire=reinterpret_cast<Fire>(&FireStub);
 source=Multiply(At(0,0,.3f),Multiply(p.camera,p.body));const auto expected=MapTrackedFire(source,Multiply(p.camera,p.body),Multiply(p.weapon,p.body));CHECK(expected);
 const auto launch=LaunchHook(receiver.data()+0x10,nullptr);CHECK(launch==&mappedLaunch&&Near(*launch,*expected));
 const auto parent=At();const stereo::Vec3 velocity{37,2,4};FireHook(receiver.data(),nullptr,launch,&parent,&velocity);CHECK(calls==1&&Near(gotLaunch,*expected)&&Near(gotParent,Multiply(p.weapon,p.body)));CHECK(gotVelocity.x==37&&gotVelocity.y==2&&gotVelocity.z==4);
 // A different player/weapon cannot borrow the authenticated pose.
 Put<void*>(weapon.data(),0x34,nullptr);CHECK(LaunchHook(receiver.data()+0x10,nullptr)==&source);Put<void*>(weapon.data(),0x34,soldier.data());
 peers[7].pose.packet.flags=0;CHECK(LaunchHook(receiver.data()+0x10,nullptr)==&source);peers[7].pose.Clear();CHECK(LaunchHook(receiver.data()+0x10,nullptr)==&source);
 // Rejected stale-name and displaced-body packets cannot take over a slot.
 p.sequence++;p.weaponName[0]='x';CHECK(client.Send(p,transport.LocalPort()));lastPump=0;Pump();CHECK(!peers[7].pose.Read(GetTickCount64()));p.weaponName=actors[7].name;p.body.values[3][0]+=10;CHECK(client.Send(p,transport.LocalPort()));lastPump=0;Pump();CHECK(!peers[7].pose.Read(GetTickCount64()));
 // Only the exact horizontal input call consumes a snap, once per event.
 p=peers[7].pose.packet;p.secret=settings.secret;p.session=123;p.sequence=50;p.player=7;p.body=actors[7].body;p.weaponName=actors[7].name;
 p.camera=p.head=p.left=p.right=p.weapon=At();p.flags=LeftValid|RightValid|WeaponHeld;p.snapSerial=1;p.snapDegrees=30;
 CHECK(client.Send(p,transport.LocalPort()));lastPump=0;Pump();CHECK(peers[7].pendingSnap==30);
 CHECK(ServerLookInput(soldier.data(),game+0x12d4ab,2)==2);CHECK(peers[7].pendingSnap==30);
 CHECK(ServerLookInput(soldier.data(),game+0x12d49e,2)==32);CHECK(ServerLookInput(soldier.data(),game+0x12d49e,2)==2);
 p.sequence++;CHECK(client.Send(p,transport.LocalPort()));lastPump=0;Pump();CHECK(!peers[7].pendingSnap);
 peers[7].pendingSnap=30;peers[7].snapTime=GetTickCount64()-500;CHECK(ServerLookInput(soldier.data(),game+0x12d49e,2)==2);
 peers[7].pendingSnap=30;peers[7].snapTime=GetTickCount64();Put<void*>(soldier.data(),0x28c,weapon.data());
 CHECK(ServerLookInput(soldier.data(),game+0x12d49e,2)==2);Put<void*>(soldier.data(),0x28c,nullptr);
 // Native crate fire owns creation/cooldown; only pose/velocity is replaced.
 memset(definition.data()+0x10,0,16);memcpy(definition.data()+0x10,"unl_hub_medic",14);Put<unsigned>(definition.data(),0x20,13);
 Put<void*>(receiver.data(),0x10,game+0x3dbf90);CHECK(ReadRoster(manager,game,ServerProfile,&actors));
 p.weaponName=actors[7].name;p.snapSerial=0;p.snapDegrees=0;p.flags|=LeftCrateHeld;p.throwSerial=1;p.throwLaunch=At(-.3f,1.3f,.4f);p.throwVelocity={2,1,4};p.sequence++;
 CHECK(Validate(p,settings.secret));CHECK(client.Send(p,transport.LocalPort()));lastPump=0;Pump();
 originalCrateLaunch=reinterpret_cast<Launch>(&LaunchStub);
 auto crateLaunch=CrateLaunchHook(receiver.data()+0x10,nullptr);CHECK(crateLaunch==&mappedLaunch);
 const auto crateExpected=Multiply(p.throwLaunch,actors[7].body);CHECK(Near(*crateLaunch,crateExpected));
 FireHook(receiver.data(),nullptr,crateLaunch,&parent,&velocity);CHECK(Near(gotLaunch,crateExpected)&&gotVelocity.x==2&&gotVelocity.y==1&&gotVelocity.z==4);
 CHECK(CrateLaunchHook(receiver.data()+0x10,nullptr)==&source);
 p.sequence++;CHECK(client.Send(p,transport.LocalPort()));lastPump=0;Pump();CHECK(CrateLaunchHook(receiver.data()+0x10,nullptr)==&source);
 puts("Native roster validation, authenticated pose ingestion, authoritative launch/parent, unchanged velocity and owner/dropout fallback passed.");return 0;
}
