#include "ControllerInput.h"
#include "InputOverlay.h"
#include "DesktopSimulation.h"
#include "NativeHands.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <MinHook.h>
#include <algorithm>
#include <cstring>
namespace bfvr::bf2142 {
namespace {
using CreateInput=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
using CreateDevice=HRESULT(STDMETHODCALLTYPE*)(void*,REFGUID,void**,IUnknown*);
using GetState=HRESULT(STDMETHODCALLTYPE*)(void*,DWORD,void*);
using GetData=HRESULT(STDMETHODCALLTYPE*)(void*,DWORD,DIDEVICEOBJECTDATA*,DWORD*,DWORD);
CreateInput originalCreate=nullptr;CreateDevice originalDevice=nullptr;
struct Device { void* object=nullptr;bool keyboard=false;GetState state=nullptr;GetData data=nullptr;void* stateTarget=nullptr;void* dataTarget=nullptr;
    InputOverlayState overlay{}; };
Device devices[2];
SRWLOCK commandLock=SRWLOCK_INIT;
ControllerCommand desired{};ULONGLONG publishedAt=0;DWORD generation=0;bool active=false;
LogFunction logLine=nullptr;bool desktopInput=false;
bool Snapshot(ControllerCommand& out,DWORD& serial) {
    DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
    if(foregroundPid!=GetCurrentProcessId())return false;
    if(!TryAcquireSRWLockShared(&commandLock))return false;
    const bool valid=active && GetTickCount64()-publishedAt<=150;
    if(valid){out=desired;serial=generation;}
    ReleaseSRWLockShared(&commandLock);
    if(valid&&out.selection.item){
        std::array<bool,10> present{};int equipped=0;std::uint64_t owner=0;
        out.selection.allowed=ReadNativeInventory(&present,&equipped,nullptr,&owner)&&owner==out.selection.owner&&
            out.selection.item<present.size()&&present[out.selection.item]&&int(out.selection.item)!=equipped;
    }
    return valid;
}
Device* Find(void* object) {for(auto& d:devices)if(d.object==object)return &d;return nullptr;}
// Each DInput device implementation has a separate trampoline when needed.
template<int Slot> HRESULT STDMETHODCALLTYPE StateHook(void* object,DWORD bytes,void* data) {
    auto& hook=devices[Slot];const HRESULT result=hook.state(object,bytes,data);
    auto* d=Find(object);if(FAILED(result) || !data || !d)return result;
    ControllerCommand c{};DWORD serial=0;Snapshot(c,serial);if(desktopInput)BlockDesktopHotkeys(c);
    OverlayDeviceState(d->overlay,d->keyboard,bytes,data,c,serial);
    return result;
}
template<int Slot> HRESULT STDMETHODCALLTYPE DataHook(void* object,DWORD stride,DIDEVICEOBJECTDATA* data,DWORD* count,DWORD flags) {
    auto& hook=devices[Slot];const DWORD capacity=count?*count:0;
    const HRESULT result=hook.data(object,stride,data,count,flags);
    auto* d=Find(object);
    if(FAILED(result) || !d || !count || !data || stride<16 || stride>sizeof(DIDEVICEOBJECTDATA) || *count>capacity)return result;
    ControllerCommand c{};DWORD serial=0;Snapshot(c,serial);if(desktopInput)BlockDesktopHotkeys(c);
    OverlayDeviceEvents(d->overlay,d->keyboard,stride,data,*count,capacity,(flags&DIGDD_PEEK)!=0,c,serial,GetTickCount());
    return result;
}
HRESULT STDMETHODCALLTYPE DeviceHook(void* input,REFGUID guid,void** output,IUnknown* outer) {
    const HRESULT hr=originalDevice(input,guid,output,outer);
    const bool keyboard=IsEqualGUID(guid,GUID_SysKeyboard)!=0,mouse=IsEqualGUID(guid,GUID_SysMouse)!=0;
    if(FAILED(hr) || !output || !*output || (!keyboard && !mouse))return hr;
    const int slot=keyboard?0:1;auto& d=devices[slot];auto** table=*reinterpret_cast<void***>(*output);
    // A recreated standard device must use the same validated API implementation.
    if(d.stateTarget) {if(d.stateTarget==table[9] && d.dataTarget==table[10]){d.object=*output;d.overlay={};}return hr;}
    d.object=*output;d.keyboard=keyboard;
    for(int other=0;other<2;++other)if(other!=slot && devices[other].stateTarget==table[9] && devices[other].dataTarget==table[10]) {
        d.stateTarget=table[9];d.dataTarget=table[10];d.state=devices[other].state;d.data=devices[other].data;return hr;
    }
    const auto stateHook=slot==0?&StateHook<0>:&StateHook<1>;
    const auto dataHook=slot==0?&DataHook<0>:&DataHook<1>;
    if(MH_CreateHook(table[9],reinterpret_cast<void*>(stateHook),reinterpret_cast<void**>(&d.state))!=MH_OK)return hr;
    if(MH_CreateHook(table[10],reinterpret_cast<void*>(dataHook),reinterpret_cast<void**>(&d.data))!=MH_OK) {MH_RemoveHook(table[9]);return hr;}
    if(MH_EnableHook(table[9])!=MH_OK || MH_EnableHook(table[10])!=MH_OK) {
        MH_DisableHook(table[9]);MH_DisableHook(table[10]);MH_RemoveHook(table[9]);MH_RemoveHook(table[10]);return hr;
    }
    d.stateTarget=table[9];d.dataTarget=table[10];logLine("Controller input connected to game %s device.",keyboard?"keyboard":"mouse");return hr;
}
HRESULT WINAPI CreateHook(HINSTANCE instance,DWORD version,REFIID iid,LPVOID* output,LPUNKNOWN outer) {
    const HRESULT hr=originalCreate(instance,version,iid,output,outer);
    if(SUCCEEDED(hr) && output && *output && !originalDevice) {
        auto** table=*reinterpret_cast<void***>(*output);
        if(MH_CreateHook(table[3],reinterpret_cast<void*>(&DeviceHook),reinterpret_cast<void**>(&originalDevice))==MH_OK && MH_EnableHook(table[3])!=MH_OK) {MH_RemoveHook(table[3]);originalDevice=nullptr;}
    }
    return hr;
}
}
void SetDesktopInput(bool enabled){desktopInput=enabled;}
bool StartControllerInput(LogFunction logger) {
    logLine=logger;
    const auto runtime=LoadLibraryExW(L"dinput8.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);if(!runtime)return false;
    const auto entry=GetProcAddress(runtime,"DirectInput8Create");if(!entry)return false;
    if(MH_CreateHook(reinterpret_cast<void*>(entry),reinterpret_cast<void*>(&CreateHook),reinterpret_cast<void**>(&originalCreate))!=MH_OK)return false;
    if(MH_EnableHook(reinterpret_cast<void*>(entry))!=MH_OK){MH_RemoveHook(reinterpret_cast<void*>(entry));return false;}
    logger("Game-local controller input enabled; focus and 150 ms freshness required.");return true;
}
void PublishControllerCommand(const ControllerCommand& c,bool enabled) {
    AcquireSRWLockExclusive(&commandLock);desired=c;active=enabled;publishedAt=GetTickCount64();++generation;ReleaseSRWLockExclusive(&commandLock);
}
void ClearControllerCommand(){PublishControllerCommand({},false);}
}
