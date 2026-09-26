#include "RemotePresentation.h"
#include <DirectXMath.h>
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142::net {
namespace {
using namespace DirectX;
Matrix Blend(const Matrix& a,const Matrix& b,float t){
 XMFLOAT4X4 av{},bv{},result{};
 for(int i=0;i<4;++i)for(int j=0;j<4;++j){av.m[i][j]=a.values[i][j];bv.m[i][j]=b.values[i][j];}
 const auto qa=XMQuaternionNormalize(XMQuaternionRotationMatrix(XMLoadFloat4x4(&av)));
 const auto qb=XMQuaternionNormalize(XMQuaternionRotationMatrix(XMLoadFloat4x4(&bv)));
 XMStoreFloat4x4(&result,XMMatrixRotationQuaternion(XMQuaternionSlerp(qa,qb,t)));
 Matrix out;for(int i=0;i<4;++i)for(int j=0;j<4;++j)out.values[i][j]=result.m[i][j];
 for(int j=0;j<3;++j)out.values[3][j]=a.values[3][j]+(b.values[3][j]-a.values[3][j])*t;
 return out;
}
// Calibration of the verified stock 80-bone rig, in radians. Its neutral
// skinning frames are not identity: zeroing the chest axis tips the visible
// armor backward. Keep these small bind-frame pitches separate from physical
// lean/look, which are anatomical frames. No mesh or animation is replaced.
constexpr float WaistBind=-.08204f,SpineBind=.06609f,ChestBind=.15091f,NeckBaseBind=.202996f;
Matrix Pitch(float angle){
 Matrix m{};m.values[0]={1,0,0,0};m.values[1]={0,std::cos(angle),std::sin(angle),0};
 m.values[2]={0,-std::sin(angle),std::cos(angle),0};m.values[3]={0,0,0,1};return m;
}
float Gap(const Matrix& a,const Matrix& b){float n=0;for(int j=0;j<3;++j){float d=a.values[3][j]-b.values[3][j];n+=d*d;}return std::sqrt(n);}
float Yaw(const Matrix& m){return std::atan2(m.values[2][0],m.values[2][2]);}
float Wrap(float a){return std::remainder(a,6.283185307f);}
float Ramp(float value,float low,float high){const float t=std::clamp((value-low)/(high-low),0.f,1.f);return t*t*(3-2*t);}
float TorsoHeading(float hips,const Packet& p){
 // Native hips keep locomotion, but their bladed gun stance should not dominate
 // a tracked chest. Flatten look with reduced confidence near vertical, where
 // forward-vector yaw can flip by 180 degrees while the user simply looks up.
 const float horizontal=std::hypot(p.head.values[2][0],p.head.values[2][2]);
 const float lookConfidence=Ramp(horizontal,.1f,.4f);
 const float lookDelta=Wrap(Yaw(p.head)-hips)*lookConfidence;
 const float look=hips+lookDelta;float reach=0;
 if((p.flags&(LeftValid|RightValid))==(LeftValid|RightValid)){
  const float x=(p.left.values[3][0]+p.right.values[3][0])*.5f-p.head.values[3][0];
  const float z=(p.left.values[3][2]+p.right.values[3][2])*.5f-p.head.values[3][2];
  float confidence=lookConfidence;
  for(const auto* hand:{&p.left,&p.right}){
   const float dx=hand->values[3][0]-p.head.values[3][0],dz=hand->values[3][2]-p.head.values[3][2];
   const float dy=hand->values[3][1]-p.head.values[3][1];
   // Both hands must reach ahead at torso/head height. Ignore unilateral waves,
   // lowered hands and behind-the-back holster grabs; no palm-axis inference.
   confidence*=Ramp(dx*std::sin(look)+dz*std::cos(look),.08f,.28f)*
       Ramp(dy,-.60f,-.35f)*(1-Ramp(dy,.20f,.50f));
  }
  if(std::hypot(x,z)>.08f)reach=std::clamp(Wrap(std::atan2(x,z)-look),-.6f,.6f)*.35f*confidence;
 }
 return hips+std::clamp(lookDelta*.75f+reach,-.78f,.78f);
}
bool Usable(const Packet& p){return p.kind==Relay&&InverseRigid(p.body)&&InverseRigid(p.camera)&&InverseRigid(p.head)&&
 (!(p.flags&LeftValid)||InverseRigid(p.left))&&(!(p.flags&RightValid)||InverseRigid(p.right))&&InverseRigid(p.weapon);}
}
Packet RemotePresentation::Update(const Packet& p,std::uint64_t now) noexcept {
 reset=!time||now<time||now-time>150||!Usable(p)||!Usable(previous)||p.session!=previous.session||p.player!=previous.player||
  p.weaponName!=previous.weaponName||((p.flags^previous.flags)&(LeftValid|RightValid|WeaponHeld|LeftCrateHeld))||
  (p.snapSerial&&p.snapSerial!=previous.snapSerial)||Gap(p.body,previous.body)>1.5f||
  std::abs(Wrap(Yaw(p.body)-Yaw(previous.body)))>.7f||Gap(p.head,previous.head)>.65f||
  ((p.flags&LeftValid)&&Gap(p.left,previous.left)>.75f)||((p.flags&RightValid)&&Gap(p.right,previous.right)>.75f);
 Packet out=p;
 if(!reset){
  const float t=1-std::exp(-float(now-time)/40.f);
  out.head=Blend(displayed.head,p.head,t);out.camera=Blend(displayed.camera,p.camera,t);
  if(p.flags&LeftValid)out.left=Blend(displayed.left,p.left,t);
  if(p.flags&RightValid)out.right=Blend(displayed.right,p.right,t);
  for(unsigned i=0;i<out.curls.size();++i)out.curls[i]=displayed.curls[i]+(p.curls[i]-displayed.curls[i])*t;
  // Preserve this sample's exact item-to-palm attachment: independent smoothing
  // makes the gun slide out of the gripping hand during turns.
  const bool left=(p.flags&LeftCrateHeld)!=0;
  const auto inverse=InverseRigid(left?p.left:p.right);
  if(inverse)out.weapon=Multiply(Multiply(p.weapon,*inverse),left?out.left:out.right);
 }
 time=now;previous=p;displayed=out;return out;
}
std::optional<BodyBones> SolveRemoteTorso(const BodyBones& native,const Packet& p) noexcept {
 if(!Usable(p)||!(p.flags&(LeftValid|RightValid)))return {};
 for(const auto& m:native)if(!InverseAnimatedBone(m))return {};
 // Native stance remains authoritative, including lying prone and ragdolls.
 if(native[0].values[1][1]<.65f||Gap(native[11],native[13])<.12f||Gap(native[11],native[13])>.65f)return {};
 const float hips=Yaw(native[0]);
 const float heading=TorsoHeading(hips,p);
 // The stock gun-aim animation pitches the chest and counter-rotates the neck.
 // Yaw-only correction leaves that pose underneath tracked hands/head. Derive
 // an upright chest from body heading and bounded physical lean instead.
 XMVECTOR up=XMVectorSet((p.head.values[3][0]-p.camera.values[3][0])*.6f,0,
                        (p.head.values[3][2]-p.camera.values[3][2])*.6f,0);
 const float lean=XMVectorGetX(XMVector3Length(up));
 if(lean>.20f)up=XMVectorScale(up,.20f/lean);
 up=XMVector3Normalize(XMVectorAdd(up,XMVectorSet(0,1,0,0)));
 const auto right=XMVector3Normalize(XMVector3Cross(up,XMVectorSet(std::sin(heading),0,std::cos(heading),0)));
 const auto forward=XMVector3Cross(right,up);
 XMFLOAT4X4 basis{};XMStoreFloat4x4(&basis,XMMATRIX(right,up,forward,XMVectorSet(0,0,0,1)));
 Matrix chest;for(int i=0;i<4;++i)for(int j=0;j<4;++j)chest.values[i][j]=basis.m[i][j];
 BodyBones out=native;
 // Rotate each joint about its own attachment, then carry its entire subtree.
 // Different weights about one common hip pivot stretched the spine and pulled
 // the chest/neck into one another. No independent positional offsets here.
 const auto joint=[&](int root,int last,Matrix target){
  const auto inverse=InverseAnimatedBone(out[root]);if(!inverse)return false;
  target.values[3]=out[root].values[3];const auto delta=Multiply(*inverse,target);
  for(int i=root;i<=last;++i){out[i]=Multiply(out[i],delta);if(!InverseAnimatedBone(out[i]))return false;}
  return true;
 };
 // The lowest spine was still carrying the stock aiming arch after the chest
 // became upright. Straighten that joint in standing poses, about its unchanged
 // hip attachment. Keep its yaw (and the accepted upper-body facing policy).
 // Fade out for strongly tilted native hips so crouch/prone retain their stance.
 const float standing=Ramp(native[0].values[1][1],.92f,.98f);
 if(standing>0){
  const float waistYaw=Yaw(native[11]);
  const auto waistUp=XMVector3Normalize(XMVectorAdd(up,XMVectorSet(0,1,0,0)));
  const auto waistRight=XMVector3Normalize(XMVector3Cross(waistUp,XMVectorSet(std::sin(waistYaw),0,std::cos(waistYaw),0)));
  XMFLOAT4X4 waistBasis{};XMStoreFloat4x4(&waistBasis,XMMATRIX(waistRight,waistUp,XMVector3Cross(waistRight,waistUp),XMVectorSet(0,0,0,1)));
  Matrix waist;for(int i=0;i<4;++i)for(int j=0;j<4;++j)waist.values[i][j]=waistBasis.m[i][j];
  if(!joint(11,74,Blend(native[11],Multiply(Pitch(WaistBind),waist),standing)))return {};
 }
 // Continue through the corrected lower spine without reintroducing its native
 // aim pitch. All branch offsets stay attached; pelvis and legs 0..10 stay native.
 // Remove the waist's neutral bone pitch before blending anatomical frames;
 // restore each joint's own bind pitch only after that blend. Otherwise an
 // upright bone basis is mistaken for an upright model (notably the chest).
 const auto lower=Multiply(Pitch(-WaistBind),out[11]);
 if(!joint(12,74,Multiply(Pitch(SpineBind),Blend(lower,chest,.6f)))||
    !joint(13,72,Multiply(Pitch(ChestBind),chest))||
    !joint(45,63,Multiply(Pitch(NeckBaseBind),Blend(chest,p.head,.15f)))||
    !joint(46,63,Blend(chest,p.head,.35f)))return {};
 return out;
}
}
