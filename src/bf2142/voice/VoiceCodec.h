#pragma once
#include "VoiceProtocol.h"
#include <opus.h>
namespace bfvr::bf2142::voice {
class Encoder {
public:
 Encoder()=default;Encoder(const Encoder&)=delete;Encoder& operator=(const Encoder&)=delete;
 bool Open();void Reset();int Encode(const std::array<short,FrameSamples>&,std::array<unsigned char,MaxPayload>&);
 ~Encoder(){if(value)opus_encoder_destroy(value);}
private:OpusEncoder* value=nullptr;
};
class Decoder {
public:
 Decoder()=default;Decoder(const Decoder&)=delete;Decoder& operator=(const Decoder&)=delete;
 bool Open();void Reset();bool Decode(const unsigned char*,int,std::array<short,FrameSamples>&);
 ~Decoder(){if(value)opus_decoder_destroy(value);}
private:OpusDecoder* value=nullptr;
};
}
