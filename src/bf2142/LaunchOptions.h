#pragma once
#include <string>
#include <vector>
namespace bfvr::bf2142 {
struct LaunchOptions {
    std::wstring gameDirectory;
    std::wstring presenter;
    std::wstring mod = L"bf2142";
    bool inspect = false;
    bool help = false;
    bool windowed = false;
    bool diagnosticStereo = false;
    bool desktopVr = false;
};
inline bool ValidModName(const std::wstring& name) {
    if (name.empty()) return false;
    for (wchar_t c : name) {
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
              (c >= L'0' && c <= L'9') || c == L'_' || c == L'-')) return false;
    }
    return true;
}
inline bool ParseOptions(const std::vector<std::wstring>& args,
                         LaunchOptions& result, std::wstring& error) {
    result = {};
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == L"--help") result.help = true;
        else if (args[i] == L"--inspect") result.inspect = true;
        else if (args[i] == L"--diagnostic-stereo") result.diagnosticStereo = true;
        else if (args[i] == L"--desktop-vr") result.desktopVr = true;
        else if (args[i] == L"--windowed") result.windowed = true;
        else if (args[i] == L"--game-dir" || args[i] == L"--mod" || args[i] == L"--presenter") {
            const auto option = args[i];
            if (++i >= args.size() || args[i].empty() || args[i].starts_with(L"--")) {
                error = L"Missing value for " + option; return false;
            }
            if (option == L"--game-dir") result.gameDirectory = args[i];
            else if (option == L"--presenter") result.presenter = args[i];
            else result.mod = args[i];
        } else { error = L"Unknown argument: " + args[i]; return false; }
    }
    if (result.help) return true;
    if (result.gameDirectory.empty()) { error = L"--game-dir is required."; return false; }
    if (!ValidModName(result.mod)) {
        error = L"--mod must contain only letters, digits, underscores or hyphens."; return false;
    }
    if ((result.desktopVr && result.diagnosticStereo) || ((result.diagnosticStereo || result.desktopVr) && !result.presenter.empty())) { error=L"Diagnostic stereo does not use a headset presenter."; return false; }
    return true;
}
// Windows CRT quoting, including quotes and trailing backslashes.
inline std::wstring QuoteArgument(const std::wstring& argument) {
    std::wstring out = L"\"";
    std::size_t slashes = 0;
    for (wchar_t c : argument) {
        if (c == L'\\') { ++slashes; continue; }
        if (c == L'"') out.append(slashes * 2 + 1, L'\\');
        else out.append(slashes, L'\\');
        slashes = 0;
        out += c;
    }
    out.append(slashes * 2, L'\\');
    out += L'"';
    return out;
}
}
