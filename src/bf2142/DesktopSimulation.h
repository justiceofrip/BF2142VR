#pragma once
#include "ControllerPolicy.h"
namespace bfvr::bf2142 {
// Explicit desktop development mode only; never active in the headset path.
void DesktopFrame(HWND window,shared::SharedRenderRequest&,shared::SharedControllerSample&,bool menu,const stereo::Pose* menuAnchor=nullptr);
void ResetDesktopSimulation();
void BlockDesktopHotkeys(ControllerCommand&);
}
