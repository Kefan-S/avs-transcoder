#ifndef _TRANSCODER_H_
#define _TRANSCODER_H_

//#define DLL_EXPORT __declspec(dllexport) //��־����������������Ϊ����Ľӿ�

#include "transcoding_type.h"
#include "global.h"

void avs_encoder_create  (c_avs_enc* p_avs_enc);
void avs_encoder_encode  (c_avs_enc* p_avs_enc);
void avs_encoder_destroy(c_avs_enc* p_avs_enc);

#endif
