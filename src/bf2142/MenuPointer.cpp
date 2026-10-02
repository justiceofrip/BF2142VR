#include "MenuPointer.h"
#include "NativeMenuState.h"
#include "NativeHudState.h"
#include "NativeHudPointer.h"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <algorithm>
#include <cmath>
namespace bfvr::bf2142 {
namespace {
stereo::Pose Pose(const shared::SharedPresentationPose& p) {
    return {{p.positionX,p.positionY,p.positionZ},{p.orientationX,p.orientationY,p.orientationZ,p.orientationW}};
}
bool Focused(HWND window) {
    DWORD pid=0;GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    DWORD owner=0;GetWindowThreadProcessId(window,&owner);
    return window && pid==GetCurrentProcessId() && owner==pid;
}
stereo::Vec3 Transform(const stereo::Pose& pose,stereo::Vec3 p) {
    const auto& q=pose.orientation;
    const stereo::Vec3 t{2*(q.y*p.z-q.z*p.y),2*(q.z*p.x-q.x*p.z),2*(q.x*p.y-q.y*p.x)};
    return {pose.position.x+p.x+q.w*t.x+q.y*t.z-q.z*t.y,
        pose.position.y+p.y+q.w*t.y+q.z*t.x-q.x*t.z,
        pose.position.z+p.z+q.w*t.z+q.x*t.y-q.y*t.x};
}
DWORD Color(DWORD bgra,DXGI_FORMAT format) {
    if(format==DXGI_FORMAT_R8G8B8A8_UNORM || format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB)
        return (bgra&0xff00ff00)|((bgra&255)<<16)|((bgra>>16)&255);
    return bgra;
}
void Disc(std::vector<DWORD>& p,UINT width,UINT height,int x,int y,int radius,DWORD color) {
    for(int j=std::max(0,y-radius);j<std::min(int(height),y+radius+1);++j)
        for(int i=std::max(0,x-radius);i<std::min(int(width),x+radius+1);++i)
            if((i-x)*(i-x)+(j-y)*(j-y)<=radius*radius)p[size_t(j)*width+i]=color;
}
void Beam(std::vector<DWORD>& pixels,UINT width,UINT height,const stereo::Pose& anchor,
    const MenuRayHit& ray,const shared::SharedPresentationView& eye,DWORD color) {
    const auto line=ProjectMenuBeam(width,height,anchor,ray,eye);if(!line)return;
    const float dx=line->x1-line->x0,dy=line->y1-line->y0;
    const int steps=std::max(1,int(std::max(std::abs(dx),std::abs(dy))));
    for(int i=0;i<=steps;++i){const float t=float(i)/steps;Disc(pixels,width,height,int(line->x0+t*dx),int(line->y0+t*dy),1,color);}
}
}
std::optional<MenuBeam> ProjectMenuBeam(UINT width,UINT height,const stereo::Pose& anchor,
    const MenuRayHit& ray,const shared::SharedPresentationView& eye) {
    if(!width||!height||width>8192||height>8192)return {};
    stereo::Pose a{Transform(anchor,ray.origin),{}},b{Transform(anchor,ray.end),{}};
    const auto av=stereo::MakeRelativePose(Pose(eye.pose),a),bv=stereo::MakeRelativePose(Pose(eye.pose),b);
    if(!av || !bv)return {};
    auto p=av->position,q=bv->position;
    constexpr float nearZ=-.03f;
    if(p.z>nearZ && q.z>nearZ)return {};
    const auto clip=[](stereo::Vec3& start,const stereo::Vec3& end) {
        if(start.z>nearZ){const float t=(nearZ-start.z)/(end.z-start.z);start={start.x+t*(end.x-start.x),start.y+t*(end.y-start.y),nearZ};}
    };
    clip(p,q);clip(q,p);
    const float l=std::tan(eye.fov.angleLeft),r=std::tan(eye.fov.angleRight),u=std::tan(eye.fov.angleUp),d=std::tan(eye.fov.angleDown);
    if(!(r>l && u>d))return {};
    float x0=(p.x/-p.z-l)/(r-l)*width,y0=(u-p.y/-p.z)/(u-d)*height;
    const float x1=(q.x/-q.z-l)/(r-l)*width,y1=(u-q.y/-q.z)/(u-d)*height;
    if(!std::isfinite(x0)||!std::isfinite(y0)||!std::isfinite(x1)||!std::isfinite(y1))return {};
    const float dx=x1-x0,dy=y1-y0;
    float lo=0,hi=1;
    const auto bound=[&](float p,float q) {if(std::abs(p)<1e-8f)return q>=0;const float t=q/p;if(p<0)lo=std::max(lo,t);else hi=std::min(hi,t);return lo<=hi;};
    if(!bound(-dx,x0)||!bound(dx,float(width-1)-x0)||!bound(-dy,y0)||!bound(dy,float(height-1)-y0))return {};
    return MenuBeam{x0+lo*dx,y0+lo*dy,x0+hi*dx,y0+hi*dy};
}
std::optional<MenuRayHit> MenuRayTarget(const shared::SharedControllerHandSample& hand,
    const stereo::Pose& anchor,UINT width,UINT height,UINT uiWidth,UINT uiHeight,bool widescreen) {
    constexpr DWORD required=shared::kControllerHandFlagAimActive|shared::kControllerHandFlagAimPositionValid|
        shared::kControllerHandFlagAimOrientationValid|shared::kControllerHandFlagAimPositionTracked|shared::kControllerHandFlagAimOrientationTracked;
    if((hand.flags&required)!=required || !width || !height || !uiWidth || !uiHeight)return {};
    const auto aim=stereo::MakeRelativePose(anchor,Pose(hand.aimPose));if(!aim)return {};
    const float h=menuWidthMeters*float(uiHeight)/uiWidth;
    const stereo::Pose quad{{0,0,-menuDistanceMeters},{}};
    const auto point=stereo::MapOpenXRAimPoseToAspectFitUiCanvas(*aim,quad,menuWidthMeters,h,uiWidth,uiHeight,widescreen?16:width,widescreen?9:height,width,height);
    if(!point)return {};
    // The canvas may be letterboxed inside the square runtime UI texture.
    const float contentWidth=std::min(menuWidthMeters,h*(widescreen?16.f/9:float(width)/height));
    const float contentHeight=std::min(h,menuWidthMeters*(widescreen?9.f/16:float(height)/width));
    return MenuRayHit{*point,aim->position,{(point->normalizedX-.5f)*contentWidth,(.5f-point->normalizedY)*contentHeight,-menuDistanceMeters}};
}
bool MenuClickState::Update(bool enabled,bool hit,bool trigger) noexcept {
    if(!enabled){*this={};return false;}
    if(!active){active=true;armed=!trigger;held=trigger;return false;}
    if(!trigger)armed=true;
    const bool fire=armed && hit && trigger && !held;held=trigger;return fire;
}
void MenuPointer::Connect(IDirect3DDevice9* device) {
    D3DDEVICE_CREATION_PARAMETERS p{};if(device && SUCCEEDED(device->GetCreationParameters(&p)))window=p.hFocusWindow;
}
bool MenuPointer::MenuVisible() const {
    // Deployment/class selection can render while the world keeps updating.
    // Its native frontend is authoritative even with a hidden/null OS cursor.
    if(!Focused(window))return false;
    if(NativeInGameMenuActive() || NativeHudMenuActive())return true;
    CURSORINFO info{sizeof(info)};
    return GetCursorInfo(&info) && (info.flags&CURSOR_SHOWING) && info.hCursor;
}
void MenuPointer::Reset(){PublishNativeHudPointer(false);input.Release(window);menu=false;visible=false;pressed=false;buttonHeld=false;ray.reset();click={};}
void MenuPointer::ExpireInput(){
    if(!Focused(window) || input.Expired(GetTickCount64())){input.Release(window);PublishNativeHudPointer(false);click={};buttonHeld=false;pressed=false;}
}
void MenuPointer::Update(bool showMenu,const shared::SharedControllerSample* sample,
    const shared::SharedRenderRequest& request,UINT w,UINT h,UINT uiW,UINT uiH,ControllerCommand& command,VrControlsMenu* controls,VrSettings* settings,bool widescreen,bool desktopMouse) {
    width=w;height=h;visible=false;ray.reset();pressed=false;
    PublishNativeHudPointer(false);
    if(!showMenu){Reset();input.ObserveGameplayEscape(command.keys[DIK_ESCAPE]!=0);return;}
    if(!menu){const auto pose=stereo::MakeYawOnlyUiAnchor(Pose(request.headPose));if(!pose)return;anchor=*pose;click={};}
    menu=true;
    const bool focused=Focused(window);
    if(!focused){input.Release(window);click={};buttonHeld=false;return;}
    const auto& hand=sample?sample->hands[1]:shared::SharedControllerHandSample{};
    const bool controller=sample && (sample->flags&shared::kControllerSampleFlagSessionFocused);
    if(controller&&!desktopMouse)ray=MenuRayTarget(hand,anchor,width,height,uiW,uiH,widescreen);
    if(ray){point=ray->canvas;visible=true;}
    else {
        POINT p{};RECT client{};
        if(GetCursorPos(&p) && ScreenToClient(window,&p) && GetClientRect(window,&client) && client.right>0 && client.bottom>0 && PtInRect(&client,p)) {
            point.normalizedX=float(p.x)/client.right;point.normalizedY=float(p.y)/client.bottom;
            point.pixelX=point.normalizedX*width;point.pixelY=point.normalizedY*height;visible=true;
        }
    }
    if(controller) {
        const bool trigger=command.buttons[0]!=0 || (hand.buttons&shared::kControllerHandButtonPrimary)!=0;
        pressed=trigger;
        // The native window callback (exe RVA 0x1570) dispatches mouse move,
        // left-down and left-up to the Flash UI. DInput-only clicks never reach
        // this listener. Use this foreground game's queue, not global input.
        command.mouseX=command.mouseY=0;command.buttons={};command.keys[DIK_RETURN]=0;
        const bool escape=command.keys[DIK_ESCAPE]!=0;command.keys[DIK_ESCAPE]=0;
        bool sent=false;
        // Desktop simulation derives its ray from the OS mouse. Feeding the
        // projected ray back through SetCursorPos creates a cursor feedback
        // loop. Keep physical mouse input native; F9 can still test VR clicks.
        const bool hit=desktopMouse?visible:ray.has_value();
        const bool fire=click.Update(true,hit,trigger);
        buttonHeld=hit && trigger && (buttonHeld || fire);
        if(controls && settings && controls->Interact(visible?point.normalizedX:-1.f,visible?point.normalizedY:-1.f,fire,escape,*settings,request.headPose.positionY)){
            input.Release(window);buttonHeld=false;pressed=trigger;return;
        }
        if(hit) {
            RECT client{};
            if(GetClientRect(window,&client) && client.right>0 && client.bottom>0) {
                POINT pixel{std::clamp(LONG(std::lround(point.normalizedX*client.right)),0L,client.right-1),
                    std::clamp(LONG(std::lround(point.normalizedY*client.bottom)),0L,client.bottom-1)};
                POINT screen=pixel;
                if((desktopMouse||ClientToScreen(window,&screen)) && Focused(window)) {
                    if(!desktopMouse)SetCursorPos(screen.x,screen.y);
                    if(NativeHudMenuActive()){
                        input.Update(window,true,nullptr,false,false);
                        PublishNativeHudPointer(!desktopMouse||buttonHeld,point.normalizedX,point.normalizedY,buttonHeld);
                        // Deployment is driven by the game's ordinary Escape action.
                        command.keys[DIK_ESCAPE]=escape?0x80:0;
                    }else input.Update(window,true,desktopMouse&&!buttonHeld?nullptr:&pixel,buttonHeld,escape);
                    sent=true;
                } else buttonHeld=false;
            }
        }
        if(!sent)input.Update(window,focused,nullptr,false,escape);
        pressed=buttonHeld;
    } else {input.Release(window);click={};buttonHeld=false;}
}
void MenuPointer::Draw(std::vector<DWORD>& left,std::vector<DWORD>& right,std::vector<DWORD>& ui,
    DXGI_FORMAT format,const shared::SharedRenderRequest& request) const {
    if(menu && visible)DrawMenuPointer(left,right,ui,width,height,format,request,anchor,point,ray,pressed);
}
void DrawMenuPointer(std::vector<DWORD>& left,std::vector<DWORD>& right,std::vector<DWORD>& ui,
    UINT width,UINT height,DXGI_FORMAT format,const shared::SharedRenderRequest& request,
    const stereo::Pose& anchor,const stereo::UiCanvasPoint& point,const std::optional<MenuRayHit>& ray,bool pressed) {
    if(!width || !height || width>8192 || height>8192 || ui.size()!=size_t(width)*height ||
        !std::isfinite(point.pixelX) || !std::isfinite(point.pixelY) || point.pixelX<0 || point.pixelY<0 ||
        point.pixelX>width || point.pixelY>height)return;
    const DWORD cyan=Color(pressed?0xffffb830:0xff20efff,format);
    if(ray && left.size()==ui.size() && right.size()==ui.size()) {
        Beam(left,width,height,anchor,*ray,request.views[0],cyan);
        Beam(right,width,height,anchor,*ray,request.views[1],cyan);
    }
    const int radius=std::max(4,int(width/180)),x=int(point.pixelX),y=int(point.pixelY);
    Disc(ui,width,height,x,y,radius+2,0xff111111);
    Disc(ui,width,height,x,y,radius,cyan);
    Disc(ui,width,height,x,y,std::max(1,radius/3),0xffffffff);
}
}

