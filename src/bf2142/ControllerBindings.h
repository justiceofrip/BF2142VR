#pragma once
#include "presenter/SharedPresentationProtocol.h"
#include <array>
#include <cmath>
#include <string>
namespace bfvr::bf2142 {
enum class BindAction : unsigned {Jump,Reload,Use,PickupKit,Deploy,Sprint,Recenter,Menu,NextItem,PreviousItem,Count};
inline constexpr unsigned bindCount=unsigned(BindAction::Count);
inline constexpr std::array<const wchar_t*,bindCount> bindNames{
    L"Jump",L"Reload",L"Use",L"PickupKit",L"Deploy",L"Sprint",L"Recenter",L"Menu",L"NextItem",L"PreviousItem"};
inline constexpr std::array<const wchar_t*,bindCount> bindLabels{
    L"JUMP",L"RELOAD",L"USE / ENTER",L"PICK UP KIT",L"DEPLOY",L"SPRINT",L"RECENTER",L"MENU / BACK",L"NEXT ITEM",L"PREVIOUS ITEM"};
// 1..6 = left primary/secondary/menu/stick/grip/trigger; 7..12 = right.
// Bit 4 adds the left trigger modifier. 0 is unbound.
inline constexpr unsigned bindingModifier=16;
struct ControllerBindings {
    std::array<unsigned,bindCount> keys{7,1,2,23,27,4,10,8,17,18};
    unsigned operator[](BindAction a) const {return keys[unsigned(a)];}
};
inline bool ValidBinding(unsigned key){const auto base=key&15;return key==0||(key<32&&base>=1&&base<=12&&!(base==6&&(key&16)));}
inline bool RawControllerButton(const shared::SharedControllerSample& s,unsigned key){
    if(key<1||key>12||!(s.flags&shared::kControllerSampleFlagSessionFocused))return false;
    const auto& h=s.hands[key>6?1:0];const auto n=(key-1)%6;
    if(n<4){constexpr DWORD bits[]={shared::kControllerHandButtonPrimary,shared::kControllerHandButtonSecondary,shared::kControllerHandButtonMenu,shared::kControllerHandButtonThumbstick};return (h.buttons&bits[n])!=0;}
    const auto flag=n==4?shared::kControllerHandFlagSqueezeActive:shared::kControllerHandFlagTriggerActive;
    const float v=n==4?h.squeezeValue:h.triggerValue;
    return (h.flags&flag)&&std::isfinite(v)&&v>.65f;
}
inline bool BoundButton(const shared::SharedControllerSample& s,unsigned key){
    if(!ValidBinding(key)||!key)return false;
    const unsigned base=key&15;
    return RawControllerButton(s,base)&&(base==6||RawControllerButton(s,6)==bool(key&16));
}
inline bool BoundAction(const shared::SharedControllerSample& s,const ControllerBindings& bindings,BindAction action){
    const unsigned key=bindings[action];
    if(BoundButton(s,key))return true;
    // An unrelated modifier must not disable sprint, recenter or menu. Only a
    // configured chord using this same button takes precedence over its base.
    if(!key||(key&16)||!ValidBinding(key)||!RawControllerButton(s,6)||!RawControllerButton(s,key))return false;
    for(auto other:bindings.keys)if(other==(key|16))return false;
    return true;
}
inline std::wstring BindingLabel(unsigned key){
    constexpr const wchar_t* names[]={L"UNBOUND",L"L PRIMARY",L"L SECONDARY",L"L MENU",L"L STICK",L"L GRIP",L"L TRIGGER",L"R PRIMARY",L"R SECONDARY",L"R MENU",L"R STICK",L"R GRIP",L"R TRIGGER"};
    if(!ValidBinding(key))return names[0];
    return std::wstring(key&16?L"LT + ":L"")+names[key&15];
}
class BindingCapture {
public:
    void Begin(unsigned action){selected=int(action);armed=false;}
    void Cancel(){selected=-1;armed=false;}
    int Selected() const {return selected;}
    bool Update(const shared::SharedControllerSample* s,ControllerBindings& bindings){
        if(selected<0)return false;
        if(!s||!(s->flags&shared::kControllerSampleFlagSessionFocused)){Cancel();return false;}
        bool any=false;for(unsigned i=1;i<=12;++i)any|=RawControllerButton(*s,i);
        if(!armed){if(!any)armed=true;return false;}
        // Left trigger can be held while choosing the second button of a chord.
        for(unsigned i=1;i<=12;++i)if(i!=6&&RawControllerButton(*s,i)){
            const unsigned key=i|(RawControllerButton(*s,6)?16:0);
            for(auto& other:bindings.keys)if(other==key)other=0;
            bindings.keys[unsigned(selected)]=key;Cancel();return true;
        }
        return false;
    }
private:
    int selected=-1;bool armed=false;
};
// Release-to-arm prevents a squeeze held through death from opening/closing a
// menu, and holds Enter long enough for the native input poll to observe it.
class DeployGrip {
public:
    bool Update(bool active,bool dead,bool grip,std::int64_t now){
        if(!active||!dead||now<=0||now<last||now-last>250000000){*this={};last=now;return false;}
        last=now;if(!grip)armed=true;
        if(grip&&!held&&armed){until=now+120000000;armed=false;}
        held=grip;return now<until;
    }
private:
    std::int64_t last=0,until=0;bool held=false,armed=false;
};
}
