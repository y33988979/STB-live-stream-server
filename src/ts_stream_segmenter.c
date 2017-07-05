
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "hi_type.h"
#include "hi_unf_common.h"
#include "hi_unf_demux.h"
#include "hi_unf_ecs.h"
#include "hi_adp_mpi.h"
#include "hi_adp_tuner.h"

#define DEMUX_ID        0
#define PORT_ID         DEFAULT_DVB_PORT

#define PAT_PID         0
#define PAT_TABLEID     0

#define INVALID_PID     0x1FFF
//#define USE_ORIGINAL_SCD

PMT_COMPACT_TBL    *g_pProgTbl = HI_NULL;

#include <errno.h>
#define TS_PKT_SIZE     188
#define HLS_SPLIT_TS
#define REMOVE_INVALID_TSPKT
typedef struct _TS_Packet
{
    HI_U32     total_count;
    HI_U32     total_size;
    HI_U32     valid_count;
    HI_U32     valid_size;
    HI_U8     *ts_data[512];

}TS_List_t;

struct options_t {
    const char *input_file;
    long segment_duration;
    const char *output_prefix;
    const char *m3u8_file;
    char *tmp_m3u8_file;
    const char *url_prefix;
    long num_segments;
    int playlist_num;
    int tsfile_cache_num;
};

struct options_t g_options =
{
    "NULL",  /* input_file */
    5,    /* segment_duration */
    "ts", /* output_prefix */
    "/tmp/hls/1.m3u8", /* m3u8_file */
    "/tmp/hls/tmp.m3u8", /* tmp_m3u8_file */
    "", /* url_prefix */
    2,  /* num_segments */
    3,  /* playlist_num */
    6   /* tsfile_cache_num */
};

TS_List_t g_ts_pkt_list;
/*
1、锁定指定频点
2、搜索PAT表
3、搜索CAT及PMT表
4、解析PMT表得到ECM PID值，解析CAT表得到EMM PID值
5、将ECM及EMM送到智能卡解密
6、智能卡返回CW值，机顶盒将CW值设到demux descrambler解扰模块，还原原始码流
7、设定音频pid，视频pid进行解码和播放
*/
typedef struct
{
    HI_HANDLE   RecHandle;
    HI_CHAR     FileName[256];
    HI_BOOL     ThreadRunFlag;
} TsFileInfo;

#ifdef USE_ORIGINAL_SCD
typedef struct hiDMX_DATA_S
{
    HI_U8   *pDataAddr;
    HI_U32   u32PhyAddr;
    HI_U32   u32Len;
} DMX_DATA_S;
extern HI_S32 HI_MPI_DMX_AcquireRecScdBuf(HI_U32 u32DmxId, DMX_DATA_S *pstBuf, HI_U32 u32TimeoutMs);
extern HI_S32 HI_MPI_DMX_ReleaseRecScdBuf(HI_U32 u32DmxId, const DMX_DATA_S *pstBuf);
#endif

FILE* hls_file_open(char *filename, int flag)
{
    FILE *fp = NULL;
    fp = fopen(filename, "w");
    if (!fp)
    {
        perror("fopen error");
        return HI_NULL;
    }
    printf("[%s] open file %s\n", __FUNCTION__, filename);
    return fp;
}

int write_index_file(const struct options_t *options, const HI_U32 first_segment, const HI_U32 last_segment, const int end) {
    FILE *index_fp;
    char write_buf[1024];
    unsigned int i;

    index_fp = fopen(options->tmp_m3u8_file, "w");
    if (!index_fp) {
        fprintf(stderr, "Could not open temporary m3u8 index file (%s), no index file will be created\n", options->tmp_m3u8_file);
        return -1;
    }

    if (options->num_segments) {
        snprintf(write_buf, 1024, "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:%lu\n#EXT-X-MEDIA-SEQUENCE:%u\n", 15, first_segment);
    }
    else {
        snprintf(write_buf, 1024, "#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:%lu\n", options->segment_duration);
    }
    if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
        fprintf(stderr, "Could not write to m3u8 index file, will not continue writing to index file\n");
        fclose(index_fp);
        return -1;
    }

    for (i = first_segment; i <= last_segment; i++) {
        snprintf(write_buf, 1024, "#EXTINF:%lu,\n%s%s-%u.ts\n", \
            options->segment_duration, options->url_prefix, options->output_prefix, i);
        if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
            fprintf(stderr, "Could not write to m3u8 index file, will not continue writing to index file\n");
            fclose(index_fp);
            return -1;
        }
    }

    if (end) {
        snprintf(write_buf, 1024, "#EXT-X-ENDLIST\n");
        if (fwrite(write_buf, strlen(write_buf), 1, index_fp) != 1) {
            fprintf(stderr, "Could not write last file and endlist tag to m3u8 index file\n");
            fclose(index_fp);
            return -1;
        }
    }

    fclose(index_fp);

    return rename(options->tmp_m3u8_file, options->m3u8_file);
}

