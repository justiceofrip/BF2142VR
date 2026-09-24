#include "DesktopSimulation.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include "BodyInventory.h"
#include "NativeOptics.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
float pitch=0,headYaw=0,headDrop=0;
int preset=0,leftPreset=0;stereo::Vec3 trim{},leftTrim{};ULONGLONG last=0;
bool Key(int k){return (GetAsyncKeyState(k)&0x8000)!=0;}
shared::SharedPresentationPose At(float x,float y,float z){shared::SharedPresentationPose p{};p.orientationW=1;p.positionX=x;p.positionY=y;p.positionZ=z;return p;}
}
void BlockDesktopHotkeys(ControllerCommand& c){
    if(Key(VK_LSHIFT))c.blockedPhysicalKeys[DIK_X]=c.blockedPhysicalKeys[DIK_Y]=1;
    for(unsigned key:{DIK_F1,DIK_F2,DIK_F3,DIK_F4,DIK_F5,DIK_F6,DIK_F7,DIK_F8,DIK_F9,DIK_F10,DIK_F11,DIK_F12,
        DIK_NUMPAD0,DIK_NUMPAD2,DIK_NUMPAD4,DIK_NUMPAD6,DIK_NUMPAD8,DIK_ADD,DIK_SUBTRACT,DIK_PRIOR,DIK_NEXT,DIK_END,
        DIK_BACKSLASH,DIK_TAB,DIK_DELETE,DIK_LBRACKET,DIK_RBRACKET,DIK_INSERT,DIK_NUMPAD7,DIK_NUMPAD9,DIK_DECIMAL,DIK_NUMPAD1,DIK_NUMPAD3,DIK_RCONTROL,DIK_RSHIFT,DIK_LCONTROL,DIK_LSHIFT,DIK_LMENU})c.blockedPhysicalKeys[key]=1;
}
void ResetDesktopSimulation(){preset=leftPreset=0;trim=leftTrim={};last=0;pitch=0;headYaw=0;headDrop=0;}
void DesktopFrame(HWND window,shared::SharedRenderRequest& request,shared::SharedControllerSample& sample,bool menu,const stereo::Pose* menuAnchor){
    request={};sample={};const auto now=GetTickCount64();const float dt=last?std::clamp(float(now-last)*.001f,0.f,.05f):0;last=now;
    request.shouldRender=request.viewsValid=request.headPoseValid=request.headPoseTracked=1;
    request.predictedDisplayTime=LONGLONG(now)*1000000;request.headPose=At(0,1.7f,0);
    if(Key(VK_END)){pitch=0;headYaw=0;headDrop=0;}
    headDrop=std::clamp(headDrop+(Key(VK_NUMPAD7)-Key(VK_NUMPAD9))*dt*.75f,0.f,1.3f);
    request.headPose.positionY-=headDrop;
    if(Key(VK_NEXT))pitch=std::max(-1.25f,pitch-dt);if(Key(VK_PRIOR))pitch=std::min(1.1f,pitch+dt);
    headYaw+=float(Key(VK_OEM_6)-Key(VK_OEM_4))*dt;
    request.headPose.orientationX=std::sin(pitch*.5f)*std::cos(headYaw*.5f);
    request.headPose.orientationY=std::sin(headYaw*.5f)*std::cos(pitch*.5f);
    request.headPose.orientationZ=-std::sin(headYaw*.5f)*std::sin(pitch*.5f);
    request.headPose.orientationW=std::cos(pitch*.5f)*std::cos(headYaw*.5f);
    RECT client{};GetClientRect(window,&client);const float aspect=client.bottom>0?float(client.right)/client.bottom:16.f/9;
    for(int eye=0;eye<2;++eye){request.views[eye].pose=request.headPose;request.views[eye].pose.positionX=eye?.032f:-.032f;const float x=std::atan(.72f*aspect),y=std::atan(.72f);request.views[eye].fov={-x,x,y,-y};}
    sample.predictedDisplayTime=request.predictedDisplayTime;
    DWORD owner=0;GetWindowThreadProcessId(GetForegroundWindow(),&owner);
    if(owner!=GetCurrentProcessId())return;
    sample.flags=shared::kControllerSampleFlagSessionFocused;
    const bool adjustLeft=Key(VK_TAB);auto& selectedPreset=adjustLeft?leftPreset:preset;auto& selectedTrim=adjustLeft?leftTrim:trim;
    for(int i=0;i<8;++i)if(Key(VK_F1+i)&&selectedPreset!=i){selectedPreset=i;selectedTrim={};}
    if(Key(VK_NUMPAD0) && preset!=8){preset=8;trim={};}
    const float handSpeed=Key(VK_DECIMAL)?2.4f:.25f;
    selectedTrim.x+=(Key(VK_NUMPAD6)-Key(VK_NUMPAD4))*handSpeed*dt;
    selectedTrim.y+=(Key(VK_ADD)-Key(VK_SUBTRACT))*handSpeed*dt;
    selectedTrim.z+=(Key(VK_NUMPAD2)-Key(VK_NUMPAD8))*handSpeed*dt;
    stereo::Vec3 position{.18f,-.20f,-.48f};
    if(preset==1){stereo::Vec3 aligned;if(ReadNativeSightAlignmentOffset(&aligned))position=aligned;}
    else if(preset>=2)position=BodySlots()[preset-2].offset;
    const auto lp=leftPreset>=2?BodySlots()[leftPreset-2].offset:stereo::Vec3{-.18f,-.22f,-.48f};
    constexpr DWORD flags=shared::kControllerHandFlagAimActive|shared::kControllerHandFlagAimPositionValid|
        shared::kControllerHandFlagAimOrientationValid|shared::kControllerHandFlagAimPositionTracked|shared::kControllerHandFlagAimOrientationTracked|
        shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripOrientationValid|
        shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked|shared::kControllerHandFlagTriggerActive|shared::kControllerHandFlagSqueezeActive;
    for(int i=0;i<2;++i){auto& h=sample.hands[i];h.flags=flags;h.gripPose=i?At(position.x+trim.x,1.7f+position.y+trim.y,position.z+trim.z):At(lp.x+leftTrim.x,1.7f+lp.y+leftTrim.y,lp.z+leftTrim.z);h.aimPose=h.gripPose;}
    sample.hands[1].flags|=shared::kControllerHandFlagThumbstickActive;
    sample.hands[1].thumbstickX=float(Key(VK_DELETE)-Key(VK_INSERT));
    auto& right=sample.hands[1];right.squeezeValue=Key(VK_RCONTROL)?1.f:0.f;sample.hands[0].squeezeValue=(Key(VK_RSHIFT)||Key(VK_OEM_5))?1.f:0.f;
    if(Key(VK_LSHIFT)&&Key('X'))sample.hands[0].buttons|=shared::kControllerHandButtonPrimary;
    if(Key(VK_LSHIFT)&&Key('Y'))sample.hands[0].buttons|=shared::kControllerHandButtonSecondary;
    right.triggerValue=Key(VK_F9)?1.f:0.f;if(Key(VK_F10))right.buttons|=shared::kControllerHandButtonSecondary;
    for(auto& h:sample.hands){h.gripPose.positionY-=headDrop;h.aimPose.positionY-=headDrop;}
    if(menu && client.right>0 && client.bottom>0){
        POINT mouse{};GetCursorPos(&mouse);ScreenToClient(window,&mouse);
        const float u=std::clamp(float(mouse.x)/client.right,0.f,1.f),v=std::clamp(float(mouse.y)/client.bottom,0.f,1.f);
        const auto origin=menuAnchor?menuAnchor->position:stereo::Vec3{0,request.headPose.positionY,0};
        const float x=origin.x+(u-.5f)*1.6f-right.aimPose.positionX,y=origin.y+(.5f-v)*1.6f/aspect-right.aimPose.positionY,z=origin.z-1.5f-right.aimPose.positionZ;
        const float length=std::sqrt(x*x+y*y+z*z),qx=y/length,qy=-x/length,qw=1-z/length,q=std::sqrt(qx*qx+qy*qy+qw*qw);
        right.aimPose.orientationX=qx/q;right.aimPose.orientationY=qy/q;right.aimPose.orientationW=qw/q;
    }
    auto& left=sample.hands[0];left.triggerValue=Key(VK_LSHIFT)?1.f:0.f;
    left.flags|=shared::kControllerHandFlagTriggerTouchActive|shared::kControllerHandFlagThumbTouchActive;
    if(Key(VK_LCONTROL))left.flags|=shared::kControllerHandFlagTriggerTouched;
    if(Key(VK_LMENU))left.flags|=shared::kControllerHandFlagThumbTouched;
    right.flags|=shared::kControllerHandFlagTriggerTouchActive|shared::kControllerHandFlagThumbTouchActive;
    if(Key(VK_NUMPAD1))right.flags|=shared::kControllerHandFlagTriggerTouched;
    if(Key(VK_NUMPAD3))right.flags|=shared::kControllerHandFlagThumbTouched;
    if(Key(VK_F11)){sample.flags=0;for(auto& h:sample.hands)h.flags=0;}
}
}
