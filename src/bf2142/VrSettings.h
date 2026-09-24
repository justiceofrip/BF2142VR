#pragma once
#include <string>
namespace bfvr::bf2142 {
struct VrSettings {
    std::wstring bodyEquipmentFile,lobbySceneFile,configPath;
    float worldScale=1.f,heightOffset=0.f,turnSpeed=600.f,snapAngle=30.f,standingHeight=0.f;
    bool controllerRelativeMovement=false;
    bool snapTurning=false,hideCrosshair=true,physicalStance=true,menuRoom=true;
    bool controllers=true,trackedWeapon=false,motionHands=true,weaponOptics=true,automaticAds=true,bodyInventory=true,fingerPoses=true,motionActions=true,leftSupportCrates=true,weaponFaceFade=false,toggleWeaponGrip=true,grenadeArc=true;
};
VrSettings LoadVrSettings(const std::wstring& logPath);
bool SaveVrPreferences(const VrSettings& settings);
}
