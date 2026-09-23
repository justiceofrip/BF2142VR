#pragma once
#include <Windows.h>
namespace bfvr::bf2142 {
// Read-only frontend state. Unknown executable/object layouts fail closed.
class NativeMenuState {
public:
    bool Connect(const BYTE* image);
    bool Active() const;
private:
    const BYTE* game=nullptr;
};
bool NativeInGameMenuActive();
}
