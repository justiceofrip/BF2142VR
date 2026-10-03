#include "SupportCrates.h"
#include "WeaponGrip.h"
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
 // A completed throw must not own firing for another timed cleanup period.
 CHECK(!f.busy);f=next();CHECK(!f.busy&&!f.holster&&!f.fire);
 // A deliberate right-hand grab preempts a pending left throw immediately.
 s.equipped=3;s.ammo[4]={true,1,1};s.leftGrip=false;next();s.leftGrip=true;CHECK(next().busy);
 s.equipped=4;CHECK(next().leftCrate);s.rightGrab=true;f=next();CHECK(!f.busy&&!f.holster&&!f.fire);s.rightGrab=false;
 // Buffer a brief early squeeze, but never grab from a long-held grip.
 const auto slot=BodySlots()[3];
 const auto resetGrab=[&](){policy.Reset();s.active=s.leftCrates=s.leftTracked=true;s.owner=7;s.equipped=3;
  s.rightGrab=s.leftGrip=false;s.ammo[4]={true,1,1};s.left.position=slot.offset;s.left.position.x-=.5f;next();next();};
 resetGrab();s.leftGrip=true;CHECK(!next().busy);
 for(int i=0;i<8;++i)CHECK(!next().busy);
 s.left.position=slot.offset;f=next();CHECK(f.select==4&&f.selectionTime==s.time);
 resetGrab();s.leftGrip=true;next();for(int i=0;i<13;++i)next();
 s.left.position=slot.offset;CHECK(!next().busy);s.leftGrip=false;next();s.leftGrip=true;CHECK(next().busy);
 // Release cancels the buffer; the padded radius remains tightly bounded.
 resetGrab();s.leftGrip=true;next();s.leftGrip=false;next();s.left.position=slot.offset;CHECK(!next().busy);
 s.left.position.x-=slot.radius+.04f;s.leftGrip=true;CHECK(!next().busy);
 s.left.position.x+=.02f;CHECK(next().select==4);
 // Unready inventory cannot be grabbed, even with a buffered grip.
 resetGrab();s.ammo[4]={false,1,1};s.leftGrip=true;next();s.left.position=slot.offset;f=next();CHECK(!f.busy&&f.unavailable[4]);
 s.ammo[4]={true,1,0};f=next();CHECK(!f.busy&&f.unavailable[4]);
 // Every interruption cancels the intent. A held squeeze must not re-arm it.
 for(int cancel=0;cancel<6;++cancel){
  resetGrab();s.leftGrip=true;next();
  switch(cancel){case 0:s.leftTracked=false;break;case 1:++s.owner;break;case 2:s.rightGrab=true;break;
   case 3:s.leftCrates=false;break;case 4:s.active=false;break;case 5:s.time+=300000000;break;}
  CHECK(!next().busy);s.leftTracked=s.leftCrates=s.active=true;s.rightGrab=false;
  s.left.position=slot.offset;CHECK(!next().busy);
  s.leftGrip=false;next();s.leftGrip=true;CHECK(next().busy);
 }
 // Reproduce crate -> empty -> immediate rifle draw at different headset rates.
 for(int hz:{72,90,144,240}){
  SupportCrates crates;WeaponGrip hands;SupportObservation o=s;o.owner=900+hz;o.time=1000000000;o.equipped=3;o.leftGrip=false;o.leftTracked=true;o.active=true;o.rightGrab=false;o.ammo[4]={true,1,1};
  WeaponGripInput right{true,false,false,o.owner,o.time,3};
  const auto frame=[&](){o.time+=1000000000/hz;right.time=o.time;right.equipped=o.equipped;hands.Update(right);return crates.Update(o);};
  frame();right.pressed=true;auto c=frame();CHECK(!hands.ResolveSupport(c,o.time));right.pressed=false;frame();
  o.leftGrip=true;c=frame();CHECK(c.select==4&&c.selectionTime>0);const auto selectionTime=c.selectionTime;c=frame();CHECK(c.selectionTime==selectionTime);
  o.equipped=4;c=frame();CHECK(c.leftCrate&&hands.ResolveSupport(c,o.time));
  for(int i=0;i<hz/4;++i){c=frame();CHECK(c.leftCrate&&hands.ResolveSupport(c,o.time));}
  o.leftGrip=false;c=frame();CHECK(c.throwNow&&c.fire&&hands.ResolveSupport(c,o.time));
  o.ammo[4].rounds=0;c=frame();CHECK(c.holster&&!c.busy&&!hands.ResolveSupport(c,o.time));
  // Native post-throw selection cannot draw a grenade on its own.
  o.equipped=7;c=frame();CHECK(!hands.ResolveSupport(c,o.time));
  right.pressed=right.slotGrab=o.rightGrab=true;c=frame();CHECK(!c.busy&&!c.holster&&hands.ResolveSupport(c,o.time));
  o.rightGrab=right.slotGrab=false;o.equipped=3;
  for(int i=0;i<hz*2;++i){c=frame();CHECK(!c.busy&&!c.holster&&hands.ResolveSupport(c,o.time));}
 }
 // A rifle stays selected and usable for as long as the offhand box is held.
 for(int hz:{72,90,144,240}){
  SupportCrates q;SupportObservation o=s;o.owner=1000+hz;o.equipped=3;o.time=1000000000;
  o.keepWeapon=true;o.active=o.leftCrates=o.leftTracked=true;o.rightGrab=o.leftGrip=false;
  o.ammo[4]={true,1,1};o.left.position=BodySlots()[3].offset;
  const auto step=[&](){o.time+=1000000000/hz;return q.Update(o);};step();o.leftGrip=true;auto c=step();
  CHECK(c.preview&&c.busy&&!c.blockFire&&!c.select&&!c.leftCrate);
  for(int i=0;i<hz;++i){c=step();CHECK(c.preview&&!c.blockFire&&!c.select);}
  o.leftGrip=false;c=step();const auto release=o.left.position;
  CHECK(c.select==4&&c.blockFire&&!c.fire&&!c.throwNow);
  o.equipped=4;o.throwReady=false;c=step();CHECK(c.leftCrate&&!c.preview&&!c.fire);
  o.left.position={4,5,6};o.throwReady=true;c=step();CHECK(c.throwNow&&c.fire&&c.throwPose.position.x==release.x);
  CHECK(!q.Update(o).throwNow);o.ammo[4]={true,0,0};c=step();CHECK(c.select==3&&c.blockFire&&!c.holster&&!c.fire);
  o.equipped=3;c=step();CHECK(!c.busy&&!c.blockFire&&!c.holster&&!c.select);
  for(int i=0;i<hz;++i)CHECK(!step().busy);
  // A deliberate grab and every focus/tracking/owner interruption cancel it.
  for(int interrupt=0;interrupt<4;++interrupt){
   q.Reset();o.equipped=3;o.keepWeapon=true;o.leftGrip=o.rightGrab=false;o.active=o.leftTracked=true;o.ammo[4]={true,1,1};o.left.position=BodySlots()[3].offset;step();o.leftGrip=true;CHECK(step().preview);
   if(interrupt==0)o.rightGrab=true;else if(interrupt==1)o.leftTracked=false;else if(interrupt==2)o.active=false;else ++o.owner;
   c=step();CHECK(!c.preview&&!c.fire&&!c.select&&!c.busy);
  }
 }
 CHECK(!SupportCrateWeapon("unl_grenade_frag"));puts("Support grab, throw, native cooldown, empty hands and interruption guards passed.");return 0;
}
