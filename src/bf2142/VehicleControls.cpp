#include "VehicleControls.h"
#include "ComfortCamera.h"
#include "TrackingMath.h"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace bfvr::bf2142 {
VehicleRole StockVehicleRole(const char* name) noexcept {
 struct Entry{const char* name;VehicleRole role;};static const Entry entries[]={
#include "VehicleProfiles.inl"
 };
 if(name)for(const auto& e:entries)if(!std::strcmp(name,e.name))return e.role;
 return VehicleRole::Unknown;
}
std::optional<stereo::Matrix4> VehicleView::Update(const VehicleSample& s,const stereo::Matrix4& native) noexcept {
 const auto inverse=InverseRigid(s.rootWorld);
 if(!s.owner||!s.seat||!s.root||s.role==VehicleRole::Unknown||!inverse||!InverseRigid(native)){Reset();return {};}
 const float hullYaw=std::atan2(s.rootWorld.values[2][0],s.rootWorld.values[2][2]);
 if(owner!=s.owner||seat!=s.seat||root!=s.root){
  owner=s.owner;seat=s.seat;root=s.root;
  localCamera=Multiply(native,*inverse);
  yawOffset=std::remainder(std::atan2(native.values[2][0],native.values[2][2])-hullYaw,6.283185307f);
 }
 // Anchor to the chassis, not the moving turret/camera. Head motion will be
 // applied once by the common stereo path; turret-follow cannot rotate it again.
 auto world=Multiply(localCamera,s.rootWorld);
 return MakeComfortCamera(world,(hullYaw+yawOffset)*57.295779513f);
}
void VehicleControls::Update(const VehicleSample& s,const shared::SharedControllerSample& input,
 const stereo::Matrix4* native,const stereo::Matrix4* target,ControllerCommand& c) noexcept {
 const auto now=input.predictedDisplayTime;
 if(!s.owner||!s.root||!s.seat||s.role==VehicleRole::Unknown||!(input.flags&shared::kControllerSampleFlagSessionFocused)||now<=0){Reset();return;}
 const auto& left=input.hands[0];
 const bool modifier=(left.flags&shared::kControllerHandFlagTriggerActive)&&std::isfinite(left.triggerValue)&&left.triggerValue>.65f;
 const bool driver=modifier&&(left.buttons&shared::kControllerHandButtonPrimary),next=modifier&&(left.buttons&shared::kControllerHandButtonSecondary);
 // Native vehicle axes are vehicle-relative, never rotated by walking direction.
 if(left.flags&shared::kControllerHandFlagThumbstickActive){
  c.keys[0x11]=std::isfinite(left.thumbstickY)&&left.thumbstickY>.25f?0x80:0;
  c.keys[0x1f]=std::isfinite(left.thumbstickY)&&left.thumbstickY<-.25f?0x80:0;
  c.keys[0x20]=std::isfinite(left.thumbstickX)&&left.thumbstickX>.25f?0x80:0;
  c.keys[0x1e]=std::isfinite(left.thumbstickX)&&left.thumbstickX<-.25f?0x80:0;
 }
 c.keys[0x1d]=0;c.snapDegrees=0;c.wheel=0; // Infantry crouch must not enable native vehicle free-look.
 if(modifier)c.keys[0x13]=c.keys[0x12]=0;
 if(owner!=s.owner||root!=s.root||now<=last||now-last>250000000){
  Reset();owner=s.owner;root=s.root;seat=s.seat;last=now;driverHeld=driver;nextHeld=next;return;
 }
 if(driver&&!driverHeld){c.keys[0x3b]=0x80;nextSeat=2;}
 if(next&&!nextHeld){c.keys[0x3a+nextSeat]=0x80;nextSeat=nextSeat==8?2:nextSeat+1;}
 driverHeld=driver;nextHeld=next;
 const float dt=std::min(float(now-last)*1.e-9f,.05f);last=now;
 if(seat!=s.seat){seat=s.seat;remainderX=remainderY=0;return;}
 if((s.role!=VehicleRole::Aimed&&s.role!=VehicleRole::PitchAimed)||!native||!target||!InverseRigid(*native)||!InverseRigid(*target))return;
 const auto& a=native->values[2];const auto& b=target->values[2];
 const float yawError=std::remainder(std::atan2(b[0],b[2])-std::atan2(a[0],a[2]),6.283185307f);
 const float pitchError=std::atan2(b[1],std::hypot(b[0],b[2]))-std::atan2(a[1],std::hypot(a[0],a[2]));
 const auto step=[dt](float error,float& remainder){
  // Small deadband avoids endlessly dithering a settled turret. No integration
  // windup at traverse limits; the engine retains speed/angle/weapon rules.
  const float speed=std::abs(error)<.003f?0.f:std::clamp(error*4000.f,-900.f,900.f);
  const float value=speed*dt+remainder;const LONG whole=static_cast<LONG>(value);remainder=value-float(whole);return whole;
 };
 if(s.role==VehicleRole::Aimed)c.mouseX+=step(yawError,remainderX);c.mouseY+=step(-pitchError,remainderY);
}
}
