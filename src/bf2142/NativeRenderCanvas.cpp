#include "NativeRenderCanvas.h"
#include "RenderCanvasPolicy.h"
#include <MinHook.h>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
// Design reference: Gawkyorange5/BF2-VR res-fix (2026-10-01). This is the
// separately profiled BF2142 window path, not the BF2 startup/import hook.
CanvasSize canvas{};HWND mainWindow=nullptr;WNDPROC nativeProc=nullptr;LogFunction logLine=nullptr;
bool ClientMouse(UINT message) {
    switch(message){
    case WM_MOUSEMOVE:case WM_LBUTTONDOWN:case WM_LBUTTONUP:case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:case WM_RBUTTONUP:case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:case WM_MBUTTONUP:case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN:case WM_XBUTTONUP:case WM_XBUTTONDBLCLK:return true;
    default:return false;
    }
}
void FitPreview(HWND window,WINDOWPOS& position) {
    if((position.flags&SWP_NOSIZE)||position.cx<=0||position.cy<=0||IsIconic(window))return;
    MONITORINFO monitor{sizeof(monitor)};
    if(!GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&monitor))return;
    RECT frame{};if(!AdjustWindowRectEx(&frame,DWORD(GetWindowLongPtrW(window,GWL_STYLE)),
        GetMenu(window)!=nullptr,DWORD(GetWindowLongPtrW(window,GWL_EXSTYLE))))return;
    const int borderW=frame.right-frame.left,borderH=frame.bottom-frame.top;
    const auto work=monitor.rcWork;
    const double w=std::max(64L,work.right-work.left-borderW-16),h=std::max(64L,work.bottom-work.top-borderH-16);
    const double fit=std::min({1.,w/1600.,h/900.});
    position.cx=int(1600*fit)+borderW;position.cy=int(900*fit)+borderH;
    if(!(position.flags&SWP_NOMOVE)){
        position.x=work.left+(work.right-work.left-position.cx)/2;
        position.y=work.top+(work.bottom-work.top-position.cy)/2;
    }
}
LRESULT CALLBACK WindowHook(HWND window,UINT message,WPARAM w,LPARAM l) {
    // This profiled native procedure belongs to the game's main window. Do not
    // intercept auxiliary/child windows or another process's input.
    if(!mainWindow && message==WM_NCCREATE){
        const auto* create=reinterpret_cast<const CREATESTRUCTA*>(l);
        if(create&&!create->hwndParent&&!(create->style&WS_CHILD))mainWindow=window;
    }
    if(window==mainWindow){
        if(message==WM_WINDOWPOSCHANGING&&l)FitPreview(window,*reinterpret_cast<WINDOWPOS*>(l));
        if(message==WM_SIZE&&w!=SIZE_MINIMIZED)l=MAKELPARAM(canvas.width,canvas.height);
        if(ClientMouse(message)){
            RECT r{};if(GetClientRect(window,&r)&&r.right>1&&r.bottom>1)
                l=MAKELPARAM(CanvasCoordinate(short(LOWORD(l)),r.right,canvas.width),
                            CanvasCoordinate(short(HIWORD(l)),r.bottom,canvas.height));
        }
    }
    const LRESULT result=CallWindowProcA(nativeProc,window,message,w,l);
    if(window==mainWindow&&message==WM_CREATE&&result!=-1){
        // Initial geometry can be chosen before WM_NCCREATE. Fit after create
        // and explicitly publish the logical size even if no resize was sent.
        SetWindowPos(window,nullptr,0,0,1600,900,SWP_NOZORDER|SWP_NOACTIVATE);
        SendMessageA(window,WM_SIZE,SIZE_RESTORED,MAKELPARAM(canvas.width,canvas.height));
    }
    if(window==mainWindow&&message==WM_NCDESTROY)mainWindow=nullptr;
    return result;
}
bool Profile(BYTE* image) {
    __try {
        const BYTE entry[]={0x55,0x8b,0xec,0x56,0x8b,0x75,8,0x85,0xf6,0x75,7};
        // WM_SIZE dispatch entry 3 (message 5, table starts at message 2),
        // storing LOWORD/HIWORD at the native window object's +0x30/+0x34.
        const BYTE storeW[]={0x0f,0xb7,0xd1,0x89,0x56,0x30};
        const BYTE storeH[]={0xc1,0xe9,0x10,0x83,0xff,4,0x89,0x4a,0x34};
        return !std::memcmp(image+0x16bc40,entry,sizeof(entry)) &&
            *reinterpret_cast<BYTE**>(image+0x16be48+3*4)==image+0x16bc7f &&
            !std::memcmp(image+0x16bc88,storeW,sizeof(storeW)) &&
            !std::memcmp(image+0x16bc94,storeH,sizeof(storeH)) &&
            image[0x16be43]==0xc2&&image[0x16be44]==0x10&&image[0x16be45]==0;
    }__except(EXCEPTION_EXECUTE_HANDLER){return false;}
}
}
bool InstallRenderCanvas(LogFunction logger) {
    wchar_t value[64]{};const DWORD count=GetEnvironmentVariableW(L"BF2142VR_RENDER_CANVAS",value,64);
    if(!count)return GetLastError()==ERROR_ENVVAR_NOT_FOUND;
    canvas=count<64?ParseCanvas(value):CanvasSize{};logLine=logger;
    auto* image=reinterpret_cast<BYTE*>(GetModuleHandleW(nullptr));
    if(!canvas.width||!Profile(image)){logger("Render canvas refused: invalid request or unrecognized native window profile.");return false;}
    void* target=image+0x16bc40;
    if(MH_CreateHook(target,reinterpret_cast<void*>(&WindowHook),reinterpret_cast<void**>(&nativeProc))!=MH_OK)return false;
    if(MH_EnableHook(target)!=MH_OK){MH_RemoveHook(target);return false;}
    logger("Render canvas requested=%ux%u; profiled native WM_SIZE and client mouse mapping enabled before window creation.",canvas.width,canvas.height);
    return true;
}
void ConfigureRenderCanvas(D3DPRESENT_PARAMETERS* p,HWND window) {
    if(!p||!canvas.width||!mainWindow)return;
    HWND target=p->hDeviceWindow?p->hDeviceWindow:window;
    if(target!=mainWindow||!p->Windowed)return;
    p->BackBufferWidth=canvas.width;p->BackBufferHeight=canvas.height;
}
void ConfirmRenderCanvas(IDirect3DDevice9* device) {
    if(!canvas.width||!device)return;
    IDirect3DSurface9* back=nullptr;
    if(FAILED(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)))return;
    D3DSURFACE_DESC desc{};const HRESULT result=back->GetDesc(&desc);back->Release();
    RECT client{};GetClientRect(mainWindow,&client);
    if(SUCCEEDED(result))logLine("Render canvas actual=%ux%u requested=%ux%u matched=%d desktop=%ldx%ld samples=%u.",
        desc.Width,desc.Height,canvas.width,canvas.height,desc.Width==canvas.width&&desc.Height==canvas.height,
        client.right,client.bottom,unsigned(desc.MultiSampleType));
}
}
