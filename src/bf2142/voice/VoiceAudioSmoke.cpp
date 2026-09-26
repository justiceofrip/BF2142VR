#include "WaveAudio.h"
#include "VoiceCodec.h"
#include <cstdio>
#include <cmath>
#include <cstring>
#include <cstdlib>
using namespace bfvr::bf2142::voice;
int main(int argc,char** argv){
 for(const auto& d:InputDevices())std::wprintf(L"Input %u: %ls\n",d.id,d.name.c_str());
 for(const auto& d:OutputDevices())std::wprintf(L"Output %u: %ls\n",d.id,d.name.c_str());
 if(argc<2)return 0;WaveAudio audio;
 const bool capture=std::strcmp(argv[1],"--capture")==0;
 if(capture){if(!audio.Capture()){std::printf("Microphone open error %u\n",audio.InputError());return 1;}
  std::array<short,FrameSamples> pcm{};unsigned frames=0;double energy=0;const auto begin=GetTickCount64();
  while(GetTickCount64()-begin<1800){if(audio.Read(pcm)){++frames;for(short p:pcm)energy+=double(p)*p;}Sleep(2);}
  std::printf("Capture frames %u, RMS %.1f; no audio saved.\n",frames,frames?std::sqrt(energy/(frames*FrameSamples)):0);return frames>=40?0:2;
 }
 if(std::strcmp(argv[1],"--tone"))return 3;
 if(!audio.Playback(argc>2?unsigned(std::strtoul(argv[2],nullptr,10)):~0u)){std::printf("Output open error %u\n",audio.OutputError());return 1;}
 Encoder encoder;Decoder decoder;if(!encoder.Open()||!decoder.Open())return 4;
 for(int frame=0;frame<30;++frame){std::array<short,FrameSamples> mono{},pcm{};for(int i=0;i<FrameSamples;++i)mono[i]=short(1400*std::sin((frame*FrameSamples+i)*6.28318530718*440/SampleRate));
  std::array<unsigned char,MaxPayload> data{};const int n=encoder.Encode(mono,data);if(n<=0||!decoder.Decode(data.data(),n,pcm))return 5;
  std::array<short,FrameSamples*2> stereo{};for(int i=0;i<FrameSamples;++i)stereo[i*2]=stereo[i*2+1]=pcm[i];if(!audio.Write(stereo))return 6;Sleep(FrameMs);
 }Sleep(100);std::puts("Real WinMM output accepted 30 Opus-decoded frames.");return 0;
}
