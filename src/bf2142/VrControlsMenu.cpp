#include "VrControlsMenu.h"
#include "voice/WaveAudio.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <string>
namespace bfvr::bf2142 {
int VrMenuHit(float x,float y,bool open) noexcept {
 if(!std::isfinite(x)||!std::isfinite(y)||x<0||x>1||y<0||y>1)return -1;
 if(!open)return x>=.76f && x<.98f && y>=.018f && y<.078f?0:-1;
 if(x>=.70f&&x<.82f&&y>=.12f&&y<.19f)return 11;
 if(x>=.83f && x<.93f && y>=.12f && y<.19f)return 10;
 if(x>=.09f && x<.48f && y>=.245f && y<.245f+9*.064f){
  const float row=(y-.245f)/.064f;return row-std::floor(row)<.85f?1+int(row):-1;
 }return -1;
}
void VrControlsMenu::Hotkey(bool down){if(down&&!keyHeld){open=!open;dirty=true;hover=-1;}keyHeld=down;}
bool VrControlsMenu::Interact(float x,float y,bool click,bool back,VrSettings& s,float headHeight){
 const int h=VrMenuHit(x,y,open);if(h!=hover){hover=h;dirty=true;}
 const bool consumed=open||h==0;
 if(back&&open){open=false;dirty=true;backHeld=true;return true;}
 if(!click||h<0)return consumed;
 dirty=true;
 if(h==0){open=true;return true;}
 if(h==10){open=false;return true;}
 if(h==11){voicePage=!voicePage;return true;}
 if(voicePage){
  const auto cycle=[](unsigned selected,const std::vector<voice::Device>& devices){
   if(devices.empty())return ~0u;if(selected==~0u)return devices.front().id;
   for(size_t i=0;i+1<devices.size();++i)if(devices[i].id==selected)return devices[i+1].id;return ~0u;
  };
  switch(h){
   case 1:s.proximityVoice=!s.proximityVoice;break;
   case 2:s.proximityMuted=!s.proximityMuted;break;
   case 3:s.voiceThreshold=s.voiceThreshold<-20?s.voiceThreshold+5:-55;break;
   case 4:s.voiceVolume=s.voiceVolume<1.9f?s.voiceVolume+.25f:0;break;
   case 5:s.voiceInput=cycle(s.voiceInput,voice::InputDevices());break;
   case 6:s.voiceOutput=cycle(s.voiceOutput,voice::OutputDevices());break;
   default:return true;
  }
  saveFailed=!SaveVrPreferences(s);return true;
 }
 switch(h){
  case 1:s.snapTurning=!s.snapTurning;break;
  case 2:s.snapAngle=s.snapAngle<29?30.f:s.snapAngle<44?45.f:s.snapAngle<59?60.f:15.f;break;
  case 3:s.hideCrosshair=!s.hideCrosshair;break;
  case 4:s.physicalStance=!s.physicalStance;break;
  case 5:if(std::isfinite(headHeight)&&headHeight>=-.5f&&headHeight<3){s.standingHeight=headHeight;recenter=true;}break;
  case 6:s.toggleWeaponGrip=!s.toggleWeaponGrip;break;
  case 7:s.grenadeArc=!s.grenadeArc;break;
  case 8:s.menuRoom=!s.menuRoom;break;
  case 9:s.controllerRelativeMovement=!s.controllerRelativeMovement;break;
 }
 saveFailed=!SaveVrPreferences(s);return true;
}
void VrControlsMenu::Draw(std::vector<DWORD>& image,UINT w,UINT h,DXGI_FORMAT format,const VrSettings& s){
 if(!w||!h||w>8192||h>8192||image.size()!=size_t(w)*h)return;
 if(dirty||cachedWidth!=w||cachedHeight!=h||cachedFormat!=format){
  constexpr int aw=1280,ah=800;BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=aw;info.bmiHeader.biHeight=-ah;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
  void* bits=nullptr;HDC dc=CreateCompatibleDC(nullptr);if(!dc)return;
  HBITMAP bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,nullptr,0);if(!bitmap){DeleteDC(dc);return;}
  const auto previous=SelectObject(dc,bitmap);std::memset(bits,0,aw*ah*4);
  const auto rect=[&](float x,float y,float width,float height,COLORREF color){RECT box{LONG(x*aw),LONG(y*ah),LONG((x+width)*aw),LONG((y+height)*ah)};auto brush=CreateSolidBrush(color);FillRect(dc,&box,brush);DeleteObject(brush);};
  const auto label=[&](float x,float y,const std::wstring& text,int size,COLORREF color,bool bold=false){
   HFONT font=CreateFontW(-size,0,0,0,bold?FW_SEMIBOLD:FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe UI");
   const auto old=SelectObject(dc,font);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);TextOutW(dc,int(x*aw),int(y*ah),text.c_str(),int(text.size()));SelectObject(dc,old);DeleteObject(font);
  };
  const COLORREF ink=RGB(226,239,241),muted=RGB(150,177,185),accent=RGB(74,215,221);
  if(!open){rect(.76f,.018f,.22f,.06f,hover==0?RGB(30,94,106):RGB(17,39,50));label(.775f,.026f,L"VR CONTROLS",28,ink,true);}
  else{
   rect(.055f,.10f,.89f,.81f,RGB(13,26,36));rect(.055f,.10f,.006f,.81f,accent);
   label(.085f,.133f,L"BATTLEFIELD 2142  /  VR CONTROLS",30,ink,true);
   rect(.83f,.12f,.10f,.07f,hover==10?RGB(40,104,112):RGB(32,53,64));label(.845f,.137f,L"CLOSE",23,ink,true);
   rect(.70f,.12f,.12f,.07f,hover==11?RGB(40,104,112):RGB(32,53,64));label(.712f,.137f,voicePage?L"CONTROLS":L"VOICE",21,ink,true);
   if(voicePage){
    const std::wstring voiceValues[]={L"PROXIMITY     "+std::wstring(s.proximityVoice?L"ON":L"OFF"),L"PROX MIC     "+std::wstring(s.proximityMuted?L"MUTED":L"VOICE ACTIVATED"),L"ACTIVATION     "+std::to_wstring(int(s.voiceThreshold))+L" dB",L"PROX VOLUME     "+std::to_wstring(int(s.voiceVolume*100))+L"%",L"MIC: "+std::wstring(s.voiceInput==~0u?L"DEFAULT":std::to_wstring(s.voiceInput)),L"OUTPUT: "+std::wstring(s.voiceOutput==~0u?L"DEFAULT":std::to_wstring(s.voiceOutput))};
    for(int i=0;i<6;++i){float y=.245f+i*.064f;rect(.09f,y,.39f,.0544f,hover==i+1?RGB(32,89,102):RGB(26,45,58));label(.103f,y+.012f,voiceValues[i],23,ink);}
    label(.535f,.24f,L"PROXIMITY + SQUAD RADIO",25,accent,true);
    const wchar_t* voiceHelp[]={L"Nearby voice activates when you speak.",L"The host chooses range and enemy chat.",L"Grip left shoulder radio: squad only.",L"Radio pauses proximity transmission.",L"Native V / B radio keys still work.",L"Prox mute does not mute native radio.",L"More negative dB: more sensitive mic.",L"Default: Windows communications device.",L"Device changes here affect proximity.",L"Set native radio devices in Windows."};
    for(int i=0;i<10;++i)label(.535f,.292f+i*.041f,voiceHelp[i],20,ink);
    const auto deviceName=[](unsigned id,const std::vector<voice::Device>& list){if(id==~0u)return std::wstring(L"Windows communications default");for(const auto& d:list)if(d.id==id)return d.name;return std::wstring(L"Unavailable device");};
    label(.103f,.67f,deviceName(s.voiceInput,voice::InputDevices()).substr(0,33),18,muted);
    label(.103f,.71f,deviceName(s.voiceOutput,voice::OutputDevices()).substr(0,33),18,muted);
   }else{
   const std::wstring values[]={L"TURNING     "+std::wstring(s.snapTurning?L"SNAP":L"SMOOTH"),L"SNAP ANGLE     "+std::to_wstring(int(s.snapAngle))+L" degrees",L"CROSSHAIR     "+std::wstring(s.hideCrosshair?L"HIDDEN":L"VISIBLE"),L"PHYSICAL STANCE     "+std::wstring(s.physicalStance?L"ON":L"OFF"),L"CALIBRATE STANDING HEIGHT",L"GRIP TO HOLSTER     "+std::wstring(s.toggleWeaponGrip?L"ON":L"OFF"),L"GRENADE AIMING ARC     "+std::wstring(s.grenadeArc?L"ON":L"OFF"),L"3D MENU ROOM     "+std::wstring(s.menuRoom?L"ON":L"OFF"),L"MOVE: "+std::wstring(s.controllerRelativeMovement?L"LEFT CONTROLLER":L"HEAD / HMD")};
   for(int i=0;i<9;++i){float y=.245f+i*.064f;rect(.09f,y,.39f,.0544f,hover==i+1?RGB(32,89,102):RGB(26,45,58));label(.103f,y+.012f,values[i],23,ink);}
   label(.535f,.24f,L"QUEST CONTROLLERS",25,accent,true);
   const wchar_t* help[]={L"Left stick: move  /  click: sprint",L"Right stick: turn  /  click: recenter",L"Right trigger: fire or throw grenade",L"Right grip: holster / squeeze hand",L"Left grip near gun: support hand",L"Raise sights to your eye: aim",L"Swing the knife: attack",L"A: jump   B: game menu / back",L"X: reload   Y: use / enter vehicle",L"Left trigger + X / Y: cycle equipment",L"Grab body slots to select equipment",L"Duck or lie down to change stance"};
   const wchar_t* vehicleHelp[]={L"Left stick: drive / steer",L"Look toward targets: aim movable guns",L"Right stick: aim / pilot pitch and roll",L"Right trigger: primary fire",L"Right grip: alternate fire / APC pod",L"Y: enter / exit vehicle or launcher",L"Hold left trigger + X: driver (seat 1)",L"Hold left trigger + Y: cycle seats 2-8",L"Press again if a seat is unavailable",L"Titan pod: enter, then right trigger",L"Right stick click: recenter",L"Aircraft pilot weapons stay fixed"};
   for(int i=0;i<12;++i)label(.535f,.292f+i*.038f,vehicle?vehicleHelp[i]:help[i],22,ink);
   label(.535f,.775f,s.controllerRelativeMovement?L"Movement follows left controller aim.":L"Movement follows headset heading.",21,muted);
   label(.535f,.813f,L"Radio: grip at left shoulder to talk",21,muted);
   }
   label(.09f,.852f,saveFailed?L"Applied now; settings file could not be saved.":L"Changes apply immediately and save automatically.  Insert also opens this panel.",21,saveFailed?RGB(255,186,91):muted);
  }
  art.resize(size_t(w)*h);auto src=static_cast<DWORD*>(bits);const bool rgba=format==DXGI_FORMAT_R8G8B8A8_UNORM||format==DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  for(UINT y=0;y<h;++y)for(UINT x=0;x<w;++x){DWORD c=src[size_t(y)*ah/h*aw+size_t(x)*aw/w];if(c){c|=0xff000000;if(rgba)c=(c&0xff00ff00)|((c&255)<<16)|((c>>16)&255);}art[size_t(y)*w+x]=c;}
  SelectObject(dc,previous);DeleteObject(bitmap);DeleteDC(dc);cachedWidth=w;cachedHeight=h;cachedFormat=format;dirty=false;
 }
 for(size_t i=0;i<art.size();++i)if(art[i])image[i]=art[i];
}
}