TS_List_t* GetAllValidTsPkt(HI_U8 *data, HI_U32 datalen)
{
    HI_S32 i, j;
    HI_S32 pid;
    HI_U8 *pdata;
    
    TS_List_t *ts_pkt_list = &g_ts_pkt_list;
    
    /* data filter , remove ts packet which pid is 0x1FFF */
    memset(ts_pkt_list, 0, sizeof(TS_List_t));

    if(datalen%TS_PKT_SIZE != 0)
    {
        printf("[%s] HI_UNF_DMX_AcquireRecData datalen=%d is not multiple of 188.\n", __FUNCTION__, datalen);
        return NULL;
    }

    j = 0;
    ts_pkt_list->valid_count = 0;
    ts_pkt_list->total_count = datalen/TS_PKT_SIZE;
    for(i=0; i<ts_pkt_list->total_count; i++){
        pdata = data + i*TS_PKT_SIZE;
        pid = pdata[1]<<8 | pdata[2];
        //printf("pid=0x%x\n", pid);
        if(pid != INVALID_PID){
            ts_pkt_list->ts_data[j++] = pdata;
            ts_pkt_list->valid_count++;
        }
    }

    ts_pkt_list->total_size = datalen;
    ts_pkt_list->valid_size = ts_pkt_list->valid_count*TS_PKT_SIZE;

    return ts_pkt_list;
}

