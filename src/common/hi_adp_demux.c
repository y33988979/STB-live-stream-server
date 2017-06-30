#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <string.h>   /* for NULL */
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "hi_type.h"

#include "hi_unf_demux.h"
#include "hi_unf_avplay.h"
#include "hi_adp.h"
#include "hi_adp_demux.h"
#include "hi_adp_search.h"

static HI_BOOL g_DmxInitFlag = HI_FALSE;
static HI_BOOL bIPInjectFlag[DMX_PORT_COUNT];
static HI_BOOL bFileInjectFlag[DMX_PORT_COUNT];

HI_HANDLE g_PortTsBuf[DMX_PORT_COUNT];

IP_PLAY_HANDLE_S IP_Play_Handles[MAX_IP_THREAD_CNT];
FILE_PLAY_HANDLE_S FILE_Play_Handles[MAX_FILE_THREAD_CNT];
ES_FILE_PLAY_HANDLE_S Es_FILE_Play_Handles[MAX_FILE_THREAD_CNT];

static HI_S32 DMX_DataRead(HI_HANDLE hChannel, HI_U32 u32TimeOutms, HI_U8 *pBuf ,
                           HI_U32* pAcquiredNum , HI_U32 * pBuffSize);

HI_S32 DMX_Init()
{
    HI_U32 i;

    if (0 == g_DmxInitFlag)
    {
        HIAPI_RUN_RETURN(HI_UNF_DMX_Init());

        for (i = 0; i < DMX_PORT_COUNT; i++)
        {
            g_PortTsBuf[i]     = HI_INVALID_HANDLE;
            bIPInjectFlag[i]   = HI_FALSE;
            bFileInjectFlag[i] = HI_FALSE;
        }

        for (i = 0; i < MAX_IP_THREAD_CNT; i++)
        {
            IP_Play_Handles[i].bUsed = HI_FALSE;
        }

        for (i = 0; i < MAX_FILE_THREAD_CNT; i++)
        {
            FILE_Play_Handles[i].bUsed = HI_FALSE;
        }

        g_DmxInitFlag = 1;
    }

    return HI_SUCCESS;
}

HI_S32 DMX_DeInit()
{
    HI_S32 ret = HI_SUCCESS;

    if (g_DmxInitFlag == 1)
    {
        HIAPI_RUN(HI_UNF_DMX_DeInit(),ret);

        g_DmxInitFlag = 0;
    }

    return ret;
}

HI_S32 DMX_Prepare(HI_U32 u32DmxID, HI_U32 u32PortID, HI_U32 u32BufSize)
{
    HI_UNF_DMX_TSBUF_STATUS_S Status;

    if ((u32DmxID >= DMX_COUNT) || (u32PortID >= DMX_PORT_COUNT))
    {
        return HI_FAILURE;
    }

    HIAPI_RUN_RETURN(HI_UNF_DMX_AttachTSPort(u32DmxID, u32PortID));

    /*for IP play*/
    if (u32PortID >= DMX_DVBPORT_COUNT)
    {
        if (HI_INVALID_HANDLE != g_PortTsBuf[u32PortID])
        {
            HIAPI_RUN_RETURN(HI_UNF_DMX_GetTSBufferStatus(g_PortTsBuf[u32PortID],&Status));
            if(Status.u32BufSize == u32BufSize)
                  return HI_SUCCESS;

            HIAPI_RUN_RETURN(HI_UNF_DMX_DestroyTSBuffer(g_PortTsBuf[u32PortID]));
        }
        g_PortTsBuf[u32PortID] = HI_INVALID_HANDLE;
     if(u32BufSize != 0)
     {
         HIAPI_RUN_RETURN(HI_UNF_DMX_CreateTSBuffer(u32PortID, u32BufSize, &g_PortTsBuf[u32PortID]));
     }
     printf(" DMX_Prepare  port %d handle = 0x%x \n",u32PortID,g_PortTsBuf[u32PortID]);
    }

    return HI_SUCCESS;
}



