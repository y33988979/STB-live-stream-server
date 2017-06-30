#ifndef     _COMMON_TUNER_H
#define     _COMMON_TUNER_H

#include "hi_type.h"
#include "hi_adp_boardcfg.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

/* **********************************public interface of Tuner********************************/
HI_S32 HIADP_Tuner_Init(HI_VOID);

HI_S32 HIADP_Tuner_Connect(HI_U32 TunerID, HI_U32 Freq, HI_U32 SymbolRate, HI_U32 ThirdParam);

HI_VOID HIADP_Tuner_DeInit(HI_VOID);

#endif
