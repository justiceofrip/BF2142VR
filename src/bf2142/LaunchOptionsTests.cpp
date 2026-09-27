#include "LaunchOptions.h"
#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <vector>
int main() {
    int failed = 0;
    auto check = [&](bool result, const char* name) {
        if (!result) { fprintf(stderr, "FAIL: %s\n", name); ++failed; }
    };
    bfvr::bf2142::LaunchOptions options;
    std::wstring error;
    using bfvr::bf2142::ParseOptions;
    check(ParseOptions({L"--help"}, options, error) && options.help, "help without game");
    check(!ParseOptions({}, options, error), "missing game");
    check(!ParseOptions({L"--game-dir"}, options, error), "missing path");
    check(!ParseOptions({L"--game-dir", L"--inspect"}, options, error), "option is not a path");
    check(ParseOptions({L"--game-dir", L"G:\\Game With Spaces", L"--inspect", L"--windowed"}, options, error)
          && options.inspect && options.windowed && options.mod == L"bf2142", "default base mod");
    check(ParseOptions({L"--game-dir", L"G:\\Game", L"--mod", L"Project_Remaster_v17"}, options, error),
          "Remaster folder");
    check(ParseOptions({L"--game-dir", L"G:\\Game", L"--presenter", L"G:\\VR Build\\BFVRPresenter.exe"}, options, error)
          && options.presenter == L"G:\\VR Build\\BFVRPresenter.exe", "presenter path with spaces");
    check(!ParseOptions({L"--game-dir", L"G:\\Game", L"--presenter"}, options, error), "missing presenter path");
    check(ParseOptions({L"--game-dir", L"G:\\Game", L"--diagnostic-stereo"},options,error) && options.diagnosticStereo,"local diagnostic mode");
    check(!ParseOptions({L"--game-dir", L"G:\\Game", L"--diagnostic-stereo", L"--presenter", L"p.exe"},options,error),"reject conflicting diagnostic/runtime modes");
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--desktop-vr"},options,error)&&options.desktopVr,"headset-free desktop mode");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--desktop-vr",L"--presenter",L"p.exe"},options,error),"desktop cannot start OpenXR");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--desktop-vr",L"--diagnostic-stereo"},options,error),"simulation and automated fixtures are separate");
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--network-observer"},options,error)&&options.networkObserver,"flat receive-only observer");
    for(const auto& incompatible:{L"--desktop-vr",L"--diagnostic-stereo"})
        check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--network-observer",incompatible},options,error),"observer never simulates tracking");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--network-observer",L"--presenter",L"p.exe"},options,error),"observer never launches a headset presenter");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--observer-profile",L"G:\\Observer"},options,error),"isolation never bypasses normal launch guard");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--network-observer",L"--observer-profile",L"G:\\Observer"},options,error),"isolated observer requires an explicit destination");
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--network-observer",L"--join-local",L"17567",L"--observer-profile",L"G:\\Observer"},options,error),"explicit isolated observer");
    check(bfvr::bf2142::GameCommand(L"BF2142.exe",options).find(L"+multi 1")!=std::wstring::npos,"native multiple-instance flag only with profile isolation");
    for (const auto& port : {L"0",L"65536",L"9999999999999999999",L"-1",L"127.0.0.1",L"17567 +foo"})
        check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--join-local",port},options,error),"reject invalid local port");
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--join-local",L"17567",L"--desktop-vr"},options,error),"desktop local server client");
    auto labCommand=bfvr::bf2142::GameCommand(L"G:\\Game Space\\BF2142.exe",options);
    int labCount=0;auto labArgs=CommandLineToArgvW(labCommand.c_str(),&labCount);
    check(labArgs && labCount==7 && std::wstring(labArgs[0])==L"G:\\Game Space\\BF2142.exe" &&
          std::wstring(labArgs[3])==L"+joinServer" && std::wstring(labArgs[4])==L"127.0.0.1" &&
          std::wstring(labArgs[5])==L"+port" && std::wstring(labArgs[6])==L"17567","native local join arguments round trip");
    if(labArgs)LocalFree(labArgs);
    check(ParseOptions({L"--game-dir",L"G:\\Game"},options,error) && !options.joinLocalPort,
          "local join does not leak into normal launches");
    check(bfvr::bf2142::GameCommand(L"G:\\Game\\BF2142.exe",options).find(L"+joinServer")==std::wstring::npos,"normal launch has no autojoin");
    for (const auto& bad : {L"../outside", L"..", L"mods/bf2142", L"C:\\other", L"bf2142\" +foo"})
        check(!ParseOptions({L"--game-dir", L"G:\\Game", L"--mod", bad}, options, error),
              "reject traversal or command fragments");
    check(!ParseOptions({L"--game-dir", L"G:\\Game", L"--unknown"}, options, error), "unknown option");
    for (const auto& path : std::vector<std::wstring>{L"", L"G:\\Game With Spaces\\", L"a\"b", L"\\", L"abc\\\\", L"plain"}) {
        const std::wstring command = L"test.exe " + bfvr::bf2142::QuoteArgument(path);
        int count = 0;
        auto args = CommandLineToArgvW(command.c_str(), &count);
        check(args && count == 2 && args[1] == path, "Windows argument round trip");
        if (args) LocalFree(args);
    }
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--flat",L"--join-server",L"play.example.org",L"--port",L"17567"},options,error)&&options.networkObserver,"remote flat addon");
    check(bfvr::bf2142::GameCommand(L"BF2142.exe",options).find(L"+joinServer play.example.org +port 17567")!=std::wstring::npos,"remote join arguments");
    for(const auto& bad:{L"x +password y",L"x\n",L"x/../../",L"\"x",L"-option",L"..",L"http://x",L"x:17567"})
        check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--join-server",bad,L"--port",L"17567"},options,error),"remote host rejects argument injection");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--join-server",L"example.org"},options,error),"remote join requires port");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--join-server",L"example.org",L"--port",L"17567",L"--join-local",L"17567"},options,error),"join modes cannot mix");
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--flat",L"--join-server",L"play.example.org",L"--port",L"17567",L"--observer-profile",L"G:\\Observer With Spaces"},options,error),"isolated flat observer may join a remote server");
    const auto cloudCommand=bfvr::bf2142::GameCommand(L"G:\\Game With Spaces\\BF2142.exe",options);
    int cloudCount=0;auto cloudArgs=CommandLineToArgvW(cloudCommand.c_str(),&cloudCount);
    check(cloudArgs && cloudCount==9 && std::wstring(cloudArgs[0])==L"G:\\Game With Spaces\\BF2142.exe" &&
          std::wstring(cloudArgs[3])==L"+multi" && std::wstring(cloudArgs[4])==L"1" &&
          std::wstring(cloudArgs[5])==L"+joinServer" && std::wstring(cloudArgs[6])==L"play.example.org" &&
          std::wstring(cloudArgs[7])==L"+port" && std::wstring(cloudArgs[8])==L"17567","isolated remote native arguments round trip");
    if(cloudArgs)LocalFree(cloudArgs);
    for(const auto& incompatible:{L"--desktop-vr",L"--diagnostic-stereo"})
        check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--flat",L"--join-server",L"play.example.org",L"--port",L"17567",L"--observer-profile",L"G:\\Observer",incompatible},options,error),"remote isolation cannot enable simulated tracking");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--flat",L"--join-server",L"play.example.org",L"--port",L"17567",L"--observer-profile",L"G:\\Observer",L"--presenter",L"p.exe"},options,error),"remote isolation cannot launch a presenter");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--join-server",L"play.example.org",L"--port",L"17567",L"--observer-profile",L"G:\\Observer"},options,error),"remote isolation requires flat mode");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--flat",L"--join-server",L"play.example.org",L"--observer-profile",L"G:\\Observer"},options,error),"remote isolation requires a complete destination");
    check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--flat",L"--join-server",L"play.example.org",L"--port",L"17567",L"--observer-profile",L"G:\\Observer\nInjected"},options,error),"remote isolation rejects initialization protocol injection");
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--flat",L"--join-server",L"play.example.org",L"--port",L"17567"},options,error) &&
          bfvr::bf2142::GameCommand(L"BF2142.exe",options).find(L"+multi")==std::wstring::npos,"normal remote join retains the single-instance guard");
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--windowed",L"--presenter",L"p.exe"},options,error),"headset quality default");
    check(bfvr::bf2142::GameCommand(L"BF2142.exe",options).find(L"+szx 1600 +szy 900")!=std::wstring::npos,"widescreen eye source fits a 1080p desktop and retains native Flash layout");
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--windowed",L"--flat"},options,error),"flat quality unchanged");
    check(bfvr::bf2142::GameCommand(L"BF2142.exe",options).find(L"+szx 1280 +szy 720")!=std::wstring::npos,"observer retains desktop size");
    check(ParseOptions({L"--game-dir",L"G:\\Game",L"--render-size",L"2064x2208"},options,error),"explicit native-eye size");
    check(bfvr::bf2142::GameCommand(L"BF2142.exe",options).find(L"+szx 2064 +szy 2208")!=std::wstring::npos,"custom source applied");
    for(const auto& size:{L"",L"1x1",L"8192x8192",L"1920x",L"1920x1080 +foo",L"-1x1080",L"9999999999x720"})
        check(!ParseOptions({L"--game-dir",L"G:\\Game",L"--render-size",size},options,error),"invalid or oversized source rejected");
    if (!failed) puts("BF2142 launcher option and quoting checks passed.");
    return failed ? 1 : 0;
}
