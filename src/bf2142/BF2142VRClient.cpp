#include "StereoSession.h"
#include "GpuFrameTransfer.h"
#include "NativeAntialiasing.h"
#include "NativeRenderCanvas.h"
#include "VrSettings.h"
#include <initializer_list>
#include "LegacyShaderMemory.h"
#include "NativeVehicle.h"
#include "NativeQueryGuard.h"
#include "ObserverProfile.h"
#include "multiplayer/NativeNetwork.h"
#include <string>
#include <windows.h>
#include <d3d9.h>
#include <MinHook.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>

// Native D3D9 connection with an opt-in BF2142 stereo session.
// No BF1942 internal offsets are used.
namespace {
HANDLE logFile = INVALID_HANDLE_VALUE;
SRWLOCK logLock = SRWLOCK_INIT;
SRWLOCK hookLock = SRWLOCK_INIT;
std::atomic<bool> started = false;
bool networkObserver = false;
bool gpuBackend = false;
unsigned requestedWorldSamples=0;bool runtimePacing=false;
std::atomic<bool> alternateImplementationLogged = false;
std::atomic<unsigned long> presentations = 0;
std::atomic<unsigned long> swapPresentations = 0;
std::atomic<unsigned long> scenes = 0;
std::atomic<unsigned long> resets = 0;
using Create9 = IDirect3D9* (WINAPI*)(UINT);
using CreateDevice = HRESULT (STDMETHODCALLTYPE*)(IDirect3D9*, UINT, D3DDEVTYPE,
    HWND, DWORD, D3DPRESENT_PARAMETERS*, IDirect3DDevice9**);
using Present = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*, const RECT*,
    const RECT*, HWND, const RGNDATA*);
using SwapPresent = HRESULT (STDMETHODCALLTYPE*)(IDirect3DSwapChain9*, const RECT*,
    const RECT*, HWND, const RGNDATA*, DWORD);
using Reset = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
using GetSwapChain = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*, UINT, IDirect3DSwapChain9**);
using CreateSwapChain = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*,
    D3DPRESENT_PARAMETERS*, IDirect3DSwapChain9**);
using EndScene = HRESULT (STDMETHODCALLTYPE*)(IDirect3DDevice9*);
Create9 originalCreate9 = nullptr;
CreateDevice originalCreateDevice = nullptr;
Present originalPresent = nullptr;
SwapPresent originalSwapPresent = nullptr;
Reset originalReset = nullptr;
GetSwapChain originalGetSwapChain = nullptr;
CreateSwapChain originalCreateSwapChain = nullptr;
EndScene originalEndScene = nullptr;
void* create9Target = nullptr;
void* createDeviceTarget = nullptr;
void* presentTarget = nullptr;
void* swapPresentTarget = nullptr;
void* resetTarget = nullptr;
void* getSwapChainTarget = nullptr;
void* createSwapChainTarget = nullptr;
void* endSceneTarget = nullptr;
using CreateEffect = HRESULT (WINAPI*)(IDirect3DDevice9*, const void*, UINT,
    const void*, void*, DWORD, void*, void**, void**);
CreateEffect originalCreateEffect=nullptr;
void* createEffectTarget=nullptr;
thread_local bool insidePresentation=false;
struct PresentationScope {
    bool outer = !insidePresentation;
    PresentationScope() { insidePresentation=true; }
    ~PresentationScope() { if (outer) insidePresentation=false; }
};