HI_S32 DMX_SectionStartDataFilter(HI_U32 u32DmxId, DMX_DATA_FILTER_S * pstDataFilter)
{
    HI_U32 i;
    HI_U8 *p;
    HI_UNF_DMX_CHAN_ATTR_S stChanAttr;
    HI_HANDLE hChan, hFilter = HI_NULL;
    HI_S32 s32Ret;
    HI_UNF_DMX_FILTER_ATTR_S stFilterAttr;
    HI_U32 u32AquiredNum = 0;
    HI_U8 u8DataBuf[MAX_SECTION_LEN * MAX_SECTION_NUM];
    HI_U32 u32BufSize[MAX_SECTION_NUM];

    stChanAttr.u32BufSize = 16 * 1024;
    stChanAttr.enChannelType = HI_UNF_DMX_CHAN_TYPE_SEC;
    stChanAttr.enCRCMode = HI_UNF_DMX_CHAN_CRC_MODE_BY_SYNTAX_AND_DISCARD;
    stChanAttr.enOutputMode = HI_UNF_DMX_CHAN_OUTPUT_MODE_PLAY;
    HIAPI_RUN_RETURN(HI_UNF_DMX_CreateChannel(u32DmxId, &stChanAttr, &hChan));

    HIAPI_RUN_RETURN(HI_UNF_DMX_SetChannelPID(hChan, pstDataFilter->u32TSPID));

    stFilterAttr.u32FilterDepth = pstDataFilter->u16FilterDepth;
    memcpy(stFilterAttr.au8Match, pstDataFilter->u8Match, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.au8Mask, pstDataFilter->u8Mask, DMX_FILTER_MAX_DEPTH);
    memcpy(stFilterAttr.au8Negate, pstDataFilter->u8Negate, DMX_FILTER_MAX_DEPTH);

    HIAPI_RUN_RETURN(HI_UNF_DMX_CreateFilter(u32DmxId, &stFilterAttr, &hFilter));

    HIAPI_RUN_RETURN(HI_UNF_DMX_SetFilterAttr(hFilter, &stFilterAttr));

    HIAPI_RUN_RETURN(HI_UNF_DMX_AttachFilter(hFilter, hChan));

    HIAPI_RUN_RETURN(HI_UNF_DMX_OpenChannel(hChan));


    memset(u8DataBuf, 0, sizeof(u8DataBuf));
    memset(u32BufSize, 0, sizeof(u32BufSize));

    s32Ret = DMX_DataRead(hChan, pstDataFilter->u32TimeOut, u8DataBuf, &u32AquiredNum , u32BufSize);
    if (HI_SUCCESS == s32Ret)
    {
        //multi-SECTION parse
         p = u8DataBuf;
        for(i=0;i<u32AquiredNum;i++)
        {
            pstDataFilter->funSectionFunCallback(p, u32BufSize[i], pstDataFilter->pSectionStruct);
            p = p+MAX_SECTION_LEN;
        }
    }

    HIAPI_RUN(HI_UNF_DMX_CloseChannel(hChan),s32Ret);
    HIAPI_RUN(HI_UNF_DMX_DetachFilter(hFilter, hChan),s32Ret);
    HIAPI_RUN(HI_UNF_DMX_DestroyFilter(hFilter),s32Ret);
    HIAPI_RUN(HI_UNF_DMX_DestroyChannel(hChan),s32Ret);

    return s32Ret;
}

/*
    Timeout:in millisecond
 */
