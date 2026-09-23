#include "BodyInventory.h"
#include "NativeHands.h"
#include <cmath>
#include <cstdio>
using namespace bfvr;using namespace bfvr::bf2142;
int main(){
    BodyInventory body;shared::SharedControllerSample sample{};
    sample.flags=shared::kControllerSampleFlagSessionFocused;
    auto& hand=sample.hands[1];hand.flags=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|
        shared::kControllerHandFlagGripOrientationValid|shared::kControllerHandFlagGripPositionTracked|
        shared::kControllerHandFlagGripOrientationTracked|shared::kControllerHandFlagSqueezeActive;
    hand.gripPose.orientationW=1;stereo::Pose head{{0,1.7f,0},{0,0,0,1}};
    std::array<bool,10> inventory{};inventory.fill(true);sample.predictedDisplayTime=1000000000;
    for(unsigned i=0;i<BodySlots().size();++i){
        body.Reset();const auto& slot=BodySlots()[i];hand.gripPose.positionX=slot.offset.x;hand.gripPose.positionY=1.7f+slot.offset.y;hand.gripPose.positionZ=slot.offset.z;
        hand.squeezeValue=0;auto r=body.Update(true,sample,head,inventory);if(r.hovered!=int(i)||r.key)return 1;
        sample.predictedDisplayTime+=16000000;hand.squeezeValue=1;r=body.Update(true,sample,head,inventory);
        if(r.selected!=int(i)||!r.key)return 2;
        sample.predictedDisplayTime+=16000000;r=body.Update(true,sample,head,inventory);if(r.selected!=-1||!r.key)return 3;
        sample.predictedDisplayTime+=150000000;r=body.Update(true,sample,head,inventory);if(r.key||r.selected!=-1)return 4;
        inventory[slot.item]=false;sample.predictedDisplayTime+=16000000;r=body.Update(true,sample,head,inventory);if(r.hovered>=0)return 5;inventory[slot.item]=true;
    }
    // Menu/focus/tracking loss cancels pending keys and suppresses held-entry grabs.
    if(body.Update(false,sample,head,inventory).key)return 6;
    sample.predictedDisplayTime+=16000000;if(body.Update(true,sample,head,inventory).selected>=0)return 7;
    sample.flags=0;if(body.Update(true,sample,head,inventory).key)return 8;sample.flags=shared::kControllerSampleFlagSessionFocused;
    hand.flags=0;if(body.Update(true,sample,head,inventory).hovered>=0)return 9;
    // A physical body turn uses the runtime quaternion convention, not the
    // game's opposite-sign yaw. Looking up/down keeps the belt upright.
    body.Reset();sample.predictedDisplayTime+=16000000;sample.flags=shared::kControllerSampleFlagSessionFocused;
    hand.flags=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripOrientationValid|shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked|shared::kControllerHandFlagSqueezeActive;
    head={{2,1.5f,3},{0,std::sin(.6f),0,std::cos(.6f)}};
    const auto offset=BodySlots()[1].offset;
    hand.gripPose.positionX=2+std::cos(1.2f)*offset.x+std::sin(1.2f)*offset.z;
    hand.gripPose.positionY=1.5f+offset.y;
    hand.gripPose.positionZ=3-std::sin(1.2f)*offset.x+std::cos(1.2f)*offset.z;
    hand.squeezeValue=0;if(body.Update(true,sample,head,inventory).hovered!=1)return 10;
    sample.predictedDisplayTime+=16000000;hand.squeezeValue=1;if(body.Update(true,sample,head,inventory).selected!=1)return 11;
    sample.predictedDisplayTime+=300000000;if(body.Update(true,sample,head,inventory).key)return 12;
    const auto soldier=reinterpret_cast<void*>(0x1000),other=reinterpret_cast<void*>(0x2000);
    if(!InventoryParentMatches(nullptr,soldier)||!InventoryParentMatches(soldier,soldier)||InventoryParentMatches(other,soldier)||InventoryParentMatches(nullptr,nullptr))return 13;
    if(BodySlots()[3].item!=4||BodySlots()[4].item!=5||BodySlots()[5].item!=7||BodySlots()[6].item!=6)return 14;
    // Looking down at a turned torso must retain the same heading. The UI yaw
    // helper collapsed toward zero with pitch, putting holsters on the wrong side.
    for(float turn:{-2.4f,-1.2f,.6f,2.4f}){
        body.Reset();const float pitch=-1.25f;
        head={{2,1.5f,3},{0,std::sin(turn*.5f),0,std::cos(turn*.5f)}};
        hand.squeezeValue=0;sample.predictedDisplayTime+=16000000;
        auto upright=body.Update(true,sample,head,inventory);if(!upright.anchorValid)return 15;
        head.orientation={std::cos(turn*.5f)*std::sin(pitch*.5f),std::sin(turn*.5f)*std::cos(pitch*.5f),
            -std::sin(turn*.5f)*std::sin(pitch*.5f),std::cos(turn*.5f)*std::cos(pitch*.5f)};
        for(int j=0;j<180;++j){sample.predictedDisplayTime+=16000000;const auto tilted=body.Update(true,sample,head,inventory);
            if(!tilted.anchorValid || std::abs(tilted.anchor.orientation.y-upright.anchor.orientation.y)>.0001f ||
                std::abs(tilted.anchor.orientation.w-upright.anchor.orientation.w)>.0001f)return 16;}
    }
    // A controller behind the back may lose tracking. Equipment must remain
    // anchored to the head/torso rather than teleport to tracking-space zero.
    const auto trackedFlags=hand.flags;hand.flags=0;sample.predictedDisplayTime+=16000000;
    const auto missing=body.Update(true,sample,head,inventory);
    if(!missing.anchorValid || missing.key || missing.hovered>=0 || missing.anchor.position.x<1.5f)return 17;
    hand.flags=trackedFlags;hand.squeezeValue=1;sample.predictedDisplayTime+=16000000;
    if(body.Update(true,sample,head,inventory).selected>=0)return 18;
    // Initial heading also stays correct if the first valid frame is pitched.
    body.Reset();sample.predictedDisplayTime+=16000000;hand.squeezeValue=0;
    const auto firstDown=body.Update(true,sample,head,inventory);
    if(std::abs(firstDown.anchor.orientation.y-std::sin(1.2f))>.0001f)return 19;
    // Invalid head input never presents equipment at a fabricated anchor.
    head.orientation={0,0,0,0};sample.predictedDisplayTime+=16000000;
    if(body.Update(true,sample,head,inventory).anchorValid)return 20;
    puts("Body slots: seven mapped locations, availability, edge grabs, bounded key duration and menu/focus/tracking release passed.");
}
