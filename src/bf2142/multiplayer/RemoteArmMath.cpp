#include "RemoteArmMath.h"
#include "../HandPoseMath.h"
#include <algorithm>
#include <cmath>
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
bool arm(BodyBones& out,const BodyBones& native,bool left,const M& target){
 const int shoulder=left?15:31,elbow=left?17:33,wrist=left?20:35;
 const auto start=pos(native[shoulder]);const auto solved=SolveArm(start,pos(target),length(sub(pos(native[elbow]),start)),length(sub(pos(native[wrist]),pos(native[elbow]))),left);
 if(!solved)return false;
 const auto inv=InverseAnimatedBone(native[wrist]);if(!inv)return false;
 for(int i=shoulder;i<elbow;++i)out[i]=limb(native[i],start,pos(native[elbow]),start,solved->elbow);
 for(int i=elbow;i<wrist;++i)out[i]=limb(native[i],pos(native[elbow]),pos(native[wrist]),solved->elbow,solved->wrist);
 auto end=target;set(end,3,solved->wrist);const auto delta=Multiply(*inv,end);
 for(int i=wrist;i<wrist+10;++i)out[i]=Multiply(native[i],delta);
 if(!left)for(int i=64;i<72;++i)out[i]=Multiply(native[i],delta); // Authored third-person item attachment.
 return true;
}
}
std::optional<Matrix> InverseAnimatedBone(const Matrix& m) noexcept {
 // Do not loosen pose validation. The native blended basis is near-rigid, but
 // transpose is not its exact inverse; repeated retargeting magnifies the drift.
 if(!InverseRigid(m))return {};
 const auto& a=m.values;
 const float det=a[0][0]*(a[1][1]*a[2][2]-a[1][2]*a[2][1])-a[0][1]*(a[1][0]*a[2][2]-a[1][2]*a[2][0])+a[0][2]*(a[1][0]*a[2][1]-a[1][1]*a[2][0]);
 if(!std::isfinite(det)||std::abs(det)<.9f)return {};
 Matrix out=identity();auto& b=out.values;
 b[0][0]=(a[1][1]*a[2][2]-a[1][2]*a[2][1])/det;
 b[0][1]=(a[0][2]*a[2][1]-a[0][1]*a[2][2])/det;
 b[0][2]=(a[0][1]*a[1][2]-a[0][2]*a[1][1])/det;
 b[1][0]=(a[1][2]*a[2][0]-a[1][0]*a[2][2])/det;
 b[1][1]=(a[0][0]*a[2][2]-a[0][2]*a[2][0])/det;
 b[1][2]=(a[0][2]*a[1][0]-a[0][0]*a[1][2])/det;
 b[2][0]=(a[1][0]*a[2][1]-a[1][1]*a[2][0])/det;
 b[2][1]=(a[0][1]*a[2][0]-a[0][0]*a[2][1])/det;
 b[2][2]=(a[0][0]*a[1][1]-a[0][1]*a[1][0])/det;
 for(int j=0;j<3;++j)for(int k=0;k<3;++k)b[3][j]-=a[3][k]*b[k][j];
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
std::optional<BodyBones> SolveRemoteArms(const BodyBones& native,const Packet& p,const PalmBinding& left,const PalmBinding& right,const BodyBones* detailedReference) noexcept {
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

 // Retain the server-driven legs/torso. Incoming hands are soldier-local poses.
 // A dropped/unreachable hand must not cancel the other hand's tracking.
 // arm() checks its target and wrist inverse before changing any output bones.
 bool applied=false;
 if(p.flags&LeftValid)applied|=arm(out,source,true,Multiply(left.wristFromPalm,p.left));
 if(p.flags&RightValid)applied|=arm(out,source,false,Multiply(right.wristFromPalm,p.right));
 if((p.flags&(LeftValid|RightValid))&&!applied)return {};
 for(const auto& m:out)if(!InverseRigid(m))return {};
 return out;
}
}
