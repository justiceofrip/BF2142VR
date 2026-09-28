#pragma once
#include "ControllerPolicy.h"
#include "stereo/UiPointerMath.h"
#include <array>
namespace bfvr::bf2142 {
struct BodySlot {const char* name;stereo::Vec3 offset;float radius;unsigned item;};
const std::array<BodySlot,7>& BodySlots() noexcept;
struct BodyInventoryResult {int hovered=-1,selected=-1;unsigned key=0;LONGLONG selectionTime=0;stereo::Pose anchor{};bool anchorValid=false;};
class BodyInventory {
public:
    BodyInventoryResult Update(bool enabled,const shared::SharedControllerSample&,const stereo::Pose& head,
        const std::array<bool,10>& inventory,int equippedItem=0) noexcept;
    void Reset() noexcept {*this={};}
private:
    bool initialized=false,gripHeld=false,gripTracked=false;
    float bodyYaw=0;
    LONGLONG lastTime=0,keyUntil=0;
    unsigned pendingKey=0,pendingItem=0;
};
}
