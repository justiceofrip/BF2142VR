#pragma once
namespace bfvr {
// A sleeping XR session must not cover an interactive native menu with an
// obsolete preview. Waking alone is insufficient: wait for a fresh source.
class DesktopMirrorVisibility {
public:
    void SetRuntimeVisible(bool visible) noexcept {
        runtimeVisible_ = visible;
        if (!visible) freshFrame_ = false;
    }
    void AcceptFrame() noexcept { if (runtimeVisible_) freshFrame_ = true; }
    bool RuntimeVisible() const noexcept { return runtimeVisible_; }
    bool Visible() const noexcept { return runtimeVisible_ && freshFrame_; }
private:
    bool runtimeVisible_ = false;
    bool freshFrame_ = false;
};
}
