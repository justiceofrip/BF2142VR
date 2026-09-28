#pragma once
namespace bfvr::bf2142 {
void SetCrosshairHidden(bool hidden);
// Scoped presentation-only state; original values restored after each HUD draw.
class CrosshairScope {
public:
 CrosshairScope();explicit CrosshairScope(const void* module,bool optic=false);
 bool Optic() const {return opticReady;}~CrosshairScope();
 CrosshairScope(const CrosshairScope&)=delete;CrosshairScope& operator=(const CrosshairScope&)=delete;
private: bool opticReady=false;float* alpha=nullptr;float saved=0;int* gui=nullptr;int savedGui=0;
};
}
