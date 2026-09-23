#include "MotionActions.h"
#include "WeaponGrip.h"
#include "GrenadeArc.h"
#include <cstdio>
#include <limits>
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do {if(!(x)){printf("Failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    CHECK(MotionWeapon("knife")==MotionKind::Knife);CHECK(MotionWeapon("unl_grenade_frag")==MotionKind::Frag);
    CHECK(MotionWeapon("eu_mg")==MotionKind::None);CHECK(MotionWeapon("knife_projectile")==MotionKind::None);
    MotionInput in{};in.owner=17;in.kind=MotionKind::Knife;in.active=true;in.head={0,1.7f,0};in.hand={.2f,1.4f,-.4f};in.time=1000000000;
    MotionActions policy;auto step=[&](int ms=20){in.time+=std::int64_t(ms)*1000000;return policy.Update(in);};
    policy.Update(in);for(int i=0;i<55;++i)CHECK(step().action==MotionKind::None);
    // A quiet trigger squeeze must not attack. A deliberate physical stroke does.
    in.trigger=true;CHECK(!step().primary);in.trigger=false;
    int attacks=0;for(int i=0;i<12;++i){in.hand.z-=.035f;const auto r=step();attacks+=r.action==MotionKind::Knife;}
    CHECK(attacks==1);
    // Continued movement cannot auto-repeat; rest rearms a second stroke.
    for(int i=0;i<20;++i){in.hand.z+=.025f;CHECK(step().action==MotionKind::None);}
    for(int i=0;i<35;++i)step();attacks=0;for(int i=0;i<12;++i){in.hand.z-=.035f;attacks+=step().action==MotionKind::Knife;}CHECK(attacks==1);
    // Drawing/recentering, tracking jumps and moving head+hand together do not attack.
    in.owner++;for(int i=0;i<12;++i){in.hand.z+=.04f;CHECK(step().action==MotionKind::None);}
    for(int i=0;i<55;++i)step();in.hand.x+=2;CHECK(step().action==MotionKind::None);
    for(int i=0;i<55;++i)step();for(int i=0;i<20;++i){in.hand.x+=.04f;in.head.x+=.04f;CHECK(step().action==MotionKind::None);}
    CHECK(step(300).action==MotionKind::None);
    // Grenades no longer consume trigger or create gesture-fire pulses.
    in.kind=MotionKind::Frag;in.grip=true;in.trigger=true;
    for(int i=0;i<100;++i){in.hand.z-=.025f;const auto r=step();CHECK(r.action==MotionKind::None&&!r.primary&&!r.primed);}
    in.grip=false;CHECK(step().action==MotionKind::None);
    WeaponGrip grip;WeaponGripInput grab{true,false,false,55,1000000000,3};
    CHECK(grip.Update(grab));grab.time+=20000000;grab.pressed=true;CHECK(!grip.Update(grab));
    grab.time+=20000000;CHECK(!grip.Update(grab)); // hold does not repeatedly toggle
    grab.time+=20000000;grab.pressed=false;CHECK(!grip.Update(grab));
    grab.time+=20000000;grab.pressed=true;CHECK(!grip.Update(grab)); // empty squeeze stays empty
    grab.time+=20000000;grab.pressed=false;grip.Update(grab);grab.time+=20000000;grab.pressed=true;CHECK(!grip.Update(grab));
    grab.active=false;grip.Update(grab);grab.active=true;grab.time+=500000000;CHECK(!grip.Update(grab)); // resume while squeezed
    grab.pressed=false;grab.time+=20000000;grip.Update(grab);grab.pressed=true;grab.slotGrab=true;grab.time+=20000000;CHECK(grip.Update(grab));
    grab.pressed=false;grab.time+=20000000;grip.Update(grab);grab.slotGrab=false;grab.pressed=true;grab.time+=20000000;CHECK(!grip.Update(grab));
    grab.equipped=2;grab.time+=20000000;CHECK(grip.Update(grab)); // wheel selection draws
    grab.owner++;grab.time+=20000000;CHECK(grip.Update(grab)); // new soldier starts normally
    GrenadeTrajectory trajectory{{0,1.5f,.2f},{0,4,20},5.3955f};
    const auto t=GrenadePoint(trajectory,1);CHECK(t&&std::abs(t->z-20.2f)<.0001f&&std::abs(t->y-(5.5f-2.69775f))<.0001f);
    CHECK(!GrenadePoint(trajectory,-1));trajectory.gravity=std::numeric_limits<float>::quiet_NaN();CHECK(!GrenadePoint(trajectory,1));trajectory.gravity=5.3955f;
    CameraInput camera{};for(int i=0;i<4;++i)camera.world.values[i][i]=1;camera.nearPlane=.04f;camera.farDelta=100;
    auto left=MakeEyeCamera(camera,{},{{-.032f,0,0},{0,0,0,1}},{-1,1,1,-1}),right=MakeEyeCamera(camera,{},{{.032f,0,0},{0,0,0,1}},{-1,1,1,-1});CHECK(left&&right);
    std::vector<DWORD> lp(320*240),rp=lp;trajectory.start={.15f,-.15f,.5f};trajectory.velocity={0,3,20};
    CHECK(DrawGrenadeArc(lp,320,240,DXGI_FORMAT_B8G8R8A8_UNORM,*left,trajectory)>20);CHECK(DrawGrenadeArc(rp,320,240,DXGI_FORMAT_B8G8R8A8_UNORM,*right,trajectory)>20);CHECK(lp!=rp&&lp[0]==0&&rp[0]==0);
    const auto close=CloseWeaponCamera(camera);CHECK(close&&close->nearPlane==.006f&&std::abs(close->farDelta+close->nearPlane-100.04f)<.0001f);CHECK(camera.nearPlane==.04f);
    camera.nearPlane=.003f;CHECK(CloseWeaponCamera(camera)->nearPlane==.003f);
    camera.nearPlane=0;CHECK(!CloseWeaponCamera(camera));
    puts("Knife gestures, trigger-grenade rollback, empty-hand squeeze / body-slot draw, stereo aim guide and close-weapon projection passed.");return 0;
}