HI_VOID* SaveRecDataThread(HI_VOID *arg)
{
    HI_S32      ret;
    TsFileInfo *Ts      = (TsFileInfo*)arg;
    FILE       *RecFile = HI_NULL;

    HI_S32     i, j, pid = 0;
    HI_S32     file_cnt = 0;
    HI_S32     output_index;
    HI_U32     first_segment = 1;
    HI_U32     last_segment = 3;
    HI_U32     file_size = 0;
    HI_U8     *pdata;
    HI_CHAR    ts_filename[256] = {0};
    TS_List_t *ts_pkt_list = &g_ts_pkt_list;
    struct options_t *options = &g_options;

    output_index = 1;
    sprintf(ts_filename, "/tmp/hls/%s-%u.ts", options->output_prefix, output_index++);

    RecFile = fopen(ts_filename, "w");
    if (!RecFile)
    {
        perror("fopen error");

        Ts->ThreadRunFlag = HI_FALSE;

        return HI_NULL;
    }

    printf("[%s] open file %s\n", __FUNCTION__, ts_filename);

    while (Ts->ThreadRunFlag)
    {
        #ifndef USE_ORIGINAL_SCD
        HI_UNF_DMX_REC_DATA_S RecData;

        ret = HI_UNF_DMX_AcquireRecData(Ts->RecHandle, &RecData, 100);
        #else
        DMX_DATA_S RecData;
        ret = HI_MPI_DMX_AcquireRecScdBuf(DEMUX_ID, &RecData, 1000);
        #endif
        if (HI_SUCCESS != ret)
        {
            if (HI_ERR_DMX_TIMEOUT == ret)
            {
                continue;
            }

            if (HI_ERR_DMX_NOAVAILABLE_DATA == ret)
            {
                continue;
            }

            printf("[%s] HI_UNF_DMX_AcquireRecData failed 0x%x\n", __FUNCTION__, ret);

            break;
        }

        #ifdef HLS_SPLIT_TS
        #ifdef REMOVE_INVALID_TSPKT

        /* data filter , remove ts packet which pid is 0x1FFF */
        ts_pkt_list = GetAllValidTsPkt(RecData.pDataAddr, RecData.u32Len);
        if(!ts_pkt_list)
            continue;
        
        for(i=0; i<ts_pkt_list->valid_count; i++){
            if (TS_PKT_SIZE != fwrite(ts_pkt_list->ts_data[i], 1, TS_PKT_SIZE, RecFile))
            {
                perror("[SaveRecDataThread1] fwrite error");
                break;
            }
        }
        file_size += ts_pkt_list->valid_size;
        
        #else//REMOVE_INVALID_TSPKT
        
        if (RecData.u32Len != fwrite(RecData.pDataAddr, 1, RecData.u32Len, RecFile))
        {
            perror("[SaveRecDataThread] fwrite error");

            break;
        }
        file_size += RecData.u32Len;
        #endif 

        /* Create new file */
        if(file_size >= 0x300000) {

            fclose(RecFile);
            file_size = 0;
            file_cnt++;
            
            if(file_cnt >= options->playlist_num)
                write_index_file(options, first_segment++, last_segment++, 0);
            if(file_cnt >= options->tsfile_cache_num)
            {
                sprintf(ts_filename, "/tmp/hls/%s-%u.ts", options->output_prefix, output_index-options->tsfile_cache_num-1);
                unlink(ts_filename);
            }

            sprintf(ts_filename, "/tmp/hls/%s-%u.ts", options->output_prefix, output_index++);
            RecFile = fopen(ts_filename, "w");
            if (!RecFile)
            {
                fprintf(stderr, "fopen error! filename=%s ,errno=%d:%s\n", \
                ts_filename, errno, strerror(errno));
                exit(1);
            }
            printf("[%s] open file %s\n", __FUNCTION__, ts_filename);
        }

        //printf("save data: total_len=%d, valid_len=%d\n", \
        //ts_pkt_list->total_count*TS_PKT_SIZE, ts_pkt_list->valid_count*TS_PKT_SIZE);
        
        #else //HLS_SPLIT_TS

        if (RecData.u32Len != fwrite(RecData.pDataAddr, 1, RecData.u32Len, RecFile))
        {
            perror("[SaveRecDataThread] fwrite error");

            break;
        }
        #endif //HLS_SPLIT_TS

        #ifndef USE_ORIGINAL_SCD
        ret = HI_UNF_DMX_ReleaseRecData(Ts->RecHandle, &RecData);
        #else
        ret = HI_MPI_DMX_ReleaseRecScdBuf(DEMUX_ID, &RecData);
        #endif
        if (HI_SUCCESS != ret)
        {
            printf("[%s] HI_UNF_DMX_ReleaseRecData failed 0x%x\n", __FUNCTION__, ret);

            break;
        }

    }

    fclose(RecFile);

    Ts->ThreadRunFlag = HI_FALSE;

    return HI_NULL;
}

HI_VOID* SaveIndexDataThread(HI_VOID *arg)
{
    HI_S32                  ret;
    TsFileInfo             *Ts          = (TsFileInfo*)arg;
    FILE                   *IndexFile   = HI_NULL;
    HI_U32                  IndexCount  = 0;
    HI_UNF_DMX_REC_INDEX_S  RecIndex[64];

    IndexFile = fopen(Ts->FileName, "w");
    if (!IndexFile)
    {
        perror("fopen error");

        Ts->ThreadRunFlag = HI_FALSE;

        return HI_NULL;
    }

    printf("[%s] open file %s\n", __FUNCTION__, Ts->FileName);

    while (Ts->ThreadRunFlag)
    {
        ret = HI_UNF_DMX_AcquireRecIndex(Ts->RecHandle, &RecIndex[IndexCount], 100);
        if (HI_SUCCESS != ret)
        {
            if ((HI_ERR_DMX_NOAVAILABLE_DATA == ret) || (HI_ERR_DMX_TIMEOUT == ret))
            {
                continue;
            }

            printf("[%s] HI_UNF_DMX_AcquireRecIndex failed 0x%x\n", __FUNCTION__, ret);

            break;
        }

        if (++IndexCount >= (sizeof(RecIndex) / sizeof(RecIndex[0])))
        {
            IndexCount = 0;

            if (sizeof(RecIndex) != fwrite(&RecIndex, 1, sizeof(RecIndex), IndexFile))
            {
                perror("[SaveIndexDataThread] fwrite error");

                break;
            }

            fflush(IndexFile);
        }
    }

    fclose(IndexFile);

    Ts->ThreadRunFlag = HI_FALSE;

    return HI_NULL;
}

