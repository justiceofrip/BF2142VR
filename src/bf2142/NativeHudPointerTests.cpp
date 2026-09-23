#include <Windows.h>
#include <cstdio>
#include <array>
static bool focused=true;
static HWND TestForeground(){return reinterpret_cast<HWND>(1);}
static DWORD TestOwner(HWND,DWORD* pid){*pid=focused?GetCurrentProcessId():0;return 1;}
#define GetForegroundWindow TestForeground
#define GetWindowThreadProcessId TestOwner
#include "NativeHudPointer.cpp"
#undef GetForegroundWindow
#undef GetWindowThreadProcessId
using namespace bfvr::bf2142;
namespace {
HudTarget target{reinterpret_cast<void*>(1),reinterpret_cast<void*>(2),reinterpret_cast<void*>(3),800,600,-400,-300};
bool menu=true,live=true;int moves=0,updates=0,presses=0,releases=0;float lastX=0,lastY=0;bool args=true;
bool __fastcall MoveSpy(void* p,void*,void* root,float x,float y){++moves;lastX=x;lastY=y;args&=p==target.pointer&&root==target.root;return true;}
bool __fastcall UpdateSpy(void*,void*,float){++updates;return true;}
bool __fastcall DownSpy(void* root,void*,int key,int player,int count){++presses;args&=root==target.root&&key==1&&player==1&&count==1;return true;}
bool __fastcall UpSpy(void* root,void*,int key,int player){++releases;args&=root==target.root&&key==1&&player==1;return true;}
}
namespace bfvr::bf2142 {
bool NativeHudMenuActive(HudTarget* out){if(out)*out=target;return menu&&live;}
bool NativeHudTarget(HudTarget* out){if(out)*out=target;return live;}
}
int main(){
    nativeMove=reinterpret_cast<Move>(MoveSpy);nativeUpdate=reinterpret_cast<Update>(UpdateSpy);down=reinterpret_cast<Down>(DownSpy);up=reinterpret_cast<Up>(UpSpy);
    PublishNativeHudPointer(true,.25f,.75f,true);UpdateHook(target.root,nullptr,.016f);
    if(moves!=1||presses!=1||lastX!=-200||lastY!=150)return 1;
    UpdateHook(target.root,nullptr,0);if(moves!=1||presses!=1||updates!=2)return 2;
    MoveHook(target.pointer,nullptr,target.root,20,30);if(lastX!=-200||lastY!=150)return 3;
    PublishNativeHudPointer(true,.3f,.5f,false);UpdateHook(target.root,nullptr,.016f);if(releases!=1||lastY!=0)return 4;
    PublishNativeHudPointer(true,.3f,.5f,true);UpdateHook(target.root,nullptr,.016f);focused=false;UpdateHook(target.root,nullptr,.016f);
    if(releases!=2)return 5;focused=true;
    PublishNativeHudPointer(true,.3f,.5f,true);UpdateHook(target.root,nullptr,.016f);request.time=GetTickCount64()-151;UpdateHook(target.root,nullptr,.016f);if(releases!=3)return 6;
    PublishNativeHudPointer(true,.3f,.5f,true);UpdateHook(target.root,nullptr,.016f);menu=false;UpdateHook(target.root,nullptr,.016f);if(releases!=4)return 7;
    menu=true;PublishNativeHudPointer(true,.3f,.5f,true);UpdateHook(target.root,nullptr,.016f);live=false;UpdateHook(target.root,nullptr,.016f);if(releases!=4)return 8;live=true;
    PublishNativeHudPointer(false);MoveHook(target.pointer,nullptr,target.root,20,30);if(lastX!=20||lastY!=30||!args)return 9;
    // A noncentral origin and different canvas dimensions still map the same
    // normalized visual point to the native hit-test and pointer draw point.
    for(auto dims:{std::array<float,4>{800,600,-400,-300},std::array<float,4>{1600,900,-800,-450},std::array<float,4>{1024,768,20,-30}}){
        target.width=dims[0];target.height=dims[1];target.left=dims[2];target.top=dims[3];
        for(float x:{0.f,.25f,.5f,1.f})for(float y:{0.f,.25f,.5f,1.f}){
            PublishNativeHudPointer(true,x,y,false);UpdateHook(target.root,nullptr,.016f);
            if(lastX!=target.left+x*target.width||lastY!=target.top+y*target.height)return 10;
        }
    }
    puts("Native HUD adapter: absolute coordinates, one click across eye replays, releases on ray/focus/age/menu loss, invalid-owner exclusion passed.");
}
