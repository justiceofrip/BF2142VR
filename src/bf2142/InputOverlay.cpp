#include "InputOverlay.h"
#include <cstring>
#include <limits>
#include <algorithm>
namespace bfvr::bf2142 {
namespace {
LONG AddMotion(LONG a,LONG b) {return static_cast<LONG>(std::clamp<LONGLONG>(LONGLONG(a)+b,LONG_MIN,LONG_MAX));}
}
void OverlayDeviceState(InputOverlayState& s,bool keyboard,DWORD bytes,void* data,const ControllerCommand& c,DWORD serial) noexcept {
    if(!data)return;
    if(keyboard && bytes==256) {
        auto* keys=static_cast<BYTE*>(data);
        for(size_t i=0;i<c.keys.size();++i){s.physicalKeys[i]=c.blockedPhysicalKeys[i]?0:keys[i];keys[i]=s.physicalKeys[i]|c.keys[i];}
    }
    if(!keyboard && (bytes==sizeof(DIMOUSESTATE) || bytes==sizeof(DIMOUSESTATE2))) {
        auto* mouse=static_cast<DIMOUSESTATE2*>(data);const size_t n=bytes==sizeof(DIMOUSESTATE)?4:8;
        for(size_t i=0;i<n;++i){s.physicalButtons[i]=mouse->rgbButtons[i];mouse->rgbButtons[i]=(c.blockedPhysicalButtons[i]?0:s.physicalButtons[i])|c.buttons[i];}
        if(serial && s.seenMotion!=serial) {
            mouse->lX=AddMotion(mouse->lX,c.mouseX);mouse->lY=AddMotion(mouse->lY,c.mouseY);mouse->lZ=AddMotion(mouse->lZ,c.wheel);
            s.seenMotion=serial;
        }
    }
}
void OverlayDeviceEvents(InputOverlayState& s,bool keyboard,DWORD stride,DIDEVICEOBJECTDATA* data,DWORD& count,DWORD capacity,bool peek,
    const ControllerCommand& c,DWORD serial,DWORD timestamp) noexcept {
    if(!data || stride<16 || stride>sizeof(DIDEVICEOBJECTDATA) || count>capacity)return;
    // A peek must see the physical state it would have consumed, but cannot
    // update any persistent state or consume a pending controller edge.
    auto physicalKeys=s.physicalKeys;auto physicalButtons=s.physicalButtons;
    for(DWORD i=0;i<count;++i) {
        DIDEVICEOBJECTDATA event{};BYTE* address=reinterpret_cast<BYTE*>(data)+size_t(i)*stride;
        std::memcpy(&event,address,stride);
        if(keyboard && event.dwOfs<256) {physicalKeys[event.dwOfs]=c.blockedPhysicalKeys[event.dwOfs]?0:BYTE(event.dwData);event.dwData=physicalKeys[event.dwOfs]|c.keys[event.dwOfs];}
        else if(!keyboard && event.dwOfs>=DIMOFS_BUTTON0 && event.dwOfs<DIMOFS_BUTTON0+8) {
            const DWORD b=event.dwOfs-DIMOFS_BUTTON0;physicalButtons[b]=BYTE(event.dwData);event.dwData=(c.blockedPhysicalButtons[b]?0:physicalButtons[b])|c.buttons[b];
        }
        std::memcpy(address,&event,stride);
    }
    if(!peek){s.physicalKeys=physicalKeys;s.physicalButtons=physicalButtons;}
    const auto append=[&](DWORD offset,DWORD value) {
        if(count>=capacity)return false;
        DIDEVICEOBJECTDATA event{};event.dwOfs=offset;event.dwData=value;event.dwTimeStamp=timestamp;event.dwSequence=serial;
        std::memcpy(reinterpret_cast<BYTE*>(data)+size_t(count)*stride,&event,stride);++count;return true;
    };
    if(keyboard) {
        for(DWORD i=0;i<256;++i)if(c.keys[i]!=s.virtualKeys[i] && append(i,c.keys[i]|physicalKeys[i]) && !peek)s.virtualKeys[i]=c.keys[i];
    } else {
        for(DWORD i=0;i<8;++i)if((c.buttons[i]!=s.virtualButtons[i] || c.blockedPhysicalButtons[i]!=s.blockedButtons[i]) &&
            append(DIMOFS_BUTTON0+i,c.buttons[i]|(c.blockedPhysicalButtons[i]?0:physicalButtons[i])) && !peek){
                s.virtualButtons[i]=c.buttons[i];s.blockedButtons[i]=c.blockedPhysicalButtons[i];
            }
        const DWORD needed=DWORD(c.mouseX!=0)+DWORD(c.mouseY!=0)+DWORD(c.wheel!=0);
        if(serial && s.seenMotion!=serial && capacity-count>=needed) {
            if(c.mouseX)append(DIMOFS_X,DWORD(c.mouseX));if(c.mouseY)append(DIMOFS_Y,DWORD(c.mouseY));if(c.wheel)append(DIMOFS_Z,DWORD(c.wheel));
            if(!peek)s.seenMotion=serial;
        }
    }
}
}
