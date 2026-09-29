#include "NativeFramePacing.h"
#include "RenderTimeBudget.h"
#include "RenderRequestWait.h"
#include <vector>
#include <array>
#include <cstdio>
using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Frame pacing line %d\n",__LINE__);return 1;}}while(0)
template<class T>void Put(BYTE* p,size_t at,T v){std::memcpy(p+at,&v,sizeof(v));}
int main(){
 // Each outer engine iteration publishes one animation timestep. Waiting
 // for ownership and then a pose must not create discarded engine iterations.
 for(std::uint64_t interval:{7u,11u,14u,22u,50u,90u}){
  std::uint64_t now=0;unsigned engineFrames=0,drawFrames=0,waits=0;
  for(unsigned frame=0;frame<60;++frame){
   ++engineFrames;const auto release=now+interval/2,pose=now+interval;
   CHECK(AwaitRenderRequest([&]{if(now<release||now<pose)return false;++drawFrames;return true;},
       []{return true;},[&]{return now;},[&]{++now;++waits;}));
  }
  CHECK(engineFrames==60&&drawFrames==60&&now==60*interval&&waits==60*interval);
 }
 {
  std::uint64_t now=0;unsigned waits=0,attempts=0;
  CHECK(AwaitRenderRequest([&]{++attempts;return true;},[]{return true;},[&]{return now;},[&]{++waits;}));
  CHECK(attempts==1&&waits==0);
  CHECK(!AwaitRenderRequest([]{return false;},[]{return true;},[&]{return now;},[&]{++now;++waits;}));
  CHECK(now==100&&waits==100); // suspended or dead consumer is bounded
  CHECK(!AwaitRenderRequest([]{return false;},[]{return false;},[&]{return now;},[&]{++waits;}));
  CHECK(waits==100);
  CHECK(AwaitRenderRequest([]{return true;},[]{return true;},[&]{return now;},[&]{++waits;})); // recovery
  CHECK(!AwaitRenderRequest([]{return false;},[]{return true;},[&]{return now--;},[&]{++waits;}));
  CHECK(waits==100); // discontinuous clock cannot extend a wait
 }
 RenderTimeBudget time;double advanced=0;
 for(int i=1;i<=1000;++i){if(i%14)time.Skip(.001);else advanced+=time.Take(.001);}advanced+=time.Take(0);
 CHECK(std::abs(advanced-1)<1.e-9);CHECK(time.Take(0)==0);
 time.Skip(.1);time.Reset();CHECK(time.Take(.01)==.01);
 for(int i=0;i<1000;++i)time.Skip(.001);CHECK(time.Take(.001)==.25);
 time.Skip(.01);CHECK(time.Take(1000.016667)==1000.016667);CHECK(time.Take(0)==0);

 std::vector<BYTE> image(0x680000);auto* g=image.data();std::array<BYTE,256> object{};
 const BYTE getter[]={0x8b,0x81,0x9c,0,0,0,0xc3};const BYTE setter[]={0x55,0x8b,0xec,0x8b,0x45,8,0x85,0xc0,0x56,0x8b,0xf1,0x7e,0x0b,0x89,0x86,0x9c,0,0,0,0x5e,0x5d,0xc2,4,0,0xe8};const BYTE suffix[]={0x84,0xc0,0x75,0x0a,0xc7,0x86,0x9c,0,0,0,0,0,0,0,0x5e,0x5d,0xc2,4,0};
 std::memcpy(g+0x1afc00,getter,sizeof(getter));std::memcpy(g+0x3030,setter,sizeof(setter));std::memcpy(g+0x304d,suffix,sizeof(suffix));std::memcpy(g+0x519464,"lockFps",8);
 Put(g,0x3049,LONG(0x2b800-0x304d));Put(g,0x243c2,g+0x519464);Put(g,0x5193e0+0x5c,g+0x2447b);Put(g,0x24492,g+0x60f3b0);Put(g,0x244b4,g+0x60f3b0);g[0x244b9]=0xe8;Put(g,0x244ba,LONG(0x3030-0x244be));Put(g,0x60f3b0,object.data());
 for(int cap:{0,40,72,100,1000}){Put(object.data(),0x9c,cap);CHECK(ResolveFrameLimiter(g)==object.data());CHECK(*reinterpret_cast<int*>(object.data()+0x9c)==cap);}
 for(int cap:{-1,1001}){Put(object.data(),0x9c,cap);CHECK(!ResolveFrameLimiter(g));}Put(object.data(),0x9c,100);
 for(unsigned at:{0x3030u,0x3049u,0x304du,0x1afc00u,0x519464u,0x243c2u,0x24492u,0x244b4u,0x244b9u,0x244bau,0x51943cu}){g[at]^=0xff;CHECK(!ResolveFrameLimiter(g));g[at]^=0xff;}
 Put(g,0x60f3b0,static_cast<void*>(nullptr));CHECK(!ResolveFrameLimiter(g));CHECK(!ResolveFrameLimiter(nullptr));
 puts("Native FPS profile: getter, setter restrictions, command linkage, valid cap and unknown-profile rejection passed.");
}