HI_S32 DmxStartRecord(HI_CHAR *Path, PMT_COMPACT_PROG *ProgInfo)
{
    HI_S32                  ret;
    HI_UNF_DMX_REC_ATTR_S   RecAttr;
    HI_HANDLE               RecHandle;
    HI_HANDLE               ChanHandle[8];
    HI_U32                  ChanCount       = 0;
    HI_BOOL                 RecordStatus    = HI_FALSE;
    pthread_t               RecThreadId     = -1;
    pthread_t               IndexThreadId   = -1;
    TsFileInfo              TsRecInfo;
    TsFileInfo              TsIndexInfo;
    HI_CHAR                 FileName[256];
    HI_U32                  i;

    RecAttr.u32DmxId        = DEMUX_ID;
    RecAttr.u32RecBufSize   = 4 * 1024 * 1024;
    RecAttr.enRecType       = HI_UNF_DMX_REC_TYPE_SELECT_PID;
    RecAttr.bDescramed      = HI_TRUE;

    if (ProgInfo->VElementPid < INVALID_PID)
    {
        RecAttr.enIndexType     = HI_UNF_DMX_REC_INDEX_TYPE_VIDEO;
        RecAttr.u32IndexSrcPid  = ProgInfo->VElementPid;
        RecAttr.enVCodecType    = HI_UNF_VCODEC_TYPE_MPEG2;
    }
    else if (ProgInfo->AElementPid < INVALID_PID)
    {
        RecAttr.enIndexType     = HI_UNF_DMX_REC_INDEX_TYPE_AUDIO;
        RecAttr.u32IndexSrcPid  = ProgInfo->AElementPid;
    }
    else
    {
        RecAttr.enIndexType     = HI_UNF_DMX_REC_INDEX_TYPE_NONE;
    }

    ret = HI_UNF_DMX_CreateRecChn(&RecAttr, &RecHandle);
    if (HI_SUCCESS != ret)
    {
		printf("[%s - %u] HI_UNF_DMX_CreateRecChn failed 0x%x\n", __FUNCTION__, __LINE__, ret);

        return ret;;
    }

    ret = HI_UNF_DMX_AddRecPid(RecHandle, PAT_PID, &ChanHandle[ChanCount]);
    if (HI_SUCCESS != ret)
    {
		printf("[%s - %u] HI_UNF_DMX_AddRecPid failed 0x%x\n", __FUNCTION__, __LINE__, ret);

        goto exit;
    }

    ++ChanCount;

    ret = HI_UNF_DMX_AddRecPid(RecHandle, ProgInfo->PmtPid, &ChanHandle[ChanCount]);
    if (HI_SUCCESS != ret)
    {
		printf("[%s - %u] HI_UNF_DMX_AddRecPid failed 0x%x\n", __FUNCTION__, __LINE__, ret);

        goto exit;
    }

    ++ChanCount;

    sprintf(FileName, "%s/rec", Path);

    if (ProgInfo->VElementPid < INVALID_PID)
    {
        ret = HI_UNF_DMX_AddRecPid(RecHandle, ProgInfo->VElementPid, &ChanHandle[ChanCount]);
        if (HI_SUCCESS != ret)
        {
    		printf("[%s - %u] HI_UNF_DMX_AddRecPid failed 0x%x\n", __FUNCTION__, __LINE__, ret);

            goto exit;
        }

        sprintf(FileName, "%s_v%u", FileName, ProgInfo->VElementPid);

        ++ChanCount;
    }

    if (ProgInfo->AElementPid < INVALID_PID)
    {
        ret = HI_UNF_DMX_AddRecPid(RecHandle, ProgInfo->AElementPid, &ChanHandle[ChanCount]);
        if (HI_SUCCESS != ret)
        {
    		printf("[%s - %u] HI_UNF_DMX_AddRecPid failed 0x%x\n", __FUNCTION__, __LINE__, ret);

            goto exit;
        }

        sprintf(FileName, "%s_a%u", FileName, ProgInfo->AElementPid);

        ++ChanCount;
    }

    if (ProgInfo->PcrPid < INVALID_PID)
    {
        ret = HI_UNF_DMX_AddRecPid(RecHandle, ProgInfo->PcrPid, &ChanHandle[ChanCount]);
        if (HI_SUCCESS != ret)
        {
    		printf("[%s - %u] HI_UNF_DMX_AddRecPid failed 0x%x\n", __FUNCTION__, __LINE__, ret);

            goto exit;
        }

        ++ChanCount;
    }

    ret = HI_UNF_DMX_StartRecChn(RecHandle);
    if (HI_SUCCESS != ret)
    {
        printf("[%s - %u] HI_UNF_DMX_StartRecChn failed 0x%x\n", __FUNCTION__, __LINE__, ret);

        goto exit;
    }

    RecordStatus = HI_TRUE;

    TsRecInfo.RecHandle     = RecHandle;
    TsRecInfo.ThreadRunFlag = HI_TRUE;
    sprintf(TsRecInfo.FileName, "%s.ts", FileName);

    ret = pthread_create(&RecThreadId, HI_NULL, SaveRecDataThread, (HI_VOID*)&TsRecInfo);
    if (0 != ret)
    {
        perror("[DmxStartRecord] pthread_create record error");

        goto exit;
    }

    TsIndexInfo.RecHandle       = RecHandle;
    TsIndexInfo.ThreadRunFlag   = HI_TRUE;
    sprintf(TsIndexInfo.FileName, "%s.ts.idx", FileName);

    ret = pthread_create(&IndexThreadId, HI_NULL, SaveIndexDataThread, (HI_VOID*)&TsIndexInfo);
    if (0 != ret)
    {
        perror("[DmxStartRecord] pthread_create index error");

        goto exit;
    }

    sleep(1);

    while (1)
    {
        HI_CHAR InputCmd[256]   = {0};

        printf("please input the q to quit!\n");

        SAMPLE_GET_INPUTCMD(InputCmd);
        if ('q' == InputCmd[0])
        {
            break;
        }
    }

exit :
    if (-1 != RecThreadId)
    {
        TsRecInfo.ThreadRunFlag = HI_FALSE;
        pthread_join(RecThreadId, HI_NULL);
    }

    if (-1 != IndexThreadId)
    {
        TsIndexInfo.ThreadRunFlag = HI_FALSE;
        pthread_join(IndexThreadId, HI_NULL);
    }

    if (RecordStatus)
    {
        ret = HI_UNF_DMX_StopRecChn(RecHandle);
        if (HI_SUCCESS != ret)
        {
            printf("[%s - %u] HI_UNF_DMX_StopRecChn failed 0x%x\n", __FUNCTION__, __LINE__, ret);
        }
    }

    for (i = 0; i < ChanCount; i++)
    {
        ret = HI_UNF_DMX_DelRecPid(RecHandle, ChanHandle[i]);
        if (HI_SUCCESS != ret)
        {
            printf("[%s - %u] HI_UNF_DMX_DelRecPid failed 0x%x\n", __FUNCTION__, __LINE__, ret);
        }
    }

    ret = HI_UNF_DMX_DestroyRecChn(RecHandle);
    if (HI_SUCCESS != ret)
    {
        printf("[%s - %u] HI_UNF_DMX_DestroyRecChn failed 0x%x\n", __FUNCTION__, __LINE__, ret);
    }

    return ret;
}

