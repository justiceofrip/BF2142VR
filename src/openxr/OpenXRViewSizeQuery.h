#pragma once
#include "OpenXRApiVersion.h"
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <string>
#define XR_NO_PROTOTYPES
#include <openxr/openxr.h>
namespace bfvr {
// Launcher preflight in the same x64 presenter executable/application identity
// used by the session, so per-application runtime resolution settings apply.
// No session, device, actions or swapchains are created by this query.
inline int QueryOpenXRViewSize(const wchar_t* folder,const wchar_t* output,bool includeMaximum=false){
 const auto path=std::wstring(folder)+L"\\runtime\\openxr\\win64\\openxr_loader.dll";
 HMODULE loader=LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);if(!loader)return 2;
 struct Loader{HMODULE p;~Loader(){FreeLibrary(p);}} loaded{loader};
 const auto get=reinterpret_cast<PFN_xrGetInstanceProcAddr>(GetProcAddress(loader,"xrGetInstanceProcAddr"));if(!get)return 2;
 PFN_xrVoidFunction raw{};if(XR_FAILED(get(XR_NULL_HANDLE,"xrCreateInstance",&raw))||!raw)return 2;
 const auto create=reinterpret_cast<PFN_xrCreateInstance>(raw);
 XrInstanceCreateInfo info{XR_TYPE_INSTANCE_CREATE_INFO};strcpy_s(info.applicationInfo.applicationName,"Battlefield 2142 VR");strcpy_s(info.applicationInfo.engineName,"BFVR");
 info.applicationInfo.applicationVersion=info.applicationInfo.engineVersion=1;info.applicationInfo.apiVersion=kRequestedOpenXRApiVersion;
 XrInstance instance{};if(XR_FAILED(create(&info,&instance))||!instance)return 3;
 if(XR_FAILED(get(instance,"xrDestroyInstance",&raw))||!raw)return 3;
 struct Instance{XrInstance value;PFN_xrDestroyInstance destroy;~Instance(){destroy(value);}} owned{instance,reinterpret_cast<PFN_xrDestroyInstance>(raw)};
 if(XR_FAILED(get(instance,"xrGetSystem",&raw))||!raw)return 3;const auto system=reinterpret_cast<PFN_xrGetSystem>(raw);
 XrSystemGetInfo systemInfo{XR_TYPE_SYSTEM_GET_INFO};systemInfo.formFactor=XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;XrSystemId id{};if(XR_FAILED(system(instance,&systemInfo,&id)))return 4;
 if(XR_FAILED(get(instance,"xrEnumerateViewConfigurationViews",&raw))||!raw)return 4;const auto views=reinterpret_cast<PFN_xrEnumerateViewConfigurationViews>(raw);
 std::array<XrViewConfigurationView,2> v{};for(auto& eye:v)eye.type=XR_TYPE_VIEW_CONFIGURATION_VIEW;uint32_t count=0;
 if(XR_FAILED(views(instance,id,XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO,2,&count,v.data()))||count!=2)return 4;
 const unsigned w=std::max(v[0].recommendedImageRectWidth,v[1].recommendedImageRectWidth),h=std::max(v[0].recommendedImageRectHeight,v[1].recommendedImageRectHeight);
 if(w<256||h<256||w>16384||h>16384)return 4;
 FILE* file=nullptr;if(_wfopen_s(&file,output,L"w")||!file)return 5;const bool ok=(includeMaximum?fprintf(file,"%u %u %u %u\n",w,h,
  std::min(v[0].maxImageRectWidth,v[1].maxImageRectWidth),std::min(v[0].maxImageRectHeight,v[1].maxImageRectHeight)):
  fprintf(file,"%u %u\n",w,h))>0;const bool closed=fclose(file)==0;return ok&&closed?0:5;
}
}
