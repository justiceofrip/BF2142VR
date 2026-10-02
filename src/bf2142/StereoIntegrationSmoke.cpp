#include "StereoSession.h"
#include "StereoCamera.h"
#include "ComfortCamera.h"
#include "TraversalControls.h"
#include "NativeVehicle.h"
#include "TrackingMath.h"
#include "ControllerInput.h"
#include "NativeHands.h"
#include "NativeOptics.h"
#include "FrameCapture.h"
#include "GrenadeArc.h"
#include "GpuFrameTransfer.h"
#include "NativeExResources.h"
#include <MinHook.h>
#include <wrl/client.h>
#include <d3d9on12.h>
#include <cstdarg>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <vector>
using namespace bfvr;
namespace bfvr::bf2142 {void TestHoldStereoConsumer(bool);unsigned TestGpuPublished();void TestOpenControls(bool);bool TestBeginPausedMenu();}
namespace {
IDirect3DDevice9* device=nullptr;
bf2142::EyeCamera currentEye{};
unsigned renders=0,presents=0,advances=0,suppressed=0,recenters=0;
bool menuFixture=false,worldActive=true;
bool nativeAaOff=false,valid=true,scopeFixture=false,reflexFixture=false,desktopFixture=false,solidFixture=false,hiddenWeapon=false;
std::vector<DWORD> lastDesktop;
double expectedNativeTime=1.0/60;
using Present=HRESULT(STDMETHODCALLTYPE*)(IDirect3DDevice9*,const RECT*,const RECT*,HWND,const RGNDATA*);
Present nativePresent=nullptr;
void Log(const char* format,...) {if(strstr(format,"6DoF neutral"))++recenters;va_list a;va_start(a,format);vprintf(format,a);va_end(a);puts("");}
HRESULT STDMETHODCALLTYPE PresentHook(IDirect3DDevice9* d,const RECT* a,const RECT* b,HWND w,const RGNDATA* r) {
    if(bf2142::SuppressStereoPresent(d)){++suppressed;return S_OK;}
    bf2142::StereoPresent(d);
    static unsigned checkedRenders=0;
    if(scopeFixture && renders!=checkedRenders){
        checkedRenders=renders;bf2142::FrameCapture check;std::vector<DWORD> desktop;
        valid=SUCCEEDED(check.Read(d,DXGI_FORMAT_B8G8R8A8_UNORM,desktop)) && valid;
        // The magnified source fills the scope texture; the desktop must still
        // contain the normal eye background after restore and HUD composition.
        valid=!desktop.empty() && desktop.back()==0xff102030 && valid;
    }
    if(desktopFixture){bf2142::FrameCapture frame;valid=SUCCEEDED(frame.Read(d,DXGI_FORMAT_B8G8R8A8_UNORM,lastDesktop)) && valid;}
    ++presents;return nativePresent(d,a,b,w,r);
}
bool __fastcall Scene(void*,void*,double delta,float) {
    ++renders;if(delta!=0){++advances;valid=valid&&delta==expectedNativeTime;}
    valid=SUCCEEDED(device->BeginScene()) && valid;
    valid=SUCCEEDED(device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff102030,1,0)) && valid;
    bf2142::EyeCamera markerHead{},markerEye{};
    static stereo::Matrix4 firstMarkerHead{};
    if(bf2142::IsScopeRender())valid=!bf2142::ReadStereoMarkerFrame(&markerHead,&markerEye)&&valid;
    else {
        valid=bf2142::ReadStereoMarkerFrame(&markerHead,&markerEye)&&valid;
        if(!bf2142::IsSecondStereoEye())firstMarkerHead=markerHead.world;
        else for(unsigned i=0;i<4;++i)for(unsigned j=0;j<4;++j)
            valid=(std::abs(firstMarkerHead.values[i][j]-markerHead.world.values[i][j])<.001f)&&valid;
    }
    const auto inverse=bf2142::InverseRigid(currentEye.world);
    D3DMATRIX world{};for(int i=0;i<4;++i)world.m[i][i]=1;
    device->SetTransform(D3DTS_WORLD,&world);
    if(inverse)device->SetTransform(D3DTS_VIEW,reinterpret_cast<const D3DMATRIX*>(&*inverse));else valid=false;
    device->SetTransform(D3DTS_PROJECTION,reinterpret_cast<const D3DMATRIX*>(&currentEye.projection));
    device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);
    device->SetTexture(0,nullptr);device->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);
    device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_LIGHTING,FALSE);
    device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
    device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
    struct V {float x,y,z;DWORD color;};
    const V vertices[]={{-.5f,-.5f,3,0xffff0000},{-.5f,.5f,3,0xffff0000},{.5f,-.5f,3,0xffff0000},{.5f,.5f,3,0xffff0000}};
    valid=SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V))) && valid;
    if(solidFixture && !hiddenWeapon){D3DRECT gun{270,190,310,230};device->Clear(1,&gun,D3DCLEAR_TARGET,0xff0000ff,1,0);}
    if(bf2142::IsScopeRender()){}
    else if(bf2142::StereoHudBegin()) {
        D3DRECT hud{0,0,40,20};device->Clear(1,&hud,D3DCLEAR_TARGET,0xff00ff00,1,0);
        bf2142::StereoHudEnd();
    } else valid=false;
    if(nativeAaOff)device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS,FALSE);
    valid=SUCCEEDED(device->EndScene()) && valid;
    // Model a native renderer that presents internally. The stereo loop must
    // hold both of these calls until readback, then present once for the pair.
    valid=SUCCEEDED(device->Present(nullptr,nullptr,nullptr,nullptr)) && valid;
    return valid;
}
std::vector<DWORD> ReadBmp(const std::wstring& name) {
    std::ifstream file(name,std::ios::binary);BITMAPFILEHEADER h{};BITMAPINFOHEADER i{};
    file.read(reinterpret_cast<char*>(&h),sizeof(h));file.read(reinterpret_cast<char*>(&i),sizeof(i));
    if(!file || h.bfType!=0x4d42 || i.biWidth!=320 || i.biHeight!=-240 || i.biBitCount!=32)return {};
    file.seekg(h.bfOffBits);std::vector<DWORD> result(320*240);file.read(reinterpret_cast<char*>(result.data()),result.size()*4);
    return file?result:std::vector<DWORD>{};
}
double Centroid(const std::vector<DWORD>& p) {
    double sum=0,n=0;for(size_t i=0;i<p.size();++i)if(p[i]==0xffff0000){sum+=i%320;++n;}
    return n>50?sum/n:-10000;
}
}
// Only native game entry points are replaced. The real session, GPU readback,
// UI capture, stereo math and presentation ownership run unchanged.
namespace bfvr::bf2142 {
bool ReadNativeStance(int*){return false;}
bool NativeSnapTurnAvailable(){return false;}
bool RequestNativeSnapTurn(float,std::int64_t){return false;}
void ClearNativeSnapTurn(){}
bool ConfigureNativeMovement(bool,float){return false;}
bool ConfigureNativeLookPitch(bool,const stereo::Pose&){return false;}
bool ReadNativeTraversal(TraversalSample*){return false;}
bool ReadNativeVehicle(VehicleSample*){return false;}
void UpdateNativeVehicle(const VehicleSample*,const shared::SharedControllerSample*,const stereo::Pose&,const stereo::Pose&,float,float,ControllerCommand&){}
void RecenterNativeVehicle(){}
void ConfigurePhysicalCamera(bool){}
void SetCrosshairHidden(bool){}
bool InstallNativeOptics(LogFunction){return true;}
void DisableNativeOptics(){valid=false;}
bool ReadNativeOptic(GunOptic* optic){
    if(!scopeFixture && !reflexFixture)return false;
    const auto* d=FindGunOptic(reflexFixture?"eu_mg":"eu_ar_rifle");auto gun=currentEye.world;
    // Keep the fixture optic 30 cm in front of the right eye as it moves.
    for(int j=0;j<3;++j)gun.values[3][j]+=gun.values[2][j]*(.3f-d->center.z)-gun.values[1][j]*d->center.y-gun.values[0][j]*d->center.x;
    *optic={d,gun,reflexFixture?1.f:2.f};return true;
}
bool BeginNativeScope(void*,const stereo::Matrix4& world,const stereo::FovTangents& fov){
    const auto camera=MakeEyeCamera({world,.04f,100}, {}, {},fov);if(!camera)return false;currentEye=*camera;return true;
}
bool InstallNativeHands(LogFunction){return true;}
void PublishNativeHands(const shared::SharedControllerSample*,const stereo::Pose&,const stereo::Pose&,float,float,const RadioFrame&){}
void ClearNativeHands(){}
unsigned TakeNetworkFistBumps(bool){return 0;}
bool ReadNativeWeaponProjection(stereo::Matrix4* p){*p=currentEye.projection;return true;}
void DrawNativeOptic(std::vector<DWORD>&,UINT,UINT,DXGI_FORMAT,const EyeCamera&){}
bool InstallNativeStereo(LogFunction){return true;}
bool NativeViewsAvailable(void*){return true;}
bool NativeWorldActive(void*){return worldActive;}
bool BeginNativeEye(void*,const stereo::Pose& reference,const shared::SharedPresentationView& eye) {
    CameraInput source{};for(int i=0;i<4;++i)source.world.values[i][i]=1;source.nearPlane=.04f;source.farDelta=100;
    const float kick=.12f*std::sin(float(renders)*.27f);
    source.world.values[1]={0,std::cos(kick),std::sin(kick),0};
    source.world.values[2]={0,-std::sin(kick),std::cos(kick),0};
    const auto stable=MakeComfortCamera(source.world,0);if(!stable)return false;source.world=*stable;
    const stereo::Pose pose{{eye.pose.positionX,eye.pose.positionY,eye.pose.positionZ},
        {eye.pose.orientationX,eye.pose.orientationY,eye.pose.orientationZ,eye.pose.orientationW}};
    const auto camera=MakeEyeCamera(source,reference,pose,{std::tan(eye.fov.angleLeft),std::tan(eye.fov.angleRight),std::tan(eye.fov.angleUp),std::tan(eye.fov.angleDown)});
    if(!camera)return false;currentEye=*camera;return true;
}
bool ReadNativeEyeCamera(EyeCamera* camera){if(!camera)return false;*camera=currentEye;return true;}
void EndNativeEye(){}
void ConfigureNativeTracking(float,float,const stereo::Pose*,const stereo::Pose*,const stereo::Pose*){}
bool StartControllerInput(LogFunction){return true;}
void PublishControllerCommand(const ControllerCommand&,bool){}
void ClearControllerCommand(){}
}
int wmain(int argc,wchar_t** argv) {
    bool gpuFixture=false;for(int i=1;i<argc;++i)gpuFixture|=wcscmp(argv[i],L"--gpu-transfer")==0;
    bool nativeEx=false;for(int i=1;i<argc;++i)nativeEx|=wcscmp(argv[i],L"--native-ex")==0;
    gpuFixture|=nativeEx;
    if(gpuFixture)SetEnvironmentVariableW(L"BF2142VR_GPU_TRANSFER",nativeEx?L"dx9ex":L"1");
    bool on12=gpuFixture&&!nativeEx;for(int i=1;i<argc;++i)on12|=wcscmp(argv[i],L"--on12")==0;
    bool msaa=false;for(int i=1;i<argc;++i){msaa|=wcscmp(argv[i],L"--msaa")==0;scopeFixture|=wcscmp(argv[i],L"--scope")==0;reflexFixture|=wcscmp(argv[i],L"--reflex")==0;desktopFixture|=wcscmp(argv[i],L"--desktop")==0;solidFixture|=wcscmp(argv[i],L"--solid")==0;}
    bool msaa8=false;for(int i=1;i<argc;++i){nativeAaOff|=wcscmp(argv[i],L"--native-aa-off")==0;msaa8|=wcscmp(argv[i],L"--msaa8")==0;}
    for(int i=1;i<argc;++i)menuFixture|=wcscmp(argv[i],L"--menus")==0;
    const unsigned passes=scopeFixture?3u:2u;
    wchar_t path[32768]{};GetModuleFileNameW(nullptr,path,32768);
    const auto folder=std::filesystem::path(path).parent_path()/L"stereo-gpu-check";
    std::filesystem::create_directories(folder);
    const std::wstring prefix=(folder/(L"frame-"+std::to_wstring(GetCurrentProcessId()))).wstring();
    if(solidFixture){
        const auto asset=prefix+L"-face.bin",config=prefix+L"-config.ini";
        std::ofstream f(asset,std::ios::binary);f.write("BFHP0001",8);
        const auto word=[&](unsigned n){f.write(reinterpret_cast<char*>(&n),4);};
        word(1);word(7);f.write("fixture",7);word(1);word(8);word(36);word(128);word(128);
        for(int z:{-1,1})for(int y:{-1,1})for(int x:{-1,1}){float v[]={x*.15f,y*.15f,z*.15f,0,0};f.write(reinterpret_cast<char*>(v),sizeof(v));}
        const unsigned short faces[]={0,2,3,0,3,1,4,5,7,4,7,6,0,1,5,0,5,4,2,6,7,2,7,3,0,4,6,0,6,2,1,3,7,1,7,5};
        f.write(reinterpret_cast<const char*>(faces),sizeof(faces));std::vector<DWORD> texture(128*128,0xffffffff);f.write(reinterpret_cast<const char*>(texture.data()),texture.size()*4);f.close();
        WritePrivateProfileStringW(L"VR",L"BodyEquipmentFile",asset.c_str(),config.c_str());
        WritePrivateProfileStringW(L"VR",L"WeaponFaceFade",L"1",config.c_str());SetEnvironmentVariableW(L"BF2142VR_CONFIG",config.c_str());
    }
    HWND window=CreateWindowExW(0,L"STATIC",L"Hidden stereo integration check",WS_OVERLAPPED,0,0,320,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    Microsoft::WRL::ComPtr<IDirect3D9> factory;
    if(nativeEx)factory.Attach(bf2142::CreateGpuTransferFactory(D3D_SDK_VERSION));
    else if(on12){
        // Developer-only backend compatibility probe.
        const auto runtime=LoadLibraryExW(L"d3d9.dll",nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);
        const auto create=runtime?reinterpret_cast<PFN_Direct3DCreate9On12>(GetProcAddress(runtime,"Direct3DCreate9On12")):nullptr;
        if(!create){puts("D3D9On12 factory unavailable.");return 77;}
        D3D9ON12_ARGS args{};args.Enable9On12=TRUE;
        factory.Attach(create(D3D_SDK_VERSION,&args,1));
        puts("Developer D3D9On12 backend selected; existing stereo/HUD/scope code unchanged.");
    }else factory.Attach(Direct3DCreate9(D3D_SDK_VERSION));
    D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.SwapEffect=D3DSWAPEFFECT_DISCARD;p.hDeviceWindow=window;
    p.BackBufferWidth=320;p.BackBufferHeight=240;p.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    if(msaa){p.MultiSampleType=D3DMULTISAMPLE_4_SAMPLES;p.EnableAutoDepthStencil=TRUE;p.AutoDepthStencilFormat=D3DFMT_D24S8;}
    bool hardware=false;for(int i=1;i<argc;++i)hardware|=wcscmp(argv[i],L"--hardware")==0;
    if(msaa8)p.MultiSampleType=D3DMULTISAMPLE_8_SAMPLES;
    if(!factory)return 77;
    bool pure=false;for(int i=1;i<argc;++i)pure|=wcscmp(argv[i],L"--pure")==0;
    const DWORD creation=(hardware?D3DCREATE_HARDWARE_VERTEXPROCESSING:D3DCREATE_SOFTWARE_VERTEXPROCESSING)|(pure?D3DCREATE_PUREDEVICE:0);
    if(nativeEx){
        Microsoft::WRL::ComPtr<IDirect3D9Ex> apiEx;IDirect3DDevice9Ex* d=nullptr;
        if(FAILED(factory.As(&apiEx))||FAILED(apiEx->CreateDeviceEx(0,D3DDEVTYPE_HAL,window,creation,&p,nullptr,&d)))return 77;device=d;
    }else if(FAILED(factory->CreateDevice(0,D3DDEVTYPE_HAL,window,creation,&p,&device)))return 77;
    if(on12){
        Microsoft::WRL::ComPtr<IDirect3DDevice9On12> interop;
        if(FAILED(device->QueryInterface(IID_PPV_ARGS(&interop)))){puts("Requested backend was not activated.");return 1;}
    }
    if(MH_Initialize()!=MH_OK)return 1;
    if(nativeEx&&!bf2142::InstallNativeExResources(device,Log))return 1;
    auto** table=*reinterpret_cast<void***>(device);
    if(MH_CreateHook(table[17],reinterpret_cast<void*>(&PresentHook),reinterpret_cast<void**>(&nativePresent))!=MH_OK || MH_EnableHook(table[17])!=MH_OK)return 2;
    if(!bf2142::StartStereo(desktopFixture?L"@desktop":L"@diagnostic",prefix,Log))return 3;
    bf2142::StereoDeviceCreated(device);
    for(unsigned pair=0;pair<60;++pair) {
        expectedNativeTime=1000.0+double(pair)/60; // native clock argument is opaque, not a clamped timestep
        valid=bf2142::RenderStereo(reinterpret_cast<void*>(1),reinterpret_cast<bf2142::NativeRender>(&Scene),expectedNativeTime,0) && valid;
        // The real game's outer loop presents again after the renderer returns.
        // This must not publish a flat scene on the UI panel or recenter the next pair.
        valid=SUCCEEDED(device->Present(nullptr,nullptr,nullptr,nullptr)) && valid;
    }
    // Hold the actual consumer fence: no flat camera, animation or Present.
    bf2142::TestHoldStereoConsumer(true);
    const auto beforeRenders=renders,beforePresents=presents,beforeAdvances=advances;
    for(int i=0;i<6;++i)valid=bf2142::RenderStereo(reinterpret_cast<void*>(1),reinterpret_cast<bf2142::NativeRender>(&Scene),1.0/120,0)&&valid;
    valid=renders==beforeRenders&&presents==beforePresents&&advances==beforeAdvances&&valid;
    bf2142::TestHoldStereoConsumer(false);
    if(desktopFixture){
        if(gpuFixture){
            const auto fast=bf2142::TestGpuPublished();
            valid=(fast==60u)&&valid;
            printf("GPU fixture: %u fast frames; scope/reflex stay on GPU.\n",fast);
        }
        valid=lastDesktop.size()==320u*240u && valid;
        if(!lastDesktop.empty())valid=lastDesktop.front()==0xff00ff00 && lastDesktop.back()==0xff102030 && Centroid(lastDesktop)>0 && valid;
        valid=renders==60*passes && advances==60 && suppressed==60*passes && presents==120 && valid;
        if(menuFixture){
            // Deployment while world rendering continues must not return to CPU transport.
            bf2142::TestOpenControls(true);
            for(unsigned frame=0;frame<30;++frame){
                valid=bf2142::RenderStereo(reinterpret_cast<void*>(1),reinterpret_cast<bf2142::NativeRender>(&Scene),expectedNativeTime,0)&&valid;
                valid=SUCCEEDED(device->Present(nullptr,nullptr,nullptr,nullptr))&&valid;
            }
            valid=bf2142::TestGpuPublished()==90&&renders==60*passes+60&&advances==90&&valid;
            valid=lastDesktop.size()==320*240&&lastDesktop.front()==0xff00ff00&&lastDesktop.back()==0xff102030&&valid;
            // Paused standalone Flash rendering: same GPU UI, no world replay.
            bf2142::TestOpenControls(false);
            for(unsigned frame=0;frame<30;++frame){
                valid=SUCCEEDED(device->BeginScene())&&valid;
                if(bf2142::TestBeginPausedMenu()){
                    D3DRECT panel{30,40,100,90};valid=SUCCEEDED(device->Clear(1,&panel,D3DCLEAR_TARGET,0xff00ffff,1,0))&&valid;
                    bf2142::StereoHudEnd();
                }else valid=false;
                valid=SUCCEEDED(device->EndScene())&&valid;
                valid=SUCCEEDED(device->Present(nullptr,nullptr,nullptr,nullptr))&&valid;
            }
            valid=bf2142::TestGpuPublished()==120&&renders==60*passes+60&&advances==90&&valid;
            valid=lastDesktop.size()==320*240&&lastDesktop[50*320+50]==0xff00ffff&&(lastDesktop.front()&0xffffff)==0&&valid;
            // Main menu has no world at all. Rapid Presents are runtime paced, not forced to 30 Hz.
            worldActive=false;
            for(unsigned frame=0;frame<30;++frame){
                valid=SUCCEEDED(device->Clear(0,nullptr,D3DCLEAR_TARGET,0xff234567,1,0))&&valid;
                valid=SUCCEEDED(device->Present(nullptr,nullptr,nullptr,nullptr))&&valid;
            }
            valid=bf2142::TestGpuPublished()==150&&renders==60*passes+60&&advances==90&&valid;
            valid=lastDesktop.size()==320*240&&lastDesktop.front()==0xff234567&&lastDesktop.back()==0xff234567&&valid;
            worldActive=true;
            valid=bf2142::RenderStereo(reinterpret_cast<void*>(1),reinterpret_cast<bf2142::NativeRender>(&Scene),expectedNativeTime,0)&&valid;
            valid=bf2142::TestGpuPublished()==151&&renders==61*passes+60&&advances==91&&valid;
            printf("Menu transitions: GPU publications=%u world replays=%u time advances=%u valid=%d\n",bf2142::TestGpuPublished(),renders,advances,valid);
        }
        bf2142::StereoReset();device->Release();DestroyWindow(window);
        puts(valid?"Desktop VR GPU composite: one eye, visible HUD, native replay and no headset/presenter passed.":"Desktop VR GPU composite FAILED.");
        return valid?0:1;
    }
    const auto left=ReadBmp(prefix+L"-3-0.bmp"),right=ReadBmp(prefix+L"-3-1.bmp"),ui=ReadBmp(prefix+L"-3-2.bmp"),moved=ReadBmp(prefix+L"-60-0.bmp");
    const double a=Centroid(left),b=Centroid(right),c=Centroid(moved);
    valid=left.size()==320u*240u && right.size()==left.size() && ui.size()==left.size() && moved.size()==left.size() && valid;
    if(!left.empty() && !right.empty() && !ui.empty()) {
        valid=left.front()==0xff102030 && right.front()==0xff102030 && ui.front()==0xff00ff00 && ui.back()==0 && valid;
        for(const auto* eye:{&left,&right,&moved})for(DWORD color:*eye)if(color==0xff00ff00)valid=false;
        if(solidFixture)valid=left[210*320+290]==0xff0000ff && right[210*320+290]==0xff0000ff && !hiddenWeapon && valid;
    }
    valid=a>0 && b>0 && c>0 && a>b && std::abs(c-a)>5 && presents==120 && recenters==1 && suppressed==60*passes && renders==60*passes && advances==60 && valid;
    printf("GPU stereo: left centroid=%.2f right=%.2f moved=%.2f; native calls=%u time advances=%u suppressed=%u actual Presents=%u\n",a,b,c,renders,advances,suppressed,presents);
    // Paused Flash menus still draw/present while the local player exists and
    // the native world renderer stops. Capture only their explicit UI batch.
    for(unsigned frame=0;frame<120;++frame){
        valid=SUCCEEDED(device->BeginScene()) && valid;
        if(bf2142::StereoHudBegin(true)){
            D3DRECT panel{30,40,100,90};valid=SUCCEEDED(device->Clear(1,&panel,D3DCLEAR_TARGET,0xff00ffff,1,0)) && valid;
            bf2142::StereoHudEnd();
        }else valid=false;
        valid=SUCCEEDED(device->EndScene()) && valid;
        valid=SUCCEEDED(device->Present(nullptr,nullptr,nullptr,nullptr)) && valid;
    }
    const auto menu=ReadBmp(prefix+L"-menu.bmp"),emptyLeft=ReadBmp(prefix+L"-menu-left.bmp"),emptyRight=ReadBmp(prefix+L"-menu-right.bmp");
    valid=menu.size()==320u*240u && emptyLeft.size()==menu.size() && emptyRight.size()==menu.size() && valid;
    if(!menu.empty())valid=menu[50*320+50]==0xff00ffff && menu.front()==0 && menu.back()==0 && valid;
    for(const auto* eye:{&emptyLeft,&emptyRight})for(auto color:*eye)if(color!=0xff000000)valid=false;
    valid=renders==60*passes && advances==60 && presents==240 && recenters==1 && valid;
    valid=bf2142::RenderStereo(reinterpret_cast<void*>(1),reinterpret_cast<bf2142::NativeRender>(&Scene),expectedNativeTime,0) && valid;
    valid=renders==61*passes && advances==61 && suppressed==61*passes && recenters==1 && valid;
    printf("Pause/resume: 120 standalone UI frames, world calls=%u neutral captures=%u; no duplicated scene.\n",renders,recenters);
    bf2142::StereoReset();device->Release();DestroyWindow(window);
    puts(valid?"Complete stereo loop: distinct eyes, head translation/yaw, clean UI isolation and extra desktop Present routing passed.":"Stereo GPU integration FAILED.");
    return valid?0:1;
}
namespace bfvr::bf2142 { void PublishNativeHudPointer(bool,float,float,bool){} }

namespace bfvr::bf2142 {
void ConfigureAutomaticAds(bool){}
void UpdateAutomaticAds(const std::array<EyeCamera,2>&){}
void RequestAutomaticAds(bool){}
void ResetAutomaticAds(){}
bool HandlesAutomaticAds(){return false;}
bool AutomaticAdsButton(bool){return false;}
}

namespace bfvr::bf2142 {bool ReadNativeInventory(std::array<bool,10>*,int*,std::array<std::array<char,49>,10>*,std::uint64_t*){return false;}bool ReadNativeSightAlignmentOffset(stereo::Vec3*){return false;}}

namespace bfvr::bf2142 {void EnableFingerPoses(bool){}}

namespace bfvr::bf2142 {void SetDesktopInput(bool){}}

namespace bfvr::bf2142 {void ConfigureMotionActions(bool){} void ResetMotionActions(){} bool UpdateNativeMotion(const shared::SharedControllerSample*,const stereo::Pose&,ControllerCommand&){return false;} }

namespace bfvr::bf2142 {void HideNativeWeaponForReplay(bool hide){hiddenWeapon=hide;} bool ReadTrackedWeaponFrame(TrackedWeaponFrame* frame,bool){
    if(!solidFixture)return false;frame->world=currentEye.world;strcpy_s(frame->name.data(),frame->name.size(),"fixture");return true;}}

namespace bfvr::bf2142 {void SetNativeWeaponHeld(bool){} bool NativeWeaponHeld(){return true;} bool NativeWeaponInputReady(){return true;} bool ReadNativeGrenadeTrajectory(GrenadeTrajectory*){return false;}}

namespace bfvr::bf2142 {struct SupportFrame;struct SupportAmmo;void PublishNativeSupport(const SupportFrame&){} bool ReadSupportAmmo(std::uint64_t,int,SupportAmmo*){return false;}}
