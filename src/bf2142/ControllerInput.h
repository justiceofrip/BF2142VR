#pragma once
#include "ControllerPolicy.h"
namespace bfvr::bf2142 {
bool StartControllerInput(LogFunction logger);
void PublishControllerCommand(const ControllerCommand& command,bool enabled);
void ClearControllerCommand();
}

namespace bfvr::bf2142 {void SetDesktopInput(bool enabled);}
