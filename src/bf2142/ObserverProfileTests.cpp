#include "ObserverProfile.h"
#include <windows.h>
#include <shlobj.h>
#include <MinHook.h>
#include <filesystem>
#include <cstdio>
#define CHECK(x) do{if(!(x)){printf("Observer profile failure %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 using namespace bfvr::bf2142;namespace fs=std::filesystem;
 wchar_t documents[MAX_PATH]{},appData[MAX_PATH]{},temp[MAX_PATH]{},after[MAX_PATH]{};
 CHECK(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL,nullptr,0,documents)));
 CHECK(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_APPDATA,nullptr,0,appData)));
 CHECK(GetTempPathW(MAX_PATH,temp));CHECK(MH_Initialize()==MH_OK);
 CHECK(!InstallObserverProfile(L"relative"));CHECK(!InstallObserverProfile(documents));
 CHECK(!InstallObserverProfile((fs::path(documents)/L"Battlefield 2142"/L"profiles").wstring()));
 auto test=fs::path(temp)/(L"BF2142VR-observer-test-"+std::to_wstring(GetCurrentProcessId()));
 CHECK(InstallObserverProfile(test.wstring()));
 CHECK(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL|CSIDL_FLAG_CREATE,nullptr,0,after)));
 CHECK(fs::equivalent(after,test));
 CHECK(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_APPDATA,nullptr,0,after))&&!wcscmp(after,appData));
 const auto lock=CreateFileW((test/L".bf2142vr-observer.lock").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
 CHECK(lock==INVALID_HANDLE_VALUE&&GetLastError()==ERROR_SHARING_VIOLATION);
 CHECK(!InstallObserverProfile((test/L"other").wstring()));
 puts("Observer process Documents isolation, native-folder fallback, primary profile rejection and exclusive lease passed.");return 0;
}
