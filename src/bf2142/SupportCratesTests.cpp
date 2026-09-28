#include "SupportCrates.h"
#include <cstdio>
#include <cstring>
using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Support test failed %d: %s\\n",__LINE__,#x);return 1;}}while(0)
int main(){
 SupportCrates policy;SupportObservation s;s.active=s.leftCrates=s.leftTracked=true;s.owner=7;s.time=1000000000;s.equipped=3;
 strcpy_s(s.names[3].data(),49,"eu_ar_rifle");strcpy_s(s.names[4].data(),49,"unl_hub_medic");s.ammo[4]={true,1,1};
 s.left.position=BodySlots()[3].offset;const auto next=[&](){s.time+=20000000;return policy.Update(s);};next();CHECK(next().hovered==4);
 s.leftGrip=true;auto f=next();CHECK(f.busy&&f.select==4&&!f.leftCrate);s.equipped=4;CHECK(next().leftCrate);
 for(int i=0;i<5;++i){s.left.position.z-=.06f;next();}s.leftGrip=false;s.left.position.z-=.06f;
 f=next();CHECK(f.fire&&f.throwNow&&f.throwVelocity.z<-1);CHECK(!policy.Update(s).throwNow);
 s.ammo[4]={true,0,0};f=next();CHECK(f.select==0&&f.holster&&f.unavailable[4]);s.equipped=3;for(int i=0;i<65;++i)next();
 s.left.position=BodySlots()[3].offset;s.leftGrip=true;auto unavailable=next();CHECK(!unavailable.busy&&unavailable.hovered<0);
 s.ammo[4].deployable=1;s.leftGrip=false;next();s.leftGrip=true;CHECK(next().select==4);
 s.time+=500000000;f=next();CHECK(!f.busy&&!f.fire&&!f.throwNow);
 s.leftGrip=false;next();s.leftGrip=true;CHECK(next().busy);s.leftTracked=false;CHECK(!next().busy);
 s.leftTracked=true;CHECK(!next().busy);s.leftGrip=false;next();s.leftGrip=true;CHECK(next().busy);
 s.owner=8;CHECK(!next().busy);s.active=false;CHECK(!next().fire);
 // Release before native equip completes: queue a gentle toss, then leave empty.
 policy.Reset();s.active=true;s.leftTracked=true;s.leftGrip=false;s.equipped=3;s.ammo[4]={true,0,1};s.left.position=BodySlots()[3].offset;next();next();
 s.leftGrip=true;CHECK(next().select==4);s.leftGrip=false;f=next();CHECK(f.select==4&&!f.throwNow);
 s.equipped=4;CHECK(next().leftCrate);s.ammo[4].rounds=1;f=next();CHECK(f.throwNow&&f.fire&&f.throwVelocity.z<-1);
 s.ammo[4].rounds=0;f=next();CHECK(f.holster&&f.select==0&&!f.leftCrate);
 CHECK(!SupportCrateWeapon("unl_grenade_frag"));puts("Support grab, throw, native cooldown, empty hands and interruption guards passed.");return 0;
}
