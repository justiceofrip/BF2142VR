#include "ControllerPolicy.h"
#include "HandPoseMath.h"
#include "VehicleControls.h"
#include "VehicleHud.h"
#include "MotionActions.h"
#include <cstdio>
#include <limits>
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Feedback regression line %d: %s\n",__LINE__,#x);return 1;}}while(0)
stereo::Matrix4 Identity(){stereo::Matrix4 m{};for(int i=0;i<4;++i)m.values[i][i]=1;return m;}
bool Near(float a,float b){return std::abs(a-b)<.0002f;}
int main(){
    // Once acquired, bringing a steeply pitched gun in line must not let go.
    CHECK(SupportGripEligible(true,true,.6f,.5f,true));
    CHECK(!SupportGripEligible(true,false,.6f,.5f,true));
    CHECK(!SupportGripEligible(false,true,.6f,.5f,true));
    CHECK(!SupportGripEligible(true,true,.6f,.9f,true));
    CHECK(!SupportGripEligible(true,true,std::numeric_limits<float>::quiet_NaN(),.5f,true));
    CHECK(!SupportGripEligible(true,true,.4f,.5f,false,.3f));
    CHECK(SupportGripEligible(true,true,.4f,.5f,false,.5f));
    auto aim=Identity();aim.values[3]={1,2,3,1};
    const auto lowered=CalibrateWeaponAim(aim,-30);
    CHECK(Near(lowered.values[2][1],-.5f)&&Near(lowered.values[3][0],1)&&Near(lowered.values[3][2],3));
    CHECK(InverseRigid(lowered));CHECK(Near(CalibrateWeaponAim(lowered,30).values[2][2],1));
    // Hull banking is retained only by the opt-in cockpit mode.
    VehicleSample vehicle{1,2,3,VehicleRole::Pilot,Identity()};VehicleView view;
    CHECK(view.Update(vehicle,Identity()));vehicle.rootWorld=CalibrateWeaponAim(Identity(),30);
    const auto level=view.Update(vehicle,vehicle.rootWorld,true),cockpit=view.Update(vehicle,vehicle.rootWorld,false);
    CHECK(level&&cockpit&&Near(level->values[2][1],0)&&Near(cockpit->values[2][1],.5f));
    // Roundtrip native aiming direction through runtime reference, including
    // a nontrivial recenter pose and a level vs pitched comfort base.
    const stereo::Pose reference{{.5f,1.6f,-.2f},{0,.25881905f,0,.96592583f}};
    for(const auto& base:{*level,*cockpit}){
        const auto anchor=VehicleHudAnchor(vehicle.rootWorld,base,reference,1.3f);CHECK(anchor);
        const auto restored=stereo::ComposeRuntimeHeadWithD3D8Camera(base,reference,*anchor,1.3f);CHECK(restored);
        for(int r=0;r<4;++r)for(int c=0;c<4;++c)CHECK(Near(restored->values[r][c],vehicle.rootWorld.values[r][c]));
    }
    CHECK(!VehicleHudAnchor(aim,Identity(),reference,0));
    shared::SharedControllerSample sample{};sample.flags=shared::kControllerSampleFlagSessionFocused;sample.predictedDisplayTime=1000000000;
    sample.hands[0].flags=shared::kControllerHandFlagTriggerActive;
    sample.hands[1].buttons=shared::kControllerHandButtonPrimary;
    ControllerBindings bindings;ControllerPolicyState state;
    auto command=MapControllers(state,sample,true,0,600,true,false,30,bindings);CHECK(command.keys[0x39]&&!command.keys[0x22]);
    sample.hands[0].triggerValue=1;sample.predictedDisplayTime+=16000000;
    command=MapControllers(state,sample,true,0,600,true,false,30,bindings);CHECK(command.keys[0x22]&&!command.keys[0x39]);
    BindingCapture capture;capture.Begin(unsigned(BindAction::PickupKit));
    CHECK(!capture.Update(&sample,bindings)); // opening click/held chord cannot bind
    sample.hands[0].triggerValue=0;sample.hands[1].buttons=0;CHECK(!capture.Update(&sample,bindings));
    sample.hands[0].buttons=shared::kControllerHandButtonMenu;CHECK(capture.Update(&sample,bindings));
    CHECK(bindings[BindAction::PickupKit]==3);sample.predictedDisplayTime+=16000000;
    command=MapControllers(state,sample,true,0,600,true,false,30,bindings);CHECK(command.keys[0x22]&&!command.keys[0x0f]);
    capture.Begin(0);sample.flags=0;CHECK(!capture.Update(&sample,bindings)&&capture.Selected()<0);
    for(unsigned i=0;i<32;++i)if(ValidBinding(i))CHECK(!BindingLabel(i).empty());CHECK(!ValidBinding(32));
    // A squeeze already held when a vehicle dies cannot auto-open deployment.
    DeployGrip deploy;std::int64_t t=1000000000;
    CHECK(!deploy.Update(true,true,true,t));t+=16000000;CHECK(!deploy.Update(true,true,true,t));
    t+=16000000;CHECK(!deploy.Update(true,true,false,t));t+=16000000;CHECK(deploy.Update(true,true,true,t));
    t+=16000000;CHECK(deploy.Update(true,true,true,t));t+=130000000;CHECK(!deploy.Update(true,true,true,t));
    CHECK(!deploy.Update(true,false,false,t+16000000));CHECK(!deploy.Update(false,true,true,t+32000000));
    MotionActions knife;MotionInput in{};in.owner=1;in.active=true;in.kind=MotionKind::Knife;in.triggerAttack=true;
    in.head={0,1.7f,0};in.hand={.2f,1.4f,-.4f};in.time=1000000000;knife.Update(in);
    for(int i=0;i<30;++i){in.time+=20000000;CHECK(!knife.Update(in).primary);}
    in.trigger=true;in.time+=20000000;auto strike=knife.Update(in);CHECK(strike.primary&&strike.triggerStrike&&strike.action==MotionKind::Knife);
    for(int i=0;i<60;++i){in.time+=20000000;CHECK(knife.Update(in).action==MotionKind::None);}
    in.trigger=false;in.time+=20000000;knife.Update(in);in.trigger=true;in.time+=20000000;CHECK(knife.Update(in).triggerStrike);
    // Focus/owner changes with a held trigger cannot create an attack.
    in.owner=2;in.time+=20000000;CHECK(!knife.Update(in).primary);
    puts("Grip retention, gun calibration, vehicle camera/HUD roundtrip, bindings/kit modifier, death redeploy and knife trigger regressions passed.");
}
