namespace {bool movementFixture=false;}
namespace bfvr::bf2142 {bool ReadNativeMovementYaw(const void*,float* out){if(!movementFixture)return false;*out=-13.25f;return true;}}
#include "NativeNetwork.cpp"
#include "NativeRemoteWeapon.cpp"
#include <vector>
#include <cstdio>
#include <cmath>
using namespace bfvr;using namespace bfvr::bf2142;using namespace bfvr::bf2142::net;
#define CHECK(x) do{if(!(x)){printf("Client network test failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
template<class T>void Put(void* p,unsigned offset,T value){memcpy(static_cast<BYTE*>(p)+offset,&value,sizeof(value));}
Matrix At(float x=0,float y=0,float z=0){Matrix m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
BodyBones authored{},rendered{};bool refreshNative=true;unsigned calls=0;void* calledSoldier=nullptr;float calledDelta=0;unsigned calledUpdate=0;
void __fastcall FinalizeStub(void* self,void*,float delta,unsigned update){++calls;calledSoldier=self;calledDelta=delta;calledUpdate=update;if(refreshNative)rendered=authored;}
bool Near(const Matrix& a,const Matrix& b){for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(std::abs(a.values[i][j]-b.values[i][j])>.001f)return false;return true;}
bool Same(const BodyBones& a,const BodyBones& b){for(int i=0;i<80;++i)if(!Near(a[i],b[i]))return false;return true;}
unsigned drawCalls=0,shadowCalls=0;std::array<DWORD,6> drawArgs{};
void __fastcall DrawStub(void*,void*,DWORD a,DWORD b,DWORD c){++drawCalls;drawArgs={a,b,c,0,0,0};}
void __fastcall ShadowStub(void*,void*,DWORD a,DWORD b,DWORD c,DWORD d,DWORD e,DWORD f){++shadowCalls;drawArgs={a,b,c,d,e,f};}
void Frame(void* soldier){lastPump=GetTickCount64();RemoteFinalizeHook(soldier,nullptr,.0125f,17);}
}
int main(){
 std::vector<BYTE> executable(0x600000);game=executable.data();enabled=true;ownerThread=GetCurrentThreadId();localId=0;
 std::array<BYTE,0x400> soldier{},localSoldier{};std::array<BYTE,0x20> skeleton{};std::array<BYTE,80*0x24> meta{};
 Put<void*>(soldier.data(),0,game+ClientProfile.soldierVt);Put<int>(soldier.data(),0x2f0,1);Put<void*>(soldier.data(),0x2c8,skeleton.data());
 Put<int>(skeleton.data(),0x14,80);Put<void*>(skeleton.data(),8,meta.data());Put<void*>(skeleton.data(),0x10,rendered.data());
 const int ids[]={0,11,12,13,14,15,16,17,18,19,20,30,31,32,33,34,35,45,46,47,64,71};
 const short parents[]={-1,0,11,12,13,14,15,16,17,18,19,13,30,31,32,33,34,13,45,46,34,34};
 for(unsigned i=0;i<std::size(ids);++i){Put<int>(meta.data(),ids[i]*0x24,ids[i]);Put<short>(meta.data(),ids[i]*0x24+6,parents[i]);}
 for(int wrist:{20,35})for(int group=0;group<3;++group)for(int joint=0;joint<3;++joint){const int id=wrist+1+group*3+joint;Put<int>(meta.data(),id*0x24,id);Put<short>(meta.data(),id*0x24+6,short(joint?id-1:wrist));}
 for(auto& m:authored)m=At();
 for(bool left:{true,false}){int s=left?15:31,e=left?17:33,w=left?20:35;float x=left?-.18f:.18f;
  authored[s]=At(x,1.4f,0);authored[e]=At(x,1.1f,0);authored[w]=At(x,1.1f,.3f);
  for(int i=s+1;i<e;++i)authored[i]=authored[s];for(int i=e+1;i<w;++i)authored[i]=authored[e];for(int i=w+1;i<w+10;++i)authored[i]=authored[w];
  authored[left?24:39]=At(x+.025f,1.1f,.38f);authored[left?21:36]=At(x-.025f,1.1f,.38f);
 }
 for(int i=64;i<72;++i)authored[i]=At(.18f,1.1f,.3f+float(i-64)*.03f);
 actors[0].soldier=localSoldier.data();actors[7].soldier=soldier.data();actors[7].weak=soldier.data();
 auto left=CaptureBodyPalm(authored,true),right=CaptureBodyPalm(authored,false);CHECK(left&&right);
 Packet p;p.kind=Mirror;p.player=7;p.session=41;p.sequence=1;p.flags=LeftValid;
 p.left=Multiply(*InverseRigid(left->wristFromPalm),authored[20]);p.left.values[3][1]+=.14f;
 p.right=Multiply(*InverseRigid(right->wristFromPalm),authored[35]);
 CHECK(poses[7].Accept(p,GetTickCount64()));originalRemoteFinalize=reinterpret_cast<RemoteFinalize>(&FinalizeStub);
 Frame(soldier.data());CHECK(calls==1&&calledSoldier==soldier.data()&&calledDelta==.0125f&&calledUpdate==17);
 CHECK(bindings[7].valid&&bindings[7].reported);CHECK(Distance(rendered[20],authored[20])>.04f);
 for(int i=0;i<80;++i)if(i<15||i>=30)CHECK(Near(rendered[i],authored[i]));
 // Reduced animation LOD may skip all arm updates. Last frame's stretched
 // IK must not become the next frame's native lengths / weapon binding.
 const auto tracked=rendered;refreshNative=false;
 poses[7].packet.left.values[3][0]+=.7f;Frame(soldier.data());CHECK(!Same(rendered,tracked));
 poses[7].packet=p;Frame(soldier.data());CHECK(Same(rendered,tracked));
 for(int i=0;i<30;++i){Frame(soldier.data());CHECK(Same(rendered,tracked));}
 // An unrelated native edit between callbacks is never overwritten by restore.
 rendered[14].values[3][0]+=.023f;const auto nativeEdit=rendered[14];Frame(soldier.data());CHECK(Near(rendered[14],nativeEdit));
 poses[7].received=GetTickCount64()-1000;Frame(soldier.data());
 for(int i=0;i<80;++i)if(i!=14)CHECK(Near(rendered[i],authored[i]));CHECK(Near(rendered[14],nativeEdit));
 refreshNative=true;poses[7].received=GetTickCount64();
 // Stale tracking, other threads, local rigs and invalid skeletons retain native animation.
 poses[7].received=GetTickCount64()-1000;Frame(soldier.data());CHECK(Same(rendered,authored));
 poses[7].received=GetTickCount64();ownerThread++;Frame(soldier.data());CHECK(Same(rendered,authored));ownerThread--;
 Frame(localSoldier.data());CHECK(Same(rendered,authored));
 Put<int>(soldier.data(),0x2f0,0);Frame(soldier.data());CHECK(Same(rendered,authored));Put<int>(soldier.data(),0x2f0,1);
 Put<short>(meta.data(),35*0x24+6,0);Frame(soldier.data());CHECK(Same(rendered,authored));Put<short>(meta.data(),35*0x24+6,34);
 Put<short>(meta.data(),25*0x24+6,20);Frame(soldier.data());CHECK(Same(rendered,authored));Put<short>(meta.data(),25*0x24+6,24);
 // Detailed reference restores a collapsed remote LOD before solving, leaving the other hand native.
 detailedReference=authored;referenceLeft=*left;referenceRight=*right;referenceValid=true;bindings[7]={};
 for(int i=18;i<30;++i)authored[i]=authored[17];for(int i=36;i<45;++i)authored[i]=authored[35];
 poses[7].received=GetTickCount64();Frame(soldier.data());CHECK(bindings[7].valid&&bindings[7].reported);CHECK(Distance(rendered[20],authored[20])>.04f);
 for(int i=30;i<80;++i)CHECK(Near(rendered[i],authored[i]));
 // Unreachable tracking falls back after the engine has refreshed every bone.
 poses[7].packet.left=At(-3,3,3);Frame(soldier.data());CHECK(Same(rendered,authored));
 // HMD head rotation is independently gated by the complete face topology.
 // Keep the unreachable arm native, while the real callback still turns head.
 for(int id=47;id<64;++id){Put<int>(meta.data(),id*0x24,id);Put<short>(meta.data(),id*0x24+6,id==47?46:id>=49&&id<=51?48:47);}
 auto trackedHead=At(2,2,2);trackedHead.values[0]={0,0,-1,0};trackedHead.values[2]={1,0,0,0};
 poses[7].packet.head=trackedHead;poses[7].received=GetTickCount64();Frame(soldier.data());
 trackedHead.values[3]=authored[47].values[3];CHECK(Near(rendered[47],trackedHead)&&bindings[7].reportedHead);
 for(int i=0;i<80;++i)if(i<47||i>=64)CHECK(Near(rendered[i],authored[i]));
 poses[7].received=GetTickCount64()-251;Frame(soldier.data());CHECK(Same(rendered,authored));poses[7].received=GetTickCount64();
 Frame(localSoldier.data());CHECK(Same(rendered,authored));
 ownerThread++;Frame(soldier.data());CHECK(Same(rendered,authored));ownerThread--;
 Put<void*>(soldier.data(),0x28c,localSoldier.data());Frame(soldier.data());CHECK(Same(rendered,authored));Put<void*>(soldier.data(),0x28c,nullptr);
 Put<DWORD>(soldier.data(),0x14,0x20);Frame(soldier.data());CHECK(Same(rendered,authored));Put<DWORD>(soldier.data(),0x14,0);
 Put<short>(meta.data(),52*0x24+6,0);Frame(soldier.data());CHECK(Same(rendered,authored));Put<short>(meta.data(),52*0x24+6,47);
 // A head-only update must not prevent the arms returning when reachable.
 poses[7].packet.left=p.left;Frame(soldier.data());CHECK(Near(rendered[47],trackedHead)&&Distance(rendered[20],authored[20])>.04f);
 poses[7].packet.head={};poses[7].packet.left=At(-3,3,3);
 // Real relays must match the currently selected native item; weapon swaps
 // restore native animation until the matching pose arrives, without freezing
 // or applying a knife attachment to the next weapon.
 auto knife=p;knife.kind=Relay;knife.flags=LeftValid|RightValid|WeaponHeld;knife.head={};
 strcpy_s(knife.weaponName.data(),knife.weaponName.size(),"knife");knife.weapon=At(.1f,1.25f,.4f);
 actors[7].weapon=localSoldier.data();actors[7].name=knife.weaponName;
 poses[7].packet=knife;poses[7].received=GetTickCount64();Frame(soldier.data());CHECK(Near(rendered[64],knife.weapon));
 actors[7].name[0]='x';Frame(soldier.data());CHECK(Same(rendered,authored));
 actors[7].name=knife.weaponName;Frame(soldier.data());CHECK(Near(rendered[64],knife.weapon));
 actors[7].weapon=nullptr;Frame(soldier.data());CHECK(Same(rendered,authored));
 // Local action retransmission is independent of pose sequence and expires.
 actors[0].weapon=localSoldier.data();localId=0;
 PublishNetworkSnap(localSoldier.data(),30);Packet action;AttachEvents(action,GetTickCount64());
 CHECK(action.snapSerial&&action.snapDegrees==30);const auto serial=action.snapSerial;
 action={};AttachEvents(action,GetTickCount64());CHECK(action.snapSerial==serial);
 action={};AttachEvents(action,events.snapTime+301);CHECK(!action.snapSerial);
 PublishNetworkSnap(soldier.data(),-30);CHECK(events.snapSerial==serial); // remote owner rejected
 PublishNetworkCrateThrow(localSoldier.data(),localSoldier.data(),At(-.3f,1.2f,.4f),{0,1,3});
 action={};action.flags=LeftValid|LeftCrateHeld;AttachEvents(action,GetTickCount64());
 CHECK(action.throwSerial&&Near(action.throwLaunch,At(-.3f,1.2f,.4f))&&action.throwVelocity.z==3);
 action={};AttachEvents(action,GetTickCount64());CHECK(!action.throwSerial); // no held crate
 action.flags=LeftValid|LeftCrateHeld;AttachEvents(action,events.throwTime+601);CHECK(!action.throwSerial);
 std::array<std::array<float,5>,2> curls{};
 movementFixture=true;PublishNetworkPose(localSoldier.data(),localSoldier.data(),At(),At(),At(),At(),At(),At(),true,true,curls);
 CHECK((outgoing.flags&MovementValid)&&outgoing.movementYawDegrees==-13.25f);
 movementFixture=false;PublishNetworkPose(localSoldier.data(),localSoldier.data(),At(),At(),At(),At(),At(),At(),true,true,curls);
 CHECK(!(outgoing.flags&MovementValid)&&outgoing.movementYawDegrees==0);
 // A flat observer cannot publish synthetic hands, fire, turn or throw state.
 observerOnly=true;outgoingTime=0;events={};
 PublishNetworkPose(localSoldier.data(),localSoldier.data(),At(),At(),At(),At(),At(),At(),true,true,curls);
 PublishNetworkSnap(localSoldier.data(),30);PublishNetworkCrateThrow(localSoldier.data(),localSoldier.data(),At(),{0,1,3});
 CHECK(!outgoingTime&&!events.snapTime&&!events.throwTime&&!NetworkClientActive());observerOnly=false;
 // Mesh suppression must remain strictly instance/owner/lease scoped. Its
 // draw path is reversible on the next call and never changes native objects.
 std::vector<BYTE> rendererImage(0x200000);render=rendererImage.data();weaponRenderImage=render;
 std::array<BYTE,0x400> remotePlayer{},remoteWeapon{},definition{};
 std::array<BYTE,0x360> geometry{};std::array<void*,2> weak{nullptr,soldier.data()},inventory{nullptr,remoteWeapon.data()};
 auto& actor=actors[7];actor.player=remotePlayer.data();actor.weak=weak.data();actor.weapon=remoteWeapon.data();actor.ai=false;
 Put<void*>(remotePlayer.data(),0,game+ClientProfile.playerVt);Put<void*>(remotePlayer.data(),0xcc,weak.data());
 Put<void*>(remoteWeapon.data(),0,game+ClientProfile.weaponVt);Put<void*>(remoteWeapon.data(),0x34,soldier.data());Put<void*>(remoteWeapon.data(),0x44,geometry.data());
 Put<void*>(remoteWeapon.data(),0x24,definition.data());Put<void*>(definition.data(),0,game+ClientProfile.weaponTemplateVt);
 memcpy(definition.data()+0x10,"eu_ar_scar11",12);Put<unsigned>(definition.data(),0x20,12);Put<unsigned>(definition.data(),0x24,15);
 Put<void*>(geometry.data(),0,render+0x1d5400);Put<void*>(geometry.data(),0x290,remoteWeapon.data());
 Put<int>(soldier.data(),0x218,1);Put<void*>(soldier.data(),0x234,inventory.data());Put<void*>(soldier.data(),0x238,inventory.data()+2);
 poses[7].packet.kind=Relay;poses[7].packet.flags=LeftValid|RightValid;poses[7].packet.weaponName={};memcpy(poses[7].packet.weaponName.data(),"eu_ar_scar11",12);poses[7].received=GetTickCount64();
 originalWeaponDraw=reinterpret_cast<MeshDraw>(&DrawStub);originalWeaponShadow=reinterpret_cast<MeshShadow>(&ShadowStub);
 const auto beforeWeapon=remoteWeapon;const auto beforeGeometry=geometry;
 CHECK(HideRemoteWeaponGeometry(geometry.data()));WeaponDrawHook(geometry.data(),nullptr,1,2,3);WeaponShadowHook(geometry.data(),nullptr,1,2,3,4,5,6);
 CHECK(!drawCalls&&!shadowCalls&&remoteWeapon==beforeWeapon&&geometry==beforeGeometry);
 poses[7].packet.flags|=WeaponHeld;WeaponDrawHook(geometry.data(),nullptr,11,22,33);WeaponShadowHook(geometry.data(),nullptr,1,2,3,4,5,6);
 CHECK(drawCalls==1&&shadowCalls==1&&drawArgs[5]==6);poses[7].packet.flags&=~WeaponHeld;
 poses[7].packet.flags|=LeftCrateHeld;CHECK(!HideRemoteWeaponGeometry(geometry.data()));poses[7].packet.flags&=~LeftCrateHeld;
 poses[7].received=GetTickCount64()-251;CHECK(!HideRemoteWeaponGeometry(geometry.data()));poses[7].received=GetTickCount64();
 poses[7].packet.weaponName[0]='x';CHECK(!HideRemoteWeaponGeometry(geometry.data()));poses[7].packet.weaponName[0]='e';
 Put<void*>(soldier.data(),0x28c,remoteWeapon.data());CHECK(!HideRemoteWeaponGeometry(geometry.data()));Put<void*>(soldier.data(),0x28c,nullptr);
 Put<void*>(soldier.data(),0x34,remoteWeapon.data());CHECK(!HideRemoteWeaponGeometry(geometry.data()));Put<void*>(soldier.data(),0x34,nullptr);
 Put<DWORD>(soldier.data(),0x14,0x20);CHECK(!HideRemoteWeaponGeometry(geometry.data()));Put<DWORD>(soldier.data(),0x14,0);
 weak[1]=localSoldier.data();CHECK(!HideRemoteWeaponGeometry(geometry.data()));weak[1]=soldier.data();
 inventory[1]=localSoldier.data();CHECK(!HideRemoteWeaponGeometry(geometry.data()));inventory[1]=remoteWeapon.data();
 localId=7;CHECK(!HideRemoteWeaponGeometry(geometry.data()));localId=0;
 ownerThread++;CHECK(!HideRemoteWeaponGeometry(geometry.data()));ownerThread--;
 poses[7].packet.kind=Mirror;CHECK(!HideRemoteWeaponGeometry(geometry.data()));settings.mirror=true;actor.ai=true;CHECK(HideRemoteWeaponGeometry(geometry.data()));
 poses[7].packet.kind=Relay;CHECK(!HideRemoteWeaponGeometry(geometry.data()));actor.ai=false;
 // Retired/missing actor and malformed geometry retain stock drawing.
 actor.weapon=nullptr;CHECK(!HideRemoteWeaponGeometry(geometry.data()));actor.weapon=remoteWeapon.data();
 Put<void*>(geometry.data(),0x290,nullptr);CHECK(!HideRemoteWeaponGeometry(geometry.data()));Put<void*>(geometry.data(),0x290,remoteWeapon.data());
 CHECK(!HideRemoteWeaponGeometry(nullptr));poses[7].packet.flags=0;CHECK(!HideRemoteWeaponGeometry(geometry.data()));
 // Cosmetic haptics cross-thread drain: per-hand bits, one-shot, focus and age gates.
 InterlockedExchange(&pendingFists,3);InterlockedExchange(&pendingFistTime,LONG(GetTickCount()));
 CHECK(TakeNetworkFistBumps(true)==3&&TakeNetworkFistBumps(true)==0);
 InterlockedExchange(&pendingFists,2);InterlockedExchange(&pendingFistTime,LONG(GetTickCount()));CHECK(!TakeNetworkFistBumps(false));
 InterlockedExchange(&pendingFists,1);InterlockedExchange(&pendingFistTime,LONG(GetTickCount()-101));CHECK(!TakeNetworkFistBumps(true));
 // Full torso/head override also restores through a skipped native LOD frame.
 // Complete verified topology and valid relay frames, rather than the earlier
 // intentionally partial mirror fixtures, exercise the production path.
 authored=detailedReference;authored[11]=At(0,.9f,0);authored[12]=At(0,1.1f,0);authored[13]=At(0,1.3f,0);
 authored[45]=At(0,1.45f,0);authored[46]=At(0,1.5f,0);for(int i=47;i<64;++i)authored[i]=At(0,1.6f,0);
 authored[72]=authored[13];authored[73]=authored[12];authored[74]=At(.03f,1.1f,0);
 // A standing waist with inherited aim pitch must be written/restored too.
 authored[11].values[1]={0,.8f,.6f,0};authored[11].values[2]={0,-.6f,.8f,0};
 for(int i=64;i<80;++i){Put<int>(meta.data(),i*0x24,i);Put<short>(meta.data(),i*0x24+6,i<72?34:i==72?13:i<75?12:0);}
 Packet complete;complete.kind=Relay;complete.player=7;complete.session=42;complete.flags=LeftValid|RightValid;
 complete.body=At();complete.camera=At(0,1.6f,0);complete.head=complete.camera;complete.head.values[0]={.8f,0,-.6f,0};complete.head.values[2]={.6f,0,.8f,0};
 complete.left=Multiply(*InverseAnimatedBone(left->wristFromPalm),authored[20]);complete.right=Multiply(*InverseAnimatedBone(right->wristFromPalm),authored[35]);complete.weapon=complete.right;
 complete.weaponName=actor.name;poses[7].packet=complete;poses[7].received=GetTickCount64();bindings[7]={};refreshNative=true;
 Frame(soldier.data());CHECK(!Near(rendered[13],authored[13])&&bindings[7].overridePose->written[13]);
 CHECK(!Near(rendered[11],authored[11])&&bindings[7].overridePose->written[11]);
 const auto fullTracked=rendered;refreshNative=false;Frame(soldier.data());CHECK(Same(rendered,fullTracked));
 for(int i=0;i<11;++i)CHECK(Near(rendered[i],authored[i]));for(int i=75;i<80;++i)CHECK(Near(rendered[i],authored[i]));
 poses[7].received=GetTickCount64()-251;Frame(soldier.data());CHECK(Same(rendered,authored));refreshNative=true;
 // Even without a local first-person callback, the 3P callback must pump/reset a lost roster.
 lastPump=0;render=nullptr;RemoteFinalizeHook(soldier.data(),nullptr,.0125f,17);CHECK(localId==256&&!poses[7].received&&Same(rendered,authored));
 puts("Third-person callback ordering, remote native rig writes, LOD fallback, stale/thread/local guards and observer receive pump passed.");return 0;
}
