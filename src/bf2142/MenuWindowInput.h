#pragma once
#include <windows.h>
namespace bfvr::bf2142 {
// Flash UI uses the game's window-message listener, separately from infantry
// DirectInput. Never inject buttons globally or deliver one edge by both paths.
class MenuWindowInput {
public:
    void Update(HWND window,bool focused,const POINT* point,bool pressed,bool escape,ULONGLONG now=GetTickCount64());
    bool Expired(ULONGLONG now) const {return mouseDown && now>=lastUpdate && now-lastUpdate>150;}
    void ObserveGameplayEscape(bool value){lastEscape=value;}
    void Release(HWND window);
private:
    bool mouseDown=false,lastEscape=false;POINT lastPoint{};ULONGLONG lastUpdate=0;
};
}
