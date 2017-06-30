/***********************************************************************************
*             Copyright 2006 - 2050, Hisilicon Tech. Co., Ltd.
*                           ALL RIGHTS RESERVED
* FileName: common_demux.h
* Description: ADP_DEMUX module external output 
*
* History:
* Version   Date             Author     DefectNum    Description
* main\1    2006-12-04   g47171     NULL         Create this file.
***********************************************************************************/

#ifndef  _COMMON_DEMUX_H
#define  _COMMON_DEMUX_H

#include "hi_type.h"
#include "hi_unf_demux.h"
#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
 #if __cplusplus
extern "C" {
 #endif /* __cplusplus */
#endif  /* __cplusplus */

#define DMX_COUNT                       7

#define DMX_DVBPORT_COUNT               3
#define DMX_RAMPORT_COUNT               4
#define DMX_PORT_COUNT                  (DMX_DVBPORT_COUNT + DMX_RAMPORT_COUNT)

#define MAX_SECTION_LEN 4096
#define MAX_SECTION_NUM 256
#define INVALID_PORT_DMX_ID 0xFFFF
#define MAX_IP_THREAD_CNT 16
#define MAX_FILE_THREAD_CNT 16

typedef HI_S32 (*T_CommSectionCallback)(const HI_U8 *pu8Buffer, HI_S32 s32BufferLength, HI_U8 *pSectionStruct);

/********************************************************************
  mark:SECTIONSVR_DATA_FILTER_S
  type:data struct
  purpose:present SECTION data filter contents, application can use it for special filtrate request
  definition:
 **********************************************************************/
typedef struct  hiDMX_DATA_FILTER_S
{
    HI_U32 u32TSPID;                /* TSPID */
    HI_U32 u32BufSize;          /* hareware BUFFER request */

    HI_U8 u8SectionType;       /* section type, 0-section 1-PES */
    HI_U8 u8Crcflag;               /* channel CRC open flag, 0-not open; 1-open */

    HI_U8  u8Match[DMX_FILTER_MAX_DEPTH];
    HI_U8  u8Mask[DMX_FILTER_MAX_DEPTH];
    HI_U8  u8Negate[DMX_FILTER_MAX_DEPTH];
    HI_U16 u16FilterDepth;          /* filtrate depth, 0xff-data use all the user set, otherwise, use DVB algorithm(fixme)*/

    HI_U32 u32TimeOut;       /* timeout, in second. 0-permanent wait */

    T_CommSectionCallback funSectionFunCallback;   /* section end callback */
    HI_U8 *pSectionStruct;
} DMX_DATA_FILTER_S;

/* Definition for IP play*/
typedef struct HiIP_PLAY_HANDLE_S
{
    pthread_t g_SocketThd;
    HI_CHAR   cMultiIPAddr[20];
    HI_U16    u32UdpPort;
    HI_U32    u32PortID;
    HI_BOOL   bStopSocketThread;
    HI_BOOL   bUsed;
} IP_PLAY_HANDLE_S;

/* Definition for File play*/
typedef struct HiFILE_PLAY_HANDLE_S
{
    FILE               *g_pTsFile;
    pthread_t           g_FileThd;
    HI_U32              u32PortID;
    HI_BOOL             bStopFileThread;
    HI_BOOL             bUsed;
} FILE_PLAY_HANDLE_S;

/* Definition for File play*/
typedef struct HiES_FILE_PLAY_HANDLE_S
{
    FILE                  *g_pEsFile;
    pthread_t           g_FileThd;
    HI_HANDLE		  havplay;
    HI_BOOL		  type;  // 0 auido  1 video
    HI_BOOL             bStopFileThread;
    HI_BOOL             bUsed;
} ES_FILE_PLAY_HANDLE_S;

HI_S32 DMX_Init();
HI_S32 DMX_DeInit();
HI_S32 DMX_SectionStartDataFilter(HI_U32 u32DmxId, DMX_DATA_FILTER_S * pstDataFilter);
HI_S32 DMX_Prepare(HI_U32 u32DmxID, HI_U32 u32PortID, HI_U32 u32BufSize);
HI_S32 DMX_IPStartInject(HI_CHAR *pMultiAddr, HI_U32 u32UdpPort, HI_U32 u32PortID);
HI_S32 DMX_IPStopInject(HI_U32 u32PortID);
HI_S32 DMX_FileStartInject(HI_CHAR *path, HI_U32 u32PortID);
HI_S32 DMX_FileStopInject(HI_U32 u32PortID);
HI_S32 DMX_EsFileStartInject(HI_CHAR *path, HI_HANDLE avplay, HI_U32 type);
HI_S32 DMX_EsFileStopInject(HI_U32 index);

#ifdef __cplusplus
 #if __cplusplus
}
 #endif /* __cplusplus */
#endif  /* __cplusplus */

#endif /* _SECTIONSVR_PUB_H*/
