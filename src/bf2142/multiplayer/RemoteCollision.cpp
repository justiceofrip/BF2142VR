#include "RemoteCollision.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142::net {
namespace {
using V=stereo::Vec3;
V Add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}V Sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}V Scale(V a,float s){return {a.x*s,a.y*s,a.z*s};}
float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}float Length(V a){return std::sqrt(Dot(a,a));}
V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
V Pos(const Matrix& a){return {a.values[3][0],a.values[3][1],a.values[3][2]};}
void Move(Matrix& m,V v){for(int i=0;i<3;++i)m.values[3][i]+=i==0?v.x:i==1?v.y:v.z;}
struct Capsule {V a,b;float radius;bool valid;};
Capsule Torso(const BodyBones& b){
 const auto a=Pos(b[11]),top=Pos(b[45]);const float height=Length(Sub(top,a)),width=Length(Sub(Pos(b[15]),Pos(b[31])));
 return {a,top,std::clamp(width*.33f,.08f,.14f),height>.15f&&height<.8f&&width>.15f&&width<.65f&&b[0].values[1][1]>.65f};
}
V Nearest(const Capsule& c,V p){const V axis=Sub(c.b,c.a);return Add(c.a,Scale(axis,std::clamp(Dot(Sub(p,c.a),axis)/Dot(axis,axis),0.f,1.f)));}
float Clearance(const Capsule& c,V shoulder,V wrist,V elbow){
 float result=Length(Sub(elbow,Nearest(c,elbow)));
 // The elbow point can be outside while an arm segment still crosses the
 // chest. Sample the interior of both segments, excluding the shoulder seam.
 for(float t:{.25f,.5f,.75f})for(bool upper:{true,false}){
  const V p=upper?Add(Scale(shoulder,1-t),Scale(elbow,t)):Add(Scale(elbow,1-t),Scale(wrist,t));
  result=std::min(result,Length(Sub(p,Nearest(c,p))));
 }
 return result;
}
V Push(const Capsule& c,V p,float skin){
 const V delta=Sub(p,Nearest(c,p));const float d=Length(delta),r=c.radius+skin;
 if(d>=r)return {};const V direction=d>.001f?Scale(delta,1/d):V{0,0,1};
 return Scale(direction,std::min(.18f,r-d));
}
}
Packet ConstrainRemoteHands(const BodyBones& b,const Packet& p) noexcept {
 const auto c=Torso(b);if(!c.valid)return p;Packet out=p;
 for(bool left:{true,false})if(p.flags&(left?LeftValid:RightValid)){
  auto& palm=left?out.left:out.right;if(!InverseRigid(palm))continue;
  const V correction=Push(c,Pos(palm),.045f);Move(palm,correction);
 }
 // Palming a pistol / supporting a rifle deliberately brings the hands close.
 if((p.flags&(LeftValid|RightValid|WeaponHeld|LeftCrateHeld))==(LeftValid|RightValid)){
  V delta=Sub(Pos(out.right),Pos(out.left));float d=Length(delta);
  if(d<.08f){const V axis=d>.001f?Scale(delta,1/d):V{1,0,0};const V move=Scale(axis,(.08f-d)*.5f);Move(out.right,move);Move(out.left,Scale(move,-1));}
 }
 const bool itemLeft=(p.flags&LeftCrateHeld)!=0;
 Move(out.weapon,Sub(Pos(itemLeft?out.left:out.right),Pos(itemLeft?p.left:p.right)));
 return out;
}
V ConstrainRemoteElbow(const BodyBones& b,V shoulder,V wrist,V elbow) noexcept {
 const auto c=Torso(b);if(!c.valid||Clearance(c,shoulder,wrist,elbow)>=c.radius+.035f)return elbow;
 V axis=Sub(wrist,shoulder);float n=Length(axis);if(n<.01f)return elbow;axis=Scale(axis,1/n);
 const V center=Add(shoulder,Scale(axis,Dot(Sub(elbow,shoulder),axis))),radial=Sub(elbow,center);
 V best=elbow;float bestClearance=Clearance(c,shoulder,wrist,elbow);
 // Rotate on the analytic elbow circle: retain both segment lengths and exact
 // wrist target. Choose the smallest clearing angle, rather than pushing a
 // bone independently and stretching/disconnecting the forearm.
 for(int step=1;step<=18;++step)for(float sign:{1.f,-1.f}){
  const float angle=sign*float(step)*.08726646f;
  const V candidate=Add(center,Add(Scale(radial,std::cos(angle)),Scale(Cross(axis,radial),std::sin(angle))));
  const float clearance=Clearance(c,shoulder,wrist,candidate);
  if(clearance>bestClearance){best=candidate;bestClearance=clearance;}
  if(clearance>=c.radius+.035f)return candidate;
 }
 return best;
}
}
