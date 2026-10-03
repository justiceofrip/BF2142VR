#include "ControllerPolicy.h"
#include "MenuPointer.h"
#include "InputOverlay.h"
#include "RecenterPolicy.h"
#include "MenuWindowInput.h"
#include <limits>
#include "TrackingMath.h"
#include "StereoCamera.h"
#include "MenuRoom.h"
#include <filesystem>
#include "stereo/UiPointerMath.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <cmath>
#include <cstdio>
using namespace bfvr;
bool Close(float a,float b){return std::abs(a-b)<.0002f;}
namespace {
bool MenuMessages() {
    // A hidden, owned window exercises delivery and ordering without touching
    // desktop focus, the physical cursor, or the user's running game.
    HWND window=CreateWindowExW(0,L"STATIC",L"BF2142 input test",0,0,0,10,10,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window)return false;
    struct Counts {int move=0,down=0,up=0,keyDown=0,keyUp=0,escape=0;bool coordinates=true;};
    const auto drain=[&](){Counts c;MSG m{};while(PeekMessageW(&m,window,0,0,PM_REMOVE)){
        if(m.message==WM_MOUSEMOVE)++c.move;
        if(m.message==WM_LBUTTONDOWN){++c.down;c.coordinates &= LOWORD(m.lParam)==123 && HIWORD(m.lParam)==45 && m.wParam==MK_LBUTTON;}
        if(m.message==WM_LBUTTONUP)++c.up;
        if(m.message==WM_CHAR && m.wParam==VK_ESCAPE)++c.escape;
        if(m.message==WM_KEYDOWN && m.wParam==VK_ESCAPE)++c.keyDown;
        if(m.message==WM_KEYUP && m.wParam==VK_ESCAPE)++c.keyUp;
    }return c;};
    drain();bf2142::MenuWindowInput input;POINT point{123,45};
    input.Update(window,true,&point,false,false);auto c=drain();bool ok=c.move==1 && !c.down && !c.up;
    input.Update(window,true,&point,true,false);c=drain();ok &= c.down==1 && !c.up && c.coordinates;
    input.Update(window,true,&point,true,false);c=drain();ok &= !c.down && !c.up;
    input.Update(window,false,&point,true,false);c=drain();ok &= c.up==1 && !c.down;
    input.ObserveGameplayEscape(true);input.Update(window,true,nullptr,false,true);c=drain();ok &= !c.escape && !c.keyDown && !c.keyUp;
    input.Update(window,true,nullptr,false,false);drain();
    input.Update(window,true,nullptr,false,true);input.Update(window,true,nullptr,false,true);c=drain();ok &= c.escape==1 && !c.keyDown && !c.keyUp;
    input.Release(window);c=drain();ok &= !c.escape && !c.keyDown && !c.keyUp;
    input.Update(window,true,&point,true,false);drain();input.Update(window,true,nullptr,false,false);c=drain();ok &= c.up==1;
    input.Update(window,true,&point,true,false,100);drain();
    ok &= !input.Expired(250) && input.Expired(251);input.Release(window);c=drain();
    ok &= c.up==1 && !input.Expired(400);
    input.Release(window);DestroyWindow(window);return ok;
}
}
int main(){
    shared::SharedRenderRequest r{};r.predictedDisplayTime=1000000000;r.headPoseValid=r.headPoseTracked=1;
    shared::SharedControllerSample s{};s.predictedDisplayTime=r.predictedDisplayTime;s.flags=shared::kControllerSampleFlagSessionFocused;
    if(!bf2142::MatchingControllerSample(s,8,r,8)||bf2142::MatchingControllerSample(s,7,r,8))return 1;
    ++s.predictedDisplayTime;if(bf2142::MatchingControllerSample(s,8,r,8))return 2;--s.predictedDisplayTime;
    bf2142::ControllerPolicyState policy{};s.hands[0].flags=shared::kControllerHandFlagThumbstickActive;s.hands[0].thumbstickY=1;
    s.hands[1].flags=shared::kControllerHandFlagTriggerActive|shared::kControllerHandFlagThumbstickActive;
    s.hands[1].triggerValue=1;s.hands[1].buttons=shared::kControllerHandButtonThumbstick;
    auto c=bf2142::MapControllers(policy,s,true,0,600);
    if(!c.keys[DIK_W] || !c.buttons[0] || !c.recenter || c.mouseX)return 3;
    s.predictedDisplayTime+=11000000;c=bf2142::MapControllers(policy,s,true,0,600);if(c.recenter)return 4;
    s.flags=0;c=bf2142::MapControllers(policy,s,true,0,600);if(c.keys[DIK_W] || c.buttons[0])return 5;
    s.flags=1;s.hands[1].buttons=0;s.hands[1].triggerValue=0;
    c=bf2142::MapControllers(policy,s,true,1.5707963f,600);if(!c.keys[DIK_D] || c.keys[DIK_W])return 6;
    c=bf2142::MapControllers(policy,s,false,0,600);if(c.keys[DIK_W] || !c.keys[DIK_UP])return 7;
    stereo::Matrix4 base{};for(int i=0;i<4;++i)base.values[i][i]=1;base.values[3][0]=30;
    stereo::Pose head{},grip{},current{};head.position.y=1.7f;grip.position={.2f,1.3f,-.4f};current=grip;
    current.position.x+=.25f;
    const auto weapon=bf2142::TrackedWeaponCamera(base,base,head,grip,current,1);
    if(!weapon || !Close(weapon->values[3][0],29.75f))return 8;
    const auto inverse=bf2142::InverseRigid(*weapon);if(!inverse)return 9;
    const auto identity=bf2142::Multiply(*weapon,*inverse);if(!Close(identity.values[3][0],0))return 10;
    // Recenter removes pitch/roll from the reference; vertical lean stays physical.
    head.orientation={.3f,0,0,.9539392f};const auto upright=stereo::MakeYawOnlyUiAnchor(head);if(!upright || !Close(upright->orientation.x,0))return 11;
    bf2142::CameraInput camera{base,.04f,300};auto eye=*upright;eye.position.y-=.3f;
    const auto moved=bf2142::MakeEyeCamera(camera,*upright,eye,{-1,1,1,-1},2,.1f);
    if(!moved || !Close(moved->world.values[3][1],-.5f))return 12;
    current.position.x+=5;if(bf2142::TrackedWeaponCamera(base,base,head,grip,current,1))return 13;
    // Device state and buffered events share motion ownership. Reading both
    // APIs must not turn or scroll twice; releases must preserve physical keys.
    bf2142::InputOverlayState keyboard{},mouse{};
    bf2142::ControllerCommand held{};held.keys[DIK_W]=0x80;held.buttons[0]=0x80;held.mouseX=11;held.wheel=-120;
    DIDEVICEOBJECTDATA events[4]{};DWORD n=0;
    bf2142::OverlayDeviceEvents(keyboard,true,sizeof(events[0]),events,n,4,true,held,1,10);
    if(n!=1 || events[0].dwOfs!=DIK_W || keyboard.virtualKeys[DIK_W])return 14;
    n=0;bf2142::OverlayDeviceEvents(keyboard,true,sizeof(events[0]),events,n,4,false,held,1,10);
    if(n!=1 || !keyboard.virtualKeys[DIK_W])return 15;
    // A full physical event buffer defers the release instead of losing it.
    n=1;events[0]={};bf2142::OverlayDeviceEvents(keyboard,true,sizeof(events[0]),events,n,1,false,{},0,11);
    if(!keyboard.virtualKeys[DIK_W])return 16;
    n=0;bf2142::OverlayDeviceEvents(keyboard,true,sizeof(events[0]),events,n,4,false,{},0,12);
    if(n!=1 || events[0].dwData || keyboard.virtualKeys[DIK_W])return 17;
    BYTE physical[256]{};physical[DIK_W]=0x80;
    bf2142::OverlayDeviceState(keyboard,true,256,physical,held,2);
    n=0;bf2142::OverlayDeviceEvents(keyboard,true,sizeof(events[0]),events,n,4,false,held,2,13);
    n=0;bf2142::OverlayDeviceEvents(keyboard,true,sizeof(events[0]),events,n,4,false,{},0,14);
    if(n!=1 || events[0].dwData!=0x80)return 18;
    DIMOUSESTATE2 m{};bf2142::OverlayDeviceState(mouse,false,sizeof(m),&m,held,3);
    if(m.lX!=11 || m.lZ!=-120 || !m.rgbButtons[0])return 19;
    n=0;bf2142::OverlayDeviceEvents(mouse,false,sizeof(events[0]),events,n,4,false,held,3,15);
    if(n!=1 || events[0].dwOfs!=DIMOFS_BUTTON0)return 20;
    n=0;bf2142::OverlayDeviceEvents(mouse,false,sizeof(events[0]),events,n,4,false,{},0,16);
    if(n!=1 || events[0].dwData)return 21;
    s.hands[0].flags|=shared::kControllerHandFlagTriggerActive;s.hands[0].triggerValue=1;
    s.hands[0].buttons=shared::kControllerHandButtonPrimary;
    c=bf2142::MapControllers(policy,s,true,0,600);if(c.wheel!=-120 || c.keys[DIK_R])return 22;
    c=bf2142::MapControllers(policy,s,true,0,600);if(c.wheel)return 23;
    s.hands[1].flags|=shared::kControllerHandFlagSqueezeActive;
    s.hands[1].squeezeValue=std::numeric_limits<float>::infinity();
    s.hands[1].thumbstickX=std::numeric_limits<float>::quiet_NaN();
    c=bf2142::MapControllers(policy,s,true,0,600);if(c.buttons[1] || c.mouseX)return 24;
    // Same controller ray drives the cursor visual and absolute click target.
    auto& hand=s.hands[1];hand={};
    hand.flags=shared::kControllerHandFlagAimActive|shared::kControllerHandFlagAimPositionValid|shared::kControllerHandFlagAimOrientationValid|
        shared::kControllerHandFlagAimPositionTracked|shared::kControllerHandFlagAimOrientationTracked;
    hand.aimPose.orientationW=1;
    auto hit=bf2142::MenuRayTarget(hand,{},1280,720,2048,2048);
    if(!hit || !Close(hit->canvas.pixelX,640) || !Close(hit->canvas.pixelY,360))return 25;
    hand.aimPose.positionX=.4f;hit=bf2142::MenuRayTarget(hand,{},1280,720,2048,2048);
    if(!hit || !Close(hit->canvas.pixelX,960) || !Close(hit->end.x,.4f))return 26;
    hand.aimPose.positionY=.3f;
    hit=bf2142::MenuRayTarget(hand,{},2528,2704,2048,2048,true);
    if(!hit||!Close(hit->canvas.normalizedX,.75f)||!Close(hit->canvas.normalizedY,1.f/6)||!Close(hit->end.y,.3f))return 125;
    const auto desktopHit=bf2142::MenuRayTarget(hand,{},2528,2704,1600,900,true);
    if(!desktopHit||!Close(desktopHit->canvas.normalizedY,hit->canvas.normalizedY))return 126;
    hand.aimPose.positionY=.7f;
    if(bf2142::MenuRayTarget(hand,{},2528,2704,2048,2048,true))return 127;
    hand.aimPose.positionY=.7f;if(bf2142::MenuRayTarget(hand,{},1280,720,2048,2048))return 27; // transparent padding
    hand.aimPose.positionY=0;hand.flags&=~shared::kControllerHandFlagAimPositionTracked;
    if(bf2142::MenuRayTarget(hand,{},1280,720,2048,2048))return 28;
    bf2142::MenuClickState click;
    if(click.Update(true,true,true) || click.Update(true,true,true))return 29; // Held fire never clicks a newly opened menu.
    if(click.Update(true,true,false) || !click.Update(true,true,true) || click.Update(true,true,true))return 30;
    click.Update(true,false,false);if(click.Update(true,false,true) || click.Update(true,true,true))return 31;
    click.Update(false,true,false);if(click.Update(true,true,true))return 32;
    std::vector<DWORD> laserLeft(320*240),laserRight(320*240),laserUi(320*240);
    r.views[0].pose.orientationW=r.views[1].pose.orientationW=1;
    r.views[0].pose.positionX=-.032f;r.views[1].pose.positionX=.032f;
    r.views[0].fov=r.views[1].fov={-.8f,.8f,.8f,-.8f};
    bf2142::MenuRayHit rayVisual{{.5f,.5f,160,120},{.2f,-.2f,-.3f},{0,0,-1.5f}};
    bf2142::DrawMenuPointer(laserLeft,laserRight,laserUi,320,240,DXGI_FORMAT_B8G8R8A8_UNORM,r,{},rayVisual.canvas,rayVisual,false);
    if(laserLeft==laserRight || laserUi[120*320+160]!=0xffffffff || laserUi.front()!=0 || laserUi.back()!=0)return 33;
    unsigned marked=0;for(auto color:laserLeft)marked+=color!=0;if(marked<20 || marked>3000)return 34;
    if(laserUi[120*320+164]!=0xff20efff)return 35;
    bf2142::DrawMenuPointer(laserLeft,laserRight,laserUi,320,240,DXGI_FORMAT_R8G8B8A8_UNORM,r,{},rayVisual.canvas,rayVisual,false);
    if(laserUi[120*320+164]!=0xffffef20)return 36;
    {
        const auto beam=bf2142::ProjectMenuBeam(320,240,{},rayVisual,r.views[0]);
        if(!beam||beam->x0<0||beam->x1>=320||beam->y0<0||beam->y1>=240)return 128;
        if(bf2142::ProjectMenuBeam(0,240,{},rayVisual,r.views[0]))return 129;
        auto behind=rayVisual;behind.origin.z=.2f;behind.end.z=1;
        if(bf2142::ProjectMenuBeam(320,240,{},behind,r.views[0]))return 130;
        auto invalid=r.views[0];invalid.fov.angleLeft=invalid.fov.angleRight;
        if(bf2142::ProjectMenuBeam(320,240,{},rayVisual,invalid))return 131;
        auto clipped=rayVisual;clipped.origin={100,0,-.001f};
        const auto edge=bf2142::ProjectMenuBeam(320,240,{},clipped,r.views[0]);
        if(!edge||edge->x0<0||edge->x0>319.01f||edge->y0<0||edge->y0>239.01f)return 132;
    }
    // Motion mode reserves left grip for support and removes thumbstick pitch.
    s={};s.flags=shared::kControllerSampleFlagSessionFocused;s.predictedDisplayTime=5000000000LL;
    s.hands[0].flags=shared::kControllerHandFlagSqueezeActive|shared::kControllerHandFlagTriggerActive;
    s.hands[0].squeezeValue=1;s.hands[1].flags=shared::kControllerHandFlagThumbstickActive;s.hands[1].thumbstickY=1;
    policy={};c=bf2142::MapControllers(policy,s,true,0,600,true);s.predictedDisplayTime+=11000000;
    c=bf2142::MapControllers(policy,s,true,0,600,true);if(c.keys[DIK_LCONTROL]||c.mouseY)return 41;
    s.hands[0].triggerValue=1;c=bf2142::MapControllers(policy,s,true,0,600,true);if(!c.keys[DIK_LCONTROL])return 42;
    if(!MenuMessages())return 43;
    // Controller request races/lateness are not headset removal. A recent
    // runtime focus observation remains usable across request boundaries.
    if(bf2142::ObserveRuntimeFocus(false,1000000000,1000000000,false) ||
        bf2142::ObserveRuntimeFocus(true,1000000000,2000000000,false))return 50;
    auto focused=bf2142::ObserveRuntimeFocus(true,1000000000,1050000000,true);
    auto unfocused=bf2142::ObserveRuntimeFocus(true,1000000000,1050000000,false);
    if(!focused||!*focused||!unfocused||*unfocused)return 51;
    bf2142::RecenterPolicy home;
    if(home.Update(false,0)||home.Update(true,100)||home.Update(false,200)||home.Update(true,700))return 44;
    if(home.Update(false,800)||home.Update(false,1600)||!home.Update(true,1700)||home.Update(true,1800))return 45;
    s.hands[1].flags|=shared::kControllerHandFlagSqueezeActive;s.hands[1].squeezeValue=1;
    c=bf2142::MapControllers(policy,s,true,0,600,true);if(!c.buttons[1])return 46;
    c=bf2142::MapControllers(policy,s,true,0,600,false);if(!c.buttons[1])return 47;
    s.hands[1].buttons=shared::kControllerHandButtonThumbstick;
    c=bf2142::MapControllers(policy,s,false,0,600,true);if(!c.recenter)return 48;
    c=bf2142::MapControllers(policy,s,false,0,600,true);if(c.recenter)return 49;
    // Desktop reserved physical keys cannot leak into native camera controls;
    // controller-generated Escape and ordinary movement still pass unchanged.
    bf2142::ControllerCommand masked;masked.blockedPhysicalKeys[DIK_F9]=1;masked.keys[DIK_ESCAPE]=0x80;
    bf2142::InputOverlayState overlay;std::array<BYTE,256> raw{};raw[DIK_F9]=raw[DIK_W]=0x80;
    bf2142::OverlayDeviceState(overlay,true,256,raw.data(),masked,1);
    if(raw[DIK_F9]||!raw[DIK_W]||!raw[DIK_ESCAPE]||overlay.physicalKeys[DIK_F9])return 60;
    DIDEVICEOBJECTDATA filtered[4]{};filtered[0].dwOfs=DIK_F9;filtered[0].dwData=0x80;DWORD count=1;
    bf2142::OverlayDeviceEvents(overlay,true,sizeof(filtered[0]),filtered,count,4,false,masked,2,100);
    if(filtered[0].dwData || overlay.physicalKeys[DIK_F9])return 61;
    masked={};raw[DIK_F9]=0x80;bf2142::OverlayDeviceState(overlay,true,256,raw.data(),masked,3);if(!raw[DIK_F9])return 62;
    // The old voice chord is gone. Only the physical shoulder interaction
    // may emit squad PTT; controller sprint remains independent.
    shared::SharedControllerSample radio{};radio.flags=shared::kControllerSampleFlagSessionFocused;radio.predictedDisplayTime=7000000000LL;
    radio.hands[0].flags=shared::kControllerHandFlagTriggerActive|shared::kControllerHandFlagThumbstickActive;
    radio.hands[0].buttons=shared::kControllerHandButtonThumbstick;
    bf2142::ControllerPolicyState radioState;
    auto radioCommand=bf2142::MapControllers(radioState,radio,true,0,600,true);if(!radioCommand.keys[DIK_LSHIFT]||radioCommand.keys[DIK_V])return 102;
    radio.hands[0].triggerValue=1;radioCommand=bf2142::MapControllers(radioState,radio,true,0,600,true);
    if(radioCommand.keys[DIK_V]||!radioCommand.keys[DIK_LSHIFT]||radioCommand.keys[DIK_R]||radioCommand.keys[DIK_E]||radioCommand.buttons[0])return 103;
    radioCommand=bf2142::MapControllers(radioState,radio,false,0,600,true);if(radioCommand.keys[DIK_V])return 104;
    radio.flags=0;radioCommand=bf2142::MapControllers(radioState,radio,true,0,600,true);if(radioCommand.keys[DIK_V])return 105;
    radio.flags=shared::kControllerSampleFlagSessionFocused;radio.hands[0].flags&=~shared::kControllerHandFlagThumbstickActive;
    radioCommand=bf2142::MapControllers(radioState,radio,true,0,600,true);if(radioCommand.keys[DIK_V])return 106;
    // Holstered weapons suppress physical fire in both state and buffered
    // input, emitting a release even when no new physical mouse event arrives.
    bf2142::InputOverlayState blockedMouse;bf2142::ControllerCommand blockFire;blockFire.blockedPhysicalButtons[0]=1;
    DIMOUSESTATE2 heldMouse{};heldMouse.rgbButtons[0]=0x80;
    bf2142::OverlayDeviceState(blockedMouse,false,sizeof(heldMouse),&heldMouse,blockFire,10);if(heldMouse.rgbButtons[0])return 63;
    count=0;bf2142::OverlayDeviceEvents(blockedMouse,false,sizeof(filtered[0]),filtered,count,4,true,blockFire,10,100);
    if(count!=1||filtered[0].dwData||blockedMouse.blockedButtons[0])return 64;
    count=0;bf2142::OverlayDeviceEvents(blockedMouse,false,sizeof(filtered[0]),filtered,count,4,false,blockFire,10,101);
    if(count!=1||filtered[0].dwData||!blockedMouse.blockedButtons[0])return 65;
    count=0;bf2142::OverlayDeviceEvents(blockedMouse,false,sizeof(filtered[0]),filtered,count,4,false,{},11,102);
    if(count!=1||filtered[0].dwData!=0x80)return 66;
    // Snap turns require neutral rearming, never repeat while held, and
    // produce a single degree pulse for the native look-input path.
    bf2142::SnapTurn snap;LONGLONG tick=1000000000;
    if(snap.Update(true,0,tick,30))return 70;
    if(snap.Update(true,1,tick+=10000000,30)!=30)return 71;
    if(snap.Update(true,1,tick+=10000000,30))return 72;
    snap.Update(true,0,tick+=10000000,30);
    if(snap.Update(true,-1,tick+=10000000,45)!=-45)return 73;
    if(snap.Update(true,1,tick+=900000000,45))return 74;
    bf2142::PhysicalStance posture;tick=1000000000;
    auto st=posture.Update(true,0,0,tick);if(st.proneKey||st.crouch)return 77;
    posture.Update(true,.4f,0,tick+=10000000);st=posture.Update(true,.4f,0,tick+=190000000);if(!st.crouch||st.proneKey)return 78;
    st=posture.Update(true,.25f,1,tick+=10000000);if(!st.crouch)return 79;
    posture.Update(true,1.1f,1,tick+=10000000);st=posture.Update(true,1.1f,1,tick+=190000000);if(!st.proneKey||st.crouch)return 80;
    st=posture.Update(true,1.1f,2,tick+=130000000);if(st.proneKey)return 81;
    posture.Update(true,0,2,tick+=10000000);posture.Update(true,0,2,tick+=190000000);
    for(int i=0;i<8;++i)st=posture.Update(true,0,2,tick+=100000000);
    if(st.stance!=0||st.crouch)return 82;
    if(posture.Update(false,1,2,tick).proneKey)return 83;
    // Reproduce returning to a different LOCAL origin: camera recenter alone
    // used to leave the original upright Y, continuously requesting crouch.
    bf2142::StandingHeightReference height;posture.Reset();tick=1000000000;
    height.Ensure(1.7f,0);
    posture.Update(true,height.Drop(1.2f),0,tick);
    st=posture.Update(true,height.Drop(1.2f),0,tick+=200000000);if(!st.crouch)return 94;
    if(!height.Recenter(1.2f))return 95;posture.Reset();
    height.Ensure(1.2f,1.7f); // A persisted preference must not undo explicit recenter.
    st=posture.Update(true,height.Drop(1.2f),1,tick+=10000000);if(st.crouch||st.proneKey||!Close(height.Drop(1.2f),0))return 96;
    posture.Update(true,height.Drop(.8f),0,tick+=10000000);
    st=posture.Update(true,height.Drop(.8f),0,tick+=200000000);if(!st.crouch)return 97;
    if(!height.Recenter(0))return 98;posture.Reset();height.Ensure(0,1.7f);
    if(!Close(height.Drop(0),0)||height.Recenter(std::numeric_limits<float>::quiet_NaN())||!Close(height.Drop(-.4f),.4f))return 99;
    // Recenter while natively prone must request a native stand-up toggle,
    // rather than forcing animation state or leaving a held crouch key.
    posture.Update(true,height.Drop(0),2,tick+=10000000);
    st=posture.Update(true,height.Drop(0),2,tick+=210000000);if(!st.proneKey||st.crouch)return 100;
    height.Reset();height.Ensure(1.2f,1.7f);if(!Close(height.Drop(1.2f),.5f))return 101;
    if(bf2142::VrMenuHit(.5f,.04f,false)!=0||bf2142::VrMenuHit(.96f,.04f,false)!=-1||bf2142::VrMenuHit(.2f,.3f,false)!=-1||bf2142::VrMenuHit(.2f,.26f,true)!=1||bf2142::VrMenuHit(.87f,.15f,true)!=10)return 84;
    bf2142::VrControlsMenu options;bf2142::VrSettings values;options.Hotkey(true);options.Hotkey(false);
    values.configPath=(std::filesystem::temp_directory_path()/L"BF2142VR-controls-test.ini").wstring();
    if(!options.Interact(.2f,.26f,true,false,values,1.7f)||!values.snapTurning)return 85;
    if(GetPrivateProfileIntW(L"VR",L"SnapTurning",0,values.configPath.c_str())!=1)return 86;
    if(!options.Interact(.2f,.514f,true,false,values,1.7f)||!options.RecenterRequested())return 87;
    if(GetPrivateProfileIntW(L"VR",L"HideWorldMarkers",0,values.configPath.c_str())!=1)return 92;
    values.hideWorldMarkers=false;
    if(!bf2142::SaveVrPreferences(values)||GetPrivateProfileIntW(L"VR",L"HideWorldMarkers",1,values.configPath.c_str())!=0)return 93;
    // Voice settings are independent from locomotion and native radio.
    if(!options.Interact(.75f,.15f,true,false,values,1.7f))return 107;
    if(!options.Interact(.2f,.32f,true,false,values,1.7f)||!values.proximityMuted)return 108;
    if(GetPrivateProfileIntW(L"VR",L"ProximityMicMuted",0,values.configPath.c_str())!=1)return 109;
    if(!values.snapTurning||!values.proximityVoice)return 110;
    options.Interact(.75f,.15f,true,false,values,1.7f);
    DeleteFileW(values.configPath.c_str());
    std::vector<DWORD> art(640*400);options.Draw(art,640,400,DXGI_FORMAT_B8G8R8A8_UNORM,values);
    if(!art[200*640+100]||art[0])return 88;
    std::vector<DWORD> roomLeft(160*100),roomRight(160*100);shared::SharedRenderRequest scene{};
    for(int i=0;i<2;++i){scene.views[i].pose.orientationW=1;scene.views[i].pose.positionX=i?.032f:-.032f;scene.views[i].fov={-.8f,.8f,.6f,-.6f};}
    // Kit chord must not also jump; releasing the modifier restores jump.
    {bf2142::ControllerPolicyState kitPolicy;shared::SharedControllerSample kit{};kit.flags=shared::kControllerSampleFlagSessionFocused;
     kit.hands[0].flags=shared::kControllerHandFlagTriggerActive;kit.hands[0].triggerValue=1;kit.hands[1].buttons=shared::kControllerHandButtonPrimary;
     auto kc=bf2142::MapControllers(kitPolicy,kit,true,0,600);if(!kc.keys[DIK_G]||kc.keys[DIK_SPACE])return 111;
     kit.hands[0].triggerValue=0;kc=bf2142::MapControllers(kitPolicy,kit,true,0,600);if(kc.keys[DIK_G]||!kc.keys[DIK_SPACE])return 112;}
    bf2142::DrawMenuRoom(roomLeft,roomRight,160,100,DXGI_FORMAT_B8G8R8A8_UNORM,scene,{});
    unsigned different=0;for(size_t i=0;i<roomLeft.size();++i){if(!(roomLeft[i]>>24))return 89;different+=roomLeft[i]!=roomRight[i];}
    if(different<20)return 90;
    if(!options.Interact(-1,-1,false,true,values,1.7f)||options.Open()||!options.FilterBack(true)||options.FilterBack(false))return 91;
    // A body grab is a single selection across state/event API ordering,
    // frequent XR republication, stale inventory snapshots and buffer pressure.
    for(bool stateFirst:{false,true}){
        bf2142::InputOverlayState input{};bf2142::ControllerCommand select{};
        select.selection={17,1000000000,3,true};std::array<BYTE,256> physicalKeys{};
        DIDEVICEOBJECTDATA selectionEvents[4]{};DWORD selected=0;unsigned edges=0;bool automatic=true;
        const auto press=[&](DWORD offset,DWORD value){if(offset==DIK_3&&(value&0x80)){++edges;if(selected==3)automatic=!automatic;else selected=3;}};
        for(DWORD sample=1;sample<=200;++sample){
            const auto state=[&](){physicalKeys={};bf2142::OverlayDeviceState(input,true,256,physicalKeys.data(),select,sample);press(DIK_3,physicalKeys[DIK_3]);};
            const auto buffered=[&](){DWORD count=0;bf2142::OverlayDeviceEvents(input,true,sizeof(selectionEvents[0]),selectionEvents,count,4,false,select,sample,sample);for(DWORD j=0;j<count;++j)press(selectionEvents[j].dwOfs,selectionEvents[j].dwData);};
            if(stateFirst){state();buffered();}else{buffered();state();}
        }
        if(edges!=1||selected!=3||!automatic)return 113;
        // A fresh grab of the already equipped rifle is consumed as a no-op.
        select.selection.gesture+=1000000000;select.selection.allowed=false;physicalKeys={};
        bf2142::OverlayDeviceState(input,true,256,physicalKeys.data(),select,201);if(physicalKeys[DIK_3])return 114;
        select.selection.allowed=true;physicalKeys={};bf2142::OverlayDeviceState(input,true,256,physicalKeys.data(),select,202);if(physicalKeys[DIK_3])return 115;
        // New deliberate intent is accepted, physical number keys stay native.
        select.selection.gesture+=1000000000;physicalKeys={};bf2142::OverlayDeviceState(input,true,256,physicalKeys.data(),select,203);if(!physicalKeys[DIK_3])return 116;
        select={};physicalKeys={};physicalKeys[DIK_3]=0x80;bf2142::OverlayDeviceState(input,true,256,physicalKeys.data(),select,204);if(!physicalKeys[DIK_3])return 117;
    }
    {
        bf2142::InputOverlayState input{};bf2142::ControllerCommand select{};select.selection={17,1000000000,3,true};DIDEVICEOBJECTDATA e[2]{};DWORD count=0;
        for(int peek=0;peek<3;++peek){count=0;bf2142::OverlayDeviceEvents(input,true,sizeof(e[0]),e,count,2,true,select,1,1);if(count!=1||!(e[0].dwData&0x80)||input.consumedSelection.gesture)return 118;}
        e[0]={};e[0].dwOfs=DIK_W;e[0].dwData=0x80;count=1;bf2142::OverlayDeviceEvents(input,true,sizeof(e[0]),e,count,1,false,select,1,1);if(input.consumedSelection.gesture)return 119;
        count=0;bf2142::OverlayDeviceEvents(input,true,sizeof(e[0]),e,count,1,false,select,1,1);if(count!=1||!(e[0].dwData&0x80)||!input.selectionRelease)return 120;
        count=0;bf2142::OverlayDeviceEvents(input,true,sizeof(e[0]),e,count,1,false,select,2,2);if(count!=1||e[0].dwData||input.selectionRelease)return 121;
        count=0;bf2142::OverlayDeviceEvents(input,true,sizeof(e[0]),e,count,1,false,select,3,3);if(count)return 122;
    }
    // A missed native draw can retry, but each attempt belongs to only one
    // input API; once acknowledged, stale samples cannot toggle fire mode.
    for(bool bufferedFirst:{false,true}){
        bf2142::InputOverlayState input{};bf2142::ControllerCommand select{};
        select.selection={17,1000000000,2,true};unsigned presses=0;
        const auto poll=[&](){
            std::array<BYTE,256> keys{};DIDEVICEOBJECTDATA events[4]{};DWORD count=0;
            const auto state=[&](){bf2142::OverlayDeviceState(input,true,256,keys.data(),select,1);presses+=(keys[DIK_2]&0x80)!=0;};
            const auto eventsPoll=[&](){bf2142::OverlayDeviceEvents(input,true,sizeof(events[0]),events,count,4,false,select,1,1);for(DWORD i=0;i<count;++i)presses+=events[i].dwOfs==DIK_2&&(events[i].dwData&0x80)!=0;};
            if(bufferedFirst){eventsPoll();state();}else{state();eventsPoll();}
        };
        poll();poll();if(presses!=1)return 123;
        select.selection.attempt=1;poll();poll();if(presses!=2)return 124;
        select.selection.allowed=false;poll();select.selection.allowed=true;
        for(unsigned i=2;i<10;++i){select.selection.attempt=i;poll();}
        if(presses!=2)return 125;
        select.selection.gesture+=1000000000;select.selection.attempt=0;poll();if(presses!=3)return 126;
    }
    puts("Controller focus/freshness, button release, head-relative movement, recenter, 6DoF scale and grip transforms passed.");
}

namespace bfvr::bf2142 { void PublishNativeHudPointer(bool,float,float,bool){} }
