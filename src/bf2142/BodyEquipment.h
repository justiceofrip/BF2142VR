#pragma once
#include "BodyInventory.h"
#include <string>
#include <string_view>
#include <vector>
#include <dxgiformat.h>
namespace bfvr::bf2142 {
using InventoryNames=std::array<std::array<char,49>,10>;
class BodyEquipment {
public:
    bool Load(const std::wstring& file) noexcept;
    bool Draw(std::vector<DWORD>& left,std::vector<DWORD>& right,UINT width,UINT height,DXGI_FORMAT,
        const shared::SharedRenderRequest&,const BodyInventoryResult&,const InventoryNames&,int equipped);
    float FaceOpacity(std::string_view name,const stereo::Matrix4& gun,const std::array<stereo::Matrix4,2>& eyes) const;
    size_t ModelCount() const {return models.size();}
private:
    struct Vertex {float x,y,z,u,v;};
    struct Material {std::vector<Vertex> vertices;std::vector<unsigned short> indices;std::vector<DWORD> texture;UINT width=0,height=0;};
    struct Model {std::string name;std::vector<Material> materials;stereo::Vec3 low{},high{};};
    bool Raster(std::vector<DWORD>& left,std::vector<DWORD>& right,UINT width,UINT height,DXGI_FORMAT,
        const shared::SharedRenderRequest&,const BodyInventoryResult&,const InventoryNames&,int equipped);
    std::array<std::vector<DWORD>,2> layers;
    std::vector<Model> models;
    std::vector<float> depth;
};
}
