#pragma once
namespace bfvr::bf2142 {
void SetCrosshairHidden(bool hidden);
// Scoped native HUD alpha; original value restored after the draw.
class CrosshairScope {
public: CrosshairScope();~CrosshairScope();
private: float* alpha=nullptr;float saved=0;
};
}
