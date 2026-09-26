#include "ObserverProfile.h"
#include <windows.h>
#include <shlobj.h>
#include <MinHook.h>
#include <filesystem>
#include <cwchar>
namespace bfvr::bf2142 {
namespace {
namespace fs=std::filesystem;
using FolderPath=HRESULT(WINAPI*)(HWND,int,HANDLE,DWORD,LPWSTR);
FolderPath originalFolderPath=nullptr;
std::wstring observerDocuments;
struct Lease {HANDLE file=INVALID_HANDLE_VALUE;~Lease(){if(file!=INVALID_HANDLE_VALUE)CloseHandle(file);}} profileLease;
bool Prefix(const fs::path& parent,const fs::path& child){
 auto a=parent.begin(),b=child.begin();for(;a!=parent.end();++a,++b)
  if(b==child.end()||_wcsicmp(a->c_str(),b->c_str()))return false;
 return true;
}
HRESULT WINAPI FolderHook(HWND window,int folder,HANDLE user,DWORD flags,LPWSTR path){
 if(!user&&path&&(folder&~CSIDL_FLAG_MASK)==CSIDL_PERSONAL&&!observerDocuments.empty()){
  wcscpy_s(path,MAX_PATH,observerDocuments.c_str());return S_OK;
 }
 return originalFolderPath(window,folder,user,flags,path);
}
}
bool InstallObserverProfile(const std::wstring& root){
 if(root.empty()||!observerDocuments.empty())return false;
 HANDLE lease=INVALID_HANDLE_VALUE;
 try {
  if(root.find_first_of(L"\r\n")!=std::wstring::npos||!fs::path(root).is_absolute())return false;
  wchar_t normal[MAX_PATH]{};if(FAILED(SHGetFolderPathW(nullptr,CSIDL_PERSONAL,nullptr,SHGFP_TYPE_CURRENT,normal)))return false;
  const auto target=fs::weakly_canonical(root),primary=fs::weakly_canonical(fs::path(normal)/L"Battlefield 2142");
  if(target.native().size()>MAX_PATH-40||Prefix(primary,target)||Prefix(target,primary))return false;
  fs::create_directories(target);
  lease=CreateFileW((target/L".bf2142vr-observer.lock").c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_DELETE_ON_CLOSE,nullptr);
  if(lease==INVALID_HANDLE_VALUE)return false;
  const auto targetFunction=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(L"shell32.dll"),"SHGetFolderPathW"));
  if(!targetFunction||MH_CreateHook(targetFunction,FolderHook,reinterpret_cast<void**>(&originalFolderPath))!=MH_OK){CloseHandle(lease);return false;}
  observerDocuments=target.wstring();
  if(MH_EnableHook(targetFunction)!=MH_OK){observerDocuments.clear();MH_RemoveHook(targetFunction);CloseHandle(lease);return false;}
  profileLease.file=lease;return true;
 }catch(...){if(lease!=INVALID_HANDLE_VALUE)CloseHandle(lease);return false;}
}
}
