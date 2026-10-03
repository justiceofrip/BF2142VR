#include "HandPoseMath.h"
#include "stereo/ArmPoleVectorMath.h"
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
using V=stereo::Vec3;using M=stereo::Matrix4;
V Add(V a,V b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
V Sub(V a,V b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
V Mul(V a,float n){return {a.x*n,a.y*n,a.z*n};}
float Dot(V a,V b){return a.x*b.x+a.y*b.y+a.z*b.z;}
V Cross(V a,V b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
float Length(V a){return std::sqrt(Dot(a,a));}
bool Finite(V a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
V Unit(V a){const float l=Length(a);return l>1.e-6f?Mul(a,1/l):V{};}
V Pos(const M& m){return {m.values[3][0],m.values[3][1],m.values[3][2]};}
V Row(const M& m,int r){return {m.values[r][0],m.values[r][1],m.values[r][2]};}
void SetPos(M& m,V v){m.values[3]={v.x,v.y,v.z,1};}
M Identity(){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;return m;}
M Facing(const M& prior,V direction){
    const V z=Unit(direction);V up=Row(prior,1);V x=Unit(Cross(up,z));
    if(Length(x)<.5f)x=Unit(Cross(V{0,0,1},z));
    if(Length(x)<.5f)x=Unit(Cross(V{1,0,0},z));
    const V y=Cross(z,x);M out=Identity();out.values[0]={x.x,x.y,x.z,0};
    out.values[1]={y.x,y.y,y.z,0};out.values[2]={z.x,z.y,z.z,0};SetPos(out,Pos(prior));return out;
}
// Shortest-arc basis rotation retains authored arm roll. Row-vector convention.
M RotateFromTo(V from,V to){
    from=Unit(from);to=Unit(to);const float c=std::clamp(Dot(from,to),-1.f,1.f);
    V axis=Cross(from,to);float s=Length(axis);
    if(s<1.e-5f){if(c>0)return Identity();axis=Unit(Cross(from,{0,1,0}));if(Length(axis)<.5f)axis=Unit(Cross(from,{1,0,0}));}
    else axis=Mul(axis,1/s);
    const float t=1-c,x=axis.x,y=axis.y,z=axis.z;M m=Identity();
    m.values[0]={t*x*x+c,t*x*y+s*z,t*x*z-s*y,0};
    m.values[1]={t*x*y-s*z,t*y*y+c,t*y*z+s*x,0};
    m.values[2]={t*x*z+s*y,t*y*z-s*x,t*z*z+c,0};return m;
}
M MoveBone(const M& bone,V oldStart,V oldEnd,V newStart,V newEnd){
    M local=bone;SetPos(local,Sub(Pos(bone),oldStart));
    M out=Multiply(local,RotateFromTo(Sub(oldEnd,oldStart),Sub(newEnd,newStart)));
    // Intermediate twist/elbow bones retain their fraction along the limb.
    const float oldLength=Length(Sub(oldEnd,oldStart));
    if(oldLength>1.e-5f)SetPos(out,Add(newStart,Mul(Pos(out),Length(Sub(newEnd,newStart))/oldLength)));
    return out;
}
bool Arm(HandBones& out,const HandBones& native,int shoulder,int elbow,int wrist,const M& target,const M& head,const M& torso,bool left){
    V root=Pos(native[shoulder]);
    // The stock 1P animation pulls shoulders toward the aiming reticle. Anchor
    // them below the tracked head in the stable body frame instead.
    M frame=torso;SetPos(frame,Pos(head));
    const auto inverseFrame=InverseRigid(frame);if(!inverseFrame)return false;
    M localRoot=Identity();SetPos(localRoot,{left?-.18f:.18f,-.22f,-.015f});
    root=Pos(Multiply(localRoot,frame));
    const float a=Length(Sub(Pos(native[elbow]),Pos(native[shoulder])));
    const float b=Length(Sub(Pos(native[wrist]),Pos(native[elbow])));
    const auto local=SolveArm(Pos(localRoot),Pos(Multiply(target,*inverseFrame)),a,b,left);if(!local)return false;
    M localElbow=Identity();SetPos(localElbow,local->elbow);
    const auto solved=ArmSolution{Pos(Multiply(localElbow,frame)),Pos(target)};
    for(int i=shoulder;i<elbow;++i)out[i]=MoveBone(native[i],Pos(native[shoulder]),Pos(native[elbow]),root,solved.elbow);
    for(int i=elbow;i<wrist;++i)out[i]=MoveBone(native[i],Pos(native[elbow]),Pos(native[wrist]),solved.elbow,solved.wrist);
    const auto inverse=InverseRigid(native[wrist]);if(!inverse)return false;
    M end=target;SetPos(end,solved.wrist);const M delta=Multiply(*inverse,end);
    for(int i=wrist;i<wrist+21;++i)out[i]=Multiply(native[i],delta);
    return true;
}
}
bool PoseFreeFingers(HandBones& bones,int wrist,const std::array<float,5>& curls) noexcept {
    if(wrist!=7 && wrist!=33)return false;
    HandBones out=bones;
    for(int finger=0;finger<5;++finger){
        if(!std::isfinite(curls[finger]))return false;
        const float curl=std::clamp(curls[finger],0.f,1.f);
        if(curl>=.9999f)continue; // Exact authored contact, including terminal bone orientation.
        const int base=wrist+1+finger*4;
        const V straight=Unit(Sub(Pos(bones[base]),Pos(bones[wrist])));
        if(Length(straight)<.5f)return false;
        V position=Pos(bones[base]);
        for(int joint=0;joint<3;++joint){
            const int b=base+joint;
            const V authored=Sub(Pos(bones[b+1]),Pos(bones[b]));
            const float length=Length(authored);if(!std::isfinite(length)||length<.002f||length>.10f)return false;
            const V desired=Unit(Add(Mul(straight,1-curl),Mul(Unit(authored),curl)));
            if(Length(desired)<.5f)return false;
            M m=Multiply(bones[b],RotateFromTo(authored,desired));SetPos(m,position);out[b]=m;
            position=Add(position,Mul(desired,length));
        }
        const auto tipSource=InverseRigid(bones[base+2]);if(!tipSource)return false;
        const auto tipDelta=Multiply(*tipSource,out[base+2]);
        out[base+3]=Multiply(bones[base+3],tipDelta);SetPos(out[base+3],position);
    }
    bones=out;return true;
}
bool PoseEmptyFingers(HandBones& bones,int wrist,const M& palm,const std::array<float,5>& curls) noexcept {
    if((wrist!=7&&wrist!=33)||!InverseRigid(palm))return false;
    HandBones out=bones;auto thumbOnly=curls;for(int i=1;i<5;++i)thumbOnly[i]=1;
    if(!PoseFreeFingers(out,wrist,thumbOnly))return false;
    const V inward=Mul(Row(palm,0),wrist==33?-1.f:1.f);
    for(int finger=1;finger<5;++finger){
        if(!std::isfinite(curls[finger]))return false;
        const float curl=std::clamp(curls[finger],0.f,1.f);
        const int base=wrist+1+finger*4;
        V along=Sub(Pos(bones[base]),Pos(bones[wrist]));along=Unit(Sub(along,Mul(inward,Dot(along,inward))));
        if(Length(along)<.5f)return false;
        // Use one anatomical hinge for the entire finger. Independent shortest
        // arcs from a tightly curled grip can flip distal roll when a segment
        // is nearly opposite the open direction (especially the right hand).
        V hinge=Unit(Cross(along,inward));
        const M baseOpen=Multiply(bones[base],RotateFromTo(Sub(Pos(bones[base+1]),Pos(bones[base])),along));
        if(Dot(hinge,Row(baseOpen,2))<0)hinge=Mul(hinge,-1);
        V position=Pos(bones[base]);constexpr float bend[]={1.15f,2.65f,3.35f};
        for(int joint=0;joint<3;++joint){
            const int b=base+joint;const V source=Sub(Pos(bones[b+1]),Pos(bones[b]));const float length=Length(source);
            if(!std::isfinite(length)||length<.002f||length>.10f)return false;
            const float angle=curl*bend[joint];const V direction=Add(Mul(along,std::cos(angle)),Mul(inward,std::sin(angle)));
            const V sourceY=Unit(source);
            const V sourceZ=Unit(Sub(Row(bones[b],2),Mul(sourceY,Dot(Row(bones[b],2),sourceY))));
            if(Length(sourceZ)<.5f)return false;
            const V sourceX=Cross(sourceY,sourceZ),targetX=Cross(direction,hinge);
            M from=Identity(),to=Identity();
            from.values[0]={sourceX.x,sourceX.y,sourceX.z,0};from.values[1]={sourceY.x,sourceY.y,sourceY.z,0};from.values[2]={sourceZ.x,sourceZ.y,sourceZ.z,0};
            to.values[0]={targetX.x,targetX.y,targetX.z,0};to.values[1]={direction.x,direction.y,direction.z,0};to.values[2]={hinge.x,hinge.y,hinge.z,0};
            const auto inverse=InverseRigid(from);if(!inverse)return false;
            out[b]=Multiply(bones[b],Multiply(*inverse,to));SetPos(out[b],position);position=Add(position,Mul(direction,length));
        }
        const auto inverse=InverseRigid(bones[base+2]);if(!inverse)return false;
        out[base+3]=Multiply(bones[base+3],Multiply(*inverse,out[base+2]));SetPos(out[base+3],position);
    }
    bones=out;return true;
}
bool PoseFingerContact(HandBones& bones,int base,V target) noexcept {
    if(base<0||base+3>=int(bones.size())||!Finite(target))return false;
    std::array<V,4> p{};float length[3]{},reach=0;
    for(int i=0;i<4;++i){if(!InverseRigid(bones[base+i]))return false;p[i]=Pos(bones[base+i]);}
    for(int i=0;i<3;++i){length[i]=Length(Sub(p[i+1],p[i]));if(length[i]<.002f||length[i]>.10f)return false;reach+=length[i];}
    if(Length(Sub(target,p[0]))>reach*.995f)return false;
    const V root=p[0];
    // FABRIK only changes the thumb chain. Every phalanx retains its length;
    // an unreachable switch leaves the normal grip pose in place.
    for(int pass=0;pass<16;++pass){
        p[3]=target;
        for(int i=2;i>=0;--i)p[i]=Add(p[i+1],Mul(Unit(Sub(p[i],p[i+1])),length[i]));
        p[0]=root;
        for(int i=0;i<3;++i)p[i+1]=Add(p[i],Mul(Unit(Sub(p[i+1],p[i])),length[i]));
        if(Length(Sub(p[3],target))<.0005f)break;
    }
    if(Length(Sub(p[3],target))>.003f)return false;
    HandBones out=bones;
    for(int i=0;i<3;++i){out[base+i]=Multiply(bones[base+i],RotateFromTo(Sub(Pos(bones[base+i+1]),Pos(bones[base+i])),Sub(p[i+1],p[i])));SetPos(out[base+i],p[i]);}
    const auto inverse=InverseRigid(bones[base+2]);if(!inverse)return false;
    out[base+3]=Multiply(bones[base+3],Multiply(*inverse,out[base+2]));SetPos(out[base+3],p[3]);bones=out;return true;
}
bool SupportGripEligible(bool pressed,bool previous,float distance,float separation) noexcept {
    return pressed && std::isfinite(distance)&&std::isfinite(separation)&&distance>=0 && distance<(previous?.30f:.20f)&&separation>.12f&&separation<.80f;
}
std::optional<ArmSolution> SolveArm(V shoulder,V wrist,float upper,float lower,bool left) noexcept {
    if(!Finite(shoulder)||!Finite(wrist)||!std::isfinite(upper)||!std::isfinite(lower)||upper<.05f||lower<.05f||upper>.6f||lower>.6f)return {};
    V d=Sub(wrist,shoulder);float length=Length(d);if(length<.015f||length>(upper+lower)*1.65f)return {};
    // Human reach varies; stretch joint spacing modestly, never detach wrists
    // from controllers to hide an unreachable target.
    const float stretch=std::max(1.f,length/(upper+lower)*1.002f);upper*=stretch;lower*=stretch;
    if(length<std::abs(upper-lower)+.001f)return {};
    d=Mul(d,1/length);stereo::ArmPoleVectorInput pi{};pi.shoulder={shoulder.x,shoulder.y,shoulder.z};pi.handTarget={wrist.x,wrist.y,wrist.z};pi.leftArm=left;
    const auto p=stereo::ComputeArmPoleVector(pi);if(!p)return {};
    V pole{p->pole[0],p->pole[1],p->pole[2]};pole=Unit(Sub(pole,Mul(d,Dot(pole,d))));
    if(Length(pole)<.5f)pole=Unit(Cross(d,{0,0,1}));if(Length(pole)<.5f)pole=Unit(Cross(d,{1,0,0}));
    const float along=(upper*upper-lower*lower+length*length)/(2*length);
    const float bend=std::sqrt(std::max(0.f,upper*upper-along*along));
    return ArmSolution{Add(shoulder,Add(Mul(d,along),Mul(pole,bend))),wrist};
}
std::optional<HandBindings> CaptureHandBindings(const HandBones& native) noexcept {
    const auto inverse=InverseRigid(native[54]);if(!inverse || !InverseRigid(native[7]) || !InverseRigid(native[33]))return {};
    return HandBindings{Multiply(native[7],*inverse),Multiply(native[33],*inverse)};
}
std::optional<HandResult> SolveTrackedHands(const HandBones& native,const HandFrame& f) noexcept {
    if(!f.rightValid || !InverseRigid(f.head) || !InverseRigid(f.rightGrip) || !InverseRigid(f.rightAim))return {};
    if(f.leftValid && (!InverseRigid(f.leftGrip) || !InverseRigid(f.leftAim)))return {};
    const M torso=f.torso?*f.torso:Identity();if(!InverseRigid(torso))return {};
    // No partial output reaches the native skeleton if any authored matrix is invalid.
    for(const auto& b:native)if(!InverseRigid(b))return {};
    const auto invWeapon=InverseRigid(native[54]);const auto invWrist=InverseRigid(native[33]);
    if(!invWeapon||!invWrist)return {};
    const auto binding=f.bindings?f.bindings:CaptureHandBindings(native);if(!binding)return {};
    const M handFromWeapon=binding->rightFromWeapon;
    if(!InverseRigid(binding->leftFromWeapon) || !InverseRigid(handFromWeapon))return {};
    const auto weaponFromHand=InverseRigid(handFromWeapon);if(!weaponFromHand)return {};
    // Ordinary firearms keep the accepted right wrist binding and held fingers.
    M weapon=f.rightAim;M wrist=Multiply(handFromWeapon,weapon);SetPos(wrist,Pos(f.rightGrip));weapon=Multiply(*weaponFromHand,wrist);
    const bool emptyHands=!f.weaponHeld&&f.handReference&&f.leftPalm&&f.rightPalm;
    HandResult result{};result.bones=native;HandBones handSource=(f.knifeGrip||emptyHands)&&f.handReference?*f.handReference:native;
    if((f.knifeGrip||emptyHands)&&f.handReference)for(const auto& bone:handSource)if(!InverseRigid(bone))return {};
    const auto restoreHand=[&](int index,const ControllerHandPose& palm){
        // Rebuild the source about the current wrist only to retain arm lengths.
        // Its fixed finger-local pose no longer comes from knife animations.
        const auto inverse=InverseRigid(palm.bones[0]);if(!inverse)return false;
        const M sourceWrist=handSource[index];
        for(int i=0;i<21;++i)handSource[index+i]=Multiply(Multiply(palm.bones[i],*inverse),sourceWrist);
        return true;
    };
    const auto rightPalm=f.rightPalm?f.rightPalm:(f.knifeGrip?CaptureControllerHand(native,33):std::nullopt);
    const auto leftPalm=f.leftPalm?f.leftPalm:((f.knifeGrip||f.pistolGrip)?CaptureControllerHand(native,7):std::nullopt);
    if(f.knifeGrip||emptyHands){
        if(!rightPalm||!leftPalm||!restoreHand(33,*rightPalm)||!restoreHand(7,*leftPalm))return {};
        wrist=Multiply(rightPalm->bones[0],f.rightGrip);weapon=KnifeInGrip(f.rightGrip);
    }
    M left=Multiply(binding->leftFromWeapon,weapon);
    bool freeLeft=false;
    const auto prepareLeft=[&](){
        if(!leftPalm||!f.handReference)return false;
        for(int i=2;i<28;++i)handSource[i]=(*f.handReference)[i];
        return restoreHand(7,*leftPalm);
    };
    if(f.leftValid){
        const float separation=Length(Sub(Pos(f.leftGrip),Pos(f.rightGrip)));
        const bool pressed=f.weaponHeld&&f.supportPressed&&f.supportReady&&!f.knifeGrip;
        result.supporting=f.pistolGrip ? leftPalm&&rightPalm&&PistolSupportEligible(pressed,f.wasSupporting,separation) :
            SupportGripEligible(pressed,f.wasSupporting,Length(Sub(Pos(left),Pos(f.leftGrip))),separation);
        if(result.supporting && f.pistolGrip){
            // Cup the firing hand, not the barrel. Close palms provide no
            // useful two-point aiming baseline: preserve right-hand direction.
            const auto inverse=InverseRigid(rightPalm->bones[0]);if(!inverse||!restoreHand(7,*leftPalm))return {};
            const M firingPalm=Multiply(*inverse,wrist);
            M cup=Identity();SetPos(cup,{-.042f,.012f,-.018f});
            left=Multiply(leftPalm->bones[0],Multiply(cup,firingPalm));
        }else if(result.supporting){
            const V localDirection=Sub(Pos(binding->leftFromWeapon),Pos(binding->rightFromWeapon));
            M aimed=Facing(f.rightAim,Sub(Pos(f.leftGrip),Pos(f.rightGrip)));
            M correction=Facing(Identity(),localDirection);const auto inverse=InverseRigid(correction);if(!inverse)return {};
            weapon=Multiply(*inverse,aimed);wrist=Multiply(handFromWeapon,weapon);SetPos(wrist,Pos(f.rightGrip));weapon=Multiply(*weaponFromHand,wrist);
            left=Multiply(binding->leftFromWeapon,weapon);
        }else if(prepareLeft()){
            left=Multiply(leftPalm->bones[0],f.leftGrip);freeLeft=true;
        }else if(f.knifeGrip||emptyHands){left=Multiply(leftPalm->bones[0],f.leftGrip);}
        else{left=Multiply(binding->leftFromWeapon,f.leftAim);SetPos(left,Pos(f.leftGrip));}
        if(!Arm(result.bones,handSource,3,5,7,left,f.head,torso,true))return {};
    }else if(f.knifeGrip||emptyHands){
        // An untracked off-hand rests beside the torso; a knife swing cannot
        // pull it along with the weapon or put it back in a native stab pose.
        M rest=Identity();SetPos(rest,{-.24f,-.4f,.15f});rest=Multiply(rest,f.head);
        left=Multiply(leftPalm->bones[0],rest);
        if(!Arm(result.bones,handSource,3,5,7,left,f.head,torso,true))return {};
    }
    if(!Arm(result.bones,handSource,29,31,33,wrist,f.head,torso,false))return {};
    if(f.knifeGrip||emptyHands||freeLeft){
        // Keep the two clavicles with the solved shoulders as well. No native
        // slash/reach roll leaks back into either free arm through a parent.
        for(int shoulder:{3,29}){
            if(shoulder==29&&!f.knifeGrip&&!emptyHands)continue;
            const auto inverse=InverseRigid(handSource[shoulder]);if(!inverse)return {};
            result.bones[shoulder-1]=Multiply(Multiply(handSource[shoulder-1],*inverse),result.bones[shoulder]);
        }
    }
    if(f.leftItem){if(!f.leftValid||!InverseRigid(*f.leftItem))return {};weapon=*f.leftItem;}
    else if(!f.weaponHeld){
        // A rigid presentation transform keeps only the sixteen gun parts
        // outside both eye frusta. Neither arm nor the native camera moves.
        SetPos(weapon,Sub(Pos(f.head),Mul(Row(f.head,2),4.f)));
    }
    const M attachment=Multiply(*invWeapon,weapon);
    for(int i=54;i<70;++i)result.bones[i]=Multiply(native[i],attachment);
    // An untracked left controller retains the game's authored support hand,
    // rigidly attached to the moved item, rather than leaving it at the reticle.
    if(!f.leftValid && !f.knifeGrip && f.weaponHeld && !Arm(result.bones,native,3,5,7,Multiply(binding->leftFromWeapon,weapon),f.head,torso,true))return {};
    if(f.fingerPoses){
        auto held=f.rightCurls;if(f.weaponHeld)held[2]=held[3]=held[4]=1;
        // A held knife/grenade keeps its authored index around the handle.
        if(f.knifeGrip && f.weaponHeld)held[1]=1;
        if(emptyHands)(void)PoseEmptyFingers(result.bones,33,f.rightGrip,held);
        else (void)PoseFreeFingers(result.bones,33,held);
        if(f.leftValid){
            auto leftCurls=f.leftCurls;
            if(result.supporting){leftCurls[1]=leftCurls[2]=leftCurls[3]=leftCurls[4]=f.pistolGrip?.72f:1.f;}
            if(freeLeft||emptyHands)(void)PoseEmptyFingers(result.bones,7,f.leftGrip,leftCurls);
            else (void)PoseFreeFingers(result.bones,7,leftCurls);
        }
    }
    if(f.leftValid&&f.leftThumbContact)(void)PoseFingerContact(result.bones,8,*f.leftThumbContact);
    return result;
}
}