HI_S32 main(HI_S32 argc, HI_CHAR *argv[])
{
	HI_S32      ret;
    HI_CHAR    *Path            = HI_NULL;
    HI_U32      Freq            = 610;
    HI_U32      SymbolRate      = 6875;
    HI_U32      ThridParam      = 64;
    HI_U32      ProgNum         = 0;
    HI_CHAR     InputCmd[256]   = {0};

    if (argc < 3)
    {
        printf( "Usage: %s path freq [srate] [qamtype or polarization]\n"
                "       qamtype or polarization: \n"
                "           For cable, used as qamtype, value can be 16|32|[64]|128|256|512 defaut[64] \n"
                "           For satellite, used as polarization, value can be [0] horizontal | 1 vertical defaut[0] \n",
                argv[0]);

        return -1;
    }

    Path        = argv[1];
    Freq        = strtol(argv[2], NULL, 0);
    SymbolRate  = (Freq > 1000) ? 27500 : 6875;
    ThridParam  = (Freq > 1000) ? 0 : 64;

    if (argc >= 4)
    {
        SymbolRate  = strtol(argv[3], NULL, 0);
    }

    if (argc >= 5)
    {
        ThridParam = strtol(argv[4], NULL, 0);
    }


	HI_SYS_Init();

    HIADP_MCE_Exit();

#if 0 //add by ychen
	ret = HI_UNF_DMX_Init();
    HI_UNF_DMX_PORT_ATTR_S Attr;
    HI_UNF_DMX_GetTSPortAttr(PORT_ID,&Attr);
    Attr.enPortType = HI_UNF_DMX_PORT_TYPE_SERIAL;
    Attr.u32SerialBitSelector = 0;
    printf("[ychen]: portId==%d, demuxId==%d\n", PORT_ID, DEMUX_ID);
    HI_UNF_DMX_SetTSPortAttr(PORT_ID+0x20+0,&Attr);
    HI_UNF_DMX_SetTSPortAttr(PORT_ID+0x20+1,&Attr);
    HI_UNF_DMX_SetTSPortAttr(PORT_ID+0x20+2,&Attr);
    HI_UNF_DMX_SetTSPortAttr(PORT_ID+0x20+3,&Attr);
    ret = HI_UNF_DMX_AttachTSPort(DEMUX_ID, PORT_ID+0x20+1);
    printf("[ychen]: portId==%d, demuxId==%d\n", PORT_ID, DEMUX_ID);
    HI_UNF_DMX_GetTSPortAttr(PORT_ID,&Attr);
    printf("[ychen]: HI_UNF_DMX_PORT_TYPE_SERIAL=%d",HI_UNF_DMX_PORT_TYPE_SERIAL);
    printf("[ychen]: portAttr: enPortType=%d, u32SerialBitSelector=%d\n",\
            Attr.enPortType, Attr.u32SerialBitSelector );
#endif
    ret = HIADP_Tuner_Init();
    if (HI_SUCCESS != ret)
    {
        printf("call HIADP_Demux_Init failed.\n");
        goto TUNER_DEINIT;
    }

    ret = HIADP_Tuner_Connect(PORT_ID, Freq, SymbolRate, ThridParam);
    if (HI_SUCCESS != ret)
    {
        printf("call HIADP_Tuner_Connect failed.\n");
        goto TUNER_DEINIT;
    }

	ret = HI_UNF_DMX_Init();
	if (HI_SUCCESS != ret)
	{
		printf("HI_UNF_DMX_Init failed 0x%x\n", ret);
		goto TUNER_DEINIT;
	}
#if 1 //add by ychen
    HI_UNF_DMX_PORT_ATTR_S Attr;
    HI_UNF_DMX_GetTSPortAttr(PORT_ID,&Attr);
    Attr.enPortType = HI_UNF_DMX_PORT_TYPE_SERIAL;
    Attr.u32SerialBitSelector = 0;
    printf("[ychen]: portId==%d, demuxId==%d\n", PORT_ID, DEMUX_ID);
    HI_UNF_DMX_SetTSPortAttr(PORT_ID+0x20+0,&Attr);
    HI_UNF_DMX_SetTSPortAttr(PORT_ID+0x20+1,&Attr);
    HI_UNF_DMX_SetTSPortAttr(PORT_ID+0x20+2,&Attr);
    HI_UNF_DMX_SetTSPortAttr(PORT_ID+0x20+3,&Attr);
#endif
    ret = HI_UNF_DMX_AttachTSPort(DEMUX_ID, PORT_ID+0x20+1);
	if (HI_SUCCESS != ret)
	{
		printf("HI_UNF_DMX_AttachTSPort failed 0x%x\n", ret);
		goto DMX_DEINIT;
	}

    HIADP_Search_Init();

    ret = HIADP_Search_GetAllPmt(DEMUX_ID, &g_pProgTbl);
    if (HI_SUCCESS != ret)
    {
        printf("call HIADP_Search_GetAllPmt failed\n");
        goto PSISI_FREE;
    }

    printf("\nPlease input the number of program to record: ");

    SAMPLE_GET_INPUTCMD(InputCmd);

    ProgNum = (atoi(InputCmd) - 1) % g_pProgTbl->prog_num;

    DmxStartRecord(Path, g_pProgTbl->proginfo + ProgNum);

PSISI_FREE:
    HIADP_Search_FreeAllPmt(g_pProgTbl);
    HIADP_Search_DeInit();

DMX_DEINIT:
    HI_UNF_DMX_DetachTSPort(DEMUX_ID);
    HI_UNF_DMX_DeInit();

TUNER_DEINIT:
	HIADP_Tuner_DeInit();
	HI_SYS_DeInit();

	return ret;
}