HI_S32 DMX_DataRead(HI_HANDLE hChannel, HI_U32 u32TimeOutms, HI_U8 *pBuf, HI_U32* pAcquiredNum , HI_U32 * pBuffSize)
{
    HI_U8  u8tableid;
    HI_UNF_DMX_DATA_S sSection[32];
    HI_U32 num, i = 0;
    HI_U32 count = 0 ;
    HI_U32 u32Times = 0;
    HI_U32 u32SecTotalNum=0, u32SecNum = 0;
    HI_U32 RequestNum = 32;
    HI_U8 u8SecGotFlag[MAX_SECTION_NUM];

    u32Times = u32TimeOutms / 10;

    memset(u8SecGotFlag, 0, sizeof(u8SecGotFlag[MAX_SECTION_NUM]));

    while (--u32Times)
    {
        num = 0;
        if ((HI_SUCCESS == HI_UNF_DMX_AcquireBuf(hChannel, RequestNum, &num, sSection, u32TimeOutms)) && (num > 0))
        {
            for (i = 0; i < num; i++)
            {
                if (sSection[i].enDataType == HI_UNF_DMX_DATA_TYPE_WHOLE)
                {
                    u8tableid = sSection[i].pu8Data[0];
                    if( ((EIT_TABLE_ID_SCHEDULE_ACTUAL_LOW <= u8tableid) && ( u8tableid <= EIT_TABLE_ID_SCHEDULE_ACTUAL_HIGH))
                        ||((EIT_TABLE_ID_SCHEDULE_OTHER_LOW <= u8tableid) && ( u8tableid <= EIT_TABLE_ID_SCHEDULE_OTHER_HIGH)))
                    {
                        u32SecNum = sSection[i].pu8Data[6]>>3;
                        u32SecTotalNum = (sSection[i].pu8Data[7]>>3) +1;
                    }
                    else if ( TDT_TABLE_ID == u8tableid ||TOT_TABLE_ID == u8tableid)
                    {
                        u32SecNum = 0;
                        u32SecTotalNum = 1;
                    }
                    else
                    {
                        u32SecNum = sSection[i].pu8Data[6];
                        u32SecTotalNum = sSection[i].pu8Data[7] + 1;
                    }

                    if(u8SecGotFlag[u32SecNum] == 0)
                    {
                        memcpy((void *)(pBuf + u32SecNum * MAX_SECTION_LEN), sSection[i].pu8Data, sSection[i].u32Size);
                        u8SecGotFlag[u32SecNum] = 1;
                        pBuffSize[  u32SecNum] = sSection[i].u32Size;
                        count++;
                    }
                }
            }

            HI_UNF_DMX_ReleaseBuf(hChannel, num, sSection);

            /* to check if all sections are received*/
            if(u32SecTotalNum == count)
                break;
        }
        else
        {
            printf("HI_UNF_DMX_AcquireBuf time out\n");
            return HI_FAILURE;
        }
    }

    if (u32Times == 0)
    {
        printf("HI_UNF_DMX_AcquireBuf time out\n");
        return HI_FAILURE;
    }

    if(pAcquiredNum != HI_NULL)
        *pAcquiredNum = u32SecTotalNum;

    return HI_SUCCESS;
}

static HI_VOID SocketThread(HI_VOID *args)
{
    HI_S32 SocketFd;
    struct sockaddr_in ServerAddr;
    in_addr_t IpAddr;
    struct ip_mreq Mreq;
    HI_U32 AddrLen;
    HI_U32 u32Handle;

    HI_UNF_STREAM_BUF_S StreamBuf;
    HI_U32 ReadLen;
    HI_U32 GetBufCount  = 0;
    HI_U32 ReceiveCount = 0;
    HI_S32 Ret;

    u32Handle = (HI_U32) args;
    if (MAX_IP_THREAD_CNT <= u32Handle)
    {
        printf("\n Socket Thread: invalid args");
        return;
    }

    SocketFd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (SocketFd < 0)
    {
        printf("create socket error [%d].\n", errno);
        return;
    }

    ServerAddr.sin_family = AF_INET;
    ServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    ServerAddr.sin_port = htons(IP_Play_Handles[u32Handle].u32UdpPort);

    if (bind(SocketFd, (struct sockaddr *)(&ServerAddr), sizeof(struct sockaddr_in)) < 0)
    {
        printf("socket bind error [%d].\n", errno);
        close(SocketFd);
        return;
    }

    IpAddr = inet_addr(IP_Play_Handles[u32Handle].cMultiIPAddr);

    printf("\n SocketThread %s 0x%x 0x%x 0x%x\n", IP_Play_Handles[u32Handle].cMultiIPAddr,
           IP_Play_Handles[u32Handle].u32UdpPort, IpAddr, ServerAddr.sin_port);

    if (IpAddr)
    {
        Mreq.imr_multiaddr.s_addr = IpAddr;
        Mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(SocketFd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &Mreq, sizeof(struct ip_mreq)))
        {
            printf("Socket setsockopt ADD_MEMBERSHIP error [%d].\n", errno);
            close(SocketFd);
            return;
        }
    }

    AddrLen = sizeof(ServerAddr);

    while (!IP_Play_Handles[u32Handle].bStopSocketThread)
    {
        Ret = HI_UNF_DMX_GetTSBuffer(g_PortTsBuf[IP_Play_Handles[u32Handle].u32PortID], 188 * 50, &StreamBuf,0);
        if (Ret != HI_SUCCESS)
        {
            GetBufCount++;
            if (GetBufCount >= 10)
            {
                printf("########## TS come too fast! #########, Ret=%d\n", Ret);
                GetBufCount = 0;
            }

            usleep(10000);
            continue;
        }

        GetBufCount = 0;

        ReadLen = recvfrom(SocketFd, StreamBuf.pu8Data, 1316, 0,
                           (struct sockaddr *)&ServerAddr, &AddrLen);
        if (ReadLen <= 0)
        {
            ReceiveCount++;
            if (ReceiveCount >= 50)
            {
                printf("########## TS come too slow or net error! #########\n");
                ReceiveCount = 0;
            }
        }
        else
        {
            ReceiveCount = 0;
            Ret = HI_UNF_DMX_PutTSBuffer(g_PortTsBuf[IP_Play_Handles[u32Handle].u32PortID], ReadLen);
            if (Ret != HI_SUCCESS)
            {
                printf("call HI_UNF_DMX_PutTSBuffer failed.\n");
            }
        }
    }

    close(SocketFd);
    return;
}

