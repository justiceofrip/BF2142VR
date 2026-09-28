#pragma once
#include <string>
#include <vector>
#include <algorithm>
namespace bfvr::bf2142 {
struct LaunchOptions {
    std::wstring gameDirectory;
    std::wstring presenter;
    unsigned joinLocalPort = 0,joinPort=0;
    std::wstring joinServer;
    std::wstring mod = L"bf2142";
    bool inspect = false;
    bool help = false;
    bool windowed = false;
    unsigned renderWidth=0,renderHeight=0;
    bool diagnosticStereo = false;
    bool desktopVr = false;
    bool networkObserver = false;
    std::wstring observerProfile;
};
inline bool ValidModName(const std::wstring& name) {
    if (name.empty()) return false;
    for (wchar_t c : name) {
        if (!((c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
              (c >= L'0' && c <= L'9') || c == L'_' || c == L'-')) return false;
    }
    return true;
}
inline bool ValidJoinHost(const std::wstring& value) {
    if(value.empty()||value.size()>253||value.front()==L'-'||value.back()==L'-')return false;
    for(wchar_t c:value)if(!((c>=L'a'&&c<=L'z')||(c>=L'A'&&c<=L'Z')||(c>=L'0'&&c<=L'9')||c==L'.'||c==L'-'))return false;
    return value.find(L"..") == std::wstring::npos && value.front()!=L'.';
}
inline bool ParseOptions(const std::vector<std::wstring>& args,
                         LaunchOptions& result, std::wstring& error) {
    result = {};
    for (std::size_t i = 0; i < args.size(); ++i) {
        if (args[i] == L"--help") result.help = true;
        else if (args[i] == L"--inspect") result.inspect = true;
        else if (args[i] == L"--diagnostic-stereo") result.diagnosticStereo = true;
        else if (args[i] == L"--desktop-vr") result.desktopVr = true;
        else if ((args[i] == L"--network-observer" || args[i] == L"--flat")) result.networkObserver = true;
        else if (args[i] == L"--render-size") {
            if(++i>=args.size()){error=L"--render-size requires WIDTHxHEIGHT.";return false;}
            const auto split=args[i].find(L'x');
            if(split==std::wstring::npos){error=L"--render-size requires WIDTHxHEIGHT.";return false;}
            const auto parse=[](const std::wstring& text,unsigned& value){
                if(text.empty()||text.size()>4)return false;value=0;
                for(wchar_t c:text){if(c<L'0'||c>L'9')return false;value=value*10+unsigned(c-L'0');}
                return value>=640&&value<=3072;
            };
            if(!parse(args[i].substr(0,split),result.renderWidth)||!parse(args[i].substr(split+1),result.renderHeight)){
                error=L"Render dimensions must be 640-3072 pixels each.";return false;
            }
        }
        else if (args[i] == L"--windowed") result.windowed = true;
        else if (args[i] == L"--game-dir" || args[i] == L"--mod" || args[i] == L"--presenter" || args[i] == L"--join-local" || args[i] == L"--observer-profile" || args[i] == L"--join-server" || args[i] == L"--port") {
            const auto option = args[i];
            if (++i >= args.size() || args[i].empty() || args[i].starts_with(L"--")) {
                error = L"Missing value for " + option; return false;
            }
            if(option==L"--join-server")result.joinServer=args[i];
            else if (option == L"--game-dir") result.gameDirectory = args[i];
            else if (option == L"--observer-profile") result.observerProfile=args[i];
            else if (option == L"--presenter") result.presenter = args[i];
            else if (option == L"--join-local" || option == L"--port") {
                unsigned port = 0;
                for (wchar_t c : args[i]) {
                    if (c < L'0' || c > L'9' || port > 6553) { error=L"Invalid local server port."; return false; }
                    port = port * 10 + unsigned(c - L'0');
                }
                if (!port || port > 65535) { error=L"Local port must be 1-65535."; return false; }
                if(option==L"--join-local")result.joinLocalPort=port;else result.joinPort=port;
            }
            else result.mod = args[i];
        } else { error = L"Unknown argument: " + args[i]; return false; }
    }
    if (result.help) return true;
    if ((!result.joinServer.empty() && (!ValidJoinHost(result.joinServer)||!result.joinPort||result.joinLocalPort)) || (result.joinServer.empty()&&result.joinPort)) {
        error=L"Use --join-server HOST --port PORT, separately from --join-local.";return false;
    }
    if (result.gameDirectory.empty()) { error = L"--game-dir is required."; return false; }
    if (!ValidModName(result.mod)) {
        error = L"--mod must contain only letters, digits, underscores or hyphens."; return false;
    }
    if (!result.observerProfile.empty() && (!result.networkObserver || (!result.joinLocalPort && result.joinServer.empty()) || result.observerProfile.find_first_of(L"\r\n")!=std::wstring::npos)) { error=L"An isolated profile requires --flat and an explicit --join-local or --join-server destination."; return false; }
    if (result.networkObserver && (result.desktopVr || result.diagnosticStereo || !result.presenter.empty())) { error=L"Network observer uses native flat rendering without simulated tracking or a presenter."; return false; }
    if ((result.desktopVr && result.diagnosticStereo) || ((result.diagnosticStereo || result.desktopVr) && !result.presenter.empty())) { error=L"Diagnostic stereo does not use a headset presenter."; return false; }
    return true;
}
// Match runtime pixel density while retaining BF2142's widescreen Flash
// canvas. 32-bit renderer/MSAA memory needs a bounded source, not arbitrary
// SteamVR supersampling allocations. Explicit --render-size always wins.
inline bool RuntimeSourceSize(unsigned eyeWidth,unsigned eyeHeight,LaunchOptions& o){
    if(o.presenter.empty()||o.renderWidth||eyeWidth<256||eyeHeight<256||eyeWidth>16384||eyeHeight>16384)return false;
    const unsigned wanted=std::max(eyeWidth,(eyeHeight*16+8)/9);
    const unsigned units=std::clamp((wanted+15)/16,80u,192u);
    o.renderWidth=units*16;o.renderHeight=units*9;return true;
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
inline std::wstring GameCommand(const std::wstring& executable, const LaunchOptions& options) {
    std::wstring command = QuoteArgument(executable) + L" +modPath mods/" + options.mod;
    if (!options.observerProfile.empty()) command += L" +multi 1";
    if (options.windowed) command += L" +fullscreen 0";
    // Keep the native window, renderer and legacy Flash canvas at one supported
    // widescreen size. Flat and simulated clients retain their desktop default.
    if(options.windowed||options.renderWidth){
        const unsigned w=options.renderWidth?options.renderWidth:options.presenter.empty()?1280:1600;
        const unsigned h=options.renderHeight?options.renderHeight:options.presenter.empty()?720:900;
        command+=L" +szx "+std::to_wstring(w)+L" +szy "+std::to_wstring(h);
    }
    if (options.joinLocalPort) command += L" +joinServer 127.0.0.1 +port " + std::to_wstring(options.joinLocalPort);
    if(!options.joinServer.empty())command+=L" +joinServer "+options.joinServer+L" +port "+std::to_wstring(options.joinPort);
    return command;
}
}
