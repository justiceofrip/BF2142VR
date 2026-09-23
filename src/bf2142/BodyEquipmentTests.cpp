#include "BodyEquipment.h"
#include "BodyEquipmentPlacement.h"
#include "StereoDiagnostics.h"
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cmath>
#include <chrono>
using namespace bfvr;using namespace bfvr::bf2142;
int wmain(int argc,wchar_t** argv){
    std::filesystem::path path;
    bool fixture=argc==1;
    if(fixture){
        path=std::filesystem::temp_directory_path()/(L"bfvr-body-test-"+std::to_wstring(GetCurrentProcessId())+L".bin");
        std::ofstream f(path,std::ios::binary);f.write("BFHP0001",8);
        const auto word=[&](unsigned n){f.write(reinterpret_cast<char*>(&n),4);};
        word(1);word(5);f.write("knife",5);word(1);word(3);word(3);word(128);word(128);
        const float vertices[]={-.04f,-.01f,-.15f,0,0, .04f,-.01f,-.15f,1,0, 0,.01f,.15f,.5f,1};
        const unsigned short indices[]={0,1,2};f.write(reinterpret_cast<const char*>(vertices),sizeof(vertices));f.write(reinterpret_cast<const char*>(indices),sizeof(indices));
        std::vector<DWORD> pixels(128*128,0xfff06020);f.write(reinterpret_cast<char*>(pixels.data()),pixels.size()*4);
    }else path=argv[1];
    BodyEquipment equipment;if(!equipment.Load(path.wstring())||!equipment.ModelCount())return 1;
    InventoryNames names{};strcpy_s(names[1].data(),49,"knife");strcpy_s(names[2].data(),49,"eu_handgun");strcpy_s(names[3].data(),49,"eu_mg");strcpy_s(names[7].data(),49,"unl_grenade_frag");strcpy_s(names[4].data(),49,"unl_hub_ammo");
    shared::SharedRenderRequest request{};request.headPose.positionY=1.7f;
    for(int i=0;i<2;++i){auto& v=request.views[i];v.pose.positionY=1.7f;v.pose.positionX=i?.032f:-.032f;v.pose.orientationX=std::sin(-.58f);v.pose.orientationW=std::cos(-.58f);v.fov={-.72f,.72f,.55f,-.55f};}
    BodyInventoryResult body;body.anchor={{0,1.7f,0},{0,0,0,1}};body.hovered=1;body.anchorValid=true;
    std::vector<DWORD> left(960*720,0xff18202a),right=left;
    const auto begin=std::chrono::steady_clock::now();equipment.Draw(left,right,960,720,DXGI_FORMAT_B8G8R8A8_UNORM,request,body,names,3);
    const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    unsigned changed=0;for(auto p:left)changed+=p!=0xff18202a;
    if(changed<100||left==right)return 2;
    if(argc>2){SaveDiagnosticFrame(argv[2],960,720,left);}
    // Test actual placement properties independently of the rasterizer.
    const auto ammo=PlaceBodyEquipment(3,{-.31f,-.106f,-.138f},{.31f,.104f,.134f});
    const auto ammoA=ammo.Transform({-.31f,-.106f,-.138f}),ammoB=ammo.Transform({.31f,.104f,.134f});
    if(std::abs(ammoB.x-ammoA.x)>.191f||std::abs(ammoB.y-ammoA.y)>.231f||std::abs(ammoB.z-ammoA.z)>.231f)return 5;
    const auto rifle=PlaceBodyEquipment(0,{-.087f,-.176f,-.427f},{.087f,.099f,.467f});
    const auto stock=rifle.Transform({0,0,-.427f}),muzzle=rifle.Transform({0,0,.467f});
    if(muzzle.y>=stock.y || stock.y>BodySlots()[0].offset.y+.061f || std::abs(stock.z-BodySlots()[0].offset.z)>.15f)return 6;
    // Missing/invalid body pose must not draw models at the origin.
    auto invalidBody=body;invalidBody.anchorValid=false;const auto prior=left;
    if(equipment.Draw(left,right,960,720,DXGI_FORMAT_B8G8R8A8_UNORM,request,invalidBody,names,3)||left!=prior)return 7;
    if(fixture){
        names={};strcpy_s(names[1].data(),49,"knife");left.assign(left.size(),0xff18202a);right=left;
        equipment.Draw(left,right,960,720,DXGI_FORMAT_B8G8R8A8_UNORM,request,body,names,1);
        for(auto p:left)if(p!=0xff18202a)return 3;
        {std::ofstream f(path,std::ios::binary|std::ios::app);f.put('x');}
        if(equipment.Load(path.wstring())||equipment.ModelCount())return 4;
        std::filesystem::remove(path);
    }
    printf("Body visuals: stereo parallax, equipped-item hiding, bounded loading; %u painted pixels, %.2f ms.\n",changed,ms);return 0;
}
