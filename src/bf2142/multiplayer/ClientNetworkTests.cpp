#include "NativeNetwork.cpp"
#include <vector>
#include <cstdio>
#include <cmath>
using namespace bfvr;using namespace bfvr::bf2142;using namespace bfvr::bf2142::net;
#define CHECK(x) do{if(!(x)){printf("Client network test failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
template<class T>void Put(void* p,unsigned offset,T value){memcpy(static_cast<BYTE*>(p)+offset,&value,sizeof(value));}
Matrix At(float x=0,float y=0,float z=0){Matrix m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
BodyBones authored{},rendered{};unsigned calls=0;void* calledSoldier=nullptr;float calledDelta=0;unsigned calledUpdate=0;
void __fastcall FinalizeStub(void* self,void*,float delta,unsigned update){++calls;calledSoldier=self;calledDelta=delta;calledUpdate=update;rendered=authored;}
bool Near(const Matrix& a,const Matrix& b){for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(std::abs(a.values[i][j]-b.values[i][j])>.001f)return false;return true;}
bool Same(const BodyBones& a,const BodyBones& b){for(int i=0;i<80;++i)if(!Near(a[i],b[i]))return false;return true;}
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
 // Stale tracking, other threads, local rigs and invalid skeletons retain native animation.
 poses[7].received=GetTickCount64()-1000;Frame(soldier.data());CHECK(Same(rendered,authored));
 poses[7].received=GetTickCount64();ownerThread++;Frame(soldier.data());CHECK(Same(rendered,authored));ownerThread--;
 Frame(localSoldier.data());CHECK(Same(rendered,authored));
 Put<int>(soldier.data(),0x2f0,0);Frame(soldier.data());CHECK(Same(rendered,authored));Put<int>(soldier.data(),0x2f0,1);
 Put<short>(meta.data(),35*0x24+6,0);Frame(soldier.data());CHECK(Same(rendered,authored));Put<short>(meta.data(),35*0x24+6,34);
 // Detailed reference restores a collapsed remote LOD before solving, leaving the other hand native.
 detailedReference=authored;referenceLeft=*left;referenceRight=*right;referenceValid=true;bindings[7]={};
 for(int i=18;i<30;++i)authored[i]=authored[17];for(int i=36;i<45;++i)authored[i]=authored[35];
 poses[7].received=GetTickCount64();Frame(soldier.data());CHECK(bindings[7].valid&&bindings[7].reported);CHECK(Distance(rendered[20],authored[20])>.04f);
 for(int i=30;i<80;++i)CHECK(Near(rendered[i],authored[i]));
 // Unreachable tracking falls back after the engine has refreshed every bone.
 poses[7].packet.left=At(-3,3,3);Frame(soldier.data());CHECK(Same(rendered,authored));
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
 // Even without a local first-person callback, the 3P callback must pump/reset a lost roster.
 lastPump=0;render=nullptr;RemoteFinalizeHook(soldier.data(),nullptr,.0125f,17);CHECK(localId==256&&!poses[7].received&&Same(rendered,authored));
 puts("Third-person callback ordering, remote native rig writes, LOD fallback, stale/thread/local guards and observer receive pump passed.");return 0;
}
