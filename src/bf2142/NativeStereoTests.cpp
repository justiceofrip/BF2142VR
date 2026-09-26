// Exercise the real native callback with a fake camera setter, not a second
// implementation of its routing. No owned game files are needed by this test.
#include "NativeStereo.cpp"
#include <cstdio>
namespace bfvr::bf2142 {
bool scopeFixture=false;
bool IsScopeRender(){return scopeFixture;}
bool IsSecondStereoEye(){return false;}
bool StereoHudBegin(bool){return false;}
void StereoHudEnd(){}
bool RenderStereo(void*,NativeRender,double,float){return true;}
bool ReadNativeComfortCamera(const stereo::Matrix4&,stereo::Matrix4*,const void*){return false;}
bool ReadNativeVehicleCamera(const stereo::Matrix4&,stereo::Matrix4*){return false;}
bool InstallNativeComfort(LogFunction){return false;}
bool InstallNativeHudPointer(LogFunction){return false;}
bool InstallNativeQueryGuard(BYTE*,LogFunction){return false;}
bool InstallNativeMenus(BYTE*,LogFunction){return false;}
bool InstallNativeWorldMarkers(BYTE*,LogFunction){return false;}
CrosshairScope::CrosshairScope(){}
CrosshairScope::~CrosshairScope(){}
}
using namespace bfvr;using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Native stereo failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
using M=stereo::Matrix4;
M At(float x,float y,float z){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
bool Same(const M& a,const M& b){return std::memcmp(&a,&b,sizeof(a))==0;}
void __fastcall StoreCamera(void* view,void*,const M* camera){Write(view,0x40,*camera);Write<DWORD>(view,0x428,15);}
void __fastcall BuildProjection(void* view,void*){Write(view,0x140,At(0,0,0));}
}
int main(){
    std::array<BYTE,viewSize> world{},weapon{},other{};
    setCamera=reinterpret_cast<CameraSetter>(&StoreCamera);
    nativeProjection=reinterpret_cast<ProjectionBuilder>(&BuildProjection);
    Write(weapon.data(),0x40,At(0,100,0));
    CameraInput input{};
    CHECK(SaveView(world.data(),&overrides[0],&input));
    CHECK(SaveView(weapon.data(),&overrides[1],&input));
    const auto savedWorld=world,savedWeapon=weapon;
    overrides[0].eye.world=At(-455.0193f,66,-54.9744f);
    overrides[1].eye.world=At(-.032f,100,0);
    overrides[1].eye.projection=At(0,0,0);overrides[1].eye.projection.values[0][0]=.78f;
    eyeActive=true;handTracking=false;scopeFixture=false;
    const auto nativeWorld=At(-455,66,-55);
    CameraSetterHook(world.data(),nullptr,&nativeWorld);
    CHECK(Same(Read<M>(world.data(),0x40),overrides[0].eye.world));
    // Loading -> spawn, opposite eye, translation and new spawn. Each incoming
    // weapon matrix ALREADY contains the current eye. No stale camera or a
    // second application of head/IPD may replace it.
    for(const auto eye:{At(-455.0193f,66,-54.9744f),At(-454.9807f,66,-55.0256f),At(123,4,567),At(-100,8,200)}){
        CameraSetterHook(weapon.data(),nullptr,&eye);
        CHECK(Same(Read<M>(weapon.data(),0x40),eye));
    }
    ProjectionHook(weapon.data(),nullptr);
    CHECK(Same(Read<M>(weapon.data(),0x140),overrides[1].eye.projection));
    scopeFixture=true;CameraSetterHook(weapon.data(),nullptr,&nativeWorld);
    CHECK(Same(Read<M>(weapon.data(),0x40),overrides[1].eye.world));
    scopeFixture=false;handTracking=true;CameraSetterHook(weapon.data(),nullptr,&nativeWorld);
    CHECK(Same(Read<M>(weapon.data(),0x40),overrides[1].eye.world));
    CameraSetterHook(other.data(),nullptr,&nativeWorld);
    CHECK(Same(Read<M>(other.data(),0x40),nativeWorld));
    EndNativeEye();CHECK(world==savedWorld&&weapon==savedWeapon);
    CameraSetterHook(weapon.data(),nullptr,&nativeWorld);
    CHECK(Same(Read<M>(weapon.data(),0x40),nativeWorld));
    puts("Native first-person camera refresh, no duplicate IPD, scope/legacy overrides and restoration passed.");
    return 0;
}
