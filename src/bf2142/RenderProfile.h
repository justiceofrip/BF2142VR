#pragma once
#include "StereoSession.h"
#include <array>
namespace bfvr::bf2142 {
// Explicit local diagnostics only. Never enabled by renderer resolution changes.
class RenderProfile {
public:
    enum Stage { Frame, NativeEye, EyeReadback, NativeScope, ScopeReadback,
        OpticComposite, OpticHud, UiReadback, Publish, Count };
    void Initialize(LogFunction log) {
        wchar_t value[8]{};enabled=GetEnvironmentVariableW(L"BF2142VR_FRAME_PROFILE",value,8)==1&&value[0]==L'1';
        logger=log;LARGE_INTEGER f{};QueryPerformanceFrequency(&f);frequency=double(f.QuadPart);
        enabled=enabled&&frequency>0;start=Now();
    }
    struct Span {
        RenderProfile& profile;Stage stage;LONGLONG begin;
        ~Span(){if(begin){profile.ticks[stage]+=Now()-begin;++profile.calls[stage];if(stage==Frame)profile.inFrame=false;}}
    };
    Span Measure(Stage stage){
        if(enabled&&stage==Frame){inFrame=true;if(!frames){ticks={};calls={};start=Now();}}
        return {*this,stage,enabled&&inFrame?Now():0};
    }
    void CompleteFrame(){
        if(!enabled||++frames<60)return;
        const auto now=Now();
        logger("Render profile: 60 render frames in %.1f ms; mean ms/pair frame=%.2f native-eye=%.2f eye-read=%.2f native-scope=%.2f scope-read=%.2f optics=%.2f optic-hud=%.2f ui-read=%.2f publish=%.2f; scope calls=%u",
            (now-start)*1000/frequency,Ms(Frame),Ms(NativeEye),Ms(EyeReadback),Ms(NativeScope),Ms(ScopeReadback),Ms(OpticComposite),Ms(OpticHud),Ms(UiReadback),Ms(Publish),calls[NativeScope]);
        frames=0;ticks={};calls={};start=now;
    }
private:
    static LONGLONG Now(){LARGE_INTEGER value{};QueryPerformanceCounter(&value);return value.QuadPart;}
    double Ms(Stage stage) const{return ticks[stage]*1000/frequency/60;}
    bool enabled=false,inFrame=false;unsigned frames=0;LogFunction logger=nullptr;double frequency=1;LONGLONG start=0;
    std::array<LONGLONG,Count> ticks{};std::array<unsigned,Count> calls{};
};
}
