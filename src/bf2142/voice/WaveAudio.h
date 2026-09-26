#pragma once
#include "VoiceProtocol.h"
#include <windows.h>
#include <mmsystem.h>
#include <string>
#include <vector>
namespace bfvr::bf2142::voice {
struct Device {unsigned id=0;std::wstring name;};
std::vector<Device> InputDevices();std::vector<Device> OutputDevices();
class WaveAudio {
public:
 WaveAudio()=default;WaveAudio(const WaveAudio&)=delete;WaveAudio& operator=(const WaveAudio&)=delete;
 bool Capture(unsigned device=~0u);bool Playback(unsigned device=~0u);
 bool Read(std::array<short,FrameSamples>&);bool Write(const std::array<short,FrameSamples*2>&);
 void CloseCapture();void ClosePlayback();~WaveAudio(){CloseCapture();ClosePlayback();}
 bool Capturing()const{return input!=nullptr;}bool Playing()const{return output!=nullptr;}
 unsigned InputError()const{return inputError;}unsigned OutputError()const{return outputError;}
private:
 struct In {WAVEHDR header{};std::array<short,FrameSamples> pcm{};};
 struct Out {WAVEHDR header{};std::array<short,FrameSamples*2> pcm{};};
 HWAVEIN input=nullptr;HWAVEOUT output=nullptr;std::array<In,8> inputs{};std::array<Out,4> outputs{};
 unsigned nextIn=0,nextOut=0,inputError=0,outputError=0;
};
}
