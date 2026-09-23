#pragma once
#include <Windows.h>
namespace bfvr::bf2142 {
struct HudTarget {void* hud=nullptr;void* root=nullptr;void* pointer=nullptr;float width=0,height=0;float left=0,top=0;};
class NativeHudState {
public:
    bool Connect(const BYTE* gameImage,const BYTE* rendererImage);
    bool Read(HudTarget* target=nullptr,bool interactive=true) const;
private:
    const BYTE* game=nullptr;const BYTE* renderer=nullptr;
};
bool NativeHudMenuActive(HudTarget* target=nullptr);
bool NativeHudTarget(HudTarget* target);
// Mirrors HUDManager::isInMenu, with unknown modes rejected.
bool InteractiveHudMode(int mode) noexcept;
}
