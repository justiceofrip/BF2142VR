#include "NativeRenderCanvas.cpp"
#include <cstdio>
#include <vector>
using namespace bfvr::bf2142;
namespace {
LPARAM observedSize=0,observedMouse=0,observedWheel=0;WPARAM sizeKind=0;
LRESULT CALLBACK Recorder(HWND w,UINT m,WPARAM a,LPARAM b){
    if(m==WM_SIZE){observedSize=b;sizeKind=a;}
    if(m==WM_MOUSEMOVE)observedMouse=b;
    if(m==WM_MOUSEWHEEL)observedWheel=b;
    return DefWindowProcA(w,m,a,b);
}
void Log(const char*,...){}
}
int main(int argc,char**){
    int failures=0;const auto check=[&](bool ok,const char* name){if(!ok){++failures;printf("FAIL %s\n",name);}};
    std::vector<BYTE> image(0x16be60);
    const BYTE entry[]={0x55,0x8b,0xec,0x56,0x8b,0x75,8,0x85,0xf6,0x75,7};
    const BYTE a[]={0x0f,0xb7,0xd1,0x89,0x56,0x30},b[]={0xc1,0xe9,0x10,0x83,0xff,4,0x89,0x4a,0x34};
    memcpy(image.data()+0x16bc40,entry,sizeof(entry));memcpy(image.data()+0x16bc88,a,sizeof(a));memcpy(image.data()+0x16bc94,b,sizeof(b));
    *reinterpret_cast<BYTE**>(image.data()+0x16be54)=image.data()+0x16bc7f;
    image[0x16be43]=0xc2;image[0x16be44]=0x10;
    check(Profile(image.data()),"native window signature and dispatch recognized");
    image[0x16bc94]^=1;check(!Profile(image.data()),"changed native size handler refused");image[0x16bc94]^=1;
    *reinterpret_cast<BYTE**>(image.data()+0x16be54)=image.data()+0x16bc80;check(!Profile(image.data()),"different message dispatch refused");
    nativeProc=Recorder;canvas={2528,2704};logLine=Log;
    WNDCLASSA cls{};cls.hInstance=GetModuleHandleW(nullptr);cls.lpszClassName="CanvasVerification";cls.lpfnWndProc=WindowHook;
    check(RegisterClassA(&cls)!=0,"test class registered");
    HWND window=CreateWindowExA(0,cls.lpszClassName,"Canvas verification",WS_OVERLAPPEDWINDOW,0,0,3000,3000,nullptr,nullptr,cls.hInstance,nullptr);
    check(window!=nullptr&&mainWindow==window,"main window adopted before native initialization");
    RECT client{};GetClientRect(window,&client);
    check(client.right<=1600&&client.bottom<=900,"preview fits monitor independently");
    check(LOWORD(observedSize)==2528&&HIWORD(observedSize)==2704,"native size remains render canvas");
    SendMessageA(window,WM_MOUSEMOVE,0,MAKELPARAM(client.right-1,client.bottom-1));
    check(LOWORD(observedMouse)==2527&&HIWORD(observedMouse)==2703,"window cursor reaches exact render corner");
    SendMessageA(window,WM_MOUSEWHEEL,0,MAKELPARAM(400,300));check(observedWheel==MAKELPARAM(400,300),"wheel screen coordinates untouched");
    SendMessageA(window,WM_SIZE,SIZE_MINIMIZED,0);check(sizeKind==SIZE_MINIMIZED&&observedSize==0,"native minimize notification preserved");
    SendMessageA(window,WM_SIZE,SIZE_RESTORED,MAKELPARAM(800,600));check(observedSize==MAKELPARAM(2528,2704),"restore retains logical canvas");
    D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.hDeviceWindow=window;
    ConfigureRenderCanvas(&p);check(p.BackBufferWidth==2528&&p.BackBufferHeight==2704,"zero-sized windowed backbuffer made explicit");
    p.BackBufferWidth=800;p.BackBufferHeight=600;ConfigureRenderCanvas(&p,window);
    check(p.BackBufferWidth==2528&&p.BackBufferHeight==2704,"reset dimensions stay coherent");
    p.hDeviceWindow=reinterpret_cast<HWND>(1);p.BackBufferWidth=123;ConfigureRenderCanvas(&p);check(p.BackBufferWidth==123,"auxiliary device unchanged");
    if(argc>1){
        IDirect3D9* api=Direct3DCreate9(D3D_SDK_VERSION);IDirect3DDevice9* device=nullptr;
        p={};p.Windowed=TRUE;p.hDeviceWindow=window;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.BackBufferFormat=D3DFMT_X8R8G8B8;
        p.EnableAutoDepthStencil=TRUE;p.AutoDepthStencilFormat=D3DFMT_D24S8;p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
        ConfigureRenderCanvas(&p,window);
        HRESULT hr=api?api->CreateDevice(0,D3DDEVTYPE_HAL,window,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,&device):E_FAIL;
        check(SUCCEEDED(hr),"real high-resolution D3D9 device created");
        if(device){
            const auto verify=[&](){IDirect3DSurface9* back=nullptr;D3DSURFACE_DESC desc{};D3DVIEWPORT9 viewport{};
                device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back);if(back){back->GetDesc(&desc);back->Release();}
                device->GetViewport(&viewport);check(desc.Width==2528&&desc.Height==2704&&viewport.Width==2528&&viewport.Height==2704,"actual backbuffer and viewport match requested source");};
            verify();p.BackBufferWidth=p.BackBufferHeight=0;ConfigureRenderCanvas(&p,window);
            check(SUCCEEDED(device->Reset(&p)),"native reset uses explicit canvas");verify();device->Release();
        }
        if(api)api->Release();
    }
    if(window)DestroyWindow(window);check(mainWindow==nullptr,"window destruction releases association");UnregisterClassA(cls.lpszClassName,cls.hInstance);
    if(!failures)puts("Canvas profile/window/input/device checks passed.");return failures?1:0;
}
