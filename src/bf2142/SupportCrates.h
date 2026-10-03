#pragma once
#include "BodyInventory.h"
#include <array>
#include <string_view>
namespace bfvr::bf2142 {
struct SupportAmmo {bool valid=false;int rounds=0,deployable=-1;};
struct SupportObservation {
    bool active=false,leftCrates=false,leftTracked=false,leftGrip=false,rightGrab=false,keepWeapon=false,throwReady=true;
    std::uint64_t owner=0;std::int64_t time=0;
    int equipped=0;std::array<std::array<char,49>,10> names{};std::array<SupportAmmo,10> ammo{};
    stereo::Pose left{},anchor{};
};
struct SupportFrame {
    bool busy=false,leftCrate=false,consumeLeft=false,fire=false,throwNow=false,holster=false,preview=false,blockFire=false;
    std::int64_t selectionTime=0;int hovered=-1,select=0,crateItem=0;stereo::Vec3 throwVelocity{};
    stereo::Pose throwPose{},heldPose{};
    std::array<bool,10> unavailable{};
};
class SupportCrates {
public:
    SupportFrame Update(const SupportObservation&);
    void Reset(){*this={};}
private:
    std::uint64_t owner=0;std::int64_t last=0,deadline=0,pulseUntil=0,grabTime=0,grabPressUntil=0;
    int crate=0,phase=0,restoreItem=0;bool pressed=false,leftObserved=false,released=false;
    stereo::Vec3 velocity{},throwVelocity{};stereo::Pose releasePose{};
    std::array<stereo::Vec3,6> history{};std::array<std::int64_t,6> times{};unsigned samples=0;
};
inline bool SupportCrateWeapon(std::string_view n) noexcept{return n=="unl_hub_ammo"||n=="unl_hub_medic";}
bool NativeSupportLaunchReady();
bool ReadSupportAmmo(std::uint64_t owner,int item,SupportAmmo*);
void PublishNativeSupport(const SupportFrame&);
}
