#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <string.h>   /* for NULL */
#include <unistd.h>

#include "hi_adp.h"
#include "hi_adp_tuner.h"
#include "hi_adp_boardcfg.h"
#include "hi_unf_frontend.h"
#include "hi_unf_demux.h"

/********************************* TUNER public function*******************************************/

HI_S32 HIADP_Tuner_Init(HI_VOID)
{
    HI_S32                          Ret;
    HI_UNF_TUNER_ATTR_S             TunerAttr;
    HI_UNF_TUNER_FE_LNB_CONFIG_S    LnbConfig;
    HI_U32                          i;

    /*Initialize Tuner*/
    Ret = HI_UNF_TUNER_Init();
    if (HI_SUCCESS != Ret)
    {
        printf("[%s] HI_UNF_TUNER_Init failed 0x%x\n", __FUNCTION__, Ret);
        return Ret;
    }

    for (i = 0; i < HI_TUNER_NUMBER; i++)
    {
        /*Open Tuner*/
        Ret = HI_UNF_TUNER_Open(i);
        if (HI_SUCCESS != Ret)
        {
            printf("[%s] HI_UNF_TUNER_Open failed 0x%x\n", __FUNCTION__, Ret);
            break;
        }

        /*get default attribute in order to set attribute next*/
        Ret = HI_UNF_TUNER_GetDeftAttr(i, &TunerAttr);
        if (HI_SUCCESS != Ret)
        {
            printf("[%s] HI_UNF_TUNER_GetDeftAttr failed 0x%x\n", __FUNCTION__, Ret);
            break;
        }

        HIADP_TUNER_GET_CONFIG(i, TunerAttr);

        printf("[%s] TunerId=%u, TunerType=%u, TunerAddr=0x%x, I2cChannel=%u, DemodAddr=0x%x, DemodType=%u\n",
            __FUNCTION__, i, TunerAttr.enTunerDevType, TunerAttr.u32TunerAddr,
            TunerAttr.enI2cChannel, TunerAttr.u32DemodAddr, TunerAttr.enDemodDevType);
        #if 1 //add by ychen
        TunerAttr.enSigType = HI_UNF_TUNER_SIG_TYPE_CAB;
        TunerAttr.enTunerDevType = HI_UNF_TUNER_DEV_TYPE_MXL214;
        TunerAttr.u32TunerAddr = 0xa0;
        TunerAttr.enDemodDevType = HI_UNF_DEMOD_DEV_TYPE_MXL254;
        TunerAttr.u32DemodAddr = 0xa0;
        TunerAttr.enOutputMode = HI_UNF_TUNER_OUTPUT_MODE_SERIAL;
        TunerAttr.enI2cChannel = 1;
        TunerAttr.u32ResetGpioNo = 76;
        printf("[ychen]: set tuner MODE_SERIAL!\n");
        #endif

        Ret = HI_UNF_TUNER_SetAttr(i, &TunerAttr);
        if (HI_SUCCESS != Ret)
        {
            printf("[%s] HI_UNF_TUNER_SetAttr failed 0x%x\n", __FUNCTION__, Ret);
            break;
        }

        /* Default use dual freqency, C band, 5150/5750 */
        if (HI_UNF_TUNER_SIG_TYPE_SAT == TunerAttr.enSigType)
        {
            LnbConfig.enLNBType = HI_UNF_TUNER_FE_LNB_DUAL_FREQUENCY;
            LnbConfig.enLNBBand = HI_UNF_TUNER_FE_LNB_BAND_C;
            LnbConfig.u32LowLO = 5150;
            LnbConfig.u32HighLO = 5750;
            HI_UNF_TUNER_SetLNBConfig(i, &LnbConfig);
        }

    #ifdef GET_TUNER_TSOUT_CONFIG
        {
            HI_UNF_TUNER_TSOUT_SET_S stTSOut;

            GET_TUNER_TSOUT_CONFIG(stTSOut);
            Ret = HI_UNF_TUNER_SetTSOUT(i, &stTSOut);
            if (HI_SUCCESS != Ret)
            {
                printf("[%s] HI_UNF_TUNER_SetTSOUT failed 0x%x\n", __FUNCTION__, Ret);
                break;
            }
        }
    #endif
    }

    if (HI_SUCCESS != Ret)
    {
        for (i = 0; i < HI_TUNER_NUMBER; i++)
        {
            HI_UNF_TUNER_Close(i);
        }

        HI_UNF_TUNER_DeInit();
    }

    return Ret;
}

/* Freq:MHZ  SymbolRate:KHZ  */
HI_S32 HIADP_Tuner_Connect(HI_U32 TunerID, HI_U32 Freq, HI_U32 SymbolRate, HI_U32 ThirdParam)
{
    HI_UNF_TUNER_CONNECT_PARA_S  ConnectPara;

    /* DVB-S/S2 demod: AVL6211 / 3136 */
    if (Freq > 1000)
    {
        ConnectPara.enSigType = HI_UNF_TUNER_SIG_TYPE_SAT;
        if (ThirdParam >= HI_UNF_TUNER_FE_POLARIZATION_BUTT)
        {
            ThirdParam = HI_UNF_TUNER_FE_POLARIZATION_H;
        }
        ConnectPara.unConnectPara.stSat.enPolar = ThirdParam;
    }
    /* DVB-C demod */
    else
    {
        ConnectPara.enSigType = HI_UNF_TUNER_SIG_TYPE_CAB;
        ConnectPara.unConnectPara.stCab.bReverse = 0;

        switch (ThirdParam)
        {
            case 16 :
                ConnectPara.unConnectPara.stCab.enModType = HI_UNF_MOD_TYPE_QAM_16;
                break;
            case 32 :
                ConnectPara.unConnectPara.stCab.enModType = HI_UNF_MOD_TYPE_QAM_32;
                break;
            case 64 :
                ConnectPara.unConnectPara.stCab.enModType = HI_UNF_MOD_TYPE_QAM_64;
                break;
            case 128 :
                ConnectPara.unConnectPara.stCab.enModType = HI_UNF_MOD_TYPE_QAM_128;
                break;
            case 256 :
                ConnectPara.unConnectPara.stCab.enModType = HI_UNF_MOD_TYPE_QAM_256;
                break;
            case 512 :
                ConnectPara.unConnectPara.stCab.enModType = HI_UNF_MOD_TYPE_QAM_512;
                break;
            default:
                ConnectPara.unConnectPara.stCab.enModType = HI_UNF_MOD_TYPE_QAM_64;
        }
    }

    ConnectPara.unConnectPara.stSat.u32Freq = Freq * 1000;
    ConnectPara.unConnectPara.stSat.u32SymbolRate = SymbolRate * 1000;

    /* connect Tuner*/
    return HI_UNF_TUNER_Connect(TunerID, &ConnectPara, 500);
}

HI_VOID HIADP_Tuner_DeInit(HI_VOID)
{
    HI_U32 i;

    for (i = 0; i < HI_TUNER_NUMBER; i++)
    {
        HI_UNF_TUNER_Close(i);
    }

    HI_UNF_TUNER_DeInit();
}

