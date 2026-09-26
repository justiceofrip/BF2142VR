#pragma once
#include "BodyInventory.h"
namespace bfvr::bf2142 {
struct RadioFrame {
    bool visible=false,hovered=false,held=false,pressed=false,click=false;
    stereo::Pose anchor{},grip{};
    stereo::Vec3 button{};
    float depression=0;
    std::int64_t time=0;
};
// All geometry and the grip contact share this torso-local placement (metres).
inline constexpr stereo::Vec3 kRadioPosition{-.22f,-.19f,-.055f};
struct RadioObservation {
    bool active=false,leftAvailable=true;
    std::uint64_t owner=0;
    stereo::Pose anchor{};
    shared::SharedControllerSample sample{};
};
class ShoulderRadio {
public:
    RadioFrame Update(const RadioObservation&) noexcept;
    void Reset() noexcept {*this={};}
private:
    bool observed=false,previousPressed=false,held=false;
    std::uint64_t owner=0;
    std::int64_t time=0;
    float depression=0;
};
stereo::Pose RadioGrip(const stereo::Pose& anchor) noexcept;
stereo::Vec3 RadioButton(const stereo::Pose& anchor,float depression) noexcept;
}
