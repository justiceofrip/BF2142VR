#include "VrSettings.h"
#include <windows.h>
#include <filesystem>
#include <cmath>
#include <cwchar>
namespace bfvr::bf2142 {
VrSettings LoadVrSettings(const std::wstring& logPath) {
    wchar_t overridePath[32768]{};
    const DWORD length=GetEnvironmentVariableW(L"BF2142VR_CONFIG",overridePath,32768);
    const auto path=length && length<32768?std::filesystem::path(overridePath):std::filesystem::path(logPath).parent_path().parent_path()/L"BF2142VR.ini";
    VrSettings s;s.configPath=path.wstring();
    const auto value=[&](const wchar_t* name,float fallback,float low,float high) {
        wchar_t text[80]{};GetPrivateProfileStringW(L"VR",name,L"",text,80,path.c_str());
        wchar_t* end=nullptr;const float v=std::wcstof(text,&end);
        return end!=text && *end==0 && std::isfinite(v) && v>=low && v<=high?v:fallback;
    };
    s.proximityVoice=GetPrivateProfileIntW(L"VR",L"ProximityVoice",1,path.c_str())!=0;
    s.proximityMuted=GetPrivateProfileIntW(L"VR",L"ProximityMicMuted",0,path.c_str())!=0;
    s.voiceThreshold=value(L"VoiceThresholdDb",-40,-65,-15);s.voiceVolume=value(L"VoiceVolume",1,0,2);
    s.voiceInput=GetPrivateProfileIntW(L"VR",L"VoiceInputDevice",UINT(-1),path.c_str());
    s.voiceOutput=GetPrivateProfileIntW(L"VR",L"VoiceOutputDevice",UINT(-1),path.c_str());
    const unsigned samples=GetPrivateProfileIntW(L"VR",L"WorldMSAASamples",8,path.c_str());
    s.worldSamples=(samples==0||samples==2||samples==4||samples==8)?samples:8;
    s.worldScale=value(L"WorldScale",1,.25f,4);
    s.heightOffset=value(L"HeightOffset",0,-1,1);
    s.turnSpeed=value(L"TurnSpeed",600,50,2000);
    s.snapAngle=value(L"SnapAngle",30,15,90);
    s.standingHeight=value(L"StandingHeight",0,-.5f,3.f);
    wchar_t direction[32]{};GetPrivateProfileStringW(L"VR",L"MovementDirection",L"head",direction,32,path.c_str());
    s.controllerRelativeMovement=_wcsicmp(direction,L"controller")==0;
    s.snapTurning=GetPrivateProfileIntW(L"VR",L"SnapTurning",0,path.c_str())!=0;
    s.hideCrosshair=GetPrivateProfileIntW(L"VR",L"HideCrosshair",1,path.c_str())!=0;
    s.hideWorldMarkers=GetPrivateProfileIntW(L"VR",L"HideWorldMarkers",1,path.c_str())!=0;
    s.physicalStance=GetPrivateProfileIntW(L"VR",L"PhysicalStance",1,path.c_str())!=0;
    s.menuRoom=GetPrivateProfileIntW(L"VR",L"MenuRoom",1,path.c_str())!=0;
    s.controllers=GetPrivateProfileIntW(L"VR",L"Controllers",1,path.c_str())!=0;
    s.trackedWeapon=GetPrivateProfileIntW(L"VR",L"TrackedWeapon",0,path.c_str())!=0;
    s.motionHands=GetPrivateProfileIntW(L"VR",L"MotionHands",1,path.c_str())!=0;
    s.weaponOptics=GetPrivateProfileIntW(L"VR",L"WeaponOptics",1,path.c_str())!=0;
    s.automaticAds=GetPrivateProfileIntW(L"VR",L"AutomaticADS",1,path.c_str())!=0;
    s.bodyInventory=GetPrivateProfileIntW(L"VR",L"BodyInventory",1,path.c_str())!=0;
    s.fingerPoses=GetPrivateProfileIntW(L"VR",L"FingerPoses",1,path.c_str())!=0;
    s.leftSupportCrates=GetPrivateProfileIntW(L"VR",L"LeftSupportCrates",1,path.c_str())!=0;
    s.motionActions=GetPrivateProfileIntW(L"VR",L"MotionActions",1,path.c_str())!=0;
    s.weaponFaceFade=false; // v24 fade rejected by the owner; old INIs cannot enable it.
    s.toggleWeaponGrip=GetPrivateProfileIntW(L"VR",L"ToggleWeaponGrip",1,path.c_str())!=0;
    s.grenadeArc=GetPrivateProfileIntW(L"VR",L"GrenadeArc",1,path.c_str())!=0;
    wchar_t equipment[32768]{};GetPrivateProfileStringW(L"VR",L"BodyEquipmentFile",L"",equipment,32768,path.c_str());
    s.bodyEquipmentFile=equipment;
    GetPrivateProfileStringW(L"VR",L"LobbySceneFile",L"",equipment,32768,path.c_str());s.lobbySceneFile=equipment;
    return s;
}
bool SaveVrPreferences(const VrSettings& s) {
    if(s.configPath.empty())return false;bool ok=true;
    const auto put=[&](const wchar_t* key,const std::wstring& v){ok=WritePrivateProfileStringW(L"VR",key,v.c_str(),s.configPath.c_str()) && ok;};
    put(L"ProximityVoice",s.proximityVoice?L"1":L"0");put(L"ProximityMicMuted",s.proximityMuted?L"1":L"0");
    put(L"VoiceThresholdDb",std::to_wstring(s.voiceThreshold));put(L"VoiceVolume",std::to_wstring(s.voiceVolume));
    put(L"VoiceInputDevice",std::to_wstring(s.voiceInput));put(L"VoiceOutputDevice",std::to_wstring(s.voiceOutput));
    put(L"WorldMSAASamples",std::to_wstring(s.worldSamples));
    put(L"MovementDirection",s.controllerRelativeMovement?L"controller":L"head");
    put(L"SnapTurning",s.snapTurning?L"1":L"0");put(L"SnapAngle",std::to_wstring(s.snapAngle));
    put(L"HideWorldMarkers",s.hideWorldMarkers?L"1":L"0");
    put(L"HideCrosshair",s.hideCrosshair?L"1":L"0");put(L"PhysicalStance",s.physicalStance?L"1":L"0");
    put(L"StandingHeight",std::to_wstring(s.standingHeight));put(L"MenuRoom",s.menuRoom?L"1":L"0");
    put(L"ToggleWeaponGrip",s.toggleWeaponGrip?L"1":L"0");put(L"GrenadeArc",s.grenadeArc?L"1":L"0");return ok;
}
}
