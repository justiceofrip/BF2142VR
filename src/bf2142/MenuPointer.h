#pragma once
#include "ControllerPolicy.h"
#include "VrControlsMenu.h"
#include "MenuWindowInput.h"
#include "stereo/UiPointerMath.h"
#include <dxgiformat.h>
#include <vector>
#include <optional>
namespace bfvr::bf2142 {
// Keep these identical to the presenter's existing quad defaults. Changing
// input must not change the user's accepted panel size or distance.
constexpr float menuWidthMeters=1.6f,menuDistanceMeters=1.5f;
struct MenuRayHit {
    stereo::UiCanvasPoint canvas{};
    stereo::Vec3 origin{},end{}; // In the menu anchor's local frame.
};
std::optional<MenuRayHit> MenuRayTarget(const shared::SharedControllerHandSample& hand,
    const stereo::Pose& anchor,UINT width,UINT height,UINT uiWidth,UINT uiHeight);
void DrawMenuPointer(std::vector<DWORD>& left,std::vector<DWORD>& right,std::vector<DWORD>& ui,
    UINT width,UINT height,DXGI_FORMAT format,const shared::SharedRenderRequest& request,
    const stereo::Pose& anchor,const stereo::UiCanvasPoint& point,const std::optional<MenuRayHit>& ray,bool pressed);
struct MenuClickState {
    bool active=false,held=false,armed=false;
    bool Update(bool enabled,bool hit,bool trigger) noexcept;
};
class MenuPointer {
public:
    void Connect(IDirect3DDevice9* device);
    bool MenuVisible() const;
    bool Active() const {return menu;}
    const stereo::Pose& Anchor() const {return anchor;}
    void Update(bool showMenu,const shared::SharedControllerSample* sample,
        const shared::SharedRenderRequest& request,UINT width,UINT height,
        UINT uiWidth,UINT uiHeight,ControllerCommand& command,VrControlsMenu* controls=nullptr,VrSettings* settings=nullptr);
    void Draw(std::vector<DWORD>& left,std::vector<DWORD>& right,std::vector<DWORD>& ui,
        DXGI_FORMAT format,const shared::SharedRenderRequest& request) const;
    void Reset();
    void ExpireInput();
private:
    HWND window=nullptr;
    bool menu=false,visible=false,pressed=false,buttonHeld=false;
    UINT width=0,height=0;
    stereo::Pose anchor{};
    stereo::UiCanvasPoint point{};
    std::optional<MenuRayHit> ray;
    MenuClickState click;
    MenuWindowInput input;
};
}

