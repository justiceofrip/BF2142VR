#include "RemotePresentation.h"
#include "RemoteCollision.h"
#include "FistBump.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include <fstream>
using namespace bfvr;using namespace bfvr::bf2142;using namespace bfvr::bf2142::net;
#define CHECK(x) do{if(!(x)){printf("Remote presentation failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
Matrix At(float x=0,float y=0,float z=0){Matrix m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
Matrix Yaw(float a){auto m=At();m.values[0]={std::cos(a),0,-std::sin(a),0};m.values[2]={std::sin(a),0,std::cos(a),0};return m;}
bool Near(const Matrix& a,const Matrix& b,float e=.001f){for(int i=0;i<4;++i)for(int j=0;j<4;++j)if(std::abs(a.values[i][j]-b.values[i][j])>e)return false;return true;}
Packet Sample(){Packet p;p.kind=Relay;p.session=42;p.player=7;p.flags=LeftValid|RightValid|WeaponHeld;p.body=At();p.camera=p.head=At(0,1.5f,0);p.left=At(-.2f,1.3f,.3f);p.right=At(.2f,1.3f,.3f);p.weapon=At(.2f,1.3f,.5f);p.curls.fill(.9f);return p;}
BodyBones Rig(){BodyBones b;for(auto& m:b)m=At();b[11]=At(0,1,0);b[12]=At(0,1.2f,0);b[13]=At(0,1.35f,0);b[45]=At(0,1.5f,0);b[46]=At(0,1.53f,0);b[47]=At(0,1.59f,0);b[15]=At(-.2f,1.4f,0);b[31]=At(.2f,1.4f,0);b[72]=b[13];b[73]=b[12];b[74]=At(.05f,1.2f,0);return b;}
float Length(stereo::Vec3 p){return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);}
stereo::Vec3 Sub(stereo::Vec3 a,stereo::Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
// Check the actual skinning hierarchy, not just individually rigid matrices.
bool Attachments(const BodyBones& before,const BodyBones& after){
 const int child[]={11,12,13,14,15,30,31,45,46,47,72,73,74};
 const int parent[]={0,11,12,13,14,13,30,13,45,46,13,12,12};
 for(unsigned i=0;i<std::size(child);++i){
  const auto a=Multiply(before[child[i]],*InverseAnimatedBone(before[parent[i]]));
  const auto b=Multiply(after[child[i]],*InverseAnimatedBone(after[parent[i]]));
  for(int j=0;j<3;++j)if(std::abs(a.values[3][j]-b.values[3][j])>.001f)return false;
 }
 return true;
}
void PitchSubtree(BodyBones& b,int root,int last,float pitch){
 auto target=At();const float c=std::cos(pitch),s=std::sin(pitch);
 target.values[1]={0,c,s,0};target.values[2]={0,-s,c,0};
 auto local=b[root];local.values[3]={0,0,0,1};target=Multiply(local,target);target.values[3]=b[root].values[3];
 const auto delta=Multiply(*InverseAnimatedBone(b[root]),target);
 for(int i=root;i<=last;++i)b[i]=Multiply(b[i],delta);
}
float ChestUp(const BodyBones& b){
 // Rounded independent fixture calibration, rather than assuming identity bind.
 auto pitch=At();const float angle=-9.f*3.14159265f/180.f;
 pitch.values[1]={0,std::cos(angle),std::sin(angle),0};pitch.values[2]={0,-std::sin(angle),std::cos(angle),0};
 return Multiply(pitch,b[13]).values[1][1];
}
int main(int argc,char** argv){
 auto p=Sample();RemotePresentation slow,fast;CHECK(Near(slow.Update(p,100).right,p.right));fast.Update(p,100);
 p.right=Yaw(.8f);p.right.values[3]={.5f,1.3f,.3f,1};p.weapon=Multiply(At(0,0,.2f),p.right);p.head.values[3][0]=.1f;
 const auto held=Multiply(p.weapon,*InverseRigid(p.right));auto a=slow.Update(p,140);fast.Update(p,110);fast.Update(p,120);fast.Update(p,130);auto b=fast.Update(p,140);
 CHECK(Near(a.right,b.right)&&Near(a.head,b.head));CHECK(a.right.values[3][0]>.2f&&a.right.values[3][0]<.5f);
 CHECK(Near(Multiply(a.weapon,*InverseRigid(a.right)),held));CHECK(InverseRigid(a.right));
 CHECK(Near(slow.Update(p,140).right,a.right)); // repeated render does not advance time
 p.flags&=~WeaponHeld;CHECK(Near(slow.Update(p,141).right,p.right)&&slow.reset);
 p.flags|=WeaponHeld;slow.Update(p,142);p.snapSerial=3;p.right.values[3][0]=.1f;CHECK(Near(slow.Update(p,143).right,p.right)&&slow.reset);
 p.snapSerial=0;slow.Update(p,144);CHECK(!slow.reset); // expiry is not a new snap
 p.session++;slow.Update(p,145);CHECK(slow.reset);p.player++;slow.Update(p,146);CHECK(slow.reset);
 p.weaponName[0]='k';slow.Update(p,147);CHECK(slow.reset);p.body=Yaw(1.f);slow.Update(p,148);CHECK(slow.reset);
 p.body.values[3][0]=20;slow.Update(p,149);CHECK(slow.reset);slow.Update(p,500);CHECK(slow.reset);
 p.head.values[3][1]+=1;slow.Update(p,510);CHECK(slow.reset);
 p.kind=Mirror;CHECK(Near(slow.Update(p,520).head,p.head)&&slow.reset);
 p.head={};slow.Update(p,530);p.kind=Relay;p.head=p.camera;CHECK(Near(slow.Update(p,540).head,p.head)&&slow.reset);
 auto rig=Rig();p=Sample();p.head=Yaw(1.f);p.head.values[3]=p.camera.values[3];p.head.values[3][0]=.2f;
 const auto torso=SolveRemoteTorso(rig,p);CHECK(torso);
 for(int i=0;i<11;++i)CHECK(Near((*torso)[i],rig[i]));for(int i=75;i<80;++i)CHECK(Near((*torso)[i],rig[i]));
 CHECK(!Near((*torso)[13],rig[13]));
 CHECK(Attachments(rig,*torso));
 CHECK(ChestUp(*torso)>.978f); // bounded body lean, independent of gun pitch
 // Raised gun animations can fold both spines and counter-bend the neck.
 // Same HMD/hips must still give the same chest, shoulders and head attachment.
 auto aiming=rig;PitchSubtree(aiming,12,74,-1.1f);PitchSubtree(aiming,13,72,.25f);
 PitchSubtree(aiming,45,63,.3f);PitchSubtree(aiming,46,63,.55f);
 const auto raised=SolveRemoteTorso(aiming,p);CHECK(raised&&Attachments(aiming,*raised));
 for(int id:{12,13,14,15,30,31,45,46,47,72,73,74})CHECK(Near((*raised)[id],(*torso)[id]));
 // Positional lean bends connected joints; it must never stretch their offsets.
 auto extreme=p;extreme.head.values[3][0]=5;extreme.head.values[3][2]=-5;
 const auto leaning=SolveRemoteTorso(rig,extreme);CHECK(leaning&&Attachments(rig,*leaning));
 CHECK(ChestUp(*leaning)>.978f);
 // Native crouch/stance stays on the hips and base spine, while the upper chest
 // remains upright enough for the head to sit above its armor.
 auto crouched=rig;PitchSubtree(crouched,0,79,.5f);
 const auto crouch=SolveRemoteTorso(crouched,p);CHECK(crouch&&Attachments(crouched,*crouch));
 for(int id=0;id<12;++id)CHECK(Near((*crouch)[id],crouched[id]));
 CHECK((*crouch)[47].values[3][1]>(*crouch)[13].values[3][1]+.15f);

 CHECK(Near(Multiply((*torso)[72],*InverseAnimatedBone((*torso)[13])),Multiply(rig[72],*InverseAnimatedBone(rig[13]))));
 CHECK(Near(Multiply((*torso)[74],*InverseAnimatedBone((*torso)[12])),Multiply(rig[74],*InverseAnimatedBone(rig[12]))));
 const auto head=SolveRemoteHead(*torso,p.head);CHECK(head);auto expected=p.head;expected.values[3]=(*torso)[47].values[3];CHECK(Near((*head)[47],expected));
 // A neutral skinned model must stay neutral. This independent rounded bind
 // fixture has bent bone frames, even though its model-space posture is upright.
 // Zeroing those frames visibly leans the armor backward; rigid-matrix checks
 // alone cannot detect that regression.
 auto bind=rig;const float degree=3.14159265f/180.f;
 PitchSubtree(bind,11,74,-5*degree);PitchSubtree(bind,12,74,9*degree);
 PitchSubtree(bind,13,72,5*degree);PitchSubtree(bind,45,63,3*degree);PitchSubtree(bind,46,63,-12*degree);
 auto restPose=Sample();restPose.head=restPose.camera=bind[47];
 restPose.left=At(-.2f,.3f,0);restPose.right=At(.2f,.3f,0); // relaxed, no reach turn
 const auto atRest=SolveRemoteTorso(bind,restPose);CHECK(atRest&&Attachments(bind,*atRest));
 for(int id:{11,12,13,45,46,47,72,73,74}){
  const auto skin=Multiply(*InverseAnimatedBone(bind[id]),(*atRest)[id]);
  CHECK(skin.values[1][1]>.9998f);
  for(int j=0;j<3;++j)CHECK(std::abs((*atRest)[id].values[3][j]-bind[id].values[3][j])<.006f);
 }
 // Standing lower-spine arch must not remain beneath an upright chest. This
 // was visible as a pushed-out midsection even with relaxed/empty hands.
 auto neutral=Sample();const auto straight=SolveRemoteTorso(rig,neutral);CHECK(straight);
 for(float arch:{-.65f,.65f}){
  auto arched=rig;PitchSubtree(arched,11,74,arch);
  const auto upright=SolveRemoteTorso(arched,neutral);CHECK(upright&&Attachments(arched,*upright));
  CHECK(Near((*upright)[11],(*straight)[11]));
  for(int id:{11,12,13,14,15,30,31,45,46,47,72,73,74})CHECK(Near((*upright)[id],(*straight)[id]));
  for(int id=0;id<11;++id)CHECK(Near((*upright)[id],arched[id]));
  for(int id=75;id<80;++id)CHECK(Near((*upright)[id],arched[id]));
 }
 // Keep the lower-spine correction out of strongly tilted crouched hips.
 auto low=rig;PitchSubtree(low,0,79,.55f);const auto lowPose=SolveRemoteTorso(low,neutral);
 CHECK(lowPose&&Near((*lowPose)[11],low[11])&&Attachments(low,*lowPose));
 // Crossing the standing/crouched blend band is continuous, not a pose snap.
 float prior=0;
 for(int i=0;i<=30;++i){
  auto transition=rig;PitchSubtree(transition,11,74,.4f);const float tilt=.15f+float(i)*.01f;
  transition[0].values[1]={0,std::cos(tilt),std::sin(tilt),0};transition[0].values[2]={0,-std::sin(tilt),std::cos(tilt),0};
  const auto eased=SolveRemoteTorso(transition,neutral);CHECK(eased&&Attachments(transition,*eased));
  const float pitch=std::atan2((*eased)[11].values[1][2],(*eased)[11].values[1][1]);
  if(i)CHECK(std::abs(pitch-prior)<.05f);prior=pitch;
 }
 // The body should square toward tracked look despite the stock bladed stance.
 const auto facing=[](const BodyBones& b){return std::atan2(b[13].values[2][0],b[13].values[2][2]);};
 auto bladed=rig;bladed[0]=Yaw(.4f);auto reachPose=Sample();reachPose.left=At(-.2f,1,0);reachPose.right=At(.2f,1,0);
 const auto square=SolveRemoteTorso(bladed,reachPose);CHECK(square&&std::abs(facing(*square))<.11f);
 CHECK(Near((*square)[0],bladed[0])&&Attachments(bladed,*square));
 // Coordinated lateral reach turns the chest modestly; hands/items remain exact
 // network targets for the existing arm solve, never rotated with the torso.
 reachPose.left=At(.05f,1.3f,.4f);reachPose.right=At(.45f,1.3f,.4f);
 const auto reaching=SolveRemoteTorso(rig,reachPose);CHECK(reaching&&Attachments(rig,*reaching));
 CHECK(facing(*reaching)>.15f&&facing(*reaching)<.22f);
 std::swap(reachPose.left,reachPose.right);const auto swapped=SolveRemoteTorso(rig,reachPose);CHECK(swapped&&Near((*swapped)[13],(*reaching)[13]));
 reachPose.left.values[3][0]*=-1;reachPose.right.values[3][0]*=-1;
 const auto mirrorReach=SolveRemoteTorso(rig,reachPose);CHECK(mirrorReach&&std::abs(facing(*mirrorReach)+facing(*reaching))<.001f);
 // A wave, a holster grab, overhead hands and lost tracking must not infer a
 // whole-body turn. Finger/wrist rotations do not change reach direction.
 reachPose=Sample();reachPose.left=At(-.2f,1,0);reachPose.right=At(.45f,1.3f,.4f);
 auto noReach=SolveRemoteTorso(rig,reachPose);CHECK(noReach&&std::abs(facing(*noReach))<.001f);
 reachPose.left=At(-.2f,1.3f,-.4f);noReach=SolveRemoteTorso(rig,reachPose);CHECK(noReach&&std::abs(facing(*noReach))<.001f);
 reachPose.left=At(.05f,2.1f,.4f);reachPose.right=At(.45f,2.1f,.4f);noReach=SolveRemoteTorso(rig,reachPose);CHECK(noReach&&std::abs(facing(*noReach))<.001f);
 reachPose.left=At(.05f,1.3f,.4f);reachPose.right=At(.45f,1.3f,.4f);reachPose.flags=RightValid;
 noReach=SolveRemoteTorso(rig,reachPose);CHECK(noReach&&std::abs(facing(*noReach))<.001f);
 reachPose.flags=LeftValid|RightValid;reachPose.left=Multiply(Yaw(2.f),reachPose.left);reachPose.right=Multiply(Yaw(-2.f),reachPose.right);
 noReach=SolveRemoteTorso(rig,reachPose);CHECK(noReach&&Near((*noReach)[13],(*reaching)[13]));
 reachPose=Sample();reachPose.head=Yaw(2.f);reachPose.head.values[3]=reachPose.camera.values[3];
 noReach=SolveRemoteTorso(rig,reachPose);CHECK(noReach&&std::abs(facing(*noReach))<.781f);
 // Crossing straight up/down must not turn the chest toward a reversed forward.
 for(float pitch:{1.55f,1.59f,-1.55f,-1.59f}){
  auto h=At();h.values[1]={0,std::cos(pitch),std::sin(pitch),0};h.values[2]={0,-std::sin(pitch),std::cos(pitch),0};
  reachPose=Sample();reachPose.head=Multiply(h,Yaw(1.f));reachPose.head.values[3]=reachPose.camera.values[3];
  noReach=SolveRemoteTorso(rig,reachPose);CHECK(noReach&&std::abs(facing(*noReach))<.001f);
 }
 auto prone=rig;prone[0].values[1]={0,0,-1,0};prone[0].values[2]={0,1,0,0};CHECK(!SolveRemoteTorso(prone,p));
 auto bad=rig;bad[13].values[0][0]=std::numeric_limits<float>::quiet_NaN();CHECK(!SolveRemoteTorso(bad,p));
 p=Sample();p.right=At(0,1.3f,.02f);p.weapon=Multiply(At(0,0,.2f),p.right);
 auto constrained=ConstrainRemoteHands(rig,p);CHECK(constrained.right.values[3][2]>.16f);
 CHECK(Near(Multiply(constrained.weapon,*InverseRigid(constrained.right)),Multiply(p.weapon,*InverseRigid(p.right))));
 CHECK(Near(constrained.left,p.left));p.flags&=~WeaponHeld;p.left=p.right=At(0,1.3f,.4f);
 constrained=ConstrainRemoteHands(rig,p);CHECK(std::abs(constrained.right.values[3][0]-constrained.left.values[3][0])>.079f);
 p.flags|=WeaponHeld;constrained=ConstrainRemoteHands(rig,p);CHECK(Near(constrained.right,p.right)&&Near(constrained.left,p.left));
 const stereo::Vec3 shoulder{-.2f,1.4f,0},wrist{.2f,1.4f,0},elbow{0,1.3f,0};const auto cleared=ConstrainRemoteElbow(rig,shoulder,wrist,elbow);
 CHECK(std::abs(Length(Sub(cleared,shoulder))-Length(Sub(elbow,shoulder)))<.0001f);
 CHECK(std::abs(Length(Sub(cleared,wrist))-Length(Sub(elbow,wrist)))<.0001f);CHECK(std::abs(cleared.z)>std::abs(elbow.z));
 // This elbow is outside the capsule; its forearm crosses the torso.
 const stereo::Vec3 sideShoulder{-.2f,1.4f,.10f},crossWrist{.2f,1.3f,0},sideElbow{-.3f,1.3f,0};
 const auto segmentClear=ConstrainRemoteElbow(rig,sideShoulder,crossWrist,sideElbow);
 CHECK(Length(Sub(segmentClear,sideElbow))>.01f);
 CHECK(std::abs(Length(Sub(segmentClear,sideShoulder))-Length(Sub(sideElbow,sideShoulder)))<.0001f);
 CHECK(std::abs(Length(Sub(segmentClear,crossWrist))-Length(Sub(sideElbow,crossWrist)))<.0001f);
 FistBumpContact c;CHECK(!c.Update({.3f,0,0},true,100));CHECK(c.Update({.09f,0,0},true,116));
 for(unsigned t=132;t<450;t+=16)CHECK(!c.Update({.08f,0,0},true,t));
 CHECK(!c.Update({.3f,0,0},true,460));CHECK(c.Update({.09f,0,0},true,480));
 CHECK(!c.Update({.3f,0,0},true,496));CHECK(!c.Update({.09f,0,0},true,512)); // cooldown
 c={};CHECK(!c.Update({.01f,0,0},true,100));CHECK(!c.Update({.02f,0,0},true,116)); // spawn overlap
 c={};CHECK(!c.Update({.4f,0,0},true,100));CHECK(!c.Update({.01f,0,0},true,116)); // discontinuity
 c={};CHECK(!c.Update({.25f,0,0},true,100));CHECK(!c.Update({.09f,0,0},true,300)); // stale
 c={};CHECK(!c.Update({.25f,0,0},true,100));CHECK(c.Update({-.09f,0,0},true,116)); // swept contact
 c={};CHECK(!c.Update({.25f,0,0},true,100));CHECK(!c.Update({.09f,0,0},false,116));
 auto local=Sample(),remote=Sample();local.player=1;local.flags=remote.flags=RightValid;local.right=At();remote.right=At(.3f,0,0);remote.session=43;
 FistBumpPeer peer;CHECK(!peer.Update(local,remote,100));remote.right=At(.09f,0,0);CHECK(peer.Update(local,remote,116)==2);
 peer={};remote.right=At(.3f,0,0);CHECK(!peer.Update(local,remote,100));remote.flags|=WeaponHeld;remote.right=At(.09f,0,0);CHECK(!peer.Update(local,remote,116));
 peer={};remote.flags=RightValid;remote.right=At(.3f,0,0);CHECK(!peer.Update(local,remote,100));remote.session++;remote.right=At(.09f,0,0);CHECK(!peer.Update(local,remote,116));
 peer={};remote.kind=Mirror;remote.right=At(.3f,0,0);CHECK(!peer.Update(local,remote,100));remote.right=At(.09f,0,0);CHECK(!peer.Update(local,remote,116));
 // World-space contact handles each player's different local body coordinates.
 peer={};remote.kind=Relay;remote.body=Yaw(3.14159265f);remote.body.values[3][0]=1;remote.right=At(.7f,0,0);
 CHECK(!peer.Update(local,remote,100));remote.right=At(.91f,0,0);CHECK(peer.Update(local,remote,116)==2);
 // Optional private captured-rig replay exercises anatomy without distributing assets.
 if(argc>=2){std::ifstream input(argv[1],std::ios::binary);BodyBones real{};CHECK(input.read(reinterpret_cast<char*>(real.data()),sizeof(real)));
  BodyBones reference=real;if(argc>=3){std::ifstream ref(argv[2],std::ios::binary);CHECK(ref.read(reinterpret_cast<char*>(reference.data()),sizeof(reference)));}
  auto left=CaptureBodyPalm(real,true),right=CaptureBodyPalm(real,false);if(!left)left=CaptureBodyPalm(reference,true);if(!right)right=CaptureBodyPalm(reference,false);CHECK(left&&right);
  auto pose=Sample();pose.head=pose.camera=real[47];pose.left=Multiply(*InverseAnimatedBone(left->wristFromPalm),reference[20]);pose.right=Multiply(*InverseAnimatedBone(right->wristFromPalm),reference[35]);
  const auto body=SolveRemoteTorso(real,pose);CHECK(body);CHECK(Attachments(real,*body));
  CHECK(ChestUp(*body)>.978f);
  CHECK((*body)[47].values[3][1]>(*body)[13].values[3][1]+.12f);
  auto rifleAim=real;PitchSubtree(rifleAim,12,74,-.9f);PitchSubtree(rifleAim,46,63,.9f);
  const auto stableBody=SolveRemoteTorso(rifleAim,pose);CHECK(stableBody&&Attachments(rifleAim,*stableBody));
  for(int id:{12,13,14,15,30,31,45,46,47,72,73,74})CHECK(Near((*stableBody)[id],(*body)[id]));
  const auto contact=ConstrainRemoteHands(*body,pose);RemoteArmContinuity history;
  const auto arms=SolveRemoteArms(*body,contact,*left,*right,&reference,&history,.016f);CHECK(arms);for(const auto& m:*arms)CHECK(InverseAnimatedBone(m));
  for(int i=0;i<11;++i)CHECK(Near((*arms)[i],real[i]));for(int i=75;i<80;++i)CHECK(Near((*arms)[i],real[i]));
 }
 // Optional owned-installation bind pose: no proprietary reference is bundled.
 if(argc>=4){
  std::ifstream file(argv[3],std::ios::binary);BodyBones ownedBind{};CHECK(file.read(reinterpret_cast<char*>(ownedBind.data()),sizeof(ownedBind)));
  auto pose=Sample();pose.head=pose.camera=ownedBind[47];pose.left=At(-.2f,.2f,0);pose.right=At(.2f,.2f,0);
  const auto ownedRest=SolveRemoteTorso(ownedBind,pose);CHECK(ownedRest&&Attachments(ownedBind,*ownedRest));
  for(int id:{11,12,13,45,46,47,72,73,74})CHECK(Near((*ownedRest)[id],ownedBind[id],.001f));
  if(argc>=5){std::ofstream out(argv[4],std::ios::binary);CHECK(out.write(reinterpret_cast<const char*>(ownedRest->data()),sizeof(*ownedRest)));}
 }
 puts("Remote smoothing, reset guards, rigid item attachment, connected torso/neck independent of gun pitch, collision constraints and fist contacts passed.");return 0;
}
