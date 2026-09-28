#include "WeaponGrip.h"
namespace bfvr::bf2142 {
bool WeaponGrip::Update(const WeaponGripInput& in) noexcept {
    if(!in.active||!in.owner||in.equipped<=0||in.time<=0){observed=false;previousTime=0;return held;}
    if(owner!=in.owner){Reset();owner=in.owner;equipped=in.equipped;}
    // Wheel/keyboard selection deliberately equips the newly selected item.
    if(equipped!=in.equipped){if(in.time>=autoEquipUntil)held=true;equipped=in.equipped;}
    const bool gap=!observed||in.time<previousTime||in.time-previousTime>250000000;
    if(!gap && in.time!=previousTime && in.pressed && !previousPressed){held=in.slotGrab;if(held)autoEquipUntil=0;}
    previousPressed=in.pressed;previousTime=in.time;observed=true;return held;
}
}
