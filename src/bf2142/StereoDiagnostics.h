#pragma once
#include "StereoSession.h"
#include <vector>
namespace bfvr::bf2142 {
void DiagnosticRequest(shared::SharedRenderRequest& request, unsigned long frame);
void SaveDiagnosticFrame(const std::wstring& path,UINT width,UINT height,const std::vector<DWORD>& pixels);
}
