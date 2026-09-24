#include "../LaunchOptions.h"
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs = std::filesystem;
namespace {
struct Process {
    PROCESS_INFORMATION info{};
    bool resumed = false;
    ~Process() {
        if (info.hProcess) {
            if (!resumed) {
                TerminateProcess(info.hProcess, 1); // Only our still-suspended child.
                WaitForSingleObject(info.hProcess, 5000);
            }
            CloseHandle(info.hProcess);
        }
        if (info.hThread) CloseHandle(info.hThread);
    }
};
bool IsImage(const fs::path& path, WORD machine = IMAGE_FILE_MACHINE_I386) {
    std::ifstream stream(path, std::ios::binary);
    IMAGE_DOS_HEADER dos{};
    stream.read(reinterpret_cast<char*>(&dos), sizeof(dos));
    if (!stream || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0) return false;
    stream.seekg(dos.e_lfanew);
    DWORD signature = 0;
    IMAGE_FILE_HEADER header{};
    stream.read(reinterpret_cast<char*>(&signature), sizeof(signature));
    stream.read(reinterpret_cast<char*>(&header), sizeof(header));
    return stream && signature == IMAGE_NT_SIGNATURE && header.Machine == machine;
}
LPTHREAD_START_ROUTINE RemoteLoadLibrary() {
    // Match upstream BFVR's same-architecture startup path. A newly suspended
    // process has not initialized its loader module list yet; Toolhelp module
    // enumeration at this point returns ERROR_PARTIAL_COPY. Windows initializes
    // the process loader before executing the remote LoadLibraryW thread.
    return reinterpret_cast<LPTHREAD_START_ROUTINE>(
        GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
}
bool CallRemoteString(HANDLE process, LPTHREAD_START_ROUTINE function,
    const std::wstring& text, DWORD& result) {
    result = 0;
    if (!function) return false;
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    void* argument = VirtualAllocEx(process, nullptr, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!argument) return false;
    SIZE_T written = 0;
    if (!WriteProcessMemory(process, argument, text.c_str(), bytes, &written) || written != bytes) {
        VirtualFreeEx(process, argument, 0, MEM_RELEASE); return false;
    }
    HANDLE thread = CreateRemoteThread(process, nullptr, 0, function, argument, 0, nullptr);
    if (!thread) { VirtualFreeEx(process, argument, 0, MEM_RELEASE); return false; }
    const DWORD wait = WaitForSingleObject(thread, 15000);
    const bool finished = wait == WAIT_OBJECT_0;
    const bool success = finished && GetExitCodeThread(thread, &result) && result != 0;
    CloseHandle(thread);
    // On timeout the caller terminates its still-suspended child. Keep argument
    // memory alive until then rather than freeing it under a running thread.
    if (finished) VirtualFreeEx(process, argument, 0, MEM_RELEASE);
    return success;
}
bool InitializeClient(Process& child, const fs::path& client, const fs::path& log, const std::wstring& presenter) {
    DWORD remoteBase = 0;
    if (!CallRemoteString(child.info.hProcess, RemoteLoadLibrary(),
        client.wstring(), remoteBase)) {
        fwprintf(stderr, L"Unable to load the BF2142 server extension into the new process (Windows error %lu).\n", GetLastError());
        return false;
    }
    // Inspect our own export without executing the client in the launcher.
    HMODULE local = LoadLibraryExW(client.c_str(), nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (!local) return false;
    const auto initializer = GetProcAddress(local, "BF2142VRInitialize");
    if (!initializer) { FreeLibrary(local); return false; }
    const auto rva = reinterpret_cast<std::uintptr_t>(initializer) - reinterpret_cast<std::uintptr_t>(local);
    FreeLibrary(local);
    const auto remote = reinterpret_cast<LPTHREAD_START_ROUTINE>(static_cast<std::uintptr_t>(remoteBase) + rva);
    DWORD result = 0;
    return CallRemoteString(child.info.hProcess, remote, log.wstring() + (presenter.empty() ? L"" : L"\n" + presenter), result);
}
int Run(int argc,wchar_t** argv){
 static_assert(sizeof(void*)==4);
 const bool check=argc==5&&std::wstring(argv[4])==L"--check";
 if(argc!=4&&!check){fwprintf(stderr,L"Usage: BF2142VRServerLauncher SERVER_DIRECTORY SERVER_SETTINGS MAP_LIST [--check]\nBF2142VR_NETWORK must point to the private network INI.\n");return 2;}
 const auto server=fs::absolute(argv[1]),config=fs::absolute(argv[2]),maps=fs::absolute(argv[3]);
 wchar_t own[MAX_PATH]{};if(!GetModuleFileNameW(nullptr,own,MAX_PATH))return 2;
 const auto runtime=fs::path(own).parent_path(),extension=runtime/L"BF2142VRServer.dll",exe=server/L"BF2142_w32ded.exe";
 if(!IsImage(exe)||!IsImage(extension)||!fs::is_regular_file(config)||!fs::is_regular_file(maps))return 2;
 wchar_t network[32768]{};if(!GetEnvironmentVariableW(L"BF2142VR_NETWORK",network,32768)||!fs::is_regular_file(network))return 2;
 fs::create_directories(runtime/L"logs");
 const auto log=runtime/L"logs"/(L"server-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64())+L".log");
 auto quote=[](const fs::path& p){return bfvr::bf2142::QuoteArgument(p.wstring());};
 auto command=quote(exe)+L" +config "+quote(config)+L" +mapList "+quote(maps);
 STARTUPINFOW startup{};startup.cb=sizeof(startup);Process child;
 if(!CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_SUSPENDED|CREATE_NO_WINDOW,nullptr,server.c_str(),&startup,&child.info))return 3;
 if(!InitializeClient(child,extension,log,L""))return 4;
 if(check){wprintf(L"Server extension loaded and initialized successfully in suspended child; %ls\n",log.c_str());return 0;}
 if(ResumeThread(child.info.hThread)==DWORD(-1))return 5;child.resumed=true;
 wprintf(L"Server PID %lu; extension log: %ls\n",child.info.dwProcessId,log.c_str());fflush(stdout);
 WaitForSingleObject(child.info.hProcess,INFINITE);DWORD code=1;GetExitCodeProcess(child.info.hProcess,&code);return int(code);
}
}
int wmain(int argc,wchar_t** argv){try{return Run(argc,argv);}catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}}
