#pragma once
#include "VrSettings.h"
#include <windows.h>
#include <dxgiformat.h>
#include <vector>
namespace bfvr::bf2142 {
int VrMenuHit(float x,float y,bool open) noexcept;
class VrControlsMenu {
public:
 void Vehicle(bool value){if(vehicle!=value){vehicle=value;dirty=true;}}
 bool Open() const {return open;}
 void Hotkey(bool down);
 bool FilterBack(bool pressed){if(!pressed)backHeld=false;return pressed && backHeld;}
 bool Interact(float x,float y,bool click,bool back,VrSettings& settings,float headHeight);
 void Draw(std::vector<DWORD>& image,UINT width,UINT height,DXGI_FORMAT format,const VrSettings& settings,bool widescreen=false);
 const std::vector<DWORD>& Artwork(UINT width,UINT height,DXGI_FORMAT,const VrSettings&,bool widescreen=false);
 UINT64 ArtworkRevision() const {return revision;}
 void Reset(){open=false;hover=-1;dirty=true;}
 bool RecenterRequested(){bool r=recenter;recenter=false;return r;}
private:
 bool vehicle=false,voicePage=false,cachedWidescreen=false;
 bool open=false,keyHeld=false,dirty=true,recenter=false,saveFailed=false,backHeld=false;
 int hover=-1;UINT cachedWidth=0,cachedHeight=0;DXGI_FORMAT cachedFormat=DXGI_FORMAT_UNKNOWN;
 std::vector<DWORD> art;UINT64 revision=0;
};
}
