#include "WaveAudio.h"
#include <cstring>
namespace bfvr::bf2142::voice {
namespace {WAVEFORMATEX Format(unsigned channels){WAVEFORMATEX f{};f.wFormatTag=WAVE_FORMAT_PCM;f.nChannels=WORD(channels);f.nSamplesPerSec=SampleRate;f.wBitsPerSample=16;f.nBlockAlign=WORD(channels*2);f.nAvgBytesPerSec=f.nSamplesPerSec*f.nBlockAlign;return f;}}
std::vector<Device> InputDevices(){std::vector<Device> result;for(unsigned i=0;i<waveInGetNumDevs();++i){WAVEINCAPSW c{};if(waveInGetDevCapsW(i,&c,sizeof(c))==MMSYSERR_NOERROR)result.push_back({i,c.szPname});}return result;}
std::vector<Device> OutputDevices(){std::vector<Device> result;for(unsigned i=0;i<waveOutGetNumDevs();++i){WAVEOUTCAPSW c{};if(waveOutGetDevCapsW(i,&c,sizeof(c))==MMSYSERR_NOERROR)result.push_back({i,c.szPname});}return result;}
bool WaveAudio::Capture(unsigned device){
 if(input)return true;auto f=Format(1);inputError=waveInOpen(&input,device,&f,0,0,device==~0u?WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE:CALLBACK_NULL);
 if(inputError){input=nullptr;return false;}nextIn=0;inputs={};
 for(auto& b:inputs){b.header.lpData=reinterpret_cast<char*>(b.pcm.data());b.header.dwBufferLength=sizeof(b.pcm);
  inputError=waveInPrepareHeader(input,&b.header,sizeof(b.header));if(!inputError)inputError=waveInAddBuffer(input,&b.header,sizeof(b.header));if(inputError){CloseCapture();return false;}}
 inputError=waveInStart(input);if(inputError){CloseCapture();return false;}return true;
}
void WaveAudio::CloseCapture(){if(!input)return;waveInReset(input);for(auto& b:inputs)if(b.header.dwFlags&WHDR_PREPARED)waveInUnprepareHeader(input,&b.header,sizeof(b.header));waveInClose(input);input=nullptr;inputs={};nextIn=0;}
bool WaveAudio::Read(std::array<short,FrameSamples>& pcm){
 if(!input)return false;auto& b=inputs[nextIn];if(!(b.header.dwFlags&WHDR_DONE))return false;
 const bool complete=b.header.dwBytesRecorded==sizeof(b.pcm);if(complete)pcm=b.pcm;
 b.header.dwBytesRecorded=0;inputError=waveInAddBuffer(input,&b.header,sizeof(b.header));nextIn=(nextIn+1)%unsigned(inputs.size());return complete&&!inputError;
}
bool WaveAudio::Playback(unsigned device){
 if(output)return true;auto f=Format(2);outputError=waveOutOpen(&output,device,&f,0,0,device==~0u?WAVE_MAPPED_DEFAULT_COMMUNICATION_DEVICE:CALLBACK_NULL);
 if(outputError){output=nullptr;return false;}nextOut=0;outputs={};
 for(auto& b:outputs){b.header.lpData=reinterpret_cast<char*>(b.pcm.data());b.header.dwBufferLength=sizeof(b.pcm);
  outputError=waveOutPrepareHeader(output,&b.header,sizeof(b.header));if(outputError){ClosePlayback();return false;}}
 return true;
}
void WaveAudio::ClosePlayback(){if(!output)return;waveOutReset(output);for(auto& b:outputs)if(b.header.dwFlags&WHDR_PREPARED)waveOutUnprepareHeader(output,&b.header,sizeof(b.header));waveOutClose(output);output=nullptr;outputs={};nextOut=0;}
bool WaveAudio::Write(const std::array<short,FrameSamples*2>& pcm){
 if(!output)return false;auto& b=outputs[nextOut];if((b.header.dwFlags&WHDR_INQUEUE)&&!(b.header.dwFlags&WHDR_DONE))return false;
 b.pcm=pcm;outputError=waveOutWrite(output,&b.header,sizeof(b.header));if(outputError)return false;nextOut=(nextOut+1)%unsigned(outputs.size());return true;
}
}
