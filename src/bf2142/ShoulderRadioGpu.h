#pragma once
#include "ShoulderRadio.h"
#include <d3d9.h>
namespace bfvr::bf2142 {
// Called within the ordinary eye scene, before native HUD isolation. Uses the
// native weapon depth projection so the fingers can occlude the casing.
bool DrawShoulderRadio(IDirect3DDevice9*,const RadioFrame&,
 const shared::SharedPresentationView&,const stereo::Matrix4& weaponProjection,float scale) noexcept;
}
