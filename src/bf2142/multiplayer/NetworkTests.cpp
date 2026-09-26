#include "NativeJoinProof.h"
#include "PoseProtocol.h"
#include "RemoteArmMath.h"
#include "LoopbackTransport.h"
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include <fstream>
using namespace bfvr;using namespace bfvr::bf2142;using namespace bfvr::bf2142::net;
#define CHECK(x) do{if(!(x)){printf("Network test failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
Matrix At(float x=0,float y=0,float z=0){Matrix m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
Matrix Yaw(float a){auto m=At();m.values[0]={std::cos(a),0,-std::sin(a),0};m.values[2]={std::sin(a),0,std::cos(a),0};return m;}
bool Near(const Matrix& a,const Matrix& b,float e=.001f){for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(std::abs(a.values[i][j]-b.values[i][j])>e)return false;return true;}
Packet Sample(){Packet p;p.session=42;p.secret[0]=91;p.player=7;p.sequence=1;p.flags=LeftValid|RightValid|WeaponHeld;p.body=At(50,1,90);p.camera=At(0,1.5f,0);p.head=p.camera;p.left=At(-.2f,1.3f,.3f);p.right=At(.2f,1.3f,.3f);p.weapon=p.right;std::memcpy(p.weaponName.data(),"eu_ar_scar11",13);return p;}
BodyBones Rig(){
 BodyBones b;for(auto& m:b)m=At();
 for(int side=0;side<2;++side){const int s=side?31:15,e=side?33:17,w=side?35:20;const float x=side?.18f:-.18f;
  b[s]=At(x,1.4f,0);b[e]=At(x,1.1f,0);b[w]=At(x,1.1f,.3f);
  for(int i=s+1;i<e;++i)b[i]=b[s];for(int i=e+1;i<w;++i)b[i]=b[e];for(int i=w+1;i<w+10;++i)b[i]=b[w];
  b[side?39:24]=At(x+.025f,1.1f,.38f);b[side?36:21]=At(x-.025f,1.1f,.38f);
 }
 for(int i=64;i<72;++i)b[i]=At(.18f,1.1f,.3f+float(i-64)*.03f);return b;
}
int TestRig(const BodyBones& b,const BodyBones* reference=nullptr){
 auto l=CaptureBodyPalm(b,true),r=CaptureBodyPalm(b,false);if(reference){if(!l)l=CaptureBodyPalm(*reference,true);if(!r)r=CaptureBodyPalm(*reference,false);}CHECK(l&&r);
 Packet p=Sample();p.left=Multiply(*InverseAnimatedBone(l->wristFromPalm),b[20]);p.right=Multiply(*InverseAnimatedBone(r->wristFromPalm),b[35]);
 const auto still=SolveRemoteArms(b,p,*l,*r,reference);CHECK(still);CHECK(Near((*still)[20],b[20],.004f));CHECK(Near((*still)[35],b[35],.004f));
 // A one-arm wave leaves legs, head, other hand and that hand's item untouched.
 p.flags=LeftValid;p.left.values[3][1]+=.14f;
 const auto wave=SolveRemoteArms(b,p,*l,*r,reference);CHECK(wave);
 for(int i=0;i<80;++i)if(i<15||i>=30)CHECK(Near((*wave)[i],b[i]));
 CHECK(Distance((*wave)[20],b[20])>.04f);
 p.flags=0;const auto stale=SolveRemoteArms(b,p,*l,*r,reference);CHECK(stale);for(int i=0;i<80;++i)CHECK(Near((*stale)[i],b[i]));
 // Cupped/held right hand keeps the authored item rigidly attached.
 p.flags=RightValid;p.right.values[3][0]+=.05f;
 const auto right=SolveRemoteArms(b,p,*l,*r,reference);CHECK(right);
 const auto before=Multiply(b[64],*InverseAnimatedBone(b[35])),after=Multiply((*right)[64],*InverseAnimatedBone((*right)[35]));CHECK(Near(before,after));
 // One unreachable arm keeps its native pose while the other still tracks.
 p.flags=LeftValid|RightValid;p.left.values[3][0]-=3.f;
 const auto partial=SolveRemoteArms(b,p,*l,*r,reference);CHECK(partial);
 for(int i=14;i<30;++i)CHECK(Near((*partial)[i],b[i]));
 CHECK(Near((*partial)[35],(*right)[35]));
 p.flags=LeftValid;CHECK(!SolveRemoteArms(b,p,*l,*r,reference));
 auto broken=b;broken[20].values[0][0]=std::numeric_limits<float>::quiet_NaN();CHECK(!SolveRemoteArms(broken,p,*l,*r));
 return 0;
}
int TestHead(const BodyBones& b){
 const auto originalHead=InverseAnimatedBone(b[47]);CHECK(originalHead);
 for(int axis=0;axis<3;++axis){
  auto tracked=Yaw(.7f);
  if(axis==1){tracked=At();tracked.values[1]={0,.8f,.6f,0};tracked.values[2]={0,-.6f,.8f,0};}
  if(axis==2){tracked=At();tracked.values[0]={.8f,.6f,0,0};tracked.values[1]={-.6f,.8f,0,0};}
  tracked.values[3]={2,-2,1,1}; // HMD translation cannot detach the remote head.
  const auto solved=SolveRemoteHead(b,tracked);CHECK(solved);
  auto expected=tracked;expected.values[3]=b[47].values[3];CHECK(Near((*solved)[47],expected,.0001f));
  CHECK(Distance((*solved)[47],b[47])<.00001f);
  const auto inverse=InverseAnimatedBone((*solved)[47]);CHECK(inverse);
  for(int i=0;i<80;++i){
   if(i<47||i>=64)CHECK(Near((*solved)[i],b[i]));
   else CHECK(Near(Multiply((*solved)[i],*inverse),Multiply(b[i],*originalHead),.0001f));
  }
  const auto again=SolveRemoteHead(*solved,tracked);CHECK(again);for(int i=0;i<80;++i)CHECK(Near((*again)[i],(*solved)[i],.0001f));
 }
 auto bad=At();bad.values[0][0]=std::numeric_limits<float>::quiet_NaN();CHECK(!SolveRemoteHead(b,bad));
 auto damaged=b;damaged[52]=bad;CHECK(!SolveRemoteHead(damaged,At()));
 // Invalid arm geometry cannot cancel an otherwise valid head rotation.
 damaged=b;damaged[20]=bad;CHECK(SolveRemoteHead(damaged,Yaw(.4f)));
 return 0;
}
int TestRemoteFingers(){
 BodyBones rig=Rig();
 for(int wrist:{20,35})for(int group=0;group<3;++group)for(int joint=0;joint<3;++joint){
  auto m=rig[wrist];m.values[3][0]+=(group-1)*.025f;m.values[3][2]+=.055f+joint*.025f;rig[wrist+1+group*3+joint]=m;
 }
 for(bool left:{true,false}){
  const int wrist=left?20:35;auto palm=At();const float sign=left?1.f:-1.f;
  palm.values[0]={0,-sign,0,0};palm.values[1]={0,0,-sign,0};palm.values[2]={1,0,0,0};
  std::array<float,5> open{},closed;closed.fill(1);
  auto a=rig,b=rig;CHECK(PoseRemoteFingers(a,left,palm,open,false));CHECK(PoseRemoteFingers(b,left,palm,closed,false));
  // Fist movement is inward; roots, wrist, other hand, arms and item remain fixed.
  CHECK(b[wrist+3].values[3][1]<a[wrist+3].values[3][1]-.01f);
  CHECK(b[wrist+6].values[3][1]<a[wrist+6].values[3][1]-.01f);
  for(int i=0;i<80;++i)if(i<=wrist||i>=wrist+10)CHECK(Near(b[i],rig[i]));
  for(int base:{wrist+1,wrist+4,wrist+7})for(int j=0;j<2;++j)CHECK(std::abs(Distance(b[base+j],b[base+j+1])-Distance(rig[base+j],rig[base+j+1]))<.0001f);
  // Different controller input affects only its matching chain.
  auto indexOnly=open;indexOnly[1]=1;auto index=rig;CHECK(PoseRemoteFingers(index,left,palm,indexOnly,false));
  for(int i=wrist+1;i<wrist+10;++i)CHECK(Near(index[i],(i>=wrist+4&&i<=wrist+6)?b[i]:a[i]));
  auto held=rig;CHECK(PoseRemoteFingers(held,left,palm,open,true));for(int i=wrist+1;i<=wrist+3;++i)CHECK(Near(held[i],rig[i]));
  auto invalid=closed;invalid[2]=std::numeric_limits<float>::quiet_NaN();auto unchanged=rig;CHECK(!PoseRemoteFingers(unchanged,left,palm,invalid,false));for(int i=0;i<80;++i)CHECK(Near(unchanged[i],rig[i]));
  auto collapsed=rig;collapsed[wrist+2]=collapsed[wrist+1];auto before=collapsed;CHECK(!PoseRemoteFingers(collapsed,left,palm,closed,false));for(int i=0;i<80;++i)CHECK(Near(collapsed[i],before[i]));
  // Soldier yaw and position cannot reverse the curl direction.
  auto transform=Yaw(.8f);transform.values[3]={4,2,-3,1};auto rotated=rig;for(auto& m:rotated)m=Multiply(m,transform);
  CHECK(PoseRemoteFingers(rotated,left,Multiply(palm,transform),closed,false));for(int i=0;i<80;++i)CHECK(Near(rotated[i],Multiply(b[i],transform),.0001f));
 }
 auto l=CaptureBodyPalm(rig,true),r=CaptureBodyPalm(rig,false);CHECK(l&&r);auto p=Sample();p.flags=LeftValid|RightValid;p.left=Multiply(*InverseAnimatedBone(l->wristFromPalm),rig[20]);p.right=Multiply(*InverseAnimatedBone(r->wristFromPalm),rig[35]);
 auto open=SolveRemoteArms(rig,p,*l,*r);p.curls.fill(1);auto closed=SolveRemoteArms(rig,p,*l,*r);CHECK(open&&closed);CHECK(Distance((*open)[26],(*closed)[26])>.01f);CHECK(Near((*open)[20],(*closed)[20]));CHECK(Near((*open)[35],(*closed)[35]));
 return 0;
}
int TestTrackedItems(){
 const auto rig=Rig();const auto l=CaptureBodyPalm(rig,true),r=CaptureBodyPalm(rig,false);CHECK(l&&r);
 auto p=Sample();p.left=Multiply(*InverseAnimatedBone(l->wristFromPalm),rig[20]);p.right=Multiply(*InverseAnimatedBone(r->wristFromPalm),rig[35]);
 p.weapon=Multiply(Yaw(.9f),At(-.1f,1.3f,.35f));
 for(const char* name:{"knife","knife_unlock","eu_ar_rifle","as_ar_rifle","unl_hub_medic","unl_hub_ammo"}){
  p.weaponName={};strcpy_s(p.weaponName.data(),p.weaponName.size(),name);const bool crate=std::strstr(name,"hub_")!=nullptr;
  p.flags=LeftValid|RightValid|(crate?LeftCrateHeld:WeaponHeld);
  auto result=SolveRemoteArms(rig,p,*l,*r);CHECK(result);CHECK(Near((*result)[64],p.weapon));
  for(int i=64;i<72;++i)CHECK(Near(Multiply((*result)[i],*InverseAnimatedBone(p.weapon)),Multiply(rig[i],*InverseAnimatedBone(rig[64]))));
  for(int i=0;i<80;++i)if(i<15||(i>=45&&i<64)||i>=72)CHECK(Near((*result)[i],rig[i]));
  // Native knife slash / rifle ADS can change the item's wrist attachment.
  // The same transmitted pose must still place mesh1 at exactly the same root.
  auto animated=rig;auto delta=Multiply(Yaw(-.7f),At(.06f,-.04f,.1f));
  for(int i=64;i<72;++i)animated[i]=Multiply(animated[i],delta);
  auto changed=SolveRemoteArms(animated,p,*l,*r);CHECK(changed);for(int i=64;i<72;++i)CHECK(Near((*changed)[i],(*result)[i]));
  // A crate follows the left item pose even when the right palm is moving.
  if(crate){auto moved=p;moved.right.values[3][0]+=.2f;changed=SolveRemoteArms(rig,moved,*l,*r);CHECK(changed);for(int i=64;i<72;++i)CHECK(Near((*changed)[i],(*result)[i]));}
 }
 // Preserve the accepted pistol binding, unknown mods, and unlike bot weapons.
 for(const char* name:{"eu_handgun","as_handgun","custom_weapon"}){
  p.weaponName={};strcpy_s(p.weaponName.data(),p.weaponName.size(),name);p.flags=RightValid|WeaponHeld;p.right.values[3][0]+=.01f;
  auto result=SolveRemoteArms(rig,p,*l,*r);CHECK(result);
  CHECK(Near(Multiply((*result)[64],*InverseAnimatedBone((*result)[35])),Multiply(rig[64],*InverseAnimatedBone(rig[35]))));
 }
 p.weaponName={};strcpy_s(p.weaponName.data(),p.weaponName.size(),"knife");p.kind=Mirror;
 auto mirror=SolveRemoteArms(rig,p,*l,*r);CHECK(mirror);CHECK(!Near((*mirror)[64],p.weapon));
 p.kind=Relay;p.weapon={};auto invalid=SolveRemoteArms(rig,p,*l,*r);CHECK(invalid);CHECK(Near((*invalid)[64],(*mirror)[64]));
 // Reproduce the outgoing knife-palm failure without a game: individually
 // accepted animated matrices fail packet validation after transpose inversion.
 auto binding=At(.03f,-.04f,.02f);binding.values[0][0]=.997f;binding.values[0][1]=.001f;
 const auto grip=Multiply(Yaw(.6f),At(.2f,1.2f,.3f)),wrist=Multiply(binding,grip);CHECK(InverseRigid(binding)&&InverseRigid(wrist));
 p=Sample();p.weaponName={};strcpy_s(p.weaponName.data(),p.weaponName.size(),"knife");p.right=Multiply(*InverseRigid(binding),wrist);CHECK(!Validate(p,p.secret));
 p.right=Multiply(*InverseAnimatedTransform(binding),wrist);CHECK(Validate(p,p.secret)&&Near(p.right,grip,.00001f));
 binding.values[0][0]=.8f;CHECK(!InverseAnimatedTransform(binding));
 return 0;
}
int TestStableObserverArms(){
 auto reference=Rig();reference[11]=At(0,1,0);reference[12]=At(0,1.15f,0);reference[13]=At(0,1.3f,0);
 reference[14]=At(-.08f,1.35f,0);reference[30]=At(.08f,1.35f,0);
 auto l=CaptureBodyPalm(reference,true),r=CaptureBodyPalm(reference,false);CHECK(l&&r);
 auto p=Sample();p.kind=Relay;p.weaponName={};strcpy_s(p.weaponName.data(),p.weaponName.size(),"eu_handgun");
 RemoteArmContinuity c1,c2;
 const auto baseline=SolveRemoteArms(reference,p,*l,*r,&reference,&c1,.016f);CHECK(baseline);
 auto recoil=reference;
 for(int side=0;side<2;++side){
  const int collar=side?30:14,wrist=side?35:20;
  const auto kick=Multiply(Yaw(side?.5f:-.4f),At(.035f,.045f,-.06f));
  for(int i=collar;i<wrist+10;++i)recoil[i]=Multiply(recoil[i],kick);
  if(side)for(int i=64;i<72;++i)recoil[i]=Multiply(recoil[i],kick);
 }
 const auto shooting=SolveRemoteArms(recoil,p,*l,*r,&reference,&c2,.016f);CHECK(shooting);
 for(int i=14;i<45;++i)CHECK(Near((*shooting)[i],(*baseline)[i]));
 for(int i=64;i<72;++i)CHECK(Near((*shooting)[i],(*baseline)[i]));
 // Stabilization cannot move the legs/head or cancel real controller movement.
 for(int i=0;i<14;++i)CHECK(Near((*shooting)[i],recoil[i]));
 for(int i=45;i<64;++i)CHECK(Near((*shooting)[i],recoil[i]));
 p.right.values[3][0]+=.06f;const auto moved=SolveRemoteArms(recoil,p,*l,*r,&reference,&c2,.016f);CHECK(moved);
 CHECK(Distance((*moved)[35],(*shooting)[35])>.05f);
 return 0;
}
int main(int argc,char** argv){
 CHECK(!TestRemoteFingers());CHECK(!TestTrackedItems());CHECK(!TestStableObserverArms());
 if(argc==2||argc==3){BodyBones b,reference;std::ifstream f(argv[1],std::ios::binary);f.read(reinterpret_cast<char*>(b.data()),sizeof(b));CHECK(f.gcount()==sizeof(b));if(argc==3){std::ifstream ref(argv[2],std::ios::binary);ref.read(reinterpret_cast<char*>(reference.data()),sizeof(reference));CHECK(ref.gcount()==sizeof(reference));}CHECK(!TestRig(b,argc==3?&reference:nullptr));CHECK(!TestHead(b));puts("Private captured 80-bone arms and head rig passed.");return 0;}
 Packet p=Sample(),out;CHECK(Validate(p,p.secret));CHECK(Decode(&p,sizeof(p),p.secret,&out));CHECK(!Decode(&p,sizeof(p)-1,p.secret,&out));CHECK(!Decode(&p,sizeof(p)+1,p.secret,&out));
 auto bad=p;bad.secret[2]^=1;CHECK(!Validate(bad,p.secret));bad=p;bad.player=256;CHECK(!Validate(bad,p.secret));bad=p;bad.kind=4;CHECK(!Validate(bad,p.secret));bad=p;bad.flags=8;CHECK(!Validate(bad,p.secret));bad=p;bad.session=0;CHECK(!Validate(bad,p.secret));
 bad=p;bad.version++;CHECK(!Validate(bad,p.secret));bad=p;bad.head.values[3][0]=5;CHECK(!Validate(bad,p.secret));bad=p;bad.curls[0]=1.1f;CHECK(!Validate(bad,p.secret));bad=p;bad.right.values[0][0]=std::numeric_limits<float>::quiet_NaN();CHECK(!Validate(bad,p.secret));bad=p;bad.weaponName.back()='x';CHECK(!Validate(bad,p.secret));
 auto moving=p;moving.flags|=MovementValid;moving.movementYawDegrees=-143.5f;
 CHECK(Validate(moving,moving.secret)&&Decode(&moving,sizeof(moving),moving.secret,&out)&&out.movementYawDegrees==-143.5f);
 bad=moving;bad.movementYawDegrees=180.01f;CHECK(!Validate(bad,p.secret));
 bad=moving;bad.movementYawDegrees=std::numeric_limits<float>::quiet_NaN();CHECK(!Validate(bad,p.secret));
 bad=moving;bad.flags&=~MovementValid;CHECK(!Validate(bad,p.secret));
 bad=moving;bad.version=2;CHECK(!Validate(bad,p.secret));
 auto event=p;event.snapSerial=1;event.snapDegrees=30;CHECK(Validate(event,event.secret));
 event.snapDegrees=91;CHECK(!Validate(event,event.secret));event.snapDegrees=0;CHECK(!Validate(event,event.secret));
 event=p;event.throwSerial=1;event.throwLaunch=At(0,1,.2f);event.throwVelocity={1,2,3};CHECK(!Validate(event,event.secret));
 event.weaponName={};std::memcpy(event.weaponName.data(),"unl_hub_medic",13);event.flags|=LeftCrateHeld;CHECK(Validate(event,event.secret));
 event.throwVelocity.x=41;CHECK(!Validate(event,event.secret));event.throwVelocity.x=1;event.throwLaunch.values[3][0]=5;CHECK(!Validate(event,event.secret));
 EventWindow events;CHECK(events.Accept(1,1,100));CHECK(!events.Accept(1,1,110));CHECK(!events.Accept(1,0,120));CHECK(events.Accept(1,2,200));
 CHECK(!events.Accept(1,1,220));CHECK(!events.Accept(1,3,201,120));CHECK(events.Accept(1,3,400,120));CHECK(events.Accept(2,1,500));
 const auto rotatedVelocity=TransformVelocity({0,1,3},Yaw(1.57079632679f));CHECK(std::abs(rotatedVelocity.x-3)<.001f&&rotatedVelocity.y==1&&std::abs(rotatedVelocity.z)<.001f);
 FreshPose fresh;CHECK(fresh.Accept(p,100));CHECK(!fresh.Accept(p,110));CHECK(fresh.Read(350));CHECK(!fresh.Read(351));p.sequence++;CHECK(fresh.Accept(p,400));CHECK(!fresh.Accept(p,399));p.session++;p.sequence=1;CHECK(!fresh.Accept(p,401));CHECK(fresh.Accept(p,1401));CHECK(Newer(0,0xffffffff));CHECK(!Newer(0xffffffff,0));fresh.Clear();CHECK(!fresh.Read(1401));
 // Server body yaw and position must not introduce a second tracked-gun turn.
 const auto camera=At(0,1.5f,0),gun=Multiply(Yaw(.75f),At(.3f,1.4f,.3f)),offset=At(.02f,-.03f,.4f);
 const auto native=Multiply(offset,camera);const auto local=MapTrackedFire(native,camera,gun);CHECK(local);CHECK(Near(*local,Multiply(offset,gun)));
 const auto body=Multiply(Yaw(1.4f),At(800,20,-600));const auto world=MapTrackedFire(Multiply(native,body),Multiply(camera,body),Multiply(gun,body));CHECK(world);CHECK(Near(*world,Multiply(*local,body),.002f));CHECK(!MapTrackedFire(At(10,1.5f,0),camera,gun));
 // A native near-rigid blended bone must cancel exactly, without changing
 // the stricter acceptance policy for network tracking or malformed frames.
 auto blend=At(.2f,.3f,.4f);blend.values[0][0]=.997f;blend.values[0][1]=.001f;
 const auto blendInverse=InverseAnimatedBone(blend);CHECK(blendInverse);CHECK(Near(Multiply(blend,*blendInverse),At(),.00001f));
 auto distorted=blend;distorted.values[0][0]=.8f;CHECK(!InverseAnimatedBone(distorted));
 auto blendedRig=Rig();for(int i=14;i<45;++i)blendedRig[i]=Multiply(blendedRig[i],blend);
 for(int i=64;i<72;++i)blendedRig[i]=Multiply(blendedRig[i],blend);
 CHECK(!TestRig(blendedRig));
 CHECK(!TestHead(Rig()));CHECK(!TestHead(blendedRig));
 CHECK(!TestRig(Rig()));auto reference=Rig(),lod=reference;for(int w:{20,35})for(int i=w+1;i<w+10;++i)lod[i]=lod[w];CHECK(!CaptureBodyPalm(lod,true));CHECK(!TestRig(lod,&reference));for(int i=18;i<30;++i)lod[i]=lod[17];CHECK(!TestRig(lod,&reference));
 // A subscription is a distinct receive lease, never a valid tracking/action packet.
 Packet sub;sub.kind=Subscribe;sub.player=8;sub.session=42;sub.sequence=1;sub.secret=p.secret;
 CHECK(Validate(sub,p.secret)&&Decode(&sub,sizeof(sub),p.secret,&out)&&out.kind==Subscribe);
 bad=sub;bad.flags=WeaponHeld;CHECK(!Validate(bad,p.secret));
 bad=sub;bad.right=At();CHECK(!Validate(bad,p.secret));
 bad=sub;bad.throwSerial=1;CHECK(!Validate(bad,p.secret));
 bad=sub;bad.snapDegrees=30;CHECK(!Validate(bad,p.secret));
 bad=sub;bad.weaponName[0]='a';CHECK(!Validate(bad,p.secret));
 bad=sub;bad.curls[1]=.5f;CHECK(!Validate(bad,p.secret));
 bad=sub;bad.throwVelocity.x=std::numeric_limits<float>::quiet_NaN();CHECK(!Validate(bad,p.secret));
 bad=sub;bad.version=3;CHECK(!Validate(bad,p.secret));
 // Actual nonblocking UDP round trip: invalid tokens are dropped, source port is retained.
 Transport a,b;CHECK(a.Open(false,1)&&b.Open(false,1));CHECK(a.LocalPort()&&b.LocalPort());p=Sample();bad=p;bad.secret[1]^=1;CHECK(a.Send(bad,b.LocalPort()));CHECK(a.Send(p,b.LocalPort()));
 unsigned port=0;bool got=false;for(int i=0;i<100&&!got;++i){got=b.Receive(&out,&port,p.secret);if(!got)Sleep(1);}CHECK(got&&port==a.LocalPort()&&out.player==7);out.kind=Relay;CHECK(b.Send(out,port));got=false;for(int i=0;i<100&&!got;++i){got=a.Receive(&out,&port,p.secret);if(!got)Sleep(1);}CHECK(got&&port==b.LocalPort()&&out.kind==Relay);CHECK(!a.Receive(&out,&port,p.secret));
 JoinChallenge challenge;challenge.player=7;challenge.session=42;challenge.secret=p.secret;challenge.nonce[0]=1;
 JoinChallenge decoded;CHECK(DecodeJoinChallenge(&challenge,sizeof(challenge),p.secret,decoded));
 CHECK(!DecodeJoinChallenge(&challenge,sizeof(challenge)-1,p.secret,decoded));
 auto wrongKey=p.secret;wrongKey[0]^=1;CHECK(!DecodeJoinChallenge(&challenge,sizeof(challenge),wrongKey,decoded));
 auto emptyNonce=challenge;emptyNonce.nonce={};CHECK(!DecodeJoinChallenge(&emptyNonce,sizeof(emptyNonce),p.secret,decoded));
 JoinProofPolicy proof;CHECK(!proof.Accept(challenge,8,42,1000));CHECK(!proof.Accept(challenge,7,43,1000));
 CHECK(proof.Accept(challenge,7,42,1000));CHECK(!proof.Accept(challenge,7,42,1200));CHECK(proof.Accept(challenge,7,42,1800));
 CHECK(!SendNativeJoinProof(nullptr,nullptr,challenge.nonce));
 puts("Pose validation, replay/dropout guards, body-space fire, independent remote arms and loopback round trip passed.");return 0;
}
