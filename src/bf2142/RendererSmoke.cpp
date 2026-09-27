#include "FrameCapture.h"
#include "NativeAntialiasing.h"
#include "NativeUiCapture.h"
#include <MinHook.h>
#include <cstdarg>
#include <windows.h>
#include <d3d9.h>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
void UiLog(const char* format,...) {
    va_list args; va_start(args,format); vprintf(format,args); va_end(args); puts("");
}
bool CheckMsaaCoverage(IDirect3D9* factory,IDirect3DDevice9* device){
    IDirect3DSurface9* saved=nullptr;IDirect3DSurface9* depth=nullptr;IDirect3DStateBlock9* state=nullptr;
    if(FAILED(device->GetRenderTarget(0,&saved))||FAILED(device->CreateStateBlock(D3DSBT_ALL,&state))){if(saved)saved->Release();return false;}
    device->GetDepthStencilSurface(&depth);state->Capture();bool ok=true;unsigned exercised=0;
    struct Vertex{float x,y,z,rhw;DWORD color;};
    const Vertex triangle[]={{20.3f,17.6f,.5f,1,0xffffffff},{279.7f,206.2f,.5f,1,0xffffffff},{28.4f,219.1f,.5f,1,0xffffffff}};
    for(unsigned samples:{0u,2u,4u,8u}){
        D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.BackBufferFormat=D3DFMT_A8R8G8B8;
        p.EnableAutoDepthStencil=TRUE;p.AutoDepthStencilFormat=D3DFMT_D24S8;
        const bool selected=bfvr::bf2142::SelectWorldSamples(factory,0,D3DDEVTYPE_HAL,samples,p);
        if(samples&&(!selected||unsigned(p.MultiSampleType)!=samples))continue;
        IDirect3DSurface9* target=nullptr;
        if(FAILED(device->CreateRenderTarget(320,240,p.BackBufferFormat,p.MultiSampleType,0,FALSE,&target,nullptr))){ok=false;continue;}
        device->SetDepthStencilSurface(nullptr);device->SetRenderTarget(0,target);D3DVIEWPORT9 viewport{0,0,320,240,0,1};device->SetViewport(&viewport);
        device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);device->SetTexture(0,nullptr);
        device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
        for(auto setting:{D3DRS_ZENABLE,D3DRS_LIGHTING,D3DRS_ALPHABLENDENABLE,D3DRS_ALPHATESTENABLE,D3DRS_SCISSORTESTENABLE})device->SetRenderState(setting,FALSE);
        device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS,TRUE);
        ok=SUCCEEDED(device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff000000,1,0))&&ok;
        ok=SUCCEEDED(device->BeginScene())&&ok;ok=SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,triangle,sizeof(Vertex)))&&ok;ok=SUCCEEDED(device->EndScene())&&ok;
        bfvr::bf2142::FrameCapture capture;std::vector<DWORD> pixels;
        ok=SUCCEEDED(capture.ReadSurface(device,target,DXGI_FORMAT_B8G8R8A8_UNORM,pixels))&&ok;
        size_t blended=0;for(auto pixel:pixels){const DWORD value=pixel&255;blended+=value>0&&value<255;}
        printf("Native %ux AA: %zu partially covered edge pixels preserved in eye readback.\n",samples,blended);
        ok=(samples?blended>100:blended==0)&&ok;++exercised;
        device->SetRenderTarget(0,saved);target->Release();
    }
    device->SetRenderTarget(0,saved);device->SetDepthStencilSurface(depth);state->Apply();state->Release();saved->Release();if(depth)depth->Release();
    return ok&&exercised>=2;
}
int wmain() {
    wchar_t self[32768]{};
    GetModuleFileNameW(nullptr, self, 32768);
    const auto folder = std::filesystem::path(self).parent_path();
    const auto log = folder / (L"renderer-smoke-" + std::to_wstring(GetCurrentProcessId()) +
        L"-" + std::to_wstring(GetTickCount64()) + L".log");
    const auto library = LoadLibraryW((folder / L"BF2142VRClient.dll").c_str());
    if (!library) { puts("Client load failed."); return 1; }
    const auto initialize = reinterpret_cast<DWORD (WINAPI*)(void*)>(
        GetProcAddress(library, "BF2142VRInitialize"));
    if (!initialize || initialize(const_cast<wchar_t*>(log.c_str())) != 1) {
        puts("Client initialization failed."); return 1;
    }
    const auto window = CreateWindowExW(0, L"STATIC", L"BF2142VR hidden renderer check",
        WS_OVERLAPPED, 0, 0, 320, 240, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    const auto factory = Direct3DCreate9(D3D_SDK_VERSION);
    if (!window || !factory) return 1;
    for (UINT adapter = 0; adapter < factory->GetAdapterCount(); ++adapter) {
        D3DADAPTER_IDENTIFIER9 identity{};
        D3DCAPS9 caps{};
        factory->GetAdapterIdentifier(adapter, 0, &identity);
        factory->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps);
        printf("Adapter %u: %s; device=%s vendor=%04lX id=%04lX vertex=%08lX pixel=%08lX\n",
            adapter, identity.Description, identity.DeviceName, identity.VendorId,
            identity.DeviceId, caps.VertexShaderVersion, caps.PixelShaderVersion);
        D3DDISPLAYMODE mode{};
        factory->GetAdapterDisplayMode(adapter, &mode);
        printf("  Display mode=%ux%u@%u format=%u; RGB32 modes=%u RGB16 modes=%u\n",
            mode.Width, mode.Height, mode.RefreshRate, static_cast<unsigned>(mode.Format),
            factory->GetAdapterModeCount(adapter, D3DFMT_X8R8G8B8),
            factory->GetAdapterModeCount(adapter, D3DFMT_R5G6B5));
    }
    D3DPRESENT_PARAMETERS parameters{};
    parameters.Windowed = TRUE;
    parameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    parameters.hDeviceWindow = window;
    parameters.BackBufferWidth = 320;
    parameters.BackBufferHeight = 240;
    parameters.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    IDirect3DDevice9* device = nullptr;
    HRESULT result = factory->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window,
        D3DCREATE_SOFTWARE_VERTEXPROCESSING, &parameters, &device);
    if (FAILED(result)) { printf("GPU device unavailable: %08lX\n", static_cast<unsigned long>(result)); return 77; }
    bool ok = true;
    bfvr::bf2142::FrameCapture capture;
    std::vector<DWORD> pixels;
    for (int frame = 0; frame < 125; ++frame) {
        ok = SUCCEEDED(device->BeginScene()) && ok;
        ok = SUCCEEDED(device->Clear(0, nullptr, D3DCLEAR_TARGET, D3DCOLOR_XRGB(25, 50, 75), 1.0f, 0)) && ok;
        if (frame==0) {
            // Native eye capture can occur within the game's Begin/EndScene.
            const HRESULT read=capture.Read(device,DXGI_FORMAT_B8G8R8A8_UNORM,pixels);
            ok = SUCCEEDED(read) && pixels.size()==320u*240u && pixels.front()==0xff19324bu && ok;
            printf("Inside-scene BGRA capture: %08lX pixel=%08lX\n",static_cast<unsigned long>(read),pixels.empty()?0:pixels.front());
            const HRESULT rgba=capture.Read(device,DXGI_FORMAT_R8G8B8A8_UNORM_SRGB,pixels);
            ok = SUCCEEDED(rgba) && !pixels.empty() && pixels.front()==0xff4b3219u && ok;
        }
        ok = SUCCEEDED(device->EndScene()) && ok;
        ok = SUCCEEDED(device->Present(nullptr, nullptr, nullptr, nullptr)) && ok;
    }
    // A translucent interface must keep its alpha and leave the world out of
    // the UI texture; the normal monitor still receives their composition.
    bfvr::bf2142::NativeUiCapture ui;
    ok = MH_Initialize()==MH_OK && ok;
    ok = ui.Connect(device,UiLog) && ok;
    ok = SUCCEEDED(device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE)) && ok;
    ok = SUCCEEDED(device->BeginScene()) && ok;
    ok = SUCCEEDED(device->Clear(0,nullptr,D3DCLEAR_TARGET,D3DCOLOR_XRGB(25,50,75),1,0)) && ok;
    const bool began=ui.Begin(device); ok=began && ok;
    if (began) {
        // Rebinding the native backbuffer must remain redirected during UI.
        IDirect3DSurface9* back=nullptr;
        ok = SUCCEEDED(device->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)) && ok;
        if (back) { ok=SUCCEEDED(device->SetRenderTarget(0,back)) && ok; back->Release(); }
        D3DRECT rect{0,0,160,120};
        ok = SUCCEEDED(device->Clear(1,&rect,D3DCLEAR_TARGET,0x80402010,1,0)) && ok;
    }
    ok = SUCCEEDED(device->EndScene()) && ok;
    if (began) {
        ok = ui.Finish(DXGI_FORMAT_B8G8R8A8_UNORM,pixels) && ok;
        ok = pixels.size()==320u*240u && pixels.front()==0x80402010 && pixels.back()==0 && ok;
        printf("Transparent UI capture: first=%08lX last=%08lX\n",pixels.front(),pixels.back());
        ok = SUCCEEDED(capture.Read(device,DXGI_FORMAT_B8G8R8A8_UNORM,pixels)) && ok;
        const DWORD composite=pixels.front();
        const auto close=[](DWORD a,DWORD b) { return a>=b-1 && a<=b+1; };
        ok = close((composite>>16)&255,76) && close((composite>>8)&255,57) && close(composite&255,53) && ok;
        ok = pixels.back()==0xff19324b && ok;
        DWORD separate=TRUE;
        ok = SUCCEEDED(device->GetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,&separate)) && separate==FALSE && ok;
        printf("Desktop UI composite: %08lX; background=%08lX; alpha state=%lu\n",composite,pixels.back(),separate);
    }
    // HUD and Flash menus can be separate batches on an intermediate native
    // target. Capture both without clearing the first, changing the native
    // target, or losing its viewport when each batch detaches.
    IDirect3DSurface9* backTarget=nullptr;IDirect3DSurface9* intermediate=nullptr;IDirect3DSurface9* nativeDepth=nullptr;
    ok=SUCCEEDED(device->GetRenderTarget(0,&backTarget)) && ok;
    ok=SUCCEEDED(device->CreateRenderTarget(320,240,D3DFMT_A8R8G8B8,D3DMULTISAMPLE_4_SAMPLES,0,FALSE,&intermediate,nullptr)) && ok;
    if(intermediate && backTarget) {
        ok=SUCCEEDED(device->CreateDepthStencilSurface(320,240,D3DFMT_D24S8,D3DMULTISAMPLE_4_SAMPLES,0,TRUE,&nativeDepth,nullptr)) && ok;
        ok=SUCCEEDED(device->SetRenderTarget(0,intermediate)) && ok;
        ok=SUCCEEDED(device->SetDepthStencilSurface(nativeDepth)) && ok;
        ok=SUCCEEDED(device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff102030,1,0)) && ok;
        const D3DVIEWPORT9 nativeViewport{10,20,200,150,0,1};
        ok=SUCCEEDED(device->SetViewport(&nativeViewport)) && ok;
        for(int batch=0;batch<2;++batch) {
            ok=SUCCEEDED(device->BeginScene()) && ok;
            const bool active=ui.Begin(device,batch==0);ok=active && ok;
            if(active) {
                ok=!ui.Begin(device,false) && ui.Active() && ok;
                D3DRECT area=batch==0?D3DRECT{20,30,50,60}:D3DRECT{60,70,90,100};
                ok=SUCCEEDED(device->Clear(1,&area,D3DCLEAR_TARGET,batch==0?0xffaabbcc:0xff445566,1,0)) && ok;
                ok=ui.Detach() && ok;
                IDirect3DSurface9* restored=nullptr;D3DVIEWPORT9 viewport{};
                ok=SUCCEEDED(device->GetRenderTarget(0,&restored)) && restored==intermediate && ok;
                if(restored)restored->Release();
                ok=SUCCEEDED(device->GetViewport(&viewport)) && viewport.X==10 && viewport.Y==20 && viewport.Width==200 && viewport.Height==150 && ok;
            }
            ok=SUCCEEDED(device->EndScene()) && ok;
        }
        ok=ui.Read(DXGI_FORMAT_B8G8R8A8_UNORM,pixels) && ok;
        ok=pixels.size()==320u*240u && pixels[40*320+30]==0xffaabbcc && pixels[80*320+70]==0xff445566 && pixels.back()==0 && ok;
        ok=SUCCEEDED(capture.ReadSurface(device,intermediate,DXGI_FORMAT_B8G8R8A8_UNORM,pixels)) && ok;
        ok=pixels.front()==0xff102030 && pixels[40*320+30]==0xff102030 && pixels.back()==0xff102030 && ok;
        // Desktop composition must restore the intermediate target and its
        // viewport/cache agreement before the native renderer continues.
        ok=ui.CompositeDesktop() && ok;
        IDirect3DSurface9* restored=nullptr;D3DVIEWPORT9 viewport{};
        ok=SUCCEEDED(device->GetRenderTarget(0,&restored)) && restored==intermediate && ok;
        if(restored)restored->Release();
        ok=SUCCEEDED(device->GetViewport(&viewport)) && viewport.X==10 && viewport.Width==200 && ok;
        IDirect3DSurface9* restoredDepth=nullptr;
        ok=SUCCEEDED(device->GetDepthStencilSurface(&restoredDepth)) && restoredDepth==nativeDepth && ok;
        if(restoredDepth)restoredDepth->Release();
        ok=SUCCEEDED(device->SetDepthStencilSurface(nullptr)) && ok;
        ok=SUCCEEDED(device->SetRenderTarget(0,backTarget)) && ok;
        puts("4x MSAA batched HUD/menu isolation and native target/depth/viewport restoration exercised.");
    }
    if(intermediate)intermediate->Release();if(backTarget)backTarget->Release();if(nativeDepth)nativeDepth->Release();
    ui.Reset();
    IDirect3DSwapChain9* chain = nullptr;
    ok = SUCCEEDED(device->GetSwapChain(0, &chain)) && ok;
    if (chain) {
        for (int frame = 0; frame < 125; ++frame)
            ok = SUCCEEDED(chain->Present(nullptr, nullptr, nullptr, nullptr, 0)) && ok;
        chain->Release();
    } else ok = false;
    // Exercise the additional-chain path as well as the implicit primary chain.
    D3DPRESENT_PARAMETERS extraParameters = parameters;
    IDirect3DSwapChain9* extra = nullptr;
    ok = SUCCEEDED(device->CreateAdditionalSwapChain(&extraParameters, &extra)) && ok;
    if (extra) {
        ok = SUCCEEDED(extra->Present(nullptr, nullptr, nullptr, nullptr, 0)) && ok;
        extra->Release();
    } else ok = false;
    capture.Reset();
    parameters.BackBufferWidth = 400;
    parameters.BackBufferHeight = 300;
    // This also proves the hooks did not retain a swap chain and block Reset.
    ok = SUCCEEDED(device->Reset(&parameters)) && ok;
    ok = SUCCEEDED(device->Clear(0,nullptr,D3DCLEAR_TARGET,D3DCOLOR_XRGB(80,90,100),1,0)) && ok;
    ok = SUCCEEDED(capture.Read(device,DXGI_FORMAT_B8G8R8A8_UNORM,pixels)) && ok;
    ok = pixels.size()==400u*300u && pixels.front()==0xff505a64u && ok;
    capture.Reset();
    ok = SUCCEEDED(device->Present(nullptr, nullptr, nullptr, nullptr)) && ok;
    chain = nullptr;
    ok = SUCCEEDED(device->GetSwapChain(0, &chain)) && ok;
    if (chain) {
        ok = SUCCEEDED(chain->Present(nullptr, nullptr, nullptr, nullptr, 0)) && ok;
        chain->Release();
    } else ok = false;
    ok=CheckMsaaCoverage(factory,device)&&ok;
    device->Release();
    factory->Release();
    DestroyWindow(window);
    // Hooks remain installed until process exit. Do not unload the DLL here.
    std::ifstream input(log, std::ios::binary);
    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    for (const auto* marker : {"Direct3DCreate9 connection installed.", "Device connection: Present=1 Reset=1",
         "Reset result=0x00000000", "EndScene count=120 result=0x00000000",
         "SwapChainPresent=1 GetSwapChain=1 CreateSwapChain=1 EndScene=1"}) {
        if (text.find(marker) == std::string::npos) { printf("Missing: %s\n", marker); ok = false; }
    }
    // A hidden D3D9Ex window may report S_PRESENT_OCCLUDED. It is a
    // successful presentation status; all actual API calls above also check
    // SUCCEEDED. Keep the hook coverage check without requiring visibility.
    for (const auto* marker : {"Present count=1 result=", "Present count=120 result=",
         "SwapChainPresent count=1 result=", "SwapChainPresent count=120 result="}) {
        const auto success=std::string(marker)+"0x00000000";
        const auto occluded=std::string(marker)+"0x08760878";
        if(text.find(success)==std::string::npos && text.find(occluded)==std::string::npos) {
            printf("Missing successful presentation: %s\n",marker);ok=false;
        }
    }
    wprintf(L"Renderer smoke log: %ls\n", log.c_str());
    puts(ok ? "D3D9 presentation, in-scene eye readback/color conversion and Reset passed." : "D3D9 check failed.");
    return ok ? 0 : 1;
}
