#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "hi_psi_si.h"
#include "hi_unf_avplay.h"
#include "search.h"
#include "HA.AUDIO.MP3.decode.h"
#include "HA.AUDIO.MP2.decode.h"
#include "HA.AUDIO.AAC.decode.h"
 #include "HA.AUDIO.DOLBYPLUS.decode.h"
#include "HA.AUDIO.DRA.decode.h"
#include "HA.AUDIO.PCM.decode.h"
#include "HA.AUDIO.WMA9STD.decode.h"
#include "HA.AUDIO.AMRNB.codec.h"

PMT_COMPACT_TBLS g_prog_tbl;

/*Get PMT Table*/
HI_S32 GetAllPmtTbl(HI_U32 u32DmxId, PMT_COMPACT_TBLS **pProgTbl)
{
    PAT_TBL         *pat_tbl;
    PMT_TBL         *pmt_tbl;
    HI_S32 i,j;
    HI_S32 s32Ret;
    HI_U32 u32Promnum;
    HI_S32 s32Videonum;
    HI_U32 s32Audionum;
    PROG_INFO        *prog_info;
    PMT_Prog         *pmt_prog;
    PMT_Stream       *pmt_stream;

    g_prog_tbl.prog_num = 0;
    g_prog_tbl.proginfo = HI_NULL_PTR;

    /*Get PAT table*/
    s32Ret = HI_API_PSISI_GetPatTbl(u32DmxId, &pat_tbl, 2000);
    if (s32Ret != HI_SUCCESS)
    {
        printf("PSISI Error: Fail to get ALL PMT table, get PAT table fail!\n");
        return s32Ret;
    }

    g_prog_tbl.prog_num = pat_tbl->ProgNum;
    g_prog_tbl.proginfo = malloc(g_prog_tbl.prog_num * sizeof(PMT_COMPACT_PROGS));
    if (g_prog_tbl.proginfo == HI_NULL_PTR)
    {
        HI_API_PSISI_FreePatTbl();
        printf("PSISI Error: Fail to get ALL PMT table, ALL PMT table alloc fail!\n");
        return HI_FAILURE;
    }

    u32Promnum = 0;

    /*search all PMT*/
    for (i = 0; i < pat_tbl->ProgNum; i++)
    {
        /*get program infor*/
        prog_info = &((PROG_INFO *)(pat_tbl->pat_buf))[i];

        if ((prog_info->ProgID != 0) && (prog_info->ProgPID != 0x1fff))
        {
            s32Ret = HI_API_PSISI_GetPmtTbl(u32DmxId, &pmt_tbl, prog_info->ProgID, 1000);
        }
        else
        {
            /*ProgID = 0 is NIT table,according to 13818-1*/
            continue;
        }

        /*Fail to Get Program*/
        if (s32Ret != HI_SUCCESS)
        {
            printf("TS Error: Fail to get PMT table of program 0x%x!\n", prog_info->ProgID);
            continue;
        }

        pmt_prog   = (PMT_Prog *)(pmt_tbl->pmt_buf);
        pmt_stream = (PMT_Stream *)(pmt_prog->ESDescPtr);

        s32Videonum = 0;
        s32Audionum = 0;

        while (pmt_stream)
        {
            switch (pmt_stream->StreamType)
            {
                /*get video pid*/
            case  VIDEO_STREAM_MPEG1:           /* ISO/IEC 11172-2 Video*/
            case  VIDEO_STREAM_MPEG2:           /* ISO/IEC 13818-2 Video */
                if (s32Videonum < 1)
                {
                    g_prog_tbl.proginfo[u32Promnum].VElementPid = pmt_stream->ElementPid;
                    g_prog_tbl.proginfo[u32Promnum].VideoType = HI_UNF_VCODEC_TYPE_MPEG2;
                }

                s32Videonum++;
                break;

            case VIDEO_STREAM_H264:             /* H.264 | ISO/IEC 14496-10 Video */

                if (s32Videonum < 1)
                {
                    g_prog_tbl.proginfo[u32Promnum].VElementPid = pmt_stream->ElementPid;
                    g_prog_tbl.proginfo[u32Promnum].VideoType = HI_UNF_VCODEC_TYPE_H264;
                }

                s32Videonum++;
                break;

            case VIDEO_STREAM_MPEG4:

                if (s32Videonum < 1)
                {
                    g_prog_tbl.proginfo[u32Promnum].VElementPid = pmt_stream->ElementPid;
                    g_prog_tbl.proginfo[u32Promnum].VideoType = HI_UNF_VCODEC_TYPE_MPEG4;
                }

                s32Videonum++;
                break;

            case  AUDIO_STREAM_MPEG1A:          /* ISO/IEC 11172-3 Audio */
            case  AUDIO_STREAM_MPEG2A:            /* ISO/IEC 13818-3 Audio */

                if (s32Audionum < MAX_AUDIO_TRACK)
                {
                    g_prog_tbl.proginfo[u32Promnum].AElementPid[s32Audionum] = pmt_stream->ElementPid;
                    g_prog_tbl.proginfo[u32Promnum].AudioType[s32Audionum] = HA_AUDIO_ID_MP3;
                }

                s32Audionum++;
                break;

            case AUDIO_STREAM_MPEG2AAC:

                if (s32Audionum < 1)
                {
                    g_prog_tbl.proginfo[u32Promnum].AElementPid[s32Audionum] = pmt_stream->ElementPid;
                    g_prog_tbl.proginfo[u32Promnum].AudioType[s32Audionum] = HA_AUDIO_ID_AAC;
                }

                s32Audionum++;
                break;

             case AUDIO_STREAM_AC3:
                
                if (s32Audionum < 1)
                {
                    g_prog_tbl.proginfo[u32Promnum].AElementPid [s32Audionum]= pmt_stream->ElementPid;
                    g_prog_tbl.proginfo[u32Promnum].AudioType[s32Audionum] = HA_AUDIO_ID_DOLBY_PLUS;
                }

                s32Audionum++;

            default:
                printf("Unsuport stream type %d !!!\n", pmt_stream->StreamType );
                break;
            }

            pmt_stream = pmt_stream->Next;
        }

        /*save audio infor*/
        g_prog_tbl.proginfo[u32Promnum].AElementNum = s32Audionum;
        g_prog_tbl.proginfo[u32Promnum].VElementNum = s32Videonum;

        if ((s32Audionum != 0) || (s32Videonum != 0))
        {
            g_prog_tbl.proginfo[u32Promnum].ProgID = prog_info->ProgID;
            g_prog_tbl.proginfo[u32Promnum].PmtPid = prog_info->ProgPID;
            g_prog_tbl.proginfo[u32Promnum].PcrPid = pmt_prog->PCR_PID;
            u32Promnum++;
        }

        /*release PMT table*/
        HI_API_PSISI_FreePmtTbl();
    }

    g_prog_tbl.prog_num = u32Promnum;

    HI_API_PSISI_FreePatTbl();

    if (g_prog_tbl.prog_num == 0)
    {
        printf("no Program searched!\n");
        return -1;
    }

    printf("\n\nALL Program Infomation:\n");

    for (i = 0; i < g_prog_tbl.prog_num; i++)
    {
        printf("Channum = %d, Program ID = %d, Channel Num = %d\n", u32DmxId, g_prog_tbl.proginfo[i].ProgID, i + 1);

        for ( j = 0 ; j < g_prog_tbl.proginfo[i].AElementNum ; j++ )
        {
            printf("\tAudio Stream PID	 = 0x%x\n",g_prog_tbl.proginfo[i].AElementPid[j]);
            switch (g_prog_tbl.proginfo[i].AudioType[j])
            {
            case HA_AUDIO_ID_MP3:
                printf("\tAudio Stream Type MP3\n");
                break;
            case HA_AUDIO_ID_AAC:
                printf("\tAudio Stream Type AAC\n");
                break;
            case HA_AUDIO_ID_DOLBY_PLUS:
                printf("\tAudio Stream Type AC3\n");
                break;
            default:
                printf("\tAudio Stream Type error\n");
            }
        }

        if (g_prog_tbl.proginfo[i].VElementNum > 0)
        {
            printf("\tVideo Stream PID	 = 0x%x\n",g_prog_tbl.proginfo[i].VElementPid);
            switch (g_prog_tbl.proginfo[i].VideoType)
            {
            case HI_UNF_VCODEC_TYPE_H264:
                printf("\tVideo Stream Type H264\n");
                break;
            case HI_UNF_VCODEC_TYPE_MPEG2:
                printf("\tVideo Stream Type MP2\n");
                break;
            case HI_UNF_VCODEC_TYPE_MPEG4:
                printf("\tVideo Stream Type MP4\n");
                break;
            default:
                printf("\tVideo Stream Type error\n");
            }
        }
    }

    printf("\n");

    *pProgTbl = &g_prog_tbl;

    return HI_SUCCESS;
}

HI_VOID FreeAllPmtTbl(HI_VOID)
{
    g_prog_tbl.prog_num = 0;
    free(g_prog_tbl.proginfo);
    g_prog_tbl.proginfo = HI_NULL_PTR;
}

