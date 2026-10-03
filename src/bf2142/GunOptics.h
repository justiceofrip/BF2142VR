#pragma once
#include "StereoCamera.h"
#include "TrackingMath.h"
#include <array>
#include <span>
#include <string_view>
#include <vector>
#include <cstdint>
#include <Windows.h>
namespace bfvr::bf2142 {
struct OpticDefinition {
    const char* name;
    stereo::Vec3 center;
    float halfWidth,halfHeight;
    bool rectangular;
    float nativeFactor,magnification;
    // SMG reflex reticles use a clean red dot instead of native zoom artwork.
    bool redDot=false;
    // Opaque model lenses need a 1x world image even for a red-dot sight.
    bool opaqueLens=false;
    // EU assault rifle: blue split brackets and a red bore-zero dot, no range HUD.
    bool euRifleBrackets=false;
};
std::span<const OpticDefinition> GunOpticDefinitions() noexcept;
const OpticDefinition* FindGunOptic(std::string_view name) noexcept;
struct GunOptic {
    const OpticDefinition* definition=nullptr;
    stereo::Matrix4 gun{};
    float magnification=1;
};
bool OpticNeedsScene(const GunOptic&) noexcept;
struct OpticView {
    stereo::Matrix4 world{};
    stereo::FovTangents fov{};
    float relief=0;
    std::array<float,2> visibility{};
};
// All coordinates use the same solved bone-54 basis as native projectile mapping.
std::optional<OpticView> MakeOpticView(const GunOptic&,const std::array<EyeCamera,2>&) noexcept;
size_t CompositeGunOptic(std::vector<DWORD>& eyePixels,const std::vector<DWORD>& scopePixels,
    unsigned width,unsigned height,const GunOptic&,const EyeCamera&,const OpticView&,unsigned eye,bool rgba,const std::vector<DWORD>& nativeHud={});
void IsolateOpticHud(std::vector<DWORD>&,const std::vector<DWORD>& baseline,unsigned width,unsigned height);
}
