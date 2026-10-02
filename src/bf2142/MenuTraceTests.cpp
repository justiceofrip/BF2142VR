#include "MenuTrace.h"
#include <cstdio>
#include <cstdarg>
#include <string>
using namespace bfvr::bf2142;
namespace {
std::string output;
void Log(const char* format,...){char text[2048]{};va_list args;va_start(args,format);vsnprintf(text,sizeof(text),format,args);va_end(args);output+=text;}
}
#define CHECK(x) do {if(!(x)){printf("FAIL %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
 SetEnvironmentVariableW(L"BF2142VR_MENU_TRACE",nullptr);SetEnvironmentVariableW(L"BFVR_DIAGNOSTICS",L"normal");
 menuTrace::Initialize(Log);CHECK(!menuTrace::enabled);
 std::vector<DWORD> pixels(2064u*2208u,0xff102030);const auto original=pixels;
 menuTrace::Frame(pixels,2064,2208,true);menuTrace::Queued(nullptr,WM_LBUTTONDOWN,true);CHECK(output.empty());
 SetEnvironmentVariableW(L"BF2142VR_MENU_TRACE",L"1");SetEnvironmentVariableW(L"BFVR_DIAGNOSTICS",L"off");
 menuTrace::Initialize(Log);CHECK(!menuTrace::enabled);
 SetEnvironmentVariableW(L"BFVR_DIAGNOSTICS",L"normal");menuTrace::Initialize(Log);CHECK(menuTrace::enabled);
 menuTrace::Queued(nullptr,WM_LBUTTONDOWN,true);menuTrace::Queued(nullptr,WM_LBUTTONUP,false);
 menuTrace::Delivered(WM_LBUTTONDOWN,false);menuTrace::Delivered(WM_LBUTTONDOWN,true);
 menuTrace::Delivered(WM_CHAR,false);menuTrace::Draw(false);menuTrace::Draw(true);menuTrace::Request(true);menuTrace::Request(false);
 CHECK(menuTrace::queuedDown==1&&menuTrace::queuedUp==0&&menuTrace::deliveredDown==1&&menuTrace::returnedDown==1);
 menuTrace::Frame(pixels,2064,2208,true);CHECK(pixels==original);
 CHECK(output.find("controller-accepted=1/2")!=output.npos&&output.find("Flash-begin/end=1/1")!=output.npos);
 const auto fingerprint=menuTrace::Fingerprint(pixels);pixels.back()^=1;
 CHECK(menuTrace::Fingerprint(pixels)!=fingerprint); // Last sample also exercises >4 GiB intermediate index multiplication on x86.
 const auto bytes=output.size();menuTrace::Frame(pixels,2064,2208,true);CHECK(output.size()==bytes&&menuTrace::changed==1);
 menuTrace::Initialize(nullptr);CHECK(!menuTrace::enabled);
 puts("Menu trace opt-in/privacy/rate-limit/high-resolution fingerprint checks passed.");return 0;
}
