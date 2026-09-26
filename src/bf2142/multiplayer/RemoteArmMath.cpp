#include "RemoteArmMath.h"
#include "RemoteCollision.h"
#include "stereo/ArmPoleVectorMath.h"
#include "../HandPoseMath.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace bfvr::bf2142::net {
namespace {
using V=stereo::Vec3;using M=Matrix;
V add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}V sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}V mul(V a,float s){return {a.x*s,a.y*s,a.z*s};}
float dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}float length(V a){return std::sqrt(dot(a,a));}
V cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}V unit(V a){const float n=length(a);return n>1.e-6f?mul(a,1/n):V{};}
V pos(const M& a){return {a.values[3][0],a.values[3][1],a.values[3][2]};}void set(M& m,int r,V v){m.values[r]={v.x,v.y,v.z,r==3?1.f:0.f};}
M identity(){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;return m;}
M rotation(V a,V b){
 a=unit(a);b=unit(b);float c=std::clamp(dot(a,b),-1.f,1.f);V axis=cross(a,b);float s=length(axis);
 if(s<1.e-5f){if(c>0)return identity();axis=unit(cross(a,{0,1,0}));if(length(axis)<.5f)axis=unit(cross(a,{1,0,0}));}
 else axis=mul(axis,1/s);
 const float t=1-c,x=axis.x,y=axis.y,z=axis.z;M m=identity();m.values[0]={t*x*x+c,t*x*y+s*z,t*x*z-s*y,0};m.values[1]={t*x*y-s*z,t*y*y+c,t*y*z+s*x,0};m.values[2]={t*x*z+s*y,t*y*z-s*x,t*z*z+c,0};return m;
}
M limb(const M& bone,V oldRoot,V oldEnd,V newRoot,V newEnd){
 M local=bone;set(local,3,sub(pos(bone),oldRoot));auto result=Multiply(local,rotation(sub(oldEnd,oldRoot),sub(newEnd,newRoot)));
 const float n=length(sub(oldEnd,oldRoot));if(n>.00001f)set(result,3,add(newRoot,mul(pos(result),length(sub(newEnd,newRoot))/n)));return result;
}
// Keep the calibrated anatomical arm basis independent of the remote stock
// aim/recoil animation. Endpoints alone do not cancel animated collar/arm roll.
// Use the already-validated detailed rig; retain current finger/item local poses.
bool StableArmBase(BodyBones& source,const BodyBones& reference,bool left){
 const int collar=left?14:30,elbow=left?17:33,wrist=left?20:35;
 const float upper=length(sub(pos(reference[elbow]),pos(reference[collar+1])));
 const float lower=length(sub(pos(reference[wrist]),pos(reference[elbow])));
 const auto invChest=InverseAnimatedBone(reference[13]),invWrist=InverseAnimatedBone(source[wrist]);
 if(!invChest||!invWrist||upper<.08f||upper>.6f||lower<.08f||lower>.6f)return false;
 const auto delta=Multiply(*invChest,source[13]);BodyBones stable=source;
 for(int i=collar;i<=wrist;++i){stable[i]=Multiply(reference[i],delta);if(!InverseAnimatedBone(stable[i]))return false;}
 // Cancel source wrist recoil from fingers and the item without changing their
 // authored local attachment or the network palm/weapon targets.
 const auto wristDelta=Multiply(*invWrist,stable[wrist]);
 for(int i=wrist+1;i<wrist+10;++i)stable[i]=Multiply(source[i],wristDelta);
 if(!left)for(int i=64;i<72;++i)stable[i]=Multiply(source[i],wristDelta);
 source=stable;return true;
}
bool arm(BodyBones& out,const BodyBones& native,bool left,const M& target,RemoteArmContinuity* continuity,float elapsed){
 const int shoulder=left?15:31,elbow=left?17:33,wrist=left?20:35;
 const auto start=pos(native[shoulder]);auto solved=SolveArm(start,pos(target),length(sub(pos(native[elbow]),start)),length(sub(pos(native[wrist]),pos(native[elbow]))),left);
 if(!solved)return false;
 if(continuity){
  const unsigned side=left?0:1;const auto old=continuity->poles[side];
  stereo::ArmPoleVectorInput pi;pi.shoulder={start.x,start.y,start.z};const auto end=pos(target);pi.handTarget={end.x,end.y,end.z};pi.leftArm=left;
  pi.previousPole={old.x,old.y,old.z};pi.hasPreviousPole=continuity->valid[side];pi.maximumAngularStepRadians=std::clamp(elapsed*6.f,.001f,.6f);
  const auto pole=stereo::ComputeArmPoleVector(pi);
  if(pole){
   const V direction=unit(sub(end,start)),original=sub(solved->elbow,start);
   const float along=dot(original,direction),bend=length(sub(original,mul(direction,along)));
   V intent{pole->pole[0],pole->pole[1],pole->pole[2]};intent=unit(sub(intent,mul(direction,dot(intent,direction))));
   if(length(intent)>.9f)solved->elbow=add(start,add(mul(direction,along),mul(intent,bend)));
   solved->elbow=ConstrainRemoteElbow(native,start,end,solved->elbow);
   continuity->poles[side]=unit(sub(sub(solved->elbow,start),mul(direction,along)));continuity->valid[side]=true;
  }
 }
 const auto inv=InverseAnimatedBone(native[wrist]);if(!inv)return false;
 for(int i=shoulder;i<elbow;++i)out[i]=limb(native[i],start,pos(native[elbow]),start,solved->elbow);
 for(int i=elbow;i<wrist;++i)out[i]=limb(native[i],pos(native[elbow]),pos(native[wrist]),solved->elbow,solved->wrist);
 auto end=target;set(end,3,solved->wrist);const auto delta=Multiply(*inv,end);
 for(int i=wrist;i<wrist+10;++i)out[i]=Multiply(native[i],delta);
 if(!left)for(int i=64;i<72;++i)out[i]=Multiply(native[i],delta); // Authored third-person item attachment.
 return true;
}
}
std::optional<Matrix> InverseAnimatedBone(const Matrix& m) noexcept {return InverseAnimatedTransform(m);}
std::optional<BodyBones> SolveRemoteHead(const BodyBones& native,const Matrix& trackedHead) noexcept {
 if(!InverseRigid(trackedHead))return {};
 const auto inverse=InverseAnimatedBone(native[47]);if(!inverse)return {};
 // The supported 3p_setup head's neutral basis is +X right, +Y up, +Z forward.
 // Keep native neck/root translation: positional body IK is a separate problem.
 Matrix target=trackedHead;target.values[3]=native[47].values[3];
 const auto delta=Multiply(*inverse,target);BodyBones out=native;
 for(int i=47;i<64;++i){
  if(!InverseAnimatedBone(native[i]))return {};
  out[i]=Multiply(native[i],delta);if(!InverseRigid(out[i]))return {};
 }
 return out;
}
std::optional<PalmBinding> CaptureBodyPalm(const BodyBones& b,bool left) noexcept {
 const int wrist=left?20:35,index=left?24:39,ring=left?21:36;
 if(!InverseRigid(b[wrist])||!InverseRigid(b[index])||!InverseRigid(b[ring]))return {};
 const auto across=sub(pos(b[index]),pos(b[ring])),along=sub(mul(add(pos(b[index]),pos(b[ring])),.5f),pos(b[wrist]));
 if(length(across)<.01f||length(across)>.15f||length(along)<.025f||length(along)>.2f)return {};
 const V z=unit(across),x=unit(cross(mul(along,-1),z)),y=cross(z,x);if(length(x)<.9f)return {};
 auto palm=identity();set(palm,0,x);set(palm,1,y);set(palm,2,z);set(palm,3,add(add(pos(b[wrist]),mul(along,.9f)),mul(x,left?.024f:-.024f)));
 const auto inverse=InverseRigid(palm);if(!inverse)return {};return PalmBinding{Multiply(b[wrist],*inverse)};
}
bool PoseRemoteFingers(BodyBones& bones,bool left,const Matrix& palm,const std::array<float,5>& curls,bool holding) noexcept {
 if(!InverseRigid(palm))return false;
 for(float v:curls)if(!std::isfinite(v)||v<0||v>1)return false;
 const int wrist=left?20:35;
 const V inward=mul({palm.values[0][0],palm.values[0][1],palm.values[0][2]},left?1.f:-1.f);
 const std::array<float,3> grouped={holding?1.f:(curls[2]+curls[3]+curls[4])/3.f,curls[1],curls[0]};
 BodyBones out=bones;
 for(int group=0;group<3;++group){
  const int base=wrist+1+group*3;const float curl=grouped[group];
  for(int j=0;j<3;++j)if(!InverseAnimatedBone(bones[base+j]))return false;
  V along=sub(pos(bones[base]),pos(bones[wrist]));
  if(group!=2)along=sub(along,mul(inward,dot(along,inward)));
  along=unit(along);if(length(along)<.9f)return false;
  V position=pos(bones[base]);
  for(int joint=0;joint<2;++joint){
   const int id=base+joint;const V old=sub(pos(bones[id+1]),pos(bones[id]));const float n=length(old);
   if(!std::isfinite(n)||n<.002f||n>.10f)return false;
   V desired;
   // Held contact and thumbs interpolate to the game's authored grip. Empty
   // index/grouped fingers curl inward from the tracked palm, independent of
   // the selected weapon's native finger animation.
   if(holding||group==2)desired=unit(add(mul(along,1-curl),mul(unit(old),curl)));
   else {const float angle=curl*(joint?2.65f:1.15f);desired=add(mul(along,std::cos(angle)),mul(inward,std::sin(angle)));}
   if(length(desired)<.9f)return false;
   out[id]=Multiply(bones[id],rotation(old,desired));set(out[id],3,position);
   position=add(position,mul(desired,n));
  }
  const auto inverse=InverseAnimatedBone(bones[base+1]);if(!inverse)return false;
  out[base+2]=Multiply(bones[base+2],Multiply(*inverse,out[base+1]));set(out[base+2],3,position);
  for(int j=0;j<3;++j)if(!InverseRigid(out[base+j]))return false;
 }
 bones=out;return true;
}
std::optional<BodyBones> SolveRemoteArms(const BodyBones& native,const Packet& p,const PalmBinding& left,const PalmBinding& right,const BodyBones* detailedReference,RemoteArmContinuity* continuity,float elapsedSeconds) noexcept {
 for(const auto& m:native)if(!InverseRigid(m))return {};
 if(!InverseRigid(left.wristFromPalm)||!InverseRigid(right.wristFromPalm))return {};
 BodyBones source=native,out=native;
 if(detailedReference){
  // BF2142's remote animation LOD can collapse the left wrist onto its elbow,
  // and finger joints onto the wrists. Reconstruct only those collapsed joints
  // from the same verified detailed rig; retain this actor's shoulders/torso.
  for(bool isLeft:{true,false}){
   const int elbow=isLeft?17:33,wrist=isLeft?20:35,index=isLeft?24:39,ring=isLeft?21:36;
   if(length(sub(pos(source[wrist]),pos(source[elbow])))<.05f){
    const auto inverse=InverseAnimatedBone((*detailedReference)[elbow]);if(!inverse)return {};
    const auto delta=Multiply(*inverse,source[elbow]);
    for(int i=elbow+1;i<wrist+10;++i)source[i]=Multiply((*detailedReference)[i],delta);
   }
   if(length(sub(pos(source[index]),pos(source[ring])))<.01f){
    const auto inverse=InverseAnimatedBone((*detailedReference)[wrist]);if(!inverse)return {};
    const auto delta=Multiply(*inverse,source[wrist]);
    for(int i=wrist+1;i<wrist+10;++i)source[i]=Multiply((*detailedReference)[i],delta);
   }
  }
 }

 // This basis stabilization is limited to actual tracked peers with a detailed
 // reference. Diagnostic mirrors and actors without that reference keep the
 // previous fallback. The accepted torso solve and local 1P arms are unchanged.
 const bool stabilize=p.kind==Relay&&continuity&&detailedReference&&
  native[0].values[1][1]>.65f&&length(sub(pos(native[13]),pos(native[11])))>.12f;
 bool stableLeft=false,stableRight=false;
 if(stabilize){
  if(p.flags&LeftValid)stableLeft=StableArmBase(source,*detailedReference,true);
  if(p.flags&RightValid)stableRight=StableArmBase(source,*detailedReference,false);
 }
 // Retain the server-driven legs/torso. Incoming hands are soldier-local poses.
 // A dropped/unreachable hand must not cancel the other hand's tracking.
 // arm() checks its target and wrist inverse before changing any output bones.
 const bool leftApplied=(p.flags&LeftValid)&&arm(out,source,true,Multiply(left.wristFromPalm,p.left),continuity,elapsedSeconds);
 const bool rightApplied=(p.flags&RightValid)&&arm(out,source,false,Multiply(right.wristFromPalm,p.right),continuity,elapsedSeconds);
 if((p.flags&(LeftValid|RightValid))&&!leftApplied&&!rightApplied)return {};
 if(leftApplied&&stableLeft)out[14]=source[14];
 if(rightApplied&&stableRight)out[30]=source[30];
 // Stock knife/rifle/crate meshes share the mesh1 origin used by the sender.
 // Keep pistol and unknown/modded weapon bindings unchanged. A diagnostic
 // bot mirror may hold another weapon and must keep its authored attachment.
 const bool trackedItem=p.kind!=Mirror&&(
  !std::strcmp(p.weaponName.data(),"knife")||!std::strcmp(p.weaponName.data(),"knife_unlock")||
  !std::strcmp(p.weaponName.data(),"eu_ar_rifle")||!std::strcmp(p.weaponName.data(),"as_ar_rifle")||
  !std::strcmp(p.weaponName.data(),"unl_hub_medic")||!std::strcmp(p.weaponName.data(),"unl_hub_ammo"));
 const bool itemHand=(p.flags&LeftCrateHeld)?leftApplied:(p.flags&WeaponHeld)&&rightApplied;
 if(trackedItem&&itemHand&&InverseRigid(p.weapon)){
  const auto inverse=InverseAnimatedBone(native[64]);if(inverse){
   const auto delta=Multiply(*inverse,p.weapon);BodyBones items=out;bool valid=true;
   for(int i=64;i<72;++i){items[i]=Multiply(native[i],delta);valid=valid&&InverseRigid(items[i]).has_value();}
   if(valid)for(int i=64;i<72;++i)out[i]=items[i];
  }
 }
 if(leftApplied){std::array<float,5> curls;std::copy_n(p.curls.begin(),5,curls.begin());
  (void)PoseRemoteFingers(out,true,p.left,curls,(p.flags&LeftCrateHeld)!=0);}
 if(rightApplied){std::array<float,5> curls;std::copy_n(p.curls.begin()+5,5,curls.begin());
  (void)PoseRemoteFingers(out,false,p.right,curls,(p.flags&WeaponHeld)!=0);}
 for(const auto& m:out)if(!InverseRigid(m))return {};
 return out;
}
}
