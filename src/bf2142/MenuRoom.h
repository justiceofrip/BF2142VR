#pragma once
#include "MenuPointer.h"
namespace bfvr::bf2142 {
float MenuStripeCoverage(float value,float period,float halfWidth,float footprint) noexcept;
void DrawMenuRoom(std::vector<DWORD>& left,std::vector<DWORD>& right,UINT width,UINT height,
 DXGI_FORMAT format,const shared::SharedRenderRequest& request,const stereo::Pose& anchor);
}
