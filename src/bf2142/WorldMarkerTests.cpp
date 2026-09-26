#include "NativeWorldMarkers.cpp"
#include "TrackingMath.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
using namespace bfvr;using namespace bfvr::bf2142;
namespace bfvr::bf2142 {
EyeCamera testHead{},testEye{};bool testActive=true,hideMarkers=false;
bool HideStereoWorldMarkers(){return hideMarkers;}
bool ReadStereoMarkerFrame(EyeCamera* head,EyeCamera* eye){if(!testActive)return false;*head=testHead;*eye=testEye;return true;}
}
#define CHECK(x) do{if(!(x)){printf("World marker failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
using M=stereo::Matrix4;
M Identity(){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;return m;}
EyeCamera Camera(float offset,stereo::FovTangents fov){EyeCamera c{};c.world=Identity();c.world.values[3][0]=offset;c.projection=*stereo::MakeD3D8ProjectionFromFovTangents(fov,.05f,2000);return c;}
bool Near(float a,float b,float epsilon=.0001f){return std::abs(a-b)<epsilon;}
stereo::Vec3 AtDepth(const WorldMarkerPoint& p,const EyeCamera& c,float depth){
 const float x=(p.x-c.projection.values[2][0])*depth/c.projection.values[0][0];
 const float y=(p.y-c.projection.values[2][1])*depth/c.projection.values[1][1];
 const auto q=stereo::TransformRowVector({x,y,depth,1},c.world);return {q.x,q.y,q.z};
}
unsigned originalCalls=0;NativeMarkerPoint dummy{};bool differentReturn=false;
NativeMarkerPoint* __fastcall Original(void*,void*,NativeMarkerPoint* out,stereo::Vec3,void* tag){
 ++originalCalls;*out={.65f,.1f,0,1};if(tag)static_cast<BYTE*>(tag)[0xc0]=1;return differentReturn?&dummy:out;
}
}
int main(){
 const auto head=Camera(0,{-1.0f,1.0f,.95f,-1.05f});
 const auto left=Camera(-.032f,{-1.45f,1.0f,.95f,-1.05f});
 const auto right=Camera(.032f,{-1.0f,1.45f,.95f,-1.05f});
 // Equal NDC edge clamps point in different directions on asymmetric eyes.
 WorldMarkerPoint flat{.65f,0,0,1,true};const auto fl=AtDepth(flat,left,100),fr=AtDepth(flat,right,100);
 CHECK(std::abs(fl.x-fr.x)>30.f);
 unsigned cases=0;
 for(float x:{-2000.f,-100.f,0.f,100.f,2000.f})for(float y:{-1200.f,0.f,1200.f})for(float z:{-100.f,0.f,100.f}){
  const auto a=ProjectWorldMarker({x,y,z},head,left),b=ProjectWorldMarker({x,y,z},head,right);
  CHECK(a&&b&&a->edge==b->edge);
  if(a->edge){const auto p=AtDepth(*a,left,100),q=AtDepth(*b,right,100);CHECK(Near(p.x,q.x,.001f)&&Near(p.y,q.y,.001f));CHECK(a->z==0&&a->distance==1);}
  else {CHECK(z>0);const auto p=AtDepth(*a,left,z),q=AtDepth(*b,right,z);CHECK(Near(p.x,x,.001f)&&Near(p.y,y,.001f)&&Near(q.x,x,.001f));CHECK(a->z==1);}
  ++cases;
 }
 // A marker in the common visible region must not become an edge arrow in
 // just one eye due to that eye's asymmetric texture center.
 for(float x:{-.63f,-.3f,0.f,.3f,.63f}){
  const auto a=ProjectWorldMarker({x*100,0,100},head,left),b=ProjectWorldMarker({x*100,0,100},head,right);
  CHECK(a&&b&&!a->edge&&!b->edge);
 }
 const float nan=std::numeric_limits<float>::quiet_NaN();
 CHECK(!ProjectWorldMarker({nan,0,1},head,left));CHECK(!ProjectWorldMarker({1,2,3},head,left,1));
 auto bad=left;bad.world.values[0][0]=3;CHECK(!ProjectWorldMarker({1,2,3},head,bad));
 bad=left;bad.projection.values[2][3]=0;CHECK(!ProjectWorldMarker({1,2,3},head,bad));
 // Rigidly translating/rotating the whole test rig cannot change projection.
 const float angle=.7f;M transform=Identity();transform.values[0]={std::cos(angle),std::sin(angle),0,0};
 transform.values[1]={-std::sin(angle),std::cos(angle),0,0};transform.values[3]={500,150,-900,1};
 auto h2=head,l2=left;h2.world=Multiply(head.world,transform);l2.world=Multiply(left.world,transform);
 const auto w=stereo::TransformRowVector({900,600,-500,1},transform);
 const auto plain=ProjectWorldMarker({900,600,-500},head,left),rotated=ProjectWorldMarker({w.x,w.y,w.z},h2,l2);
 CHECK(plain&&rotated&&Near(plain->x,rotated->x,.001f)&&Near(plain->y,rotated->y,.001f));
 // Native wrapper preserves its return pointer and side effects, and is a
 // no-op for non-world labels, unglued labels, and non-stereo/scope contexts.
 originalProject=reinterpret_cast<Project>(&Original);testHead=head;testEye=left;
 std::array<BYTE,0xd0> tag{};int type=3;unsigned flags=0x20;
 std::memcpy(tag.data()+0xc,&type,4);std::memcpy(tag.data()+0xb8,&flags,4);
 NativeMarkerPoint out{};CHECK(ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data())==&out);CHECK(!Near(out.x,.65f));
 testActive=false;ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data());CHECK(out.x==.65f);
 testActive=true;tag[0xc]=0;ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data());CHECK(out.x==.65f);
 tag[0xc]=3;tag[0xb8]=0;ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data());CHECK(out.x==.65f);
 tag[0xb8]=0x20;differentReturn=true;CHECK(ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data())==&dummy);CHECK(out.x==.65f);
 CHECK(originalCalls==5);CHECK(!Profile(nullptr));
 // The hidden sentinel covers both icon positions even outside ordinary eye
 // replay (including scopes). Names/unclamped labels retain native results.
 differentReturn=false;hideMarkers=true;testActive=false;
 for(int i=0;i<2;++i){ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data());CHECK(out.x==0&&out.y==0&&out.z==-1&&out.distance==0&&tag[0xc0]==0);}
 for(BYTE otherType:{BYTE(0),BYTE(1),BYTE(2)}){tag[0xc]=otherType;ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data());CHECK(out.x==.65f&&out.z==0);}
 tag[0xc]=3;tag[0xb8]=0;ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data());CHECK(out.x==.65f);
 tag[0xb8]=0x20;differentReturn=true;CHECK(ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data())==&dummy);CHECK(out.x==.65f);
 differentReturn=false;hideMarkers=false;ProjectHook(nullptr,nullptr,&out,{900,0,100},tag.data());CHECK(out.x==.65f);
 printf("%u asymmetric-eye marker cases, common edge direction, rigid-frame invariance and native fallback guards passed.\n",cases);
 return 0;
}
