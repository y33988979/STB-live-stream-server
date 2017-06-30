/******************************************************************************

Copyright (C), 2004-2007, Hisilicon Tech. Co., Ltd.
******************************************************************************
File Name     : hi_psi_si.h
Version       : Initial
Author        : Hisilicon multimedia software group
Created       : 2007-04-10
Last Modified :
Description   : Hi3560 PSI/SI function define
Function List :
History       :
* Version   Date         Author     DefectNum    Description
* main\1    2007-04-10   Hs         NULL         Create this file.
******************************************************************************/
#ifndef __SEARCH_H__
#define __SEARCH_H__

#include "hi_type.h"

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif
#endif /* __cplusplus */

#define VIDEO_STREAM_MPEG1 0x01
#define VIDEO_STREAM_MPEG2 0x02
#define VIDEO_STREAM_MPEG4 0x10
#define VIDEO_STREAM_H264 0x1B

#define AUDIO_STREAM_MPEG1A   0x03
#define AUDIO_STREAM_MPEG2A   0x04
#define AUDIO_STREAM_MPEG2AAC 0x0f
#define AUDIO_STREAM_AC3      0x81


#define INVALID_TS_PID 0x1fff
#define MAX_AUDIO_TRACK 4

typedef struct hiPMT_COMPACT_PROGS_S
{
    HI_U32 ProgID;          /* program ID */
    HI_U32 PmtPid;          /*program PMT PID*/
    HI_U32 PcrPid;          /*program PCR PID*/

    HI_UNF_VCODEC_TYPE_E     VideoType;
    HI_U32               VElementNum;        /* video stream number */
    HI_U32               VElementPid;        /* the first video stream PID*/

    HI_U32   AudioType[MAX_AUDIO_TRACK];
    HI_U32               AElementNum;        /* audio stream number */
    HI_U32               AElementPid[MAX_AUDIO_TRACK];        /* the first audio stream PID*/
} PMT_COMPACT_PROGS;

typedef struct hiPMT_COMPACT_TBLS_S
{
    HI_U32            prog_num;
    PMT_COMPACT_PROGS *proginfo;
} PMT_COMPACT_TBLS;

HI_S32 GetAllPmtTbl(HI_U32 u32DmxId, PMT_COMPACT_TBLS **pProgTbl);

HI_VOID FreeAllPmtTbl(HI_VOID);

#ifdef __cplusplus
 #if __cplusplus
}
 #endif
#endif /* __cplusplus */

#endif /* __SERARCH_H__ */
