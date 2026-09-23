#include "BodyEquipment.h"
#include "BodyEquipmentPlacement.h"
#include "WeaponFaceFade.h"
#include <fstream>
#include <filesystem>
#include <cmath>
#include <limits>
#include <algorithm>
namespace bfvr::bf2142 {
namespace {
stereo::Pose Pose(const shared::SharedPresentationPose& p){return {{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}};}
stereo::Vec3 Transform(const stereo::Pose& p,stereo::Vec3 v){const auto q=p.orientation;const stereo::Vec3 t{2*(q.y*v.z-q.z*v.y),2*(q.z*v.x-q.x*v.z),2*(q.x*v.y-q.y*v.x)};
    return {p.position.x+v.x+q.w*t.x+q.y*t.z-q.z*t.y,p.position.y+v.y+q.w*t.y+q.z*t.x-q.x*t.z,p.position.z+v.z+q.w*t.z+q.x*t.y-q.y*t.x};}
struct Screen {float x,y,z,u,v;};
}
bool BodyEquipment::Load(const std::wstring& path) noexcept {
    models.clear();try {
        if(path.empty() || std::filesystem::file_size(path)>32*1024*1024)return false;
        std::ifstream f(std::filesystem::path(path),std::ios::binary);char magic[8]{};f.read(magic,8);
        if(std::string_view(magic,8)!="BFHP0001")return false;
        const auto word=[&](){unsigned n=0;f.read(reinterpret_cast<char*>(&n),4);if(!f)throw 1;return n;};
        const unsigned count=word();if(!count||count>128)return false;
        std::vector<Model> result;
        for(unsigned i=0;i<count;++i){
            Model model;const auto n=word();if(!n||n>48)return false;model.name.resize(n);f.read(model.name.data(),n);
            for(char c:model.name)if(!((c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'))return false;
            const auto nm=word();if(!nm||nm>16)return false;
            stereo::Vec3 low{1.e9f,1.e9f,1.e9f},high{-1.e9f,-1.e9f,-1.e9f};
            for(unsigned j=0;j<nm;++j){
                Material m;const auto nv=word(),ni=word();m.width=word();m.height=word();
                if(!nv||nv>65535||!ni||ni>60000||ni%3||m.width!=128||m.height!=128)return false;
                m.vertices.resize(nv);m.indices.resize(ni);m.texture.resize(size_t(m.width)*m.height);
                f.read(reinterpret_cast<char*>(m.vertices.data()),nv*sizeof(Vertex));f.read(reinterpret_cast<char*>(m.indices.data()),ni*2);
                f.read(reinterpret_cast<char*>(m.texture.data()),m.texture.size()*4);if(!f)return false;
                for(const auto& v:m.vertices){
                    if(!std::isfinite(v.x)||!std::isfinite(v.y)||!std::isfinite(v.z)||!std::isfinite(v.u)||!std::isfinite(v.v)||
                       std::abs(v.x)>2||std::abs(v.y)>2||std::abs(v.z)>2||std::abs(v.u)>64||std::abs(v.v)>64)return false;
                    low={std::min(low.x,v.x),std::min(low.y,v.y),std::min(low.z,v.z)};
                    high={std::max(high.x,v.x),std::max(high.y,v.y),std::max(high.z,v.z)};
                }
                for(auto index:m.indices)if(index>=nv)return false;
                model.materials.push_back(std::move(m));
            }
            model.low=low;model.high=high;result.push_back(std::move(model));
        }
        if(f.peek()!=std::char_traits<char>::eof())return false;
        models=std::move(result);return true;
    }catch(...){models.clear();return false;}
}
bool BodyEquipment::Draw(std::vector<DWORD>& left,std::vector<DWORD>& right,UINT w,UINT h,DXGI_FORMAT format,
    const shared::SharedRenderRequest& request,const BodyInventoryResult& body,const InventoryNames& names,int equipped){
    if(!body.anchorValid||models.empty()||!w||!h||w>4096||h>4096||left.size()!=size_t(w)*h||right.size()!=left.size())return false;
    // Holster props use their stock distant LOD. Bound raster work independently
    // of headset resolution; keep aiming, hands and world at full resolution.
    const UINT rw=std::min(640u,std::max(1u,w/2)),rh=std::max(1u,UINT(uint64_t(h)*rw/w));
    for(auto& layer:layers)layer.assign(size_t(rw)*rh,0);
    if(!Raster(layers[0],layers[1],rw,rh,format,request,body,names,equipped))return false;
    for(unsigned eye=0;eye<2;++eye){auto& target=eye?right:left;const auto& source=layers[eye];
        for(UINT y=0;y<h;++y){const size_t sy=size_t(y)*rh/h;
            for(UINT x=0;x<w;++x){const DWORD color=source[sy*rw+size_t(x)*rw/w];if(color)target[size_t(y)*w+x]=color;}}
    }
    return true;
}
bool BodyEquipment::Raster(std::vector<DWORD>& left,std::vector<DWORD>& right,UINT w,UINT h,DXGI_FORMAT format,
    const shared::SharedRenderRequest& request,const BodyInventoryResult& body,const InventoryNames& names,int equipped){
    if(!body.anchorValid||models.empty()||!w||!h||w>4096||h>4096||left.size()!=size_t(w)*h||right.size()!=left.size())return false;
    bool painted=false;
    const bool rgba=format==DXGI_FORMAT_R8G8B8A8_UNORM||format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    for(unsigned eye=0;eye<2;++eye){
        auto& pixels=eye?right:left;depth.assign(pixels.size(),std::numeric_limits<float>::max());
        const auto& view=request.views[eye];const float l=std::tan(view.fov.angleLeft),r=std::tan(view.fov.angleRight),u=std::tan(view.fov.angleUp),d=std::tan(view.fov.angleDown);
        if(!(r>l&&u>d))continue;
        const auto anchor=stereo::MakeRelativePose(Pose(view.pose),body.anchor);if(!anchor)continue;
        for(unsigned slotIndex=0;slotIndex<BodySlots().size();++slotIndex){
            const auto& slot=BodySlots()[slotIndex];if(int(slot.item)==equipped||!names[slot.item][0])continue;
            const auto end=std::find(names[slot.item].begin(),names[slot.item].end(),char(0));
            const std::string name(names[slot.item].begin(),end);
            const Model* model=nullptr;for(const auto& m:models)if(m.name==name){model=&m;break;}
            if(!model)for(const auto& m:models)if(m.name==name+"_rifle"){model=&m;break;}
            if(!model)continue;
            const bool hover=int(slotIndex)==body.hovered;
            const auto placement=PlaceBodyEquipment(slotIndex,model->low,model->high);
            if(placement.scale<=0)continue;
            for(const auto& material:model->materials){
                std::vector<Screen> projected;projected.reserve(material.vertices.size());
                for(auto vertex:material.vertices){
                    const auto p=Transform(*anchor,placement.Transform({vertex.x,vertex.y,vertex.z}));
                    if(p.z>=-.035f){projected.push_back({0,0,-1,0,0});continue;}
                    const float z=-p.z;projected.push_back({(p.x/z-l)/(r-l)*w,(u-p.y/z)/(u-d)*h,z,vertex.u,vertex.v});
                }
                for(size_t t=0;t<material.indices.size();t+=3){
                    const auto a=projected[material.indices[t]],b=projected[material.indices[t+1]],c=projected[material.indices[t+2]];
                    if(a.z<=0||b.z<=0||c.z<=0)continue;
                    const float den=(b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y);if(!std::isfinite(den)||std::abs(den)<.1f)continue;
                    const float minX=std::max(0.f,std::min({a.x,b.x,c.x})),maxX=std::min(float(w-1),std::max({a.x,b.x,c.x}));
                    const float minY=std::max(0.f,std::min({a.y,b.y,c.y})),maxY=std::min(float(h-1),std::max({a.y,b.y,c.y}));
                    if(!(maxX>=minX && maxY>=minY))continue;
                    for(int y=int(minY);y<=int(maxY);++y)for(int x=int(minX);x<=int(maxX);++x){
                        const float wa=((b.y-c.y)*(x+.5f-c.x)+(c.x-b.x)*(y+.5f-c.y))/den;
                        const float wb=((c.y-a.y)*(x+.5f-c.x)+(a.x-c.x)*(y+.5f-c.y))/den,wc=1-wa-wb;
                        if(std::min({wa,wb,wc})<0)continue;
                        const float iz=wa/a.z+wb/b.z+wc/c.z,z=1/iz;const auto at=size_t(y)*w+x;if(z>=depth[at])continue;
                        const float tx=(wa*a.u/a.z+wb*b.u/b.z+wc*c.u/c.z)*z,ty=(wa*a.v/a.z+wb*b.v/b.z+wc*c.v/c.z)*z;
                        const unsigned xx=std::min(material.width-1,unsigned((tx-std::floor(tx))*material.width)),yy=std::min(material.height-1,unsigned((ty-std::floor(ty))*material.height));
                        DWORD color=material.texture[size_t(yy)*material.width+xx]|0xff000000;
                        if(hover)color=0xff000000|((((color>>16)&255)/2+30)<<16)|((((color>>8)&255)/2+90)<<8)|((color&255)/2+110);
                        if(rgba)color=(color&0xff00ff00)|((color&255)<<16)|((color>>16)&255);
                        pixels[at]=color;depth[at]=z;painted=true;
                    }
                }
            }
        }
    }
    return painted;
}
}

namespace bfvr::bf2142 {
float BodyEquipment::FaceOpacity(std::string_view name,const stereo::Matrix4& gun,const std::array<stereo::Matrix4,2>& eyes) const {
    const Model* model=nullptr;for(const auto& m:models)if(m.name==name){model=&m;break;}
    const auto inverse=InverseRigid(gun);if(!model||!inverse)return 1;
    float opacity=1;
    for(const auto& eye:eyes){
        if(!InverseRigid(eye))return 1;const auto local=Multiply(eye,*inverse);const auto& r=local.values[3];const stereo::Vec3 point{r[0],r[1],r[2]};
        const auto& lo=model->low;const auto& hi=model->high;
        if(point.x<lo.x-.07f||point.x>hi.x+.07f||point.y<lo.y-.07f||point.y>hi.y+.07f||point.z<lo.z-.07f||point.z>hi.z+.07f)continue;
        float distance2=1.e9f;std::vector<float> hits;
        for(const auto& material:model->materials)for(size_t i=0;i<material.indices.size();i+=3){
            const auto get=[&](size_t j){const auto& v=material.vertices[material.indices[j]];return stereo::Vec3{v.x,v.y,v.z};};
            const auto a=get(i),b=get(i+1),c=get(i+2);
            distance2=std::min(distance2,face::TriangleDistance2(point,a,b,c));
            const float t=face::RayHit(point,a,b,c);if(t>0)hits.push_back(t);
        }
        std::sort(hits.begin(),hits.end());unsigned crossings=0;float previous=-1;
        for(float hit:hits)if(hit-previous>.0001f){++crossings;previous=hit;}
        opacity=std::min(opacity,face::Opacity(std::sqrt(distance2),(crossings&1)!=0));
    }
    return opacity;
}
}
