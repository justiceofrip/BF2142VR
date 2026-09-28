#include "WristMenu.h"
#include <cstdio>
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Wrist test line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 shared::SharedControllerSample s{};s.flags=shared::kControllerSampleFlagSessionFocused;s.predictedDisplayTime=1000000000;
 auto& l=s.hands[0];auto& r=s.hands[1];
 l.flags=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripOrientationValid|shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked;
 r.flags=shared::kControllerHandFlagAimActive|shared::kControllerHandFlagAimPositionValid|shared::kControllerHandFlagAimOrientationValid|shared::kControllerHandFlagAimPositionTracked|shared::kControllerHandFlagAimOrientationTracked|shared::kControllerHandFlagTriggerActive;
 l.gripPose.orientationW=r.aimPose.orientationW=1;l.gripPose.positionY=-.045f;l.gripPose.positionZ=-.59f;r.aimPose.positionZ=-.1f;
 WristMenu menu;auto tick=[&](bool enabled=true){s.predictedDisplayTime+=11000000;return menu.Update(enabled,s,{});};
 auto reveal=[&](){WristFrame f;for(int i=0;i<39;++i)f=tick();return f;};
 auto f=tick();CHECK(!f.visible);f=reveal();CHECK(f.visible&&f.hovered&&!f.pressed);CHECK(f.rayEnd.z<-.49f);
 r.triggerValue=1;f=tick();CHECK(f.pressed);ControllerCommand c{};c.buttons[0]=0x80;menu.Apply(c,false);CHECK(c.keys[0x1c]&&!c.buttons[0]&&c.blockedPhysicalButtons[0]);
 for(int i=0;i<20;++i)f=tick();CHECK(!f.pressed&&menu.Consuming()); // held trigger never repeats
 c={};menu.Apply(c,true);CHECK(!c.keys[0x1c]); // don't reopen an already visible menu
 menu.Reset();f=tick();CHECK(!f.pressed);for(int i=0;i<3;++i)CHECK(!tick().pressed); // held on entry
 reveal();r.triggerValue=0;tick();r.triggerValue=1;CHECK(tick().pressed);
 CHECK(!tick(false).visible);c={};menu.Apply(c,false);CHECK(!c.keys[0x1c]&&!c.blockedPhysicalButtons[0]);
 r.triggerValue=0;tick();l.gripPose.positionX=.8f;CHECK(!tick().visible);l.gripPose.positionX=0;r.aimPose.positionX=.3f;CHECK(reveal().visible&&!tick().hovered);
 r.triggerValue=1;CHECK(!tick().pressed);r.aimPose.positionX=0;CHECK(!tick().pressed); // dragged held trigger onto button
 r.triggerValue=0;tick();r.triggerValue=1;s.predictedDisplayTime+=500000000;CHECK(!tick().pressed); // tracking gap
 s.flags=0;CHECK(!tick().visible);s.flags=shared::kControllerSampleFlagSessionFocused;l.flags=0;CHECK(!tick().visible);
 menu.Reset();r.triggerValue=0;l.flags=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripOrientationValid|shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked;
 l.gripPose.positionX=.15f;CHECK(!reveal().visible);l.gripPose.positionX=0;for(int i=0;i<20;++i)CHECK(!tick().visible);l.gripPose.positionX=.2f;CHECK(!tick().visible);l.gripPose.positionX=0;CHECK(!tick().visible);
 puts("Wrist gaze, stereo-space ray, deploy pulse, fire suppression, tracking loss and held-trigger guards passed.");
}
