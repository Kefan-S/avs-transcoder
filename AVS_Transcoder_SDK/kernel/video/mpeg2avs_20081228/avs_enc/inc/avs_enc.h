#ifndef _AVS_ENC_H_
#define _AVS_ENC_H_
#include "global.h"
#define DLL_EXPORT __declspec(dllexport) //��־����������������Ϊ����Ľӿ�
DLL_EXPORT void avs_encoder_create  (c_avs_enc* p_avs_enc);
DLL_EXPORT void avs_encoder_encode  (c_avs_enc* p_avs_enc);
DLL_EXPORT void avs_encoder_destropy(c_avs_enc* p_avs_enc);
#endif