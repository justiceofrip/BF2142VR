#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "LoopbackTransport.h"
namespace bfvr::bf2142::net {
Settings LoadSettings(){
 Settings s;wchar_t path[32768]{};const auto n=GetEnvironmentVariableW(L"BF2142VR_NETWORK",path,32768);
 if(!n||n>=32768||GetPrivateProfileIntW(L"Network",L"Enabled",0,path)!=1)return s;
 wchar_t secret[64]{};GetPrivateProfileStringW(L"Network",L"Secret",L"",secret,64,path);
 if(wcslen(secret)!=32)return s;
 auto hex=[](wchar_t c){if(c>=L'0'&&c<=L'9')return int(c-L'0');if(c>=L'a'&&c<=L'f')return int(c-L'a'+10);if(c>=L'A'&&c<=L'F')return int(c-L'A'+10);return -1;};
 unsigned any=0;for(int i=0;i<16;++i){const int a=hex(secret[i*2]),b=hex(secret[i*2+1]);if(a<0||b<0)return {};s.secret[i]=std::uint8_t(a*16+b);any|=s.secret[i];}
 s.port=GetPrivateProfileIntW(L"Network",L"Port",17568,path);if(!any||!s.port||s.port>65535)return {};
 s.voicePort=GetPrivateProfileIntW(L"Voice",L"Port",17569,path);
 s.voiceRange=float(GetPrivateProfileIntW(L"Voice",L"RangeMetres",20,path));
 s.enemyProximity=GetPrivateProfileIntW(L"Voice",L"EnemyProximity",1,path)!=0;
 s.voiceEnabled=GetPrivateProfileIntW(L"Voice",L"Enabled",0,path)==1&&s.voicePort&&s.voicePort<=65535&&s.voicePort!=s.port&&s.voiceRange>=2&&s.voiceRange<=100;
 s.mirror=GetPrivateProfileIntW(L"Network",L"MirrorToBot",0,path)==1;s.enabled=true;return s;
}
bool Transport::Open(bool server,unsigned port){
 Close();expectedPort=server?0:port;if(!port||port>65535)return false;WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))return false;started=true;
 socket=::socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);if(socket==INVALID_SOCKET){Close();return false;}
 if(server){BOOL exclusive=TRUE;setsockopt(socket,SOL_SOCKET,SO_EXCLUSIVEADDRUSE,reinterpret_cast<const char*>(&exclusive),sizeof(exclusive));}
 sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=htons(u_short(server?port:0));
 u_long nonblocking=1;
 if(bind(socket,reinterpret_cast<sockaddr*>(&a),sizeof(a))||ioctlsocket(socket,FIONBIO,&nonblocking)){Close();return false;}
 return true;
}
bool Transport::Receive(Packet* p,unsigned* port,const Secret& key){
 if(socket==INVALID_SOCKET||!p||!port)return false;
 std::array<char,1201> data{};sockaddr_in a{};int size=sizeof(a);
 for(int i=0;i<32;++i){
  size=sizeof(a);const auto n=recvfrom(socket,data.data(),int(data.size()),0,reinterpret_cast<sockaddr*>(&a),&size);
  if(n==SOCKET_ERROR)return false;
  if(a.sin_family!=AF_INET||a.sin_addr.s_addr!=htonl(INADDR_LOOPBACK))continue;
  if(expectedPort==ntohs(a.sin_port)&&DecodeJoinChallenge(data.data(),std::size_t(n),key,challenge)){haveChallenge=true;continue;}
  if(!Decode(data.data(),std::size_t(n),key,p))continue;
  *port=ntohs(a.sin_port);return true;
 }
 return false;
}
bool Transport::Send(const Packet& p,unsigned port){
 if(socket==INVALID_SOCKET||!port||port>65535)return false;sockaddr_in a{};a.sin_family=AF_INET;a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);a.sin_port=htons(u_short(port));
 return sendto(socket,reinterpret_cast<const char*>(&p),sizeof(p),0,reinterpret_cast<sockaddr*>(&a),sizeof(a))==sizeof(p);
}
unsigned Transport::LocalPort() const {sockaddr_in a{};int n=sizeof(a);return socket!=INVALID_SOCKET&&!getsockname(socket,reinterpret_cast<sockaddr*>(&a),&n)&&a.sin_addr.s_addr==htonl(INADDR_LOOPBACK)?ntohs(a.sin_port):0;}
void Transport::Close(){haveChallenge=false;if(socket!=INVALID_SOCKET){closesocket(socket);socket=INVALID_SOCKET;}if(started){WSACleanup();started=false;}}
}
