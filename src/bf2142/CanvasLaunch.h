#pragma once
#include "LaunchOptions.h"
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <stdexcept>
namespace bfvr::bf2142 {
inline void QueryCanvas(LaunchOptions& options,const std::filesystem::path& log) {
    if(!options.headsetResolution)return;
    const auto output=log.wstring()+L".viewsize";
    std::wstring command=QuoteArgument(options.presenter)+L" --query-bf2142-render-size "+QuoteArgument(output);
    STARTUPINFOW startup{sizeof(startup)};startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
    PROCESS_INFORMATION child{};DWORD code=1;bool completed=false;
    if(CreateProcessW(options.presenter.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,nullptr,
        std::filesystem::path(options.presenter).parent_path().c_str(),&startup,&child)){
        CloseHandle(child.hThread);
        completed=WaitForSingleObject(child.hProcess,15000)==WAIT_OBJECT_0;
        if(completed)GetExitCodeProcess(child.hProcess,&code);
        else {TerminateProcess(child.hProcess,1);WaitForSingleObject(child.hProcess,1000);}
        CloseHandle(child.hProcess);
    }
    CanvasSize recommended{},maximum{},selected{};
    if(completed&&!code){std::ifstream input(output);std::string extra;
        if(input>>recommended.width>>recommended.height>>maximum.width>>maximum.height && !(input>>extra))
            selected=RecommendCanvas(recommended,maximum);
    }
    DeleteFileW(output.c_str());
    std::ofstream evidence(log.wstring()+L".resolution.txt");
    if(selected.width){
        options.renderWidth=selected.width;options.renderHeight=selected.height;options.renderCanvas=true;
        wprintf(L"Headset source: runtime %ux%u, requested %ux%u%s. Awaiting actual renderer confirmation.\n",
            recommended.width,recommended.height,selected.width,selected.height,
            selected.width!=recommended.width||selected.height!=recommended.height?L" (bounded for this 32-bit renderer)":L"");
        evidence<<"runtime="<<recommended.width<<'x'<<recommended.height<<" maximum="<<maximum.width<<'x'<<maximum.height
            <<" requested="<<selected.width<<'x'<<selected.height<<"; confirm actual source in renderer log\n";
    }else{
        wprintf(L"Headset size query unavailable; retaining the menu-compatible 1600x900 source.\n");
        evidence<<"query failed; source fallback=1600x900; exit="<<code<<" completed="<<completed<<'\n';
    }
}
// Only the newly launched child inherits this setting; restore the launcher's
// environment afterward. Clear stale inherited experiments on normal launches.
class CanvasEnvironment {
    std::wstring previous;bool existed=false;
public:
    explicit CanvasEnvironment(const LaunchOptions& options){
        const DWORD size=GetEnvironmentVariableW(L"BF2142VR_RENDER_CANVAS",nullptr,0);
        if(size){previous.resize(size);GetEnvironmentVariableW(L"BF2142VR_RENDER_CANVAS",previous.data(),size);previous.resize(size-1);existed=true;}
        const auto value=std::to_wstring(options.renderWidth)+L"x"+std::to_wstring(options.renderHeight);
        if(!SetEnvironmentVariableW(L"BF2142VR_RENDER_CANVAS",options.renderCanvas?value.c_str():nullptr))
            throw std::runtime_error("Unable to configure the child's render canvas.");
    }
    ~CanvasEnvironment(){SetEnvironmentVariableW(L"BF2142VR_RENDER_CANVAS",existed?previous.c_str():nullptr);}
};
}
