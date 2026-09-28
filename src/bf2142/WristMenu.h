#pragma once
#include "ControllerPolicy.h"
#include <optional>
#include <wrl/client.h>
namespace bfvr::bf2142 {
constexpr float wristWidth=.11f,wristHeight=.045f;
struct WristFrame {bool visible=false,hovered=false,pressed=false;stereo::Pose panel{};stereo::Vec3 rayStart{},rayEnd{};};
// Gaze reveals a single deploy shortcut; the full native menu remains at its
// accepted world-stable size/distance. This never modifies hand/weapon poses.
class WristMenu {
public:
 WristFrame Update(bool enabled,const shared::SharedControllerSample&,const stereo::Pose& head) noexcept;
 void Apply(ControllerCommand&,bool menuVisible) const noexcept;
 void Reset() noexcept {*this={};}
 bool Consuming() const {return consuming;}
private:
 WristFrame frame{};bool armed=false,triggerHeld=false,consuming=false;
 LONGLONG previous=0,pressedUntil=0,gazeSince=0;bool revealed=false;
};
class WristMenuGpu {
public:
 bool Draw(IDirect3DDevice9*,const WristFrame&,const shared::SharedPresentationView&,const stereo::Matrix4&,float scale);
 void Reset(){texture.Reset();device=nullptr;}
private:
 Microsoft::WRL::ComPtr<IDirect3DTexture9> texture;IDirect3DDevice9* device=nullptr;
};
}