HI_VOID FileTsTthread(HI_VOID *args)
{
    HI_UNF_STREAM_BUF_S StreamBuf;
    HI_U32 Readlen;
    HI_S32 Ret;
    HI_U32 u32Handle = 0;

    u32Handle = (HI_U32)args;
    if (MAX_FILE_THREAD_CNT <= u32Handle)
    {
        printf("\n File TS Thread: invalid args");
        return;
    }

    while (!FILE_Play_Handles[u32Handle].bStopFileThread)
    {
        Ret = HI_UNF_DMX_GetTSBuffer(g_PortTsBuf[FILE_Play_Handles[u32Handle].u32PortID], 188 * 50, &StreamBuf,0);
        if (Ret != HI_SUCCESS)
        {
            usleep(10*1000);
            continue;
        }

        Readlen = fread((void *)StreamBuf.pu8Data, sizeof(HI_S8), 0x1000, FILE_Play_Handles[u32Handle].g_pTsFile);
        if (Readlen <= 0)
        {
            printf("read ts file error!\n");
            rewind(FILE_Play_Handles[u32Handle].g_pTsFile);
            continue;
        }

        Ret = HI_UNF_DMX_PutTSBuffer(g_PortTsBuf[FILE_Play_Handles[u32Handle].u32PortID], Readlen);
        if (Ret != HI_SUCCESS)
        {
            printf("call HI_UNF_DMX_PutTSBuffer failed.\n");
        }
    }

    return;
}


HI_VOID FileEsTthread(HI_VOID *args)
{
    HI_UNF_STREAM_BUF_S StreamBuf;
    HI_U32 Readlen;
    HI_S32 Ret;
    HI_U32 u32Handle = 0;
    HI_UNF_AVPLAY_BUFID_E bufid = HI_UNF_AVPLAY_BUF_ID_ES_AUD;

    u32Handle = (HI_U32)args;
    if (MAX_FILE_THREAD_CNT <= u32Handle)
    {
        printf("\n File TS Thread: invalid args");
        return;
    }

    if(Es_FILE_Play_Handles[u32Handle].type == 0)
        bufid = HI_UNF_AVPLAY_BUF_ID_ES_AUD;
    else if(Es_FILE_Play_Handles[u32Handle].type == 1)
        bufid = HI_UNF_AVPLAY_BUF_ID_ES_VID;

    while (!Es_FILE_Play_Handles[u32Handle].bStopFileThread)
    {
        Ret = HI_UNF_AVPLAY_GetBuf(Es_FILE_Play_Handles[u32Handle].havplay , bufid , 0x4000, &StreamBuf,0);
        if (Ret != HI_SUCCESS)
        {
            usleep(10*1000);
            continue;
        }

        Readlen = fread((void *)StreamBuf.pu8Data, sizeof(HI_S8), 0x4000, Es_FILE_Play_Handles[u32Handle].g_pEsFile);
        if (Readlen <= 0)
        {
            printf("read ts file error!\n");
            rewind(Es_FILE_Play_Handles[u32Handle].g_pEsFile);
            continue;
        }

        Ret = HI_UNF_AVPLAY_PutBuf(Es_FILE_Play_Handles[u32Handle].havplay , bufid , Readlen , 0);
        if (Ret != HI_SUCCESS)
        {
            printf("call HI_UNF_DMX_PutTSBuffer failed.\n");
        }
    }

    return;
}

