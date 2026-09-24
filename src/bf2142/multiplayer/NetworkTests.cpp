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
int main(int argc,char** argv){
 if(argc==2||argc==3){BodyBones b,reference;std::ifstream f(argv[1],std::ios::binary);f.read(reinterpret_cast<char*>(b.data()),sizeof(b));CHECK(f.gcount()==sizeof(b));if(argc==3){std::ifstream ref(argv[2],std::ios::binary);ref.read(reinterpret_cast<char*>(reference.data()),sizeof(reference));CHECK(ref.gcount()==sizeof(reference));}CHECK(!TestRig(b,argc==3?&reference:nullptr));puts("Private captured 80-bone rig passed.");return 0;}
 Packet p=Sample(),out;CHECK(Validate(p,p.secret));CHECK(Decode(&p,sizeof(p),p.secret,&out));CHECK(!Decode(&p,sizeof(p)-1,p.secret,&out));CHECK(!Decode(&p,sizeof(p)+1,p.secret,&out));
 auto bad=p;bad.secret[2]^=1;CHECK(!Validate(bad,p.secret));bad=p;bad.player=256;CHECK(!Validate(bad,p.secret));bad=p;bad.kind=4;CHECK(!Validate(bad,p.secret));bad=p;bad.flags=8;CHECK(!Validate(bad,p.secret));bad=p;bad.session=0;CHECK(!Validate(bad,p.secret));
 bad=p;bad.version++;CHECK(!Validate(bad,p.secret));bad=p;bad.head.values[3][0]=5;CHECK(!Validate(bad,p.secret));bad=p;bad.curls[0]=1.1f;CHECK(!Validate(bad,p.secret));bad=p;bad.right.values[0][0]=std::numeric_limits<float>::quiet_NaN();CHECK(!Validate(bad,p.secret));bad=p;bad.weaponName.back()='x';CHECK(!Validate(bad,p.secret));
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
 CHECK(!TestRig(Rig()));auto reference=Rig(),lod=reference;for(int w:{20,35})for(int i=w+1;i<w+10;++i)lod[i]=lod[w];CHECK(!CaptureBodyPalm(lod,true));CHECK(!TestRig(lod,&reference));for(int i=18;i<30;++i)lod[i]=lod[17];CHECK(!TestRig(lod,&reference));
 // Actual nonblocking UDP round trip: invalid tokens are dropped, source port is retained.
 Transport a,b;CHECK(a.Open(false,1)&&b.Open(false,1));CHECK(a.LocalPort()&&b.LocalPort());p=Sample();bad=p;bad.secret[1]^=1;CHECK(a.Send(bad,b.LocalPort()));CHECK(a.Send(p,b.LocalPort()));
 unsigned port=0;bool got=false;for(int i=0;i<100&&!got;++i){got=b.Receive(&out,&port,p.secret);if(!got)Sleep(1);}CHECK(got&&port==a.LocalPort()&&out.player==7);out.kind=Relay;CHECK(b.Send(out,port));got=false;for(int i=0;i<100&&!got;++i){got=a.Receive(&out,&port,p.secret);if(!got)Sleep(1);}CHECK(got&&port==b.LocalPort()&&out.kind==Relay);CHECK(!a.Receive(&out,&port,p.secret));
 puts("Pose validation, replay/dropout guards, body-space fire, independent remote arms and loopback round trip passed.");return 0;
}