void Log(const char* format, ...) {
    char text[2048]{};
    va_list arguments;
    va_start(arguments, format);
    const int count = vsnprintf(text, sizeof(text), format, arguments);
    va_end(arguments);
    if (count <= 0 || logFile == INVALID_HANDLE_VALUE) return;
    const DWORD length = static_cast<DWORD>(count < static_cast<int>(sizeof(text)) ? count : sizeof(text) - 1);
    AcquireSRWLockExclusive(&logLock);
    DWORD written = 0;
    WriteFile(logFile, text, length, &written, nullptr);
    WriteFile(logFile, "\r\n", 2, &written, nullptr);
    ReleaseSRWLockExclusive(&logLock);
}
LONG CALLBACK DiagnosticException(EXCEPTION_POINTERS* fault) {
    if(fault->ExceptionRecord->ExceptionCode!=EXCEPTION_ACCESS_VIOLATION)return EXCEPTION_CONTINUE_SEARCH;
    HMODULE faultModule=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(fault->ContextRecord->Eip),&faultModule);
    if(faultModule && faultModule!=GetModuleHandleW(nullptr) && faultModule!=GetModuleHandleW(L"RendDX9_ori.dll") && faultModule!=GetModuleHandleW(L"BF2142VRClient.dll"))return EXCEPTION_CONTINUE_SEARCH;
    static std::atomic<bool> recorded=false;if(recorded.exchange(true))return EXCEPTION_CONTINUE_SEARCH;
    const auto describe=[](DWORD address) {
        HMODULE module=nullptr;wchar_t path[MAX_PATH]{};
        if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(address),&module)) {
            GetModuleFileNameW(module,path,MAX_PATH);
            const wchar_t* name=wcsrchr(path,L'\\');if(!name)name=path;else ++name;
            Log("Diagnostic stack: %ls+0x%08lX",name,address-reinterpret_cast<DWORD>(module));
        }
    };
    Log("Diagnostic exception 0x%08lX; ECX=%08lX",fault->ExceptionRecord->ExceptionCode,fault->ContextRecord->Ecx);
    describe(fault->ContextRecord->Eip);
    DWORD frame=fault->ContextRecord->Ebp;
    __try {
        for(int i=0;i<16;++i) {
            const auto* words=reinterpret_cast<const DWORD*>(frame);describe(words[1]);
            if(words[0]<=frame || words[0]-frame>1024*1024)break;frame=words[0];
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
    return EXCEPTION_CONTINUE_SEARCH;
}
bool Hook(void* target, void* detour, void** original, void*& installedTarget) {
    AcquireSRWLockExclusive(&hookLock);
    if (installedTarget) {
        const bool same = installedTarget == target;
        ReleaseSRWLockExclusive(&hookLock);
        if (!same && !alternateImplementationLogged.exchange(true))
            Log("A different D3D9 implementation was encountered; left it unchanged.");
        return same;
    }
    MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status == MH_OK) {
        status = MH_EnableHook(target);
        if (status != MH_OK) MH_RemoveHook(target);
    }
    if (status == MH_OK) installedTarget = target;
    ReleaseSRWLockExclusive(&hookLock);
    if (status != MH_OK) Log("Hook not installed: %s", MH_StatusToString(status));
    return status == MH_OK;
}
// D3DX can fail without an error-message buffer; the retail loader then
// dereferences that null buffer. Preserve the API result, but record the real
// failure before that secondary crash erases it. No per-frame sampling.
HRESULT WINAPI CreateEffectHook(IDirect3DDevice9* device,const void* data,UINT size,
    const void* defines,void* includes,DWORD flags,void* pool,void** effect,void** errors) {
    const HRESULT result=originalCreateEffect(device,data,size,defines,includes,flags,pool,effect,errors);
    if(FAILED(result)) {
        MEMORYSTATUSEX memory{};memory.dwLength=sizeof(memory);GlobalMemoryStatusEx(&memory);
        Log("Shader creation failed: hr=0x%08lX bytes=%u flags=0x%08lX thread=%lu effect=%p errors=%p available-VA-MiB=%llu available-physical-MiB=%llu device-state=0x%08lX",
            static_cast<unsigned long>(result),size,flags,GetCurrentThreadId(),effect?*effect:nullptr,errors?*errors:nullptr,
            memory.ullAvailVirtual/(1024*1024),memory.ullAvailPhys/(1024*1024),
            static_cast<unsigned long>(device?device->TestCooperativeLevel():E_POINTER));
    }
    return result;
}
void ConnectShaderFailureLog() {
    const auto module=GetModuleHandleW(L"d3dx9_29.dll");
    bfvr::bf2142::InstallLegacyShaderMemory(module,Log);
    const auto entry=module?GetProcAddress(module,"D3DXCreateEffect"):nullptr;
    if(entry) Hook(reinterpret_cast<void*>(entry),reinterpret_cast<void*>(&CreateEffectHook),
        reinterpret_cast<void**>(&originalCreateEffect),createEffectTarget);
}
void CountBoundary(std::atomic<unsigned long>& counter, const char* name, HRESULT result) {
    const auto count = counter.fetch_add(1, std::memory_order_relaxed) + 1;
    // These are API call counts, not distinct frame counts: a device Present
    // may itself call swap-chain Present, and a frame may contain several scenes.
    if (count == 1 || count == 120)
        Log("%s count=%lu result=0x%08lX", name, count, static_cast<unsigned long>(result));
}
HRESULT STDMETHODCALLTYPE PresentHook(IDirect3DDevice9* device, const RECT* source,
    const RECT* destination, HWND window, const RGNDATA* dirty) {
    PresentationScope scope;
    if (scope.outer && bfvr::bf2142::SuppressStereoPresent(device)) return S_OK;
    if (scope.outer && networkObserver) bfvr::bf2142::TickNetworkClient();
    if (scope.outer) bfvr::bf2142::StereoPresent(device);
    const HRESULT result = originalPresent(device, source, destination, window, dirty);
    CountBoundary(presentations, "Present", result);
    return result;
}
HRESULT STDMETHODCALLTYPE SwapPresentHook(IDirect3DSwapChain9* chain, const RECT* source,
    const RECT* destination, HWND window, const RGNDATA* dirty, DWORD flags) {
    PresentationScope scope;
    if (scope.outer) {
        IDirect3DDevice9* device=nullptr;
        IDirect3DSwapChain9* primary=nullptr;
        if (SUCCEEDED(chain->GetDevice(&device))) {
            if (SUCCEEDED(device->GetSwapChain(0,&primary))) {
                const bool isPrimary=primary==chain;
                primary->Release();
                if (isPrimary && bfvr::bf2142::SuppressStereoPresent(device)) { device->Release(); return S_OK; }
                if (isPrimary && networkObserver) bfvr::bf2142::TickNetworkClient();
                if (isPrimary) bfvr::bf2142::StereoPresent(device);
            }
            device->Release();
        }
    }
    const HRESULT result = originalSwapPresent(chain, source, destination, window, dirty, flags);
    CountBoundary(swapPresentations, "SwapChainPresent", result);
    return result;
}
bool ConnectSwapChain(IDirect3DSwapChain9* chain) {
    if (!chain) return false;
    void** table = *reinterpret_cast<void***>(chain);
    return Hook(table[3], reinterpret_cast<void*>(&SwapPresentHook),
        reinterpret_cast<void**>(&originalSwapPresent), swapPresentTarget);
}
bool ConnectDefaultSwapChain(IDirect3DDevice9* device) {
    IDirect3DSwapChain9* chain = nullptr;
    if (FAILED(device->GetSwapChain(0, &chain)) || !chain) return false;
    const bool connected = ConnectSwapChain(chain);
    // Do not retain swap-chain references: they would prevent a later Reset.
    chain->Release();
    return connected;
}
HRESULT STDMETHODCALLTYPE GetSwapChainHook(IDirect3DDevice9* device, UINT index,
    IDirect3DSwapChain9** output) {
    const HRESULT result = originalGetSwapChain(device, index, output);
    if (SUCCEEDED(result) && output && *output) ConnectSwapChain(*output);
    return result;
}
HRESULT STDMETHODCALLTYPE CreateSwapChainHook(IDirect3DDevice9* device,
    D3DPRESENT_PARAMETERS* parameters, IDirect3DSwapChain9** output) {
    const HRESULT result = originalCreateSwapChain(device, parameters, output);
    if (SUCCEEDED(result) && output && *output) ConnectSwapChain(*output);
    return result;
}
HRESULT STDMETHODCALLTYPE EndSceneHook(IDirect3DDevice9* device) {
    const HRESULT result = originalEndScene(device);
    CountBoundary(scenes, "EndScene", result);
    return result;
}
HRESULT STDMETHODCALLTYPE ResetHook(IDirect3DDevice9* device,
    D3DPRESENT_PARAMETERS* parameters) {
    bfvr::bf2142::StereoReset();
    D3DDEVICE_CREATION_PARAMETERS canvasCreation{};device->GetCreationParameters(&canvasCreation);
    bfvr::bf2142::ConfigureRenderCanvas(parameters,canvasCreation.hFocusWindow);
    D3DPRESENT_PARAMETERS saved{};bool changed=false;
    if(parameters){saved=*parameters;IDirect3D9* api=nullptr;D3DDEVICE_CREATION_PARAMETERS creation{};
        if(SUCCEEDED(device->GetDirect3D(&api))){
            if(SUCCEEDED(device->GetCreationParameters(&creation)))changed=bfvr::bf2142::SelectWorldSamples(api,creation.AdapterOrdinal,creation.DeviceType,requestedWorldSamples,*parameters);
            api->Release();}}
    if(parameters&&runtimePacing&&parameters->PresentationInterval!=D3DPRESENT_INTERVAL_IMMEDIATE){parameters->PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;changed=true;}
    HRESULT result = originalReset(device, parameters);
    if(FAILED(result)&&changed){*parameters=saved;result=originalReset(device,parameters);Log("VR MSAA reset fallback: original native settings retained.");}
    if(SUCCEEDED(result)&&parameters)Log("World AA reset: samples=%u quality=%lu.",unsigned(parameters->MultiSampleType),parameters->MultiSampleQuality);
    const auto count = resets.fetch_add(1, std::memory_order_relaxed) + 1;
    if (count <= 16) Log("Reset result=0x%08lX size=%ux%u windowed=%d",
        static_cast<unsigned long>(result), parameters ? parameters->BackBufferWidth : 0,
        parameters ? parameters->BackBufferHeight : 0, parameters ? parameters->Windowed : 0);
    if (SUCCEEDED(result)) {ConnectDefaultSwapChain(device);bfvr::bf2142::ConfirmRenderCanvas(device);}
    return result;
}
HRESULT STDMETHODCALLTYPE CreateDeviceHook(IDirect3D9* factory, UINT adapter,
    D3DDEVTYPE type, HWND window, DWORD behavior,
    D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** output) {
    // The interop overlays query and restore state; pure devices cannot do that.
    if(gpuBackend){behavior&=~D3DCREATE_PUREDEVICE;bfvr::bf2142::ConfigureGpuDiagnostics(Log);}
    bfvr::bf2142::ConfigureRenderCanvas(parameters,window);
    D3DPRESENT_PARAMETERS saved{};bool changed=false;
    if(parameters){saved=*parameters;changed=bfvr::bf2142::SelectWorldSamples(factory,adapter,type,requestedWorldSamples,*parameters);}
    if(parameters&&runtimePacing&&parameters->PresentationInterval!=D3DPRESENT_INTERVAL_IMMEDIATE){parameters->PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;changed=true;}
    HRESULT result = originalCreateDevice(factory, adapter, type, window, behavior, parameters, output);
    if(FAILED(result)&&changed){*parameters=saved;result=originalCreateDevice(factory,adapter,type,window,behavior,parameters,output);Log("VR MSAA creation fallback: original native settings retained.");}
    if(SUCCEEDED(result)&&parameters)Log("World AA creation: samples=%u quality=%lu; autoDepth=%d depthFormat=%u swap=%u; nativeSamples=%u requested=%u.",unsigned(parameters->MultiSampleType),parameters->MultiSampleQuality,parameters->EnableAutoDepthStencil,unsigned(parameters->AutoDepthStencilFormat),unsigned(parameters->SwapEffect),unsigned(saved.MultiSampleType),requestedWorldSamples);
    Log("CreateDevice result=0x%08lX adapter=%u size=%ux%u windowed=%d flags=0x%08lX",
        static_cast<unsigned long>(result), adapter, parameters ? parameters->BackBufferWidth : 0,
        parameters ? parameters->BackBufferHeight : 0, parameters ? parameters->Windowed : 0, behavior);
    if (SUCCEEDED(result) && output && *output) {
        bfvr::bf2142::ConfirmRenderCanvas(*output);
        ConnectShaderFailureLog();
        if (networkObserver) {
            auto renderer=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9_ori.dll"));
            if(!renderer)renderer=reinterpret_cast<BYTE*>(GetModuleHandleW(L"RendDX9.dll"));
            if(!bfvr::bf2142::InstallNativeQueryGuard(renderer,Log))Log("Observer renderer query guard unavailable: profile mismatch.");
            bfvr::bf2142::InstallNetworkObserver(Log);
        }
        else bfvr::bf2142::InstallNativeVehicle(Log);
        bfvr::bf2142::StereoDeviceCreated(*output);
        void** table = *reinterpret_cast<void***>(*output);
        const bool present = Hook(table[17], reinterpret_cast<void*>(&PresentHook),
            reinterpret_cast<void**>(&originalPresent), presentTarget);
        const bool reset = Hook(table[16], reinterpret_cast<void*>(&ResetHook),
            reinterpret_cast<void**>(&originalReset), resetTarget);
        const bool getSwap = Hook(table[14], reinterpret_cast<void*>(&GetSwapChainHook),
            reinterpret_cast<void**>(&originalGetSwapChain), getSwapChainTarget);
        const bool createSwap = Hook(table[13], reinterpret_cast<void*>(&CreateSwapChainHook),
            reinterpret_cast<void**>(&originalCreateSwapChain), createSwapChainTarget);
        const bool endScene = Hook(table[42], reinterpret_cast<void*>(&EndSceneHook),
            reinterpret_cast<void**>(&originalEndScene), endSceneTarget);
        const bool swapPresent = ConnectDefaultSwapChain(*output);
        Log("Device connection: Present=%d Reset=%d SwapChainPresent=%d GetSwapChain=%d CreateSwapChain=%d EndScene=%d",
            present, reset, swapPresent, getSwap, createSwap, endScene);
    }
    return result;
}
IDirect3D9* WINAPI Create9Hook(UINT version) {
    IDirect3D9* factory = gpuBackend ? bfvr::bf2142::CreateGpuTransferFactory(version) : nullptr;
    if(gpuBackend)Log("Experimental D3D9On12 factory: %s",factory?"active":"unavailable; ordinary D3D9 retained");
    if(!factory)factory=originalCreate9(version);
    Log("Direct3DCreate9 sdk=%u success=%d", version, factory != nullptr);
    if (factory) {
        void** table = *reinterpret_cast<void***>(factory);
        Hook(table[16], reinterpret_cast<void*>(&CreateDeviceHook),
            reinterpret_cast<void**>(&originalCreateDevice), createDeviceTarget);
    }
    return factory;
}
}
extern "C" DWORD WINAPI BF2142VRInitialize(void* parameter) {
    if (!parameter || started.exchange(true)) return 0;
    const std::wstring configuration=static_cast<const wchar_t*>(parameter);
    const auto separator=configuration.find(L'\n');
    const std::wstring logPath=configuration.substr(0,separator);
    const auto profileSeparator=separator==std::wstring::npos?std::wstring::npos:configuration.find(L'\n',separator+1);
    const std::wstring presenter=separator==std::wstring::npos?L"":configuration.substr(separator+1,profileSeparator==std::wstring::npos?std::wstring::npos:profileSeparator-separator-1);
    const std::wstring observerProfile=profileSeparator==std::wstring::npos?L"":configuration.substr(profileSeparator+1);
    logFile = CreateFileW(logPath.c_str(), GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (logFile == INVALID_HANDLE_VALUE) return 0;
    if(presenter==L"@diagnostic")AddVectoredExceptionHandler(0,DiagnosticException);
    Log("BF2142VR v30; pid=%lu; stereo-requested=%d; native input retained", GetCurrentProcessId(),!presenter.empty());
    HMODULE runtime = LoadLibraryW(L"d3d9.dll");
    if (!runtime) { Log("D3D9 load failed: %lu", GetLastError()); return 0; }
    wchar_t runtimePath[32768]{};
    GetModuleFileNameW(runtime, runtimePath, 32768);
    char utf8[32768]{};
    WideCharToMultiByte(CP_UTF8, 0, runtimePath, -1, utf8, sizeof(utf8), nullptr, nullptr);
    Log("D3D9 runtime: %s", utf8);
    const auto entry = GetProcAddress(runtime, "Direct3DCreate9");
    if (!entry) { Log("Direct3DCreate9 is unavailable."); return 0; }
    const MH_STATUS status = MH_Initialize();
    if (status != MH_OK) { Log("MinHook init failed: %s", MH_StatusToString(status)); return 0; }
    if(!observerProfile.empty()){
        if(presenter!=L"@observer"||!bfvr::bf2142::InstallObserverProfile(observerProfile)){Log("Observer profile isolation failed; new child will not start.");return 0;}
        Log("Observer Documents isolated; exclusive per-profile lease held.");
    }
    if (!Hook(reinterpret_cast<void*>(entry), reinterpret_cast<void*>(&Create9Hook),
        reinterpret_cast<void**>(&originalCreate9), create9Target)) return 0;
    Log("Direct3DCreate9 connection installed.");
    networkObserver=presenter==L"@observer";
    gpuBackend=!presenter.empty()&&!networkObserver&&bfvr::bf2142::GpuTransferRequested();
    if(!networkObserver&&!bfvr::bf2142::InstallRenderCanvas(Log))return 0;
    runtimePacing=!presenter.empty()&&presenter.front()!=L'@';
    if(!presenter.empty()&&!networkObserver)requestedWorldSamples=bfvr::bf2142::LoadVrSettings(logPath).worldSamples;
    if (networkObserver) Log("Flat network observer requested: receive-only poses, native camera/input; no OpenXR.");
    if (!presenter.empty() && !networkObserver && !bfvr::bf2142::StartStereo(presenter,logPath,Log)) {
        Log("Headset session could not start: %lu",GetLastError()); return 0;
    }
    return 1;
}
// No custom DllMain: the static CRT retains its normal thread notifications.
