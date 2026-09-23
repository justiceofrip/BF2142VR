#include "MenuWindowInput.h"
namespace bfvr::bf2142 {
namespace {
bool Owned(HWND w){DWORD pid=0;GetWindowThreadProcessId(w,&pid);return w && pid==GetCurrentProcessId() && IsWindow(w);}
LPARAM Pixel(POINT p){return MAKELPARAM(static_cast<WORD>(p.x),static_cast<WORD>(p.y));}
constexpr LPARAM downKey=1|(1<<16);
}
void MenuWindowInput::Release(HWND window){
    if(Owned(window)){
        if(mouseDown)PostMessageW(window,WM_LBUTTONUP,0,Pixel(lastPoint));
    }
    mouseDown=false;lastEscape=false;
}
void MenuWindowInput::Update(HWND window,bool focused,const POINT* point,bool pressed,bool escape,ULONGLONG now){
    if(!focused||!Owned(window)){Release(window);return;}
    lastUpdate=now;
    const bool down=point && pressed;
    if(point){lastPoint=*point;PostMessageW(window,WM_MOUSEMOVE,mouseDown?MK_LBUTTON:0,Pixel(lastPoint));}
    if(down!=mouseDown){
        if(PostMessageW(window,down?WM_LBUTTONDOWN:WM_LBUTTONUP,down?MK_LBUTTON:0,Pixel(lastPoint)))mouseDown=down;
    }
    // The native listener dispatches Escape on WM_CHAR (RVA 0x1733).
    // Send that one edge explicitly; no dependency on TranslateMessage or
    // physical keyboard state, and no synthetic key remains held.
    if(escape && !lastEscape)PostMessageW(window,WM_CHAR,VK_ESCAPE,downKey);
    lastEscape=escape;
}
}
