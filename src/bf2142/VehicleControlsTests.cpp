#include "VehicleControls.h"
#include "ComfortCamera.h"
#include "TrackingMath.h"
#include <cstdio>
#include <cmath>
#include <limits>
using namespace bfvr;using namespace bfvr::bf2142;using M=stereo::Matrix4;
#define CHECK(x) do{if(!(x)){printf("Vehicle failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
M At(float x=0,float y=0,float z=0){M m{};for(int i=0;i<4;++i)m.values[i][i]=1;m.values[3]={x,y,z,1};return m;}
bool Near(float a,float b){return std::abs(a-b)<.0002f;}
int main(){
 CHECK(StockVehicleRole("eu_fav")==VehicleRole::Driver);CHECK(StockVehicleRole("eu_fav_secondposition")==VehicleRole::Aimed);
 CHECK(StockVehicleRole("us_heavy_mech")==VehicleRole::Aimed);CHECK(StockVehicleRole("us_ag")==VehicleRole::Pilot);
 CHECK(StockVehicleRole("as_tank")==VehicleRole::PitchAimed);CHECK(StockVehicleRole("unknown")==VehicleRole::Unknown);
 VehicleSample s{1,2,3,VehicleRole::Aimed,At()};VehicleView view;M native=At(0,2,1);
 auto stable=view.Update(s,native);CHECK(stable&&Near(stable->values[3][1],2));
 // Turret turns cannot become a second headset turn. Chassis translation and
 // intentional vehicle yaw are retained; chassis banking stays out of the view.
 native=*MakeComfortCamera(native,45);stable=view.Update(s,native);CHECK(stable&&Near(stable->values[2][0],0));
 s.rootWorld=*MakeComfortCamera(At(5,3,7),90);stable=view.Update(s,native);CHECK(stable&&Near(stable->values[2][0],1)&&Near(stable->values[3][0],6));
 s.seat++;stable=view.Update(s,native);CHECK(stable&&Near(stable->values[2][0],native.values[2][0]));
 s={1,2,3,VehicleRole::Aimed,At()};VehicleControls policy;shared::SharedControllerSample input{};input.flags=shared::kControllerSampleFlagSessionFocused;input.predictedDisplayTime=1000000000;
 input.hands[0].flags=shared::kControllerHandFlagTriggerActive|shared::kControllerHandFlagThumbstickActive;input.hands[0].thumbstickY=1;
 M base=At(),target=*MakeComfortCamera(base,30);ControllerCommand c;
 policy.Update(s,input,&base,&target,c);CHECK(c.mouseX==0);
 input.predictedDisplayTime+=20000000;c={};c.keys[0x1d]=0x80;policy.Update(s,input,&base,&target,c);CHECK(c.mouseX>0&&c.mouseX<=18&&!c.keys[0x1d]&&c.keys[0x11]);
 // Frame-to-frame closed-loop correction decays once the turret reaches gaze.
 base=target;input.predictedDisplayTime+=20000000;c={};policy.Update(s,input,&base,&target,c);CHECK(c.mouseX==0);
 input.hands[0].triggerValue=1;input.hands[0].buttons=shared::kControllerHandButtonPrimary;
 input.predictedDisplayTime+=20000000;c={};policy.Update(s,input,&base,&target,c);CHECK(c.keys[0x3b]&&!c.wheel);
 input.predictedDisplayTime+=20000000;c={};policy.Update(s,input,&base,&target,c);CHECK(!c.keys[0x3b]);
 for(int seat=2;seat<=8;++seat){input.hands[0].buttons=0;input.predictedDisplayTime+=20000000;c={};policy.Update(s,input,&base,&target,c);
  input.hands[0].buttons=shared::kControllerHandButtonSecondary;input.predictedDisplayTime+=20000000;c={};policy.Update(s,input,&base,&target,c);CHECK(c.keys[0x3a+seat]&&!c.keys[0x12]);}
 s.role=VehicleRole::Pilot;base=At();input.predictedDisplayTime+=20000000;c={};c.mouseX=4;c.mouseY=7;policy.Update(s,input,&base,&target,c);CHECK(c.mouseX==4&&c.mouseY==7);
 input.flags=0;input.predictedDisplayTime+=20000000;c={};policy.Update(s,input,&base,&target,c);CHECK(!c.keys[0x3b]&&!c.mouseX);
 input.flags=shared::kControllerSampleFlagSessionFocused;s.role=VehicleRole::Aimed;input.predictedDisplayTime+=20000000;c={};policy.Update(s,input,&base,&target,c);CHECK(!c.mouseX); // focus return primes safely
 target.values[0][0]=std::numeric_limits<float>::quiet_NaN();input.predictedDisplayTime+=20000000;c={};policy.Update(s,input,&base,&target,c);CHECK(!c.mouseX);
 puts("Vehicle chassis/head separation, aiming bounds, aircraft guard, freshness and seat edges passed.");return 0;
}
