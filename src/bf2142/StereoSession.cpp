#include "StereoSession.h"
#include "RenderProfile.h"
#include "FramePixels.h"
#include "GpuFrameTransfer.h"
#include "GpuGunOptics.h"
#include "LaunchOptions.h"
#include "FrameCapture.h"
#include "NativeUiCapture.h"
#include "StereoDiagnostics.h"
#include "ControllerInput.h"
#include "MenuPointer.h"
#include "MenuTrace.h"
#include "VrSettings.h"
#include "TrackingMath.h"
#include "NativeHands.h"
#include "multiplayer/NativeNetwork.h"
#include "NativeOptics.h"
#include "BodyInventory.h"
#include "EquipmentHaptics.h"
#include "BodyEquipment.h"
#include "WristMenu.h"
#include "NativeFramePacing.h"
#include "ShoulderRadioGpu.h"
#include "voice/VoiceClient.h"
#include "SupportCrates.h"
#include "DesktopSimulation.h"
#include "EyeRestore.h"
#include "WeaponGrip.h"
#include "RenderTimeBudget.h"
#include "RenderRequestWait.h"
#include "GrenadeArc.h"
#include "NativeComfort.h"
#include "NativeVehicle.h"
#include "NativeCrosshair.h"
#include "VrControlsMenu.h"
#include "MenuRoom.h"
#include "MenuRoomGpu.h"
#include "MenuOverlayGpu.h"
#include "RecenterPolicy.h"
#include "stereo/UiPointerMath.h"
#include "presenter/SharedControlChannel.h"
#include "presenter/SharedTextureProducer.h"
#include <wrl/client.h>
#include <array>
#include <algorithm>
#include <cmath>
#include <vector>
namespace bfvr::bf2142 {
namespace {
using Microsoft::WRL::ComPtr;
shared::SharedControlChannel channel;
shared::SharedTextureProducer producer;
FrameCapture capture;
GpuFrameTransfer gpuFrames;GpuGunOptics gpuOptics;
bool gpuRequested=false,gpuAttempted=false,gpuReady=false,gpuPair=false,gpuMenuBlocked=false;
unsigned gpuPublished=0;
EyeRestore eyeRestore;MenuRoomGpu menuRoomGpu;MenuOverlayGpu menuOverlayGpu;
std::vector<DWORD> scopePixels;
bool opticsReady=false;
NativeUiCapture uiCapture;
bool uiReady=false, outsideUiReady=false, warnedUi=false;
RecenterPolicy homePolicy;
int activeEye=-1;
unsigned suppressedPresents=0;
LogFunction logger=nullptr;
IDirect3DDevice9* gameDevice=nullptr; // Borrowed: resources released before Reset.
HANDLE presenterProcess=nullptr;
bool diagnostic=false,desktop=false,desktopCaptureHeld=false,widescreenUi=false;
RenderProfile renderProfile;
HWND desktopWindow=nullptr;
RenderTimeBudget renderTime;bool opticHudReady=false;
std::vector<DWORD> opticHudPixels,opticHudBaseline;
BodyInventory bodyInventory;WeaponGrip weaponGrip;TraversalControls traversalControls;
BodyEquipment bodyEquipment;SupportCrates supportCrates;SupportFrame supportFrame;
BodyInventoryResult bodyFrame;
ShoulderRadio shoulderRadio;RadioFrame radioFrame;bool radioDrawn=false,bodyGpuDrawn=false;
bool frameLimitAttempted=false;int savedNativeFrameCap=-1;
void RestoreFramePacing(){
    if(savedNativeFrameCap>0)SetNativeFrameRate(savedNativeFrameCap,nullptr,true);
    savedNativeFrameCap=-1;frameLimitAttempted=false;
}
WristMenu wristMenu;WristMenuGpu wristGpu;WristFrame wristFrame;
InventoryNames inventoryNames{};
int equippedItem=0;bool bodyActive=false;
EquipmentHaptics rightEquipmentCue,leftEquipmentCue;
std::wstring diagnosticPrefix;
VrSettings settings;
ControllerPolicyState controllerPolicy;
MenuPointer menuPointer;VrControlsMenu controlsMenu;PhysicalStance physicalStance;
bool lastRequestMainMenu=false;StandingHeightReference standingHeight;
bool previousGameplay=false,haveGripReference=false,homeHeld=false;
stereo::Pose gripReference{},gripHead{};
bool enabled=false, transport=false, nativeInstalled=false;
bool haveReference=false, pairReady=false, insideRender=false;
bool lastRenderResult=true;
void* lastRenderer=nullptr;
LONG pending=0, recenterSequence=0;
ULONGLONG lastMenuCapture=0;
UINT width=0,height=0;
DXGI_FORMAT colorFormat=DXGI_FORMAT_UNKNOWN;
shared::SharedRenderRequest request{};
stereo::Pose reference{};
std::array<std::vector<DWORD>,shared::kTextureCount> pixels;
std::vector<DWORD> desktopPixels;

unsigned long pairs=0,menuFrames=0;
void ProducerLog(void*,const wchar_t* text) {
    char message[2048]{};
    WideCharToMultiByte(CP_UTF8,0,text,-1,message,sizeof(message),nullptr,nullptr);
    logger("%s",message);
}
LONG ReadCounter(volatile LONG& counter) { return InterlockedCompareExchange(&counter,0,0); }
void Fail(const char* reason,HRESULT hr=E_FAIL) {
    if (!enabled) return;
    RestoreFramePacing();
    SetNativeWeaponHeld(true);
    logger("Stereo stopped: %s (0x%08lX). Normal game rendering continues.",reason,static_cast<unsigned long>(hr));
    enabled=false; pairReady=false; ClearNativeSnapTurn();ConfigureNativeMovement(false,0);ConfigureNativeLookPitch(false,{});ResetAutomaticAds();ClearControllerCommand();ClearNativeHands();menuPointer.Reset();
    if (auto* block=channel.Get()) {
        shared::PublishState(&block->producerState,shared::ProcessState::Failed);
        InterlockedExchange(&block->shutdownRequested,1);
        (void)channel.SignalProducerUpdate();
    }
}
bool Healthy() {
    if (!enabled) return false;
    if (diagnostic || desktop) return true;
    auto* b=channel.Get();
    if (!b || ReadCounter(b->shutdownRequested) ||
        shared::ReadState(&b->presenterState)==shared::ProcessState::Failed ||
        WaitForSingleObject(presenterProcess,0)!=WAIT_TIMEOUT) {
        Fail("headset presenter closed or failed"); return false;
    }
    return true;
}
bool PrepareTransport() {
    if (!Healthy() || !gameDevice) return false;
    if (transport) return true;
    auto* b=channel.Get();
    if (!diagnostic && !desktop && shared::ReadState(&b->presenterState)!=shared::ProcessState::RequirementsReady) return false;
    ComPtr<IDirect3DSurface9> back;
    D3DSURFACE_DESC d{};
    if (FAILED(gameDevice->GetBackBuffer(0,0,D3DBACKBUFFER_TYPE_MONO,&back)) || FAILED(back->GetDesc(&d))) return false;
    if (!d.Width || !d.Height || d.Width>8192 || d.Height>8192) { Fail("unsupported capture dimensions"); return false; }
    colorFormat=(diagnostic||desktop)?DXGI_FORMAT_B8G8R8A8_UNORM:static_cast<DXGI_FORMAT>(b->requirements.format);
    if (colorFormat!=DXGI_FORMAT_B8G8R8A8_UNORM && colorFormat!=DXGI_FORMAT_B8G8R8A8_UNORM_SRGB &&
        colorFormat!=DXGI_FORMAT_R8G8B8A8_UNORM && colorFormat!=DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) {
        Fail("unsupported OpenXR color format"); return false;
    }
    // Source transport can be BGRA even when the XR destination is RGBA.
    // The established presenter already converts supported color formats.
    if(gpuRequested)colorFormat=DXGI_FORMAT_B8G8R8A8_UNORM;
    shared::SharedTextureRequirements r{};
    r.boundedCpuUpload=true; // Bound menu/scope CPU uploads inside the 32-bit game.
    r.adapterLuid={b->requirements.adapterLuidLow,b->requirements.adapterLuidHigh};
    r.minimumFeatureLevel=static_cast<D3D_FEATURE_LEVEL>(b->requirements.minimumFeatureLevel);
    // Use the game's current source resolution. The existing presenter scales
    // into the runtime's eye swapchains without changing the per-eye FOV.
    width=d.Width; height=d.Height;
    r.leftWorldWidth=r.rightWorldWidth=r.uiWidth=width;
    r.leftWorldHeight=r.rightWorldHeight=r.uiHeight=height; r.format=colorFormat;
    if (!diagnostic && !desktop && !producer.Initialize(channel.Name().c_str(),r,ProducerLog,nullptr)) { Fail("shared texture creation"); return false; }
    for (auto& slot:pixels) slot.assign(static_cast<size_t>(width)*height,0);
    if (!diagnostic && !desktop) producer.CopyDescriptions(b->textures,shared::kTextureCount);
    shared::PublishState(&b->producerState,shared::ProcessState::TexturesReady);
    (void)channel.SignalProducerUpdate();
    transport=true;
    logger("Stereo transport ready: source=%ux%u format=%u; separate eye textures and HUD; source MSAA=%u quality=%lu.",width,height,unsigned(colorFormat),unsigned(d.MultiSampleType),d.MultiSampleQuality);
    return true;
}
stereo::Pose Pose(const shared::SharedPresentationPose& p) {
    return {{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}};
}
void Recenter(bool calibrateStanding=false) {
    ClearNativeSnapTurn();ConfigureNativeMovement(false,0);ConfigureNativeLookPitch(false,{});RecenterNativeVehicle();
    const auto head=Pose(request.headPose);
    if(calibrateStanding && standingHeight.Recenter(head.position.y)){
        physicalStance.Reset();
        logger("Explicit recenter: standing height recalibrated; physical posture state cleared.");
    }
    const auto upright=stereo::MakeYawOnlyUiAnchor(head);
    reference=upright?*upright:head;
    wristMenu.Reset();wristFrame={};menuPointer.Reset();shoulderRadio.Reset();radioFrame={};supportCrates.Reset();supportFrame={};PublishNativeSupport({});bodyInventory.Reset();traversalControls.Reset();ResetMotionActions();
    recenterSequence=request.recenterForwardSequence;haveReference=true;haveGripReference=false;
    logger("6DoF neutral pose captured; upright reference; position scale=%.3f; height offset=%.3f.",settings.worldScale,settings.heightOffset);
}
bool GetRequest(bool gameplay=false,bool pausedMenu=false) {
    menuPointer.ExpireInput();
    if (!PrepareTransport()) return false;
    auto* b=channel.Get();
    // Never overwrite a texture set that the consumer still owns.
    if (ReadCounter(b->consumedFrameSequence)!=ReadCounter(b->frameSequence)) return false;
    if (!pending) {
        pending=InterlockedIncrement(&b->renderReadySequence);
        (void)channel.SignalProducerUpdate();
    }
    if (diagnostic) { DiagnosticRequest(b->renderRequest,pairs); InterlockedExchange(&b->renderRequestSequence,pending); }
    if(desktop){
        DesktopFrame(desktopWindow,b->renderRequest,b->controllerSample,pausedMenu||!gameplay||controlsMenu.Open()||menuPointer.MenuVisible(),menuPointer.Active()?&menuPointer.Anchor():nullptr);
        InterlockedExchange(&b->controllerSampleSequence,pending);InterlockedExchange(&b->renderRequestSequence,pending);
    }
    if (ReadCounter(b->renderRequestSequence)!=pending) {
        (void)channel.WaitForPresenterUpdate(12);
        if (ReadCounter(b->renderRequestSequence)!=pending) return false;
    }
    request=b->renderRequest;
    if (!request.shouldRender || !request.viewsValid || !request.headPoseValid) {
        RestoreFramePacing();wristMenu.Reset();wristFrame={};
        ClearNativeSnapTurn();ConfigureNativeMovement(false,0);ConfigureNativeLookPitch(false,{});ResetAutomaticAds();homePolicy.Update(false,GetTickCount64());menuPointer.Reset();
        ClearControllerCommand();ClearNativeHands();shoulderRadio.Reset();radioFrame={};controllerPolicy={};pending=0;return false;
    }
    DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
    const bool home=foregroundPid==GetCurrentProcessId() && (GetAsyncKeyState(VK_HOME)&0x8000)!=0;
    const bool explicitRecenter=(haveReference && request.recenterForwardSequence!=recenterSequence) || (home && !homeHeld);
    if (!haveReference || explicitRecenter || (gameplay && !previousGameplay)) Recenter(explicitRecenter);
    homeHeld=home;previousGameplay=gameplay;
    shared::SharedControllerSample sample{};
    const LONG controllerSequence=ReadCounter(b->controllerSampleSequence);
    MemoryBarrier();sample=b->controllerSample;MemoryBarrier();
    const bool accepted=!diagnostic && settings.controllers && controllerSequence==ReadCounter(b->controllerSampleSequence) &&
        MatchingControllerSample(sample,controllerSequence,request,pending);
    if(pausedMenu||!gameplay)menuTrace::Request(accepted);
    const auto runtimeFocus=ObserveRuntimeFocus(controllerSequence>0 && controllerSequence==ReadCounter(b->controllerSampleSequence),
        sample.predictedDisplayTime,request.predictedDisplayTime,(sample.flags&shared::kControllerSampleFlagSessionFocused)!=0);
    if(!diagnostic && runtimeFocus && homePolicy.Update(*runtimeFocus,GetTickCount64()))Recenter();
    ConfigureNativeTracking(settings.worldScale,settings.heightOffset,nullptr,nullptr,nullptr);
    controlsMenu.Hotkey(!diagnostic && foregroundPid==GetCurrentProcessId() && (GetAsyncKeyState(VK_INSERT)&0x8000)!=0);
    lastRequestMainMenu=!gameplay;
    const bool showMenu=pausedMenu || !gameplay || controlsMenu.Open() || (!diagnostic && menuPointer.MenuVisible());
    const bool runtimePacing=accepted && foregroundPid==GetCurrentProcessId() && gameplay && !showMenu && !diagnostic && !desktop;
    if(!runtimePacing)RestoreFramePacing();
    if(runtimePacing && !frameLimitAttempted){
        frameLimitAttempted=true;const int cap=SetNativeFrameRate(0,&savedNativeFrameCap);
        logger("Runtime frame pacing: native game.lockFps request=0 result=%d (-1=unrecognized; positive=native restriction retained).",cap);
    }
    wristFrame=wristMenu.Update(accepted&&gameplay&&!showMenu&&!explicitRecenter,sample,Pose(request.headPose));
    TraversalSample traversal;
    const bool traversalValid=accepted && gameplay && !showMenu && ReadNativeTraversal(&traversal);
    VehicleSample vehicle;const bool vehicleKnown=gameplay&&ReadNativeVehicle(&vehicle);
    const bool vehicleValid=accepted&&!showMenu&&vehicleKnown;
    controlsMenu.Vehicle(vehicleKnown);
    const bool chuteHands=traversalValid&&traversal.mode==TraversalMode::Parachute;
    const bool mounted=(traversalValid && traversal.mode!=TraversalMode::Foot)||vehicleKnown;
    RequestAutomaticAds(accepted && gameplay && !showMenu && !mounted && settings.motionHands);
    ControllerCommand command{};
    const bool nativeSnap=accepted && gameplay && !showMenu && settings.snapTurning && NativeSnapTurnAvailable();
    if(!nativeSnap)ClearNativeSnapTurn();
    const float movementYaw=vehicleKnown?0.f:LocomotionYaw(settings.controllerRelativeMovement,reference,Pose(request.headPose),sample);
    const bool nativeMovement=ConfigureNativeMovement(accepted&&gameplay&&!showMenu&&!mounted,movementYaw);
    ConfigureNativeLookPitch(accepted&&gameplay&&!showMenu&&!mounted&&settings.motionHands,Pose(request.headPose));
    if(accepted) {
        command=MapControllers(controllerPolicy,sample,gameplay && !showMenu,nativeMovement?0.f:movementYaw,settings.turnSpeed,settings.motionHands&&!vehicleKnown,nativeSnap,settings.snapAngle);
        if(command.snapDegrees && !command.recenter)
            RequestNativeSnapTurn(command.snapDegrees,sample.predictedDisplayTime);
        if(gameplay && !showMenu && !mounted && HandlesAutomaticAds())command.buttons[1]=0;
        if(controlsMenu.FilterBack(command.keys[1]!=0))command.keys[1]=0;
        if(command.recenter){Recenter(true);ClearNativeHands();}
        const auto& hand=sample.hands[1];
        constexpr DWORD gripMask=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripOrientationValid|
            shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked;
        if(gameplay && !settings.motionHands && settings.trackedWeapon && (hand.flags&gripMask)==gripMask) {
            const auto grip=Pose(hand.gripPose);
            if(!haveGripReference){gripReference=grip;gripHead=reference;haveGripReference=true;logger("Controller weapon reference captured (visual tracking mode).");}
            ConfigureNativeTracking(settings.worldScale,settings.heightOffset,&gripHead,&gripReference,&grip);
        }
    } else { controllerPolicy={}; }
    std::array<bool,10> inventory{};
    std::uint64_t inventoryOwner=0;
    const bool inventoryValid=accepted && gameplay && !showMenu && !mounted && settings.motionHands && ReadNativeInventory(&inventory,&equippedItem,&inventoryNames,&inventoryOwner);
    bodyActive=inventoryValid && settings.bodyInventory;
    auto body=bodyInventory.Update(inventoryValid,sample,Pose(request.headPose),inventory,equippedItem);
    if(!settings.bodyInventory){body.hovered=body.selected=-1;body.key=0;}
    bodyActive=bodyActive && body.anchorValid;
    if(bodyActive){
        if(body.hovered>=0 || body.key)command.buttons[1]=0; // A body grab consumes grip; unsupported native alt-fire remains available elsewhere.
        if(body.key>=2&&body.key<=10)command.selection={inventoryOwner,body.selectionTime,body.key-1,false};
    }
    bodyFrame=body;
    constexpr DWORD gripFlags=shared::kControllerHandFlagGripActive|shared::kControllerHandFlagGripPositionValid|shared::kControllerHandFlagGripOrientationValid|
        shared::kControllerHandFlagGripPositionTracked|shared::kControllerHandFlagGripOrientationTracked|shared::kControllerHandFlagSqueezeActive;
    const auto& right=sample.hands[1];
    const bool gripTracked=(right.flags&gripFlags)==gripFlags;
    if(!settings.toggleWeaponGrip || (accepted && gameplay && !showMenu && !mounted && !inventoryValid))weaponGrip.Reset();
    bool held=weaponGrip.Update({settings.toggleWeaponGrip && inventoryValid && gripTracked && !command.recenter,
        std::isfinite(right.squeezeValue)&&right.squeezeValue>.65f,body.selected>=0,inventoryOwner,sample.predictedDisplayTime,equippedItem});
    RadioObservation radioObs;radioObs.active=inventoryValid&&body.anchorValid&&!command.recenter&&foregroundPid==GetCurrentProcessId();
    radioObs.leftAvailable=!supportFrame.busy;radioObs.owner=inventoryOwner;radioObs.anchor=body.anchor;radioObs.sample=sample;
    radioFrame=shoulderRadio.Update(radioObs);
    command.keys[0x2f]=radioFrame.pressed?0x80:0; // stock squad push-to-talk V
    if(radioFrame.held)command.keys[0x1d]=0; // left squeeze belongs to the radio
    if(radioFrame.click)InterlockedIncrement(&b->hapticRadioLeftSequence);
    SupportObservation obs;obs.active=inventoryValid&&body.anchorValid&&!command.recenter;
    obs.rightGrab=body.selected>=0;obs.leftCrates=settings.leftSupportCrates;obs.owner=inventoryOwner;obs.time=sample.predictedDisplayTime;
    obs.equipped=equippedItem;obs.names=inventoryNames;obs.anchor=body.anchor;obs.left=Pose(sample.hands[0].gripPose);
    obs.leftTracked=!radioFrame.held&&(sample.hands[0].flags&gripFlags)==gripFlags;
    obs.leftGrip=std::isfinite(sample.hands[0].squeezeValue)&&sample.hands[0].squeezeValue>.65f;
    if(obs.active&&obs.leftCrates)for(int i=1;i<10;++i)if(SupportCrateWeapon(obs.names[i].data()))ReadSupportAmmo(obs.owner,i,&obs.ammo[i]);
    const bool wasCrateBusy=supportFrame.busy;
    supportFrame=supportCrates.Update(obs);PublishNativeSupport(supportFrame);
    const bool effectiveHeld=weaponGrip.ResolveSupport(supportFrame,sample.predictedDisplayTime);
    held=weaponGrip.Held();SetNativeWeaponHeld(effectiveHeld);
    if(accepted && gameplay && !showMenu && settings.toggleWeaponGrip && inventoryValid){
        command.buttons[1]=0;
        if(!NativeWeaponInputReady()){command.buttons[0]=0;command.blockedPhysicalButtons[0]=command.blockedPhysicalButtons[1]=1;}
    }
    RequestAutomaticAds(accepted && gameplay && !showMenu && !mounted && settings.motionHands && held&&!supportFrame.busy);
    const bool equipmentFocused=accepted&&gameplay&&!showMenu&&!mounted&&!command.recenter&&foregroundPid==GetCurrentProcessId();
    const bool rightTarget=body.hovered>=0&&(!held||BodySlots()[body.hovered].item!=unsigned(equippedItem));
    if(rightEquipmentCue.Update(equipmentFocused&&bodyActive,rightTarget?body.hovered:-1,body.selected>=0,sample.predictedDisplayTime))
        InterlockedIncrement(&b->hapticEquipmentRightSequence);
    if(leftEquipmentCue.Update(equipmentFocused&&obs.leftTracked&&obs.leftCrates,supportFrame.hovered,
        supportFrame.busy&&!wasCrateBusy,sample.predictedDisplayTime))InterlockedIncrement(&b->hapticEquipmentLeftSequence);
    if(supportFrame.busy){
        command.buttons[0]=supportFrame.fire?0x80:0;command.buttons[1]=0;
        command.blockedPhysicalButtons[0]=command.blockedPhysicalButtons[1]=1;
        if(supportFrame.select>0&&supportFrame.select<10)
            command.selection={inventoryOwner,supportFrame.selectionTime,unsigned(supportFrame.select),false};
        RequestAutomaticAds(false);
    }
    PublishNativeHands(accepted && gameplay && !showMenu && !command.recenter && (!mounted||chuteHands) && settings.motionHands?&sample:nullptr,
        reference,Pose(request.headPose),settings.worldScale,settings.heightOffset,radioFrame);
    if(UpdateNativeMotion(accepted && gameplay && !showMenu && !mounted && settings.motionHands && held && !command.recenter && !body.key && !supportFrame.busy && !wristFrame.hovered && !wristMenu.Consuming()?&sample:nullptr,Pose(request.headPose),command))
        InterlockedIncrement(&b->hapticNativeMenuHoverSequence);
    int stance=0;
    const bool physical=accepted && gameplay && !showMenu && settings.physicalStance && inventoryValid && ReadNativeStance(&stance);
    if(physical)standingHeight.Ensure(request.headPose.positionY,settings.standingHeight);
    const auto posture=physicalStance.Update(physical,standingHeight.Drop(request.headPose.positionY),stance,sample.predictedDisplayTime);
    ConfigurePhysicalCamera(settings.physicalStance);
    if(physical){command.keys[0x1d]=posture.crouch?0x80:0;command.keys[0x2c]=posture.proneKey?0x80:0;}
    if(traversalValid&&!command.recenter)traversalControls.Update(traversal,sample,Pose(request.headPose),command);
    else traversalControls.Reset();
    if(vehicleValid&&accepted){command.keys[0x22]=0;command.keys[0x39]=(sample.hands[1].buttons&shared::kControllerHandButtonPrimary)?0x80:0;}
    UpdateNativeVehicle(vehicleValid&&!command.recenter?&vehicle:nullptr,accepted?&sample:nullptr,reference,Pose(request.headPose),settings.worldScale,settings.heightOffset,command);
    menuPointer.Update(showMenu,accepted?&sample:nullptr,request,width,height,
        (diagnostic||desktop)?(widescreenUi?1600:width):b->requirements.uiWidth,(diagnostic||desktop)?(widescreenUi?900:height):b->requirements.uiHeight,command,&controlsMenu,&settings,widescreenUi,desktop);
    if(controlsMenu.RecenterRequested()){Recenter(true);ClearNativeHands();command.keys[0x2f]=0;}
    SetCrosshairHidden(settings.hideCrosshair);
    if(AutomaticAdsButton(accepted && gameplay && !showMenu && !mounted && !command.recenter && !body.key && !supportFrame.busy))command.buttons[1]=0x80;
    const auto fistBumps=TakeNetworkFistBumps(accepted&&gameplay&&!showMenu&&!mounted&&!command.recenter&&
        settings.motionHands&&runtimeFocus.value_or(false)&&foregroundPid==GetCurrentProcessId());
    if(fistBumps&1)InterlockedIncrement(&b->hapticFistLeftSequence);
    if(fistBumps&2)InterlockedIncrement(&b->hapticFistRightSequence);
    voice::PublishControls(voice::VoicePreferences(settings),accepted&&gameplay&&!showMenu&&!command.recenter&&runtimeFocus.value_or(false)&&foregroundPid==GetCurrentProcessId(),radioFrame.pressed);
    wristMenu.Apply(command,showMenu);
    if(desktop)BlockDesktopHotkeys(command);
    if(accepted)PublishControllerCommand(command,true);else {ClearControllerCommand();ClearNativeHands();}
    return true;
}
bool ReadFrame(std::vector<DWORD>& output) {
    auto profile=renderProfile.Measure(RenderProfile::EyeReadback);
    const HRESULT hr=capture.Read(gameDevice,colorFormat,output);
    if (FAILED(hr)) { Fail("D3D9 eye readback",hr); return false; }
    if (capture.Width()!=width || capture.Height()!=height) {
        Fail("game resolution changed; relaunch VR at the new resolution"); return false;
    }
    return true;
}
bool BeginGpuPair(){
    if(!gpuRequested||diagnostic||DiagnosticCpuTransfer())return false;
    if(!gpuAttempted){
        gpuAttempted=true;
        const auto hr=gpuFrames.Initialize(gameDevice,width,height,desktop?nullptr:producer.Device());
        gpuReady=SUCCEEDED(hr);
        logger("Experimental GPU frame transport: ready=%d hr=0x%08lX; %ux%u; GPU optics when available; CPU compatibility fallback retained.",gpuReady,hr,width,height);
    }
    if(gpuReady && menuPointer.Active() && !menuOverlayGpu.Prepare(gameDevice,width,height)){
        gpuMenuBlocked=true;return false;
    }
    return gpuReady;
}
bool StageGpuMenu(IDirect3DSurface9* ui,bool backbuffer){
    for(unsigned eye=0;eye<2;++eye)if(FAILED(gpuFrames.Stage(eye,nullptr)))return false;
    if(FAILED(backbuffer?gpuFrames.StageBackbuffer(2):gpuFrames.Stage(2,ui)))return false;
    if(lastRequestMainMenu && settings.menuRoom){
        if(!menuRoomGpu.DrawTo(gameDevice,{gpuFrames.Surface(0),gpuFrames.Surface(1)},width,height,request,menuPointer.Anchor())){
            // Optional backdrop failures keep a complete native UI with black eyes.
            if(FAILED(gpuFrames.Stage(0,nullptr))||FAILED(gpuFrames.Stage(1,nullptr))||
                FAILED(backbuffer?gpuFrames.StageBackbuffer(2):gpuFrames.Stage(2,ui)))return false;
        }
    }
    return true;
}
bool ReadGpuEyes(){
    if(!gpuPair)return true;
    auto timing=renderProfile.Measure(RenderProfile::EyeReadback);
    for(unsigned eye=0;eye<2;++eye){
        const auto hr=capture.ReadSurface(gameDevice,gpuFrames.Surface(eye),colorFormat,pixels[eye]);
        if(FAILED(hr)){Fail("GPU snapshot read for native optic/guide",hr);return false;}
    }
    gpuPair=false;return true;
}
void Publish(bool world) {
    auto profile=renderProfile.Measure(RenderProfile::Publish);
    auto* b=channel.Get();
    const bool gpu=gpuPair;
    // Explicit menu tracing retains its sampled image diagnostic. Normal GPU
    // menus never read pixels back; enabling this private probe costs a UI read.
    if(!world && gpu && menuTrace::enabled)
        (void)capture.ReadSurface(gameDevice,gpuFrames.Surface(2),colorFormat,pixels[2],true);
    if(!world)menuTrace::Frame(pixels[2],width,height,lastRequestMainMenu);
    if(!world && !gpu && lastRequestMainMenu && settings.menuRoom && !diagnostic){
        // A failed optional backdrop retains the native background; no slow
        // CPU fallback is inserted into the user's VR frame loop.
        (void)menuRoomGpu.Draw(gameDevice,pixels[0],pixels[1],width,height,colorFormat,request,menuPointer.Anchor());
    }
    if(gpu){
        if(menuPointer.Active()){
            const auto& art=controlsMenu.Artwork(width,height,DXGI_FORMAT_B8G8R8A8_UNORM,settings,widescreenUi);
            if(!menuOverlayGpu.Draw({gpuFrames.Surface(0),gpuFrames.Surface(1),gpuFrames.Surface(2)},
                menuPointer.Visual(),request,art,controlsMenu.ArtworkRevision())){Fail("GPU menu overlay");return;}
        }
        if(desktop){
            if((world&&!eyeRestore.DrawTexture(gameDevice,gpuFrames.Texture(0))) ||
                !eyeRestore.DrawTexture(gameDevice,gpuFrames.Texture(2),world)){
                Fail("GPU desktop eye/HUD composite");return;
            }
        }else if(!desktop){
            const auto hr=gpuFrames.Export();
            if(FAILED(hr)||!producer.PublishGpuFrame(gpuFrames.Exported())){Fail("GPU stereo export/publication",FAILED(hr)?hr:E_FAIL);return;}
        }
        ++gpuPublished;
        if(gpuPublished==1)logger("GPU stereo publication active: no world/UI pixel readback on this frame.");
    }else{
        if(menuPointer.Active() && !diagnostic)controlsMenu.Draw(pixels[2],width,height,colorFormat,settings,widescreenUi);
        menuPointer.Draw(pixels[0],pixels[1],pixels[2],colorFormat,request);
        std::array<shared::SharedTexturePixels,shared::kTextureCount> frame{};
        for (size_t i=0;i<frame.size();++i) frame[i]={pixels[i].data(),width*4,width,height,colorFormat};
        if (!diagnostic && !desktop && !producer.PublishFrame(frame)) { Fail("shared stereo texture upload"); return; }
    }
    if(desktop&&!gpu){
        if(world)desktopPixels=pixels[0];else desktopPixels.assign(size_t(width)*height,0xff000000);
        BlendDesktopHud(desktopPixels.data(),pixels[2].data(),desktopPixels.size());
        if(!eyeRestore.Draw(gameDevice,desktopPixels,width,height,colorFormat)){logger("Desktop composite failed at %s hr=0x%08lX.",eyeRestore.LastStage(),eyeRestore.LastResult());Fail("desktop VR composite",eyeRestore.LastResult());return;}
    }
    if(desktop){
        DWORD focused=0;GetWindowThreadProcessId(GetForegroundWindow(),&focused);
        const bool captureNow=focused==GetCurrentProcessId() && (GetAsyncKeyState(VK_F12)&0x8000);
        if(captureNow && !desktopCaptureHeld){
            if(gpu){
                if(!ReadGpuEyes())return;
                if(FAILED(capture.ReadSurface(gameDevice,gpuFrames.Surface(2),colorFormat,pixels[2],true)))return;
            }
            SaveDiagnosticFrame(diagnosticPrefix+L"-desktop-left.bmp",width,height,pixels[0]);
            SaveDiagnosticFrame(diagnosticPrefix+L"-desktop-right.bmp",width,height,pixels[1]);
            SaveDiagnosticFrame(diagnosticPrefix+L"-desktop-ui.bmp",width,height,pixels[2]);
            logger("Desktop capture saved; body active=%d hover=%d equipped=%d",bodyActive,bodyFrame.hovered,equippedItem);
        }
        desktopCaptureHeld=captureNow;
    }
    const bool menu=menuPointer.Active();
    if(menu) {
        const auto& a=menuPointer.Anchor();
        b->frameUiWorldAnchor={};
        b->frameUiWorldAnchor.positionX=a.position.x;b->frameUiWorldAnchor.positionY=a.position.y;b->frameUiWorldAnchor.positionZ=a.position.z;
        b->frameUiWorldAnchor.orientationX=a.orientation.x;b->frameUiWorldAnchor.orientationY=a.orientation.y;
        b->frameUiWorldAnchor.orientationZ=a.orientation.z;b->frameUiWorldAnchor.orientationW=a.orientation.w;
    }
    InterlockedExchange(&b->frameUiWorldAnchorValid,menu?1:0);
    InterlockedExchange(&b->frameUiReferenceMode,static_cast<LONG>(menu || !world?shared::UiReferenceMode::WorldLocked:shared::UiReferenceMode::HeadLocked));
    InterlockedIncrement(&b->producedFrameCount);
    InterlockedExchange(&b->frameSequence,pending);
    if (diagnostic||desktop) InterlockedExchange(&b->consumedFrameSequence,pending);
    (void)channel.SignalProducerUpdate();
    if (!world && diagnostic && ++menuFrames==120) {
        SaveDiagnosticFrame(diagnosticPrefix+L"-menu.bmp",width,height,pixels[2]);
        SaveDiagnosticFrame(diagnosticPrefix+L"-menu-left.bmp",width,height,pixels[0]);
        SaveDiagnosticFrame(diagnosticPrefix+L"-menu-right.bmp",width,height,pixels[1]);
        logger("Diagnostic menu captured; both projection textures are empty.");
    }
    if (world) {
        ++pairs;
        if (diagnostic && (pairs==3 || pairs==30 || pairs==60)) {
            for(size_t i=0;i<pixels.size();++i) SaveDiagnosticFrame(diagnosticPrefix+L"-"+std::to_wstring(pairs)+L"-"+std::to_wstring(i)+L".bmp",width,height,pixels[i]);
            logger("Diagnostic eye/UI images saved at pair %lu.",pairs);
        }
        if (!gpu && (pairs==1 || pairs==120)) {
            size_t different=0;
            for (size_t i=0;i<pixels[0].size();++i) different+=pixels[0][i]!=pixels[1][i];
            logger("Stereo pair=%lu sequence=%ld differing-eye-pixels=%zu; head=(%.3f,%.3f,%.3f).",
                pairs,pending,different,request.headPose.positionX,request.headPose.positionY,request.headPose.positionZ);
        }
    }
    pending=0;
}
struct CameraScope { ~CameraScope() { EndNativeEye(); } };
}
bool StartStereo(const std::wstring& presenter,const std::wstring& log,LogFunction logCallback) {
    logger=logCallback;menuTrace::Initialize(logger);renderProfile.Initialize(logger);gpuRequested=GpuTransferRequested();
    settings=LoadVrSettings(log);
    wchar_t canvasRequest[64]{};widescreenUi=GetEnvironmentVariableW(L"BF2142VR_RENDER_CANVAS",canvasRequest,64)>0;
    if(!settings.lobbySceneFile.empty())logger("Private walker lobby assets: %s.",menuRoomGpu.LoadScene(settings.lobbySceneFile)?"loaded":"unavailable; basic room retained");
    ConfigureAutomaticAds(settings.automaticAds);
    const auto name=L"Local\\BF2142VR-"+std::to_wstring(GetCurrentProcessId())+L"-"+std::to_wstring(GetTickCount64());
    if (!channel.Create(name.c_str(),GetCurrentProcessId())) return false;
    if(presenter==L"@desktop"){
        desktop=true;enabled=true;diagnosticPrefix=log;ResetDesktopSimulation();SetDesktopInput(true);
        logger("Desktop VR development mode: simulated poses, real native stereo/hands/ADS; no OpenXR session.");
        if(settings.controllers)StartControllerInput(logger);return true;
    }
    if (presenter==L"@diagnostic") { diagnostic=true; diagnosticPrefix=log; enabled=true; logger("Diagnostic stereo: synthetic poses and local image capture; controller commands remain disabled."); if(settings.controllers)StartControllerInput(logger); return true; }
    channel.Get()->producerFlags=shared::kProducerFlagRuntimeTimedRender | shared::kProducerFlagFullEyeTextureFov |
        shared::kProducerFlagOwnControllerMappings | shared::kProducerFlagBattlefield2142 |
        (widescreenUi?shared::kProducerFlagWidescreenUiCanvas:0);
    std::wstring command=QuoteArgument(presenter)+L" --channel "+QuoteArgument(name)+
        L" --run-until-stopped --log "+QuoteArgument(log+L".openxr.log");
    STARTUPINFOW startup{}; startup.cb=sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(presenter.c_str(),command.data(),nullptr,nullptr,FALSE,CREATE_NO_WINDOW,
        nullptr,nullptr,&startup,&process)) return false;
    CloseHandle(process.hThread); presenterProcess=process.hProcess;
    enabled=true;
    if(settings.controllers && !StartControllerInput(logger)) logger("Controller input unavailable; keyboard/mouse retained.");
    logger("OpenXR presenter started pid=%lu; channel=%ls; stereo requested.",process.dwProcessId,name.c_str());
    return true;
}
void StereoDeviceCreated(IDirect3DDevice9* device) {
    if (!enabled) return;
    gameDevice=device;
    menuPointer.Connect(device);
    if(desktop){D3DDEVICE_CREATION_PARAMETERS p{};if(SUCCEEDED(device->GetCreationParameters(&p))){desktopWindow=p.hFocusWindow;SetWindowTextW(desktopWindow,L"BF2142 Desktop VR | F1 hold F2 sight F3-F8 slots | RCtrl grab RShift support F9 trigger F10 menu");}}
    if (!nativeInstalled) nativeInstalled=InstallNativeStereo(logger);
    if (nativeInstalled && settings.motionHands && !diagnostic)settings.motionHands=InstallNativeHands(logger);
    EnableFingerPoses(settings.fingerPoses);ConfigureMotionActions(settings.motionActions);
    if(settings.bodyInventory && !settings.bodyEquipmentFile.empty())logger("Body equipment models loaded: %d",bodyEquipment.Load(settings.bodyEquipmentFile.c_str())?int(bodyEquipment.ModelCount()):0);
    if(nativeInstalled && settings.motionHands && settings.weaponOptics && !diagnostic){
        opticsReady=InstallNativeOptics(logger);
        if(!opticsReady)logger("VR optics unavailable: zoom profile did not connect; existing ADS retained.");
    }
    if (!nativeInstalled) Fail("native camera/frame profile could not be connected");
    if (enabled && !uiCapture.Connect(device,logger)) Fail("native interface capture could not be connected");
}
bool RenderStereo(void* renderer,NativeRender original,double delta,float interpolation) {
    if(insideRender)return original(renderer,delta,interpolation);
    if(!enabled || !NativeViewsAvailable(renderer)){renderTime.Reset();return original(renderer,delta,interpolation);}
    if(lastRenderer!=renderer)renderTime.Reset();
    lastRenderer=renderer;
    if (pairReady || !AwaitRenderRequest([]{return GetRequest(true);},[]{return enabled;},
            []{return GetTickCount64();},[]{(void)channel.WaitForPresenterUpdate(1);})) {
        // Timeout/disconnect retains the last stereo scene. Normal consumer
        // waits stay inside this engine frame, so native equip/reload clocks
        // cannot race ahead of the animation evaluation that makes guns ready.
        if(enabled){renderTime.Skip(delta);return lastRenderResult;}
        renderTime.Reset();return original(renderer,delta,interpolation);
    }
    struct ProfileFrame {
        ~ProfileFrame(){renderProfile.CompleteFrame();}
    } profileFrame;
    auto profile=renderProfile.Measure(RenderProfile::Frame);
    delta=renderTime.Take(delta);
    if(pairs==1)logger("Native renderer time argument retained: %.6f, interpolation=%.6f.",delta,double(interpolation));
    opticHudReady=false;opticHudPixels.clear();
    gpuPair=BeginGpuPair();
    insideRender=true;
    struct RenderScope { ~RenderScope() { insideRender=false; activeEye=-1; } } rendering;
    bool result=false, rendered=false;
    try {
        // The native renderer owns its D3D9 state cache. Re-enter with that
        // cache and the device in agreement; an external state-block Apply
        // here would desynchronize the engine's cached state from the GPU.
        suppressedPresents=0;
        std::array<EyeCamera,2> eyeCameras{};
        for (int eye=0;eye<2;++eye) {
            activeEye=eye;
            outsideUiReady=false;
            radioDrawn=false;bodyGpuDrawn=false;uiReady=false; // Accumulate HUD and menu batches within this eye only.
            if (!BeginNativeEye(renderer,reference,request.views[eye])) {
                Fail("invalid native camera or headset pose");
                return rendered?result:original(renderer,delta,interpolation);
            }
            {
                CameraScope camera;
                // Only the first eye advances renderer-side animation time.
                // The outer game simulation and input processing still run once.
                bool eyeResult=false;
                {auto timing=renderProfile.Measure(RenderProfile::NativeEye);eyeResult=original(renderer,eye==0?delta:0.0,interpolation);}
                if (!rendered) result=eyeResult;
                rendered=true;
                if(!ReadNativeEyeCamera(&eyeCameras[eye]))return result;
            }
            if(gpuPair){
                auto timing=renderProfile.Measure(RenderProfile::EyeReadback);
                const auto hr=gpuFrames.StageBackbuffer(unsigned(eye));
                if(FAILED(hr)){Fail("GPU eye snapshot",hr);return result;}
            }else if (!ReadFrame(pixels[eye])) return result;
        }
        if(!diagnostic && opticsReady)UpdateAutomaticAds(eyeCameras);
        GunOptic optic;
        // Diagnostic fixtures can supply a synthetic native optic. Ordinary
        // diagnostic game runs have no hand adapter and always return false.
        if((opticsReady || diagnostic) && !menuPointer.Active() && ReadNativeOptic(&optic)){
            const auto scope=MakeOpticView(optic,eyeCameras);
            const bool gpuOptic=scope&&gpuPair&&gpuOptics.Prepare(gameDevice,width,height);
            if(gpuOptic){
                bool captured=true;
                if(optic.magnification>1.01f){
                    activeEye=2;
                    captured=false;
                    if(BeginNativeScope(renderer,scope->world,scope->fov)){
                        CameraScope camera;
                        {auto timing=renderProfile.Measure(RenderProfile::NativeScope);original(renderer,0.0,interpolation);}
                        captured=gpuOptics.CaptureScope();
                    }
                }
                if(!captured){Fail("GPU optic scope capture");return result;}
                auto timing=renderProfile.Measure(RenderProfile::OpticComposite);
                auto* hud=opticHudReady?uiCapture.ResolveTexture(true):nullptr;
                auto* baseline=hud?uiCapture.ResolveTexture():nullptr;
                for(unsigned i=0;i<2;++i)if(!gpuOptics.Draw(gpuFrames.Surface(i),optic,eyeCameras[i],*scope,i,hud,baseline)){
                    Fail("GPU optic composition");return result;
                }
                if(!eyeRestore.DrawTexture(gameDevice,gpuFrames.Texture(0))){Fail("GPU optic desktop restore");return result;}
            }else{
                if(scope){
                    if(!ReadGpuEyes())return result;
                    auto timing=renderProfile.Measure(RenderProfile::OpticHud);
                    if(opticHudReady&&uiCapture.Read(colorFormat,opticHudBaseline)&&uiCapture.ReadOptic(colorFormat,opticHudPixels))
                        IsolateOpticHud(opticHudPixels,opticHudBaseline,width,height);
                    else opticHudPixels.clear();
                }
                if(scope && optic.magnification<=1.01f){
                    // Reflex glass retains the normal stereo world. Only draw the
                    // sight's collimated dot; never re-render or magnify this eye.
                    const bool rgba=colorFormat==DXGI_FORMAT_R8G8B8A8_UNORM||colorFormat==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                    auto timing=renderProfile.Measure(RenderProfile::OpticComposite);
                    for(unsigned i=0;i<2;++i)CompositeGunOptic(pixels[i],{},width,height,optic,eyeCameras[i],*scope,i,rgba,opticHudPixels);
                }else if(scope){
                    activeEye=2;
                    bool captured=false;
                    if(BeginNativeScope(renderer,scope->world,scope->fov)){
                        CameraScope camera;
                        {auto timing=renderProfile.Measure(RenderProfile::NativeScope);original(renderer,0.0,interpolation);}
                        auto timing=renderProfile.Measure(RenderProfile::ScopeReadback);
                        captured=SUCCEEDED(capture.Read(gameDevice,colorFormat,scopePixels)) && scopePixels.size()==pixels[0].size();
                    }
                    if(captured){
                        const bool rgba=colorFormat==DXGI_FORMAT_R8G8B8A8_UNORM||colorFormat==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
                        auto timing=renderProfile.Measure(RenderProfile::OpticComposite);
                        for(unsigned i=0;i<2;++i)CompositeGunOptic(pixels[i],scopePixels,width,height,optic,eyeCameras[i],*scope,i,rgba,opticHudPixels);
                    }else{DisableNativeOptics();opticsReady=false;}
                    // The extra native Present stayed suppressed. Restore a normal
                    // eye before the one real desktop Present/OBS capture.
                    if(!eyeRestore.Draw(gameDevice,pixels[0],width,height,colorFormat)){
                        DisableNativeOptics();opticsReady=false;Fail("restore normal desktop eye after optic replay");return result;
                    }
                }
            }
        }
        if(settings.grenadeArc && !menuPointer.Active()){
            GrenadeTrajectory arc;
            if(ReadNativeGrenadeTrajectory(&arc)){
                if(!ReadGpuEyes())return result;
                for(unsigned eye=0;eye<2;++eye)DrawGrenadeArc(pixels[eye],width,height,colorFormat,eyeCameras[eye],arc);
                eyeRestore.Draw(gameDevice,pixels[0],width,height,colorFormat);
            }
        }
        activeEye=-1;
        pairReady=true;lastRenderResult=result;
        // NativeRender calls Present internally. Both calls were suppressed,
        // so eye readback happened before DISCARD could invalidate pixels.
        insideRender=false;
        if (enabled) gameDevice->Present(nullptr,nullptr,nullptr,nullptr);
        return result;
    } catch (...) {
        if (uiCapture.Active()) uiCapture.Detach();
        EndNativeEye(); Fail("stereo frame allocation or runtime exception");
        return rendered?result:original(renderer,delta,interpolation);
    }
}
bool StereoOpticHudBegin(){
    if(!insideRender||activeEye!=1||!opticsReady||menuPointer.Active())return false;
    GunOptic optic;if(!ReadNativeOptic(&optic))return false;
    return uiCapture.BeginOptic(gameDevice);
}
void StereoOpticHudEnd(){opticHudReady=uiCapture.EndOptic();}
bool SuppressStereoPresent(IDirect3DDevice9* device) {
    if (!insideRender || device!=gameDevice) return false;
    ++suppressedPresents;
    return true;
}
bool HideStereoWorldMarkers() {return enabled && settings.hideWorldMarkers;}
bool ReadStereoMarkerFrame(EyeCamera* head,EyeCamera* eye) {
    if(!head||!eye||!insideRender||activeEye<0||activeEye>1||!request.headPoseValid||!request.viewsValid||!ReadNativeEyeCamera(eye))return false;
    const auto center=stereo::ComposeRuntimeHeadWithD3D8Camera(eye->world,Pose(request.views[activeEye].pose),Pose(request.headPose),settings.worldScale);
    const auto& a=request.views[0].fov;const auto& b=request.views[1].fov;
    const stereo::FovTangents common{std::tan(std::max(a.angleLeft,b.angleLeft)),std::tan(std::min(a.angleRight,b.angleRight)),
        std::tan(std::min(a.angleUp,b.angleUp)),std::tan(std::max(a.angleDown,b.angleDown))};
    const auto projection=stereo::MakeD3D8ProjectionFromFovTangents(common,.01f,2000.f);
    if(!center||!projection)return false;head->world=*center;head->projection=*projection;return true;
}
bool IsSecondStereoEye() { return insideRender && activeEye>0; }
bool IsScopeRender() {return insideRender && activeEye==2;}
bool StereoHudBegin(bool standaloneMenu) {
    if (!enabled || !gameDevice || uiCapture.Active() || IsScopeRender()) return false;
    if(!insideRender && (!standaloneMenu || !lastRenderer || !NativeWorldActive(lastRenderer) || (!diagnostic && !menuPointer.MenuVisible())))return false;
    // A native Flash/HUD boundary is positive evidence of UI. While paused it
    // can execute without NativeRender; capture just that batch, never the full
    // scene from an extra desktop Present.
    if(insideRender && activeEye>=0 && activeEye<2 && !radioDrawn && radioFrame.visible && !menuPointer.Active()){
        radioDrawn=true;stereo::Matrix4 projection{};
        if(ReadNativeWeaponProjection(&projection))DrawShoulderRadio(gameDevice,radioFrame,request.views[activeEye],projection,settings.worldScale);
    }
    if(insideRender && activeEye>=0 && activeEye<2 && !bodyGpuDrawn && !menuPointer.Active()){
        bodyGpuDrawn=true;stereo::Matrix4 projection{};
        if(ReadNativeWeaponProjection(&projection)){
            if(bodyActive){
                auto visible=inventoryNames;for(int i=1;i<10;++i)if(supportFrame.unavailable[i])visible[i]={};
                bodyEquipment.DrawGpu(gameDevice,request.views[activeEye],projection,settings.worldScale,bodyFrame,visible,NativeWeaponHeld()?equippedItem:0);
            }
            wristGpu.Draw(gameDevice,wristFrame,request.views[activeEye],projection,settings.worldScale);
        }
    }
    const bool clear=insideRender?!uiReady:!outsideUiReady;
    const bool captured=uiCapture.Begin(gameDevice,clear);
    if (!captured && !warnedUi) {
        warnedUi=true; logger("Native UI isolation unavailable for this target.");
    }
    return captured;
}
void StereoHudEnd() {
    if (!uiCapture.Active()) return;
    const bool detached=uiCapture.Detach();
    if(insideRender)uiReady=detached;else outsideUiReady=detached;
}
void StereoPresent(IDirect3DDevice9* device) {
    if (!enabled || insideRender || device!=gameDevice) return;
    menuPointer.ExpireInput();
    try {
        if (pairReady) {
            pairReady=false;
            if (!Healthy()) return;
            if(gpuPair){
                auto timing=renderProfile.Measure(RenderProfile::UiReadback);
                const auto hr=gpuFrames.Stage(2,uiReady?uiCapture.Surface():nullptr);
                if(FAILED(hr)){Fail("GPU UI snapshot",hr);return;}
            }else{
                if (uiReady) {auto timing=renderProfile.Measure(RenderProfile::UiReadback);uiReady=uiCapture.Read(colorFormat,pixels[2]) && pixels[2].size()==size_t(width)*height;}
                if (!uiReady) pixels[2].assign(size_t(width)*height,0);
            }
            if (pairs==0 || pairs==119) logger("Stereo boundary: suppressed presents=%u, HUD isolated=%d.",suppressedPresents,uiReady);
            Publish(true);
            if (uiReady && !desktop) uiCapture.CompositeDesktop();
            uiReady=false;
        } else if(outsideUiReady) {
            // Continue the XR request/input loop when a pause menu has stopped
            // world rendering. Both B and Home/recenter remain live here.
            outsideUiReady=false;
            if(GetRequest(true,true)) {
                gpuPair=BeginGpuPair();
                if(gpuPair){if(!StageGpuMenu(uiCapture.Surface(),false)){Fail("GPU paused menu snapshot");return;}}
                else{
                    if(!uiCapture.Read(colorFormat,pixels[2]) || pixels[2].size()!=size_t(width)*height)return;
                    std::fill(pixels[0].begin(),pixels[0].end(),0xff000000);
                    std::fill(pixels[1].begin(),pixels[1].end(),0xff000000);
                }
                Publish(false);
            }
            if(!desktop)uiCapture.CompositeDesktop();
        } else if ((!lastRenderer || !NativeWorldActive(lastRenderer)) &&
                   ((gpuRequested&&!diagnostic&&!gpuMenuBlocked&&!DiagnosticCpuTransfer()&&(!gpuAttempted||gpuReady)) ||
                    GetTickCount64()-lastMenuCapture>=33) && GetRequest()) {
            // The request/consumer handshake paces GPU menus to the runtime.
            // Keep the readback throttle for the CPU compatibility path only.
            lastMenuCapture=GetTickCount64();
            // Native Render may be followed by additional desktop Presents.
            // A missing eye pair is NOT evidence that the player left the world.
            // Never publish the full scene as a menu or reset gameplay tracking.
            gpuPair=BeginGpuPair();
            if(gpuPair){if(!StageGpuMenu(nullptr,true)){Fail("GPU frontend menu snapshot");return;}}
            else{
                if (!ReadFrame(pixels[2])) return;
                std::fill(pixels[0].begin(),pixels[0].end(),0xff000000);
                std::fill(pixels[1].begin(),pixels[1].end(),0xff000000);
            }
            Publish(false); // Login/loading/menu UI remains accessible in the headset.
        }
    } catch (...) { Fail("stereo presentation allocation or runtime exception"); }
}
void StereoReset() {
    menuOverlayGpu.Reset();gpuOptics.Reset();gpuFrames.Reset();gpuReady=false;gpuAttempted=false;gpuPair=false;gpuMenuBlocked=false;
    renderTime.Reset();opticHudReady=false;opticHudPixels.clear();opticHudBaseline.clear();
    RestoreFramePacing();
    RecenterNativeVehicle();traversalControls.Reset();weaponGrip.Reset();SetNativeWeaponHeld(true);controlsMenu.Reset();physicalStance.Reset();ConfigurePhysicalCamera(false);standingHeight.Reset();
    ClearNativeSnapTurn();ConfigureNativeMovement(false,0);ConfigureNativeLookPitch(false,{});ResetAutomaticAds();ClearControllerCommand();ClearNativeHands(); controllerPolicy={}; haveGripReference=false; previousGameplay=false;
    menuPointer.Reset();shoulderRadio.Reset();radioFrame={};supportCrates.Reset();supportFrame={};PublishNativeSupport({});bodyInventory.Reset();ResetMotionActions();rightEquipmentCue.Reset();leftEquipmentCue.Reset();ResetDesktopSimulation();
    uiCapture.Reset(); uiReady=false;outsideUiReady=false;homePolicy={}; warnedUi=false;
    lastMenuCapture=0;
    lastRenderResult=true;
    bodyEquipment.ResetGpu();wristGpu.Reset();wristMenu.Reset();wristFrame={};
    capture.Reset();eyeRestore.Reset();menuRoomGpu.Reset();scopePixels.clear(); pairReady=false; pending=0; haveReference=false; lastRenderer=nullptr;
}
}


#ifdef BF2142_GPU_FIXTURE
namespace bfvr::bf2142 {unsigned TestGpuPublished(){return gpuPublished;}
void TestOpenControls(bool open){controlsMenu.Hotkey(false);if(controlsMenu.Open()!=open)controlsMenu.Hotkey(true);controlsMenu.Hotkey(false);}
bool TestBeginPausedMenu(){
    // Hidden fixtures have no foreground OS cursor. Exercise the same capture
    // path using its diagnostic visibility override, then publish on the real GPU path.
    const bool wasDiagnostic=diagnostic;diagnostic=true;
    const bool result=StereoHudBegin(true);diagnostic=wasDiagnostic;return result;
}
void TestHoldStereoConsumer(bool hold){
 if(auto* b=channel.Get())InterlockedExchange(&b->consumedFrameSequence,ReadCounter(b->frameSequence)-(hold?1:0));
}}
#endif
