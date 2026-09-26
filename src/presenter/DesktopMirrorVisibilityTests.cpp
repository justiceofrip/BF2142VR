#include "presenter/DesktopMirrorVisibility.h"
#include <cstdio>
#include <cstdlib>
void Check(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "%s\n", message); std::exit(1); }
}
int main() {
    bfvr::DesktopMirrorVisibility mirror;
    Check(!mirror.Visible(), "Startup must expose the native login.");
    mirror.AcceptFrame();
    mirror.SetRuntimeVisible(true);
    Check(!mirror.Visible(), "A pre-visibility frame cannot cover the native login.");
    mirror.AcceptFrame();
    Check(mirror.Visible(), "A fresh visible XR frame should restore recording.");
    for (int i=0;i<20;++i) mirror.SetRuntimeVisible(true);
    Check(mirror.Visible(), "Visible source gaps must retain the last recording frame.");
    mirror.SetRuntimeVisible(false);
    Check(!mirror.Visible(), "Standby must immediately reveal the live native window.");
    for (int i=0;i<20;++i) mirror.AcceptFrame();
    mirror.SetRuntimeVisible(true);
    Check(!mirror.Visible(), "Standby copies must not reappear when XR wakes.");
    mirror.SetRuntimeVisible(true);
    Check(!mirror.Visible(), "Wake without a new source must not show the frozen login.");
    mirror.AcceptFrame();
    Check(mirror.Visible(), "The first new source after wake restores the preview.");
    mirror.SetRuntimeVisible(false);
    mirror.SetRuntimeVisible(true);
    Check(!mirror.Visible(), "Repeated sleep/wake must invalidate the previous source.");
    std::puts("Desktop mirror startup, source gap, standby and wake transitions passed.");
}
