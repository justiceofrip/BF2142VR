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
 // Team filtering reads only the signature-verified native player getter.
 Put<int>(player.data(),0xd8,2);CHECK(VoiceTeam(actors[7])==0);
 const BYTE teamGetter[]={0x8b,0x81,0xd8,0,0,0,0xc3};
 const BYTE teamCaller[]={0x8b,0x16,0x8b,0xce,0xff,0x92,0xf8,0,0,0};
 memcpy(game+0x13e930,teamGetter,sizeof(teamGetter));memcpy(game+0x106ebc,teamCaller,sizeof(teamCaller));
 Put<void*>(game+ServerProfile.playerVt,0xf8,game+0x13e930);CHECK(VoiceTeam(actors[7])==2);
 Put<int>(player.data(),0xd8,7);CHECK(VoiceTeam(actors[7])==0);Put<int>(player.data(),0xd8,1);CHECK(VoiceTeam(actors[7])==1);
 game[0x13e932]^=1;CHECK(VoiceTeam(actors[7])==0);game[0x13e932]^=1;
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
 // Real dedicated movement callback uses the same heading as the client.
 // Flat players, AI, mounts, stale poses and unrelated callers stay native.
 source=At(10,2,30);const auto originalMovement=source;
 auto& movementPeer=peers[7];movementPeer.pose.packet.flags|=MovementValid;
 movementPeer.pose.packet.movementYawDegrees=-37;movementPeer.pose.received=GetTickCount64();lastPump=GetTickCount64();
 CHECK(ServerMovementCamera(soldier.data(),game+0x12d7d6,&source)!=&source);
 CHECK(Near(movementCamera,*MakeMovementCamera(source,-37))&&Near(source,originalMovement));
 CHECK(ServerMovementCamera(soldier.data(),game+0x12d7d7,&source)==&source);
 actors[7].ai=true;CHECK(ServerMovementCamera(soldier.data(),game+0x12d7d6,&source)==&source);actors[7].ai=false;
 movementPeer.pose.packet.flags&=~MovementValid;CHECK(ServerMovementCamera(soldier.data(),game+0x12d7d6,&source)==&source);
 movementPeer.pose.packet.flags|=MovementValid;movementPeer.pose.received=GetTickCount64()-151;
 CHECK(ServerMovementCamera(soldier.data(),game+0x12d7d6,&source)==&source);
 movementPeer.pose.received=GetTickCount64();Put<void*>(soldier.data(),0x34,weapon.data());
 CHECK(ServerMovementCamera(soldier.data(),game+0x12d7d6,&source)==&source);Put<void*>(soldier.data(),0x34,nullptr);
 ownerThread++;CHECK(ServerMovementCamera(soldier.data(),game+0x12d7d6,&source)==&source);ownerThread--;
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
 // An authenticated, unspawned flat observer receives another player's relay
 // without acquiring a tracked-fire/movement pose of its own.
 std::array<BYTE,0x400> observerPlayer{};std::array<BYTE,20> observerNode{};
 Put<void*>(observerPlayer.data(),0,game+ServerProfile.playerVt);
 Put<void*>(observerNode.data(),0,sentinel.data());Put<void*>(observerNode.data(),4,node.data());
 Put<void*>(observerNode.data(),8,sentinel.data());Put<unsigned>(observerNode.data(),12,8);
 Put<void*>(observerNode.data(),16,observerPlayer.data());Put<void*>(node.data(),8,observerNode.data());Put<unsigned>(pm.data(),0x5c,2);
 Transport observer,intruder;CHECK(observer.Open(false,1)&&intruder.Open(false,1));
 Packet subscribe;subscribe.kind=Subscribe;subscribe.player=8;subscribe.session=789;subscribe.sequence=1;subscribe.secret=settings.secret;
 CHECK(observer.Send(subscribe,transport.LocalPort()));lastPump=0;Pump();
 CHECK(peers[8].subscription.Read(GetTickCount64())&&!peers[8].pose.Read(GetTickCount64()));
 CHECK(peers[8].port==observer.LocalPort()&&!peers[8].pendingSnap&&!peers[8].throwTime);
 p.sequence++;CHECK(client.Send(p,transport.LocalPort()));lastPump=0;Pump();
 Packet received;unsigned sender=0;CHECK(observer.Receive(&received,&sender,settings.secret));
 CHECK(received.kind==Relay&&received.player==7&&sender==transport.LocalPort());
 // An active slot's port/session cannot be stolen; a VR sender cannot turn
 // itself into a subscription until its previous tracking lease expires.
 subscribe.sequence++;CHECK(intruder.Send(subscribe,transport.LocalPort()));lastPump=0;Pump();CHECK(peers[8].port==observer.LocalPort());
 subscribe.player=7;CHECK(client.Send(subscribe,transport.LocalPort()));lastPump=0;Pump();CHECK(!peers[7].subscription.received);
 peers[8].subscription.received=GetTickCount64()-1001;
 p.sequence++;CHECK(client.Send(p,transport.LocalPort()));lastPump=0;Pump();CHECK(!observer.Receive(&received,&sender,settings.secret));
 // Missing native player invalidates the lease, even if a UDP port remains open.
 Put<void*>(node.data(),8,sentinel.data());Put<unsigned>(pm.data(),0x5c,1);lastPump=0;Pump();CHECK(!peers[8].port);
 puts("Native roster validation, authenticated pose ingestion, authoritative launch/parent, unchanged velocity and owner/dropout fallback passed.");return 0;
}