/*****************************************************************************
* Function:      AvStartAvPlay
* Description:   start AV play
* Data Accessed:
* Data Updated:
* Input:             HI_U16 videopid, HI_U16 audiopid, HI_U16 pcrpid, HI_U16 telpid, HI_U16 subpid
* Output:           None
* Return:          None
* Others: None
*****************************************************************************/
HI_S32  DMX_IPStartInject(HI_CHAR *pMultiAddr, HI_U32 u32UdpPort, HI_U32 u32PortID)
{
    HI_S32 i;

    if (HI_TRUE == bIPInjectFlag[u32PortID])
    {
        DMX_IPStopInject(u32PortID);
    }

    for (i = 0; i < MAX_IP_THREAD_CNT; i++)
    {
        if (HI_FALSE == IP_Play_Handles[i].bUsed)
        {
            break;
        }
    }

    if (MAX_IP_THREAD_CNT == i)
    {
        return HI_FAILURE;
    }

    strcpy(IP_Play_Handles[i].cMultiIPAddr, pMultiAddr);
    IP_Play_Handles[i].u32UdpPort = u32UdpPort;
    IP_Play_Handles[i].u32PortID = u32PortID;
    IP_Play_Handles[i].bStopSocketThread = HI_FALSE;
    pthread_create(&IP_Play_Handles[i].g_SocketThd, HI_NULL, (HI_VOID *)SocketThread, (HI_VOID *)i);
    IP_Play_Handles[i].bUsed = HI_TRUE;

    bIPInjectFlag[u32PortID] = HI_TRUE;

    return HI_SUCCESS;
}

/*****************************************************************************
* Function:      AvStartAvPlay
* Description:   stop AV play
* Data Accessed:
* Data Updated:
* Input:             HI_U16 videopid, HI_U16 audiopid, HI_U16 pcrpid, HI_U16 telpid, HI_U16 subpid
* Output:           None
* Return:          None
* Others: None
*****************************************************************************/
HI_S32  DMX_IPStopInject(HI_U32 u32PortID)
{
    HI_U32 i;

    if ((u32PortID < DMX_DVBPORT_COUNT) || (u32PortID >= DMX_PORT_COUNT))
    {
        return HI_FAILURE;
    }

    if (HI_FALSE == bIPInjectFlag[u32PortID])
    {
        return HI_SUCCESS;
    }

    for (i = 0; i < MAX_IP_THREAD_CNT; i++)
    {
        if ((u32PortID == IP_Play_Handles[i].u32PortID) && (HI_TRUE == IP_Play_Handles[i].bUsed))
        {
            break;
        }
    }

    if (MAX_IP_THREAD_CNT == i)
    {
        return HI_FAILURE;
    }

    IP_Play_Handles[i].bStopSocketThread = HI_TRUE;
    pthread_join(IP_Play_Handles[i].g_SocketThd, HI_NULL);

    IP_Play_Handles[i].bUsed = HI_FALSE;

    bIPInjectFlag[u32PortID] = HI_FALSE;

    return HI_SUCCESS;
}

HI_S32  DMX_FileStartInject(HI_CHAR *path, HI_U32 u32PortID) //flag = 0 TS:flag =1 :ES
{
    HI_S32 i, s32Ret;
    FILE *pTsFile;

    if (HI_TRUE == bFileInjectFlag[u32PortID])
    {
        DMX_FileStopInject(u32PortID);
    }

    for (i = 0; i < MAX_FILE_THREAD_CNT; i++)
    {
        if (HI_FALSE == FILE_Play_Handles[i].bUsed)
        {
            break;
        }
    }

    if (MAX_IP_THREAD_CNT == i)
    {
        return HI_FAILURE;
    }

    pTsFile = fopen(path, "rb");
    if (!pTsFile)
    {
        printf("open file %s error!\n", path);
        return HI_FAILURE;
    }

    FILE_Play_Handles[i].g_pTsFile = pTsFile;
    FILE_Play_Handles[i].u32PortID = u32PortID;
    FILE_Play_Handles[i].bStopFileThread = HI_FALSE;
    s32Ret = pthread_create(&FILE_Play_Handles[i].g_FileThd, HI_NULL, (HI_VOID *)FileTsTthread, (HI_VOID *)i);
    if (s32Ret != HI_SUCCESS)
    {
        fclose(pTsFile);
        return HI_FAILURE;
    }

    FILE_Play_Handles[i].bUsed = HI_TRUE;
    bFileInjectFlag[u32PortID] = HI_TRUE;

    return HI_SUCCESS;
}


