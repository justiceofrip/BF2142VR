#pragma once
#include "../NativeHands.h"
#include "PoseProtocol.h"
namespace bfvr::bf2142 {
void InstallNetworkClient(LogFunction);
void InstallNetworkObserver(LogFunction);
void TickNetworkClient();
// Cross-thread cosmetic event drain; inactive/menu/focus loss discards contacts.
unsigned TakeNetworkFistBumps(bool active);
bool NetworkClientActive();
void PublishNetworkPose(void* soldier,void* weapon,const net::Matrix& body,const net::Matrix& camera,
 const net::Matrix& head,const net::Matrix& leftPalm,const net::Matrix& rightPalm,const net::Matrix& weaponLocal,bool leftValid,bool held,
 const std::array<std::array<float,5>,2>& curls,bool leftCrate=false);
void PublishNetworkSnap(void* soldier,float degrees);
void PublishNetworkCrateThrow(void* soldier,void* weapon,const net::Matrix& launch,stereo::Vec3 velocity);
void ApplyRemoteNetworkPose(void* soldier);
}
