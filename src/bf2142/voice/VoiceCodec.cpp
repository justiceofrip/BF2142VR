#include "VoiceCodec.h"
namespace bfvr::bf2142::voice {
bool Encoder::Open(){if(value)return true;int error=0;value=opus_encoder_create(SampleRate,1,OPUS_APPLICATION_VOIP,&error);if(!value||error)return false;
 return opus_encoder_ctl(value,OPUS_SET_BITRATE(24000))==OPUS_OK&&opus_encoder_ctl(value,OPUS_SET_COMPLEXITY(5))==OPUS_OK&&opus_encoder_ctl(value,OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE))==OPUS_OK&&opus_encoder_ctl(value,OPUS_SET_INBAND_FEC(1))==OPUS_OK&&opus_encoder_ctl(value,OPUS_SET_PACKET_LOSS_PERC(5))==OPUS_OK;}
void Encoder::Reset(){if(value)opus_encoder_ctl(value,OPUS_RESET_STATE);}
int Encoder::Encode(const std::array<short,FrameSamples>& pcm,std::array<unsigned char,MaxPayload>& data){return value?opus_encode(value,pcm.data(),FrameSamples,data.data(),MaxPayload):OPUS_INVALID_STATE;}
bool Decoder::Open(){if(value)return true;int error=0;value=opus_decoder_create(SampleRate,1,&error);return value&&error==OPUS_OK;}
void Decoder::Reset(){if(value)opus_decoder_ctl(value,OPUS_RESET_STATE);}
bool Decoder::Decode(const unsigned char* data,int bytes,std::array<short,FrameSamples>& pcm){
 if(!value||bytes<0||bytes>MaxPayload||(bytes&&(!data||opus_packet_get_nb_samples(data,bytes,SampleRate)!=FrameSamples)))return false;
 return opus_decode(value,bytes?data:nullptr,bytes,pcm.data(),FrameSamples,0)==FrameSamples;
}
}