HI_S32  DMX_FileStopInject(HI_U32 u32PortID)
{
    HI_UNF_AVPLAY_STOP_OPT_S stAVStop;
    HI_S32 i;

    if (HI_FALSE == bFileInjectFlag[u32PortID])
    {
        return HI_SUCCESS;
    }

    for (i = 0; i < MAX_FILE_THREAD_CNT; i++)
    {
        if ((HI_TRUE == FILE_Play_Handles[i].bUsed) && (u32PortID == FILE_Play_Handles[i].u32PortID))
        {
            break;
        }
    }

    FILE_Play_Handles[i].bStopFileThread = HI_TRUE;
    pthread_join(FILE_Play_Handles[i].g_FileThd, HI_NULL);

    fclose(FILE_Play_Handles[i].g_pTsFile);
    FILE_Play_Handles[i].bUsed = HI_FALSE;

    stAVStop.enMode = HI_UNF_AVPLAY_STOP_MODE_BLACK;
    stAVStop.u32TimeoutMs = 0;

    bFileInjectFlag[u32PortID] = HI_FALSE;

    return HI_SUCCESS;
}


HI_S32  DMX_EsFileStartInject(HI_CHAR *path, HI_HANDLE avplay, HI_U32 type) //flag = 0 TS:flag =1 :ES
{
    HI_S32 i, s32Ret;
    FILE *pEsFile;

    for (i = 0; i < MAX_FILE_THREAD_CNT; i++)
    {
        if (HI_TRUE == Es_FILE_Play_Handles[i].bUsed && Es_FILE_Play_Handles[i].havplay == avplay)
        {
            DMX_EsFileStopInject(i);
            break;
        }
    }

    for (i = 0; i < MAX_FILE_THREAD_CNT; i++)
    {
        if (HI_FALSE == Es_FILE_Play_Handles[i].bUsed)
        {
            break;
        }
    }

    if (MAX_IP_THREAD_CNT == i)
    {
        return HI_FAILURE;
    }

    pEsFile = fopen(path, "rb");
    if (!pEsFile)
    {
        printf("open file %s error!\n", path);
        return HI_FAILURE;
    }

    Es_FILE_Play_Handles[i].g_pEsFile = pEsFile;
    Es_FILE_Play_Handles[i].havplay = avplay;
    Es_FILE_Play_Handles[i].bStopFileThread = HI_FALSE;
    Es_FILE_Play_Handles[i].type = type;
    s32Ret = pthread_create(&Es_FILE_Play_Handles[i].g_FileThd, HI_NULL, (HI_VOID *)FileEsTthread, (HI_VOID *)i);
    if (s32Ret != HI_SUCCESS)
    {
        fclose(pEsFile);
        return HI_FAILURE;
    }

    Es_FILE_Play_Handles[i].bUsed = HI_TRUE;
    return HI_SUCCESS;
}


HI_S32  DMX_EsFileStopInject(HI_U32 index)
{

    Es_FILE_Play_Handles[index].bStopFileThread = HI_TRUE;
    pthread_join(Es_FILE_Play_Handles[index].g_FileThd, HI_NULL);

    fclose(Es_FILE_Play_Handles[index].g_pEsFile);
    Es_FILE_Play_Handles[index].bUsed = HI_FALSE;

    return HI_SUCCESS;
}
#if 0
HI_S32 HIADP_Demux_Init(HI_U32 DmxPortID,HI_U32 TsPortID)
{
    HI_S32 Ret;
    Ret = HI_UNF_DMX_Init();
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_DMX_Init failed.\n");
        return Ret;
    }

    Ret = HI_UNF_DMX_AttachTSPort(DmxPortID, TsPortID);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_DMX_AttachTSPort failed.\n");
        HI_UNF_DMX_DeInit();
        return Ret;
    }

    return HI_SUCCESS;
}

HI_S32 HIADP_Demux_DeInit(HI_U32 DmxPortID)
{
    HI_S32 Ret;

    Ret = HI_UNF_DMX_DetachTSPort(DmxPortID);
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_DMX_DetachTSPort failed.\n");
        return Ret;
    }

    Ret = HI_UNF_DMX_DeInit();
    if (Ret != HI_SUCCESS)
    {
        printf("call HI_UNF_DMX_AttachTSPort failed.\n");
        return Ret;
    }

    return HI_SUCCESS;
}
#endif
