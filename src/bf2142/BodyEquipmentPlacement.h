#pragma once
#include "BodyInventory.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
// Fit the holstered representation, not the held/native weapon. Utility world
// models (e.g. the 62 cm ammo box) are deployment props, not belt-sized objects.
struct BodyEquipmentPlacement {
    stereo::Vec3 center{},position{};
    float scale=1;
    unsigned slot=0;
    stereo::Vec3 Transform(stereo::Vec3 p) const noexcept {
        p={(p.x-center.x)*scale,(p.y-center.y)*scale,(p.z-center.z)*scale};
        stereo::Vec3 v{};
        if(slot<=2)v={p.x,-p.z,-p.y}; // muzzle/blade down; stock/handle up
        else if(slot==3 || slot==4)v={-p.z,p.y,-p.x}; // thin face against left hip
        else if(slot==6)v={p.z,p.y,p.x};
        else v={p.x,p.y,-p.z}; // grenade stays upright
        return {position.x+v.x,position.y+v.y,position.z+v.z};
    }
};
inline BodyEquipmentPlacement PlaceBodyEquipment(unsigned slot,stereo::Vec3 low,stereo::Vec3 high) noexcept {
    BodyEquipmentPlacement p;p.slot=slot;
    if(slot>=BodySlots().size()){p.scale=0;return p;}
    p.center={(low.x+high.x)*.5f,(low.y+high.y)*.5f,(low.z+high.z)*.5f};p.position=BodySlots()[slot].offset;
    const stereo::Vec3 extent{high.x-low.x,high.y-low.y,high.z-low.z};
    stereo::Vec3 size=slot<=2?stereo::Vec3{extent.x,extent.z,extent.y}:extent;
    if(slot==3 || slot==4 || slot==6)size={extent.z,extent.y,extent.x};
    const stereo::Vec3 limit=slot==0?stereo::Vec3{.24f,.80f,.26f}:slot==1?stereo::Vec3{.10f,.29f,.20f}:
        slot==2?stereo::Vec3{.08f,.30f,.08f}:slot==5?stereo::Vec3{.09f,.14f,.09f}:stereo::Vec3{.19f,.23f,.23f};
    if(!std::isfinite(size.x)||!std::isfinite(size.y)||!std::isfinite(size.z)||size.x<=0||size.y<=0||size.z<=0){p.scale=0;return p;}
    p.scale=std::min({1.f,limit.x/size.x,limit.y/size.y,limit.z/size.z});
    // Grab the stock/handle at the slot; the long part hangs below it. Centering
    // the whole rifle/knife here placed half its length above the shoulder/chest.
    if(slot<=2)p.position.y+=(slot==0?.06f:.04f)-size.y*p.scale*.5f;
    return p;
}
}
