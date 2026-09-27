#include "LaunchOptions.h"
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
bool InitializeClient(Process& child, const fs::path& client, const fs::path& log, const std::wstring& presenter,const std::wstring& observerProfile) {
    DWORD remoteBase = 0;
    if (!CallRemoteString(child.info.hProcess, RemoteLoadLibrary(),
        client.wstring(), remoteBase)) {
        fwprintf(stderr, L"Unable to load the BF2142 client into the new process (Windows error %lu).\n", GetLastError());
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
    return CallRemoteString(child.info.hProcess, remote, log.wstring() + (presenter.empty() ? L"" : L"\n" + presenter) + (observerProfile.empty()?L"":L"\n"+observerProfile), result);
}
bool AlreadyRunning(const fs::path& executable) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) throw std::runtime_error("Could not inspect running processes.");
    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    bool found = false;
    if (Process32FirstW(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, executable.filename().c_str()) == 0) {
                found = true; break;
            }
        } while (Process32NextW(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return found;
}
int Run(int argc, wchar_t** argv) {
    static_assert(sizeof(void*) == 4, "The launcher must be x86.");
    bfvr::bf2142::LaunchOptions options;
    std::wstring error;
    if (!bfvr::bf2142::ParseOptions(std::vector<std::wstring>(argv + 1, argv + argc), options, error)) {
        fwprintf(stderr, L"%ls\nUse --help for usage.\n", error.c_str()); return 2;
    }
    if (options.help) {
        wprintf(L"BF2142VR development launcher\n"
            L"Usage: BF2142VRLauncher --game-dir PATH [--mod FOLDER] [--windowed] [--render-size WIDTHxHEIGHT] [--join-local PORT | --join-server HOST --port PORT] [--flat] [--inspect] [--presenter PATH]\n"
            L"Use --presenter with the x64 BFVRPresenter.exe to enable experimental head-tracked stereo.\n"
            L"--diagnostic-stereo uses synthetic head poses and saves local eye/UI images; no headset input is generated.\n"
            L"--desktop-vr runs stereo, native VR hands and menus with keyboard simulated controllers, without OpenXR.\n"
            L"--network-observer receives multiplayer arms in the native flat view; requires private network configuration.\n"
            L"--observer-profile PATH isolates Documents for a second flat client; requires an explicit join destination.\n"
            L"Windowed headset launches default to 1600x900 per eye; --render-size overrides this.\n"
            L"--inspect validates paths and x86 images without starting the game.\n");
        return 0;
    }
    const fs::path game = fs::absolute(options.gameDirectory);
    const fs::path executable = game / L"BF2142.exe";
    wchar_t self[32768]{};
    const DWORD count = GetModuleFileNameW(nullptr, self, 32768);
    if (!count || count == 32768) throw std::runtime_error("Cannot resolve launcher location.");
    const fs::path folder = fs::path(self).parent_path();
    const fs::path client = folder / L"BF2142VRClient.dll";
    if (!IsImage(executable) || !IsImage(game / L"RendDX9.dll") || !IsImage(client)) {
        fwprintf(stderr, L"Expected x86 BF2142.exe, RendDX9.dll and adjacent BF2142VRClient.dll.\n"); return 2;
    }
    if (!fs::is_directory(game / L"mods" / options.mod)) {
        fwprintf(stderr, L"The selected mod is not installed under the game's mods folder.\n"); return 2;
    }
    if (!options.presenter.empty()) {
        options.presenter=fs::absolute(options.presenter).wstring();
        if (!IsImage(options.presenter,IMAGE_FILE_MACHINE_AMD64)) {
            fwprintf(stderr,L"--presenter must point to the x64 BFVRPresenter.exe.\n"); return 2;
        }
    }
    wprintf(L"Game: %ls\nMod: %ls\nClient: %ls\n", executable.c_str(), options.mod.c_str(), client.c_str());
    if (options.inspect) { wprintf(L"Inspection passed; no game was launched.\n"); return 0; }
    if (AlreadyRunning(executable) && options.observerProfile.empty()) {
        fwprintf(stderr, L"BF2142 is already running. Exit it normally before using this launcher.\n"); return 2;
    }
    fs::create_directories(folder / L"logs");
    SYSTEMTIME now{};
    GetSystemTime(&now);
    wchar_t logName[128]{};
    swprintf_s(logName, L"bf2142-renderer-%04u%02u%02u-%02u%02u%02u-%lu.log",
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, GetCurrentProcessId());
    const fs::path logPath = folder / L"logs" / logName;
    std::wstring command = bfvr::bf2142::GameCommand(executable.wstring(), options);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    Process child;
    if (!CreateProcessW(executable.c_str(), command.data(), nullptr, nullptr, FALSE,
        CREATE_SUSPENDED, nullptr, game.c_str(), &startup, &child.info)) {
        fwprintf(stderr, L"BF2142 could not start (Windows error %lu).\n", GetLastError()); return 1;
    }
    wprintf(L"Renderer log: %ls\n", logPath.c_str());
    if (!InitializeClient(child, client, logPath, options.networkObserver?L"@observer":options.desktopVr?L"@desktop":options.diagnosticStereo?L"@diagnostic":options.presenter,options.observerProfile)) {
        fwprintf(stderr, L"Renderer initialization failed. The new suspended process will be closed.\n"); return 1;
    }
    if (ResumeThread(child.info.hThread) == static_cast<DWORD>(-1)) return 1;
    child.resumed = true;
    wprintf(L"BF2142 started. %ls\n", options.networkObserver ? L"Flat network observer; no tracking, presenter or local VR controls." : options.desktopVr ? L"Desktop VR simulation; no headset or presenter required." : options.diagnosticStereo
        ? L"Diagnostic stereo with synthetic tracking; no headset required."
        : options.presenter.empty() ? L"Desktop renderer mode."
        : L"Experimental stereo, 6DoF head tracking and controller controls enabled.");
    const DWORD wait = WaitForSingleObject(child.info.hProcess, INFINITE);
    DWORD exitCode = 1;
    if (wait == WAIT_OBJECT_0) GetExitCodeProcess(child.info.hProcess, &exitCode);
    wprintf(L"BF2142 exited with code %lu. Log: %ls\n", exitCode, logPath.c_str());
    return exitCode == 0 ? 0 : 1;
}
}
int wmain(int argc, wchar_t** argv) {
    try { return Run(argc, argv); }
    catch (const std::exception& error) {
        fprintf(stderr, "Launcher error: %s\n", error.what()); return 1;
    }
}
