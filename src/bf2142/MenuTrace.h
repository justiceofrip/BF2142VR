#pragma once
#include <windows.h>
#include <psapi.h>
#include <atomic>
#include <cstdint>
#include <cwchar>
#include <vector>
namespace bfvr::bf2142::menuTrace {
// Explicit private diagnostics only. Never record text, keys, account data or
// image content; fingerprints only distinguish changing from stale UI images.
using Logger=void(*)(const char*,...);
inline Logger logger=nullptr;
inline bool enabled=false;
inline std::atomic<unsigned> drawBegin{0},drawEnd{0},queuedDown{0},queuedUp{0};
inline std::atomic<unsigned> deliveredDown{0},deliveredUp{0},returnedDown{0},returnedUp{0};
inline unsigned frames=0,changed=0,acceptedRequests=0,requests=0;
inline uint64_t lastFingerprint=0;
inline ULONGLONG lastReport=0;
inline void Initialize(Logger log) {
    wchar_t flag[8]{},level[16]{};
    const DWORD count=GetEnvironmentVariableW(L"BF2142VR_MENU_TRACE",flag,8);
    const DWORD length=GetEnvironmentVariableW(L"BFVR_DIAGNOSTICS",level,16);
    enabled=log&&count==1&&flag[0]==L'1'&&length<16&&
        (!_wcsicmp(level,L"normal")||!_wcsicmp(level,L"deep"));
    logger=log;frames=changed=acceptedRequests=requests=0;lastFingerprint=0;lastReport=0;
    drawBegin=0;drawEnd=0;queuedDown=0;queuedUp=0;
    deliveredDown=0;deliveredUp=0;returnedDown=0;returnedUp=0;
}
inline bool Button(UINT message){return message==WM_LBUTTONDOWN||message==WM_LBUTTONUP;}
inline void Queued(HWND window,UINT message,bool success){
    if(!enabled||!Button(message))return;
    if(success)(message==WM_LBUTTONDOWN?queuedDown:queuedUp).fetch_add(1);
    logger("Menu trace: tick=%llu queue %s ok=%d source-thread=%lu window-thread=%lu.",
        GetTickCount64(),message==WM_LBUTTONDOWN?"down":"up",success,
        GetCurrentThreadId(),GetWindowThreadProcessId(window,nullptr));
}
inline void Delivered(UINT message,bool returned){
    if(!enabled||!Button(message))return;
    auto& count=message==WM_LBUTTONDOWN?(returned?returnedDown:deliveredDown):(returned?returnedUp:deliveredUp);
    count.fetch_add(1);
    logger("Menu trace: tick=%llu native %s %s thread=%lu.",GetTickCount64(),
        message==WM_LBUTTONDOWN?"down":"up",returned?"returned":"entered",GetCurrentThreadId());
}
inline void Draw(bool ended){if(enabled)(ended?drawEnd:drawBegin).fetch_add(1);}
inline void Request(bool accepted){if(enabled){++requests;if(accepted)++acceptedRequests;}}
inline uint64_t Fingerprint(const std::vector<DWORD>& pixels){
    uint64_t hash=14695981039346656037ull;
    if(pixels.empty())return hash;
    // Fixed sample count keeps diagnostic cost independent of eye resolution.
    for(size_t i=0;i<1024;++i){hash^=pixels[size_t(uint64_t(pixels.size()-1)*i/1023)];hash*=1099511628211ull;}
    return hash;
}
inline void Frame(const std::vector<DWORD>& pixels,UINT w,UINT h,bool mainMenu){
    if(!enabled)return;
    const auto fingerprint=Fingerprint(pixels);if(frames&&fingerprint!=lastFingerprint)++changed;
    lastFingerprint=fingerprint;++frames;
    const auto now=GetTickCount64();if(lastReport&&now-lastReport<5000)return;
    DWORD foreground=0;GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    PROCESS_MEMORY_COUNTERS_EX memory{};memory.cb=sizeof(memory);
    const bool haveMemory=GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory),sizeof(memory))!=FALSE;
    MEMORYSTATUSEX status{sizeof(status)};const bool haveStatus=GlobalMemoryStatusEx(&status)!=FALSE;
    logger("Menu trace: tick=%llu canvas=%ux%u main=%d foreground=%d frames=%u changed-samples=%u controller-accepted=%u/%u Flash-begin/end=%u/%u queued-down/up=%u/%u delivered-down/up=%u/%u returned-down/up=%u/%u private-MiB=%llu available-VA-MiB=%llu memory-valid=%d/%d.",
        now,w,h,mainMenu,foreground==GetCurrentProcessId(),frames,changed,acceptedRequests,requests,
        drawBegin.load(),drawEnd.load(),queuedDown.load(),queuedUp.load(),deliveredDown.load(),deliveredUp.load(),returnedDown.load(),returnedUp.load(),
        static_cast<unsigned long long>(memory.PrivateUsage)/(1024*1024),status.ullAvailVirtual/(1024*1024),haveMemory,haveStatus);
    lastReport=now;
}
}
