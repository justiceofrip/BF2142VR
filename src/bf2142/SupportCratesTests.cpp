#include "SupportCrates.h"
#include <cstdio>
#include <cstring>
using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Support test failed %d: %s\\n",__LINE__,#x);return 1;}}while(0)
int main(){
 SupportCrates policy;SupportObservation s;s.active=s.leftCrates=s.leftTracked=true;s.owner=7;s.time=1000000000;s.equipped=3;
 strcpy_s(s.names[3].data(),49,"eu_ar_rifle");strcpy_s(s.names[4].data(),49,"unl_hub_medic");s.ammo[4]={true,1,1};
 s.left.position=BodySlots()[3].offset;const auto next=[&](){s.time+=20000000;return policy.Update(s);};next();next();
 s.leftGrip=true;auto f=next();CHECK(f.busy&&f.select==4&&!f.leftCrate);s.equipped=4;CHECK(next().leftCrate);
 for(int i=0;i<5;++i){s.left.position.z-=.06f;next();}s.leftGrip=false;s.left.position.z-=.06f;
 f=next();CHECK(f.fire&&f.throwNow&&f.throwVelocity.z<-1);CHECK(!policy.Update(s).throwNow);
 s.ammo[4]={true,0,0};f=next();CHECK(f.select==3&&f.unavailable[4]);s.equipped=3;next();
 s.left.position=BodySlots()[3].offset;s.leftGrip=true;CHECK(!next().busy);
 s.ammo[4].deployable=1;s.leftGrip=false;next();s.leftGrip=true;CHECK(next().select==4);
 s.time+=500000000;f=next();CHECK(!f.busy&&!f.fire&&!f.throwNow);
 s.leftGrip=false;next();s.leftGrip=true;CHECK(next().busy);s.leftTracked=false;CHECK(!next().busy);
 s.leftTracked=true;CHECK(!next().busy);s.leftGrip=false;next();s.leftGrip=true;CHECK(next().busy);
 s.owner=8;CHECK(!next().busy);s.active=false;CHECK(!next().fire);
 CHECK(!SupportCrateWeapon("unl_grenade_frag"));puts("Support grab, throw, native cooldown, restoration and interruption guards passed.");return 0;
}
