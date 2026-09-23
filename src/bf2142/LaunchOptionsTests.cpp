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
    if (!failed) puts("BF2142 launcher option and quoting checks passed.");
    return failed ? 1 : 0;
}
