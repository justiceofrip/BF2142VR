#pragma once
#include "ControllerPolicy.h"
#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0800
#endif
#include <dinput.h>
namespace bfvr::bf2142 {
struct InputOverlayState {
    std::array<BYTE,256> virtualKeys{},physicalKeys{};
    std::array<BYTE,8> virtualButtons{},physicalButtons{},blockedButtons{};
    DWORD seenMotion=0,selectionRelease=0;EquipmentSelection consumedSelection{};
};
void OverlayDeviceState(InputOverlayState&,bool keyboard,DWORD bytes,void* data,const ControllerCommand&,DWORD serial) noexcept;
void OverlayDeviceEvents(InputOverlayState&,bool keyboard,DWORD stride,DIDEVICEOBJECTDATA* data,DWORD& count,DWORD capacity,bool peek,
    const ControllerCommand&,DWORD serial,DWORD timestamp) noexcept;
}
