#include "WeaponGrip.h"
#include "SupportCrates.h"
namespace bfvr::bf2142 {
bool WeaponGrip::ResolveSupport(const SupportFrame& support,std::int64_t time) noexcept {
    if(support.holster)Holster(time);
    // Publish only this final state. Toggling false/true in the same XR update
    // repeatedly arms the native fire-transition guard while holding a crate.
    return support.leftCrate||held;
}
bool WeaponGrip::Update(const WeaponGripInput& in) noexcept {
    if(!in.active||!in.owner||in.equipped<=0||in.time<=0){observed=false;previousTime=0;return held;}
    if(owner!=in.owner){Reset();owner=in.owner;equipped=in.equipped;}
    // Wheel/keyboard selection deliberately equips the newly selected item.
    if(equipped!=in.equipped){if(in.time>=autoEquipUntil)held=true;equipped=in.equipped;}
    const bool gap=!observed||in.time<previousTime||in.time-previousTime>250000000;
    if(!gap && in.time!=previousTime && in.pressed){
        // BodyInventory can complete an early squeeze on entry to the slot.
        // That one-shot acquisition owns the draw even without a second edge.
        if(in.slotGrab){held=true;autoEquipUntil=0;}
        else if(!previousPressed)held=false;
    }
    previousPressed=in.pressed;previousTime=in.time;observed=true;return held;
}
}
