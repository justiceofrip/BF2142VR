#include "StereoCamera.h"
#include <cmath>
#include <cstdio>
#include <limits>
using namespace bfvr;
bool near(float a, float b) { return std::abs(a-b) < 0.0001f; }
int main() {
    bf2142::CameraInput source{};
    for (int i=0; i<4; ++i) source.world.values[i][i]=1;
    source.world.values[3][0]=100; source.world.values[3][1]=2;
    source.nearPlane=.041f; source.farDelta=330;
    stereo::Pose neutral{}, left{}, right{};
    neutral.position.y=left.position.y=right.position.y=1.7f;
    left.position.x=-.032f; right.position.x=.032f;
    stereo::FovTangents fov{-1.1f,.9f,1.0f,-.8f};
    auto l=bf2142::MakeEyeCamera(source,neutral,left,fov);
    auto r=bf2142::MakeEyeCamera(source,neutral,right,fov);
    if (!l || !r || !near(l->world.values[3][0],99.968f) ||
        !near(r->world.values[3][0],100.032f) || !near(l->world.values[3][1],2)) return 1;
    const auto n=stereo::TransformRowVector({0,0,.041f,1},l->projection);
    const auto f=stereo::TransformRowVector({0,0,330.041f,1},l->projection);
    if (!near(n.z/n.w,0) || !near(f.z/f.w,1)) return 2;
    // Physical forward motion converts XR -Z into game +Z, relative to neutral.
    left.position.z=-.25f;
    auto moved=bf2142::MakeEyeCamera(source,neutral,left,fov);
    if (!moved || !near(moved->world.values[3][2],.25f)) return 3;
    // A 90-degree game-camera turn rotates eye separation into world Z.
    source.world.values[0][0]=0; source.world.values[0][2]=1;
    source.world.values[2][0]=-1; source.world.values[2][2]=0;
    left.position.z=0;
    auto turned=bf2142::MakeEyeCamera(source,neutral,left,fov);
    if (!turned || !near(turned->world.values[3][2],-.032f)) return 4;
    left.orientation.w=std::numeric_limits<float>::quiet_NaN();
    if (bf2142::MakeEyeCamera(source,neutral,left,fov)) return 5;
    left.orientation.w=1; source.farDelta=-1;
    if (bf2142::MakeEyeCamera(source,neutral,left,fov)) return 6;
    puts("BF2142 eye separation, neutral height, pose composition and native depth interval passed.");
    return 0;
}
