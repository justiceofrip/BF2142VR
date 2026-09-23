#include "LobbyScene.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdio>
#include <limits>
using namespace bfvr::bf2142;
#define CHECK(x) do{if(!(x)){printf("Lobby asset failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 wchar_t temp[MAX_PATH]{},name[MAX_PATH]{};CHECK(GetTempPathW(MAX_PATH,temp));CHECK(GetTempFileNameW(temp,L"bfl",0,name));
 struct Cleanup{const wchar_t* name;~Cleanup(){DeleteFileW(name);}} cleanup{name};
 std::vector<char> data;auto append=[&](const auto& value){const auto* first=reinterpret_cast<const char*>(&value);data.insert(data.end(),first,first+sizeof(value));};
 for(char c:std::string("BFLS0001"))append(c);for(unsigned n:{1u,3u,3u,4u,4u})append(n);
 for(int i=0;i<3;++i)for(float n:{float(i==1),float(i==2),0.f,0.f,0.f,1.f,0.f,0.f})append(n);
 for(WORD n:{WORD(0),WORD(1),WORD(2)})append(n);for(int i=0;i<16;++i)append(DWORD(0xff112233));
 LobbyScene scene;auto load=[&](const std::vector<char>& bytes){std::ofstream f(std::filesystem::path(name),std::ios::binary|std::ios::trunc);f.write(bytes.data(),bytes.size());f.close();return scene.Load(name);};
 CHECK(load(data)&&scene.Ready());scene.ResetDevice();CHECK(scene.Ready());
 auto bad=data;bad[0]='X';CHECK(!load(bad)&&!scene.Ready());
 bad=data;bad.resize(bad.size()-1);CHECK(!load(bad));
 bad=data;bad.push_back(0);CHECK(!load(bad));
 for(unsigned offset:{8u,12u,16u,20u,24u}){bad=data;const unsigned huge=0xffffffff;std::memcpy(bad.data()+offset,&huge,4);CHECK(!load(bad));}
 bad=data;const float nan=std::numeric_limits<float>::quiet_NaN();std::memcpy(bad.data()+28,&nan,4);CHECK(!load(bad));
 bad=data;const float zero=0;std::memcpy(bad.data()+28+20,&zero,4);CHECK(!load(bad));
 bad=data;const WORD invalid=3;std::memcpy(bad.data()+28+96,&invalid,2);CHECK(!load(bad));
 CHECK(load(data)&&scene.Ready());CHECK(!scene.Load(L"")&&!scene.Ready());
 puts("Lobby pack format, count/allocation bounds, truncation, indices, finite normals, reload and device reset passed.");
}
