#pragma once
#include <string>
namespace bfvr::bf2142 {
// Before native main: redirect only this process's Documents lookup and hold an
// exclusive lease for its lifetime. Failure means do not resume the new child.
bool InstallObserverProfile(const std::wstring& documentsRoot);
}
