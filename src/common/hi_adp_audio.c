/**
 \file
 \brief common function of PVR
 \copyright Shenzhen Hisilicon Co., Ltd.
 \date 2008-2018
 \version draft
 \author z40717
 \date 2010-6-12
 */

#include "hi_unf_gpio.h"
#include "hi_adp_audio.h"
#include "hi_adp_mpi.h"
#include "hi_unf_sio.h"

/* BEGIN AIAO_Init: */
static int g_fdDma = -1;
#define DMA_FILE "/dev/dma"
static HI_S32 HIADP_Dma_Open(HI_VOID)
{
    g_fdDma = open(DMA_FILE, O_RDWR);
    if (g_fdDma < 0)
    {
        printf("can't open DMA\n");
        return -1;
    }

    return 0;
}

static HI_VOID HIADP_Dma_Close(HI_VOID)
{
    if (g_fdDma > 0)
    {
        close(g_fdDma);
    }
}


#ifdef TLV320_AUDIO_DEVICE_ENABLE
static int g_fdTLV320AIC31 = -1;
 #define TLV320AIC31_FILE "/dev/tlv320aic31"
 #define     TLV320_SET_I2S_MODE 0x35
typedef struct
{
    unsigned int chip_num      : 3;
    unsigned int audio_in_out  : 2;
    unsigned int if_mute_route : 1;
    unsigned int if_powerup    : 1;
    unsigned int input_level   : 7;
    unsigned int sample        : 4;
    unsigned int data_length   : 2;
    unsigned int ctrl_mode     : 1;
    unsigned int dac_path      : 2;
    unsigned int trans_mode    : 2;
    unsigned int i2s_mode      : 2;
    unsigned int i2s_master    : 1;
    unsigned int reserved      : 3;
} Audio_Ctrl;

static HI_S32 HIADP_TLV320_Open(HI_U32 I2S_Mode)
{
    HI_S32 s32Ret;
    Audio_Ctrl stTlvAic31Codec_Ctrl;

	AudioSlicTlv320RST();
	AudioSPIPinSharedEnable();
    g_fdTLV320AIC31 = open(TLV320AIC31_FILE, O_RDWR);
    if (g_fdTLV320AIC31 < 0)
    {
        printf("can't open AIC31\n");
        return -1;
    }

    if ((I2S_Mode == AIO_MODE_I2S_SLAVE) || (I2S_Mode == AIO_MODE_I2S_MASTER))
    {
        stTlvAic31Codec_Ctrl.i2s_mode = 0;
    }
    else if ((I2S_Mode == AIO_MODE_PCM_SLAVE_STD) || (I2S_Mode == AIO_MODE_PCM_MASTER_STD))
    {
        stTlvAic31Codec_Ctrl.i2s_mode = 1;
    }
    else if ((I2S_Mode == AIO_MODE_PCM_SLAVE_NSTD) || (I2S_Mode == AIO_MODE_PCM_MASTER_NSTD))
    {
        stTlvAic31Codec_Ctrl.i2s_mode = 2;
    }

    if ((I2S_Mode == AIO_MODE_I2S_SLAVE) || (I2S_Mode == AIO_MODE_PCM_SLAVE_STD)
        || (I2S_Mode == AIO_MODE_PCM_SLAVE_NSTD))
    {
        stTlvAic31Codec_Ctrl.i2s_master = 1;
    }
    else
    {
        stTlvAic31Codec_Ctrl.i2s_master = 0;
    }

    s32Ret = ioctl(g_fdTLV320AIC31, TLV320_SET_I2S_MODE, &stTlvAic31Codec_Ctrl);
    if (s32Ret < 0)
    {
        return s32Ret;
    }

    return 0;
}

static HI_VOID HIADP_TLV320_Close(HI_VOID)
{
    if (g_fdTLV320AIC31 > 0)
    {
        close(g_fdTLV320AIC31);
    }
}

#endif

#ifdef SLIC_AUDIO_DEVICE_ENABLE
static int g_fdSlic = -1;
  #define SLIC_FILE "/dev/le89116"
  #define IOCTL_SLAC_INITIAL 201
  #define IOCTL_SET_LINE_STATE 7
  #define IOCTL_GET_LINE_STATE 17

/** Ring control option */
  #define FORCE_SIGNED_ENUM FORCE_STANDARD_C_ENUM_SIZE - 1
  #define FORCE_STANDARD_C_ENUM_SIZE (0x7fff)
typedef enum
{
    VP_LINE_STANDBY,        /**< Low power line feed state */
    VP_LINE_TIP_OPEN,       /**< Tip open circuit state */
    VP_LINE_ACTIVE,         /**< Line Feed w/out VF */
    VP_LINE_ACTIVE_POLREV,  /**< Polarity Reversal Line Feed w/out VF */
    VP_LINE_TALK,           /**< Normal off-hook Active State; Voice Enabled */
    VP_LINE_TALK_POLREV,    /**< Normal Active with reverse polarity;
                             *   Voice Enabled */

    VP_LINE_OHT,            /**< On-Hook tranmission state */
    VP_LINE_OHT_POLREV,     /**< Polarity Reversal On-Hook tranmission state */

    VP_LINE_DISCONNECT,     /**< Denial of service */
    VP_LINE_RINGING,        /**< Ringing state */
    VP_LINE_RINGING_POLREV, /**< Ringing w/Polarity Reversal */

    VP_LINE_FXO_OHT,        /**< FXO Line providing Loop Open w/VF */
    VP_LINE_FXO_LOOP_OPEN,  /**< FXO Line providing Loop Open w/out VF */
    VP_LINE_FXO_LOOP_CLOSE, /**< FXO Line providing Loop Close w/out VF */
    VP_LINE_FXO_TALK,       /**< FXO Line providing Loop Close w/VF */
    VP_LINE_FXO_RING_GND,   /**< FXO Line providing Ring Ground (GS only)*/

    VP_LINE_STANDBY_POLREV, /**< Low power polrev line feed state */
    VP_LINE_PARK,           /**< Park mode */
    VP_LINE_RING_OPEN,      /**< Ring open */
    VP_LINE_HOWLER,         /**< Howler */
    VP_LINE_TESTING,        /**< Testing */
    VP_LINE_DISABLED,       /**< Disabled */
    VP_LINE_NULLFEED,       /**< Null-feed */

    VP_NUM_LINE_STATES,
    VP_LINE_STATE_ENUM_RSVD = FORCE_SIGNED_ENUM,
    VP_LINE_STATE_ENUM_SIZE = FORCE_STANDARD_C_ENUM_SIZE /* Portability Req.*/
} VpLineStateType;

typedef struct VpSetLineState_TYPE
{
    HI_VOID *       pLineCtx;
    VpLineStateType state;
} VpSetLineState_TYPE;

HI_S32 HIADP_SLIC_Open(HI_VOID)
{
    HI_S32 Ret;
	
	AudioSlicTlv320RST();
	AudioSPIPinSharedEnable();

    g_fdSlic = open(SLIC_FILE, O_RDWR);
    if (g_fdSlic < 0)
    {
        printf("can't open SLIC le89116\n");
        return -1;
    }

    Ret = ioctl(g_fdSlic, IOCTL_SLAC_INITIAL, NULL);
    if (Ret < 0)
    {
        return Ret;
    }

    sleep(1);
    return 0;
}

HI_S32 HIADP_SLIC_Close(HI_VOID)
{
    if (g_fdSlic > 0)
    {
        close(g_fdSlic);
        g_fdSlic = -1;
    }

    return HI_SUCCESS;
}

HI_S32 HIADP_SLIC_GetHookOff(HI_BOOL *pbEnable)
{
    VpSetLineState_TYPE stState;
    HI_S32 Ret;

    stState.pLineCtx = NULL;
    *pbEnable = HI_FALSE;
    Ret = ioctl(g_fdSlic, IOCTL_GET_LINE_STATE, &stState);
    if (Ret == HI_SUCCESS)
    {
        if (VP_LINE_TALK == stState.state)
        {
            *pbEnable = HI_TRUE;
        }
    }

    return Ret;
}

HI_S32 HIADP_SLIC_GetHookOn(HI_BOOL *pbEnable)
{
    VpSetLineState_TYPE stState;
    HI_S32 Ret;

    stState.pLineCtx = NULL;
    *pbEnable = HI_FALSE;
    Ret = ioctl(g_fdSlic, IOCTL_GET_LINE_STATE, &stState);
    if (Ret == HI_SUCCESS)
    {
        if (VP_LINE_STANDBY == stState.state)
        {
            *pbEnable = HI_TRUE;
        }
    }

    return Ret;
}

HI_S32 HIADP_SLIC_SetRinging(HI_BOOL bEnable)
{
    VpSetLineState_TYPE stState;
    HI_S32 Ret;

    Ret = ioctl(g_fdSlic, IOCTL_GET_LINE_STATE, &stState);
    if (Ret == HI_SUCCESS)
    {
        if ((VP_LINE_STANDBY == stState.state) && (HI_TRUE == bEnable))
        {
            stState.state = VP_LINE_RINGING;
            Ret = ioctl(g_fdSlic, IOCTL_SET_LINE_STATE, &stState);
        }
        else if ((VP_LINE_RINGING == stState.state) && (HI_FALSE == bEnable))
        {
            stState.state = VP_LINE_STANDBY;
            Ret = ioctl(g_fdSlic, IOCTL_SET_LINE_STATE, &stState);
        }
    }

    return Ret;
}

 #endif

#ifdef SLIC_AUDIO_DEVICE_ENABLE
#define AIAO_I2S_INTERFACE_MODE AIO_MODE_PCM_MASTER_STD
 #define AIAO_SOUND_MODE AUDIO_SOUND_MODE_MOMO
#else
#define AIAO_I2S_INTERFACE_MODE AIO_MODE_I2S_MASTER
#endif
 #define AIAO_BIT_DEPTH HI_UNF_BIT_DEPTH_16

#ifdef TLV320_AUDIO_DEVICE_ENABLE 
 #define AIAO_SOUND_MODE AUDIO_SOUND_MODE_STEREO
#endif

static HI_S32 g_AiaoDevId = -1;    /* SIO 0*/
static HI_S32 g_AiCh = -1;
static HI_S32 g_AoCh = -1;

#if (!defined (CHIP_TYPE_hi3712))
static HI_S32 HIADP_AiAo_Open( HI_UNF_SAMPLE_RATE_E enSamplerate, HI_U32 u32SamplePerFrame)
{
    HI_S32 nRet;
    AIO_ATTR_S stAttr;

    nRet = AudioSIOPinSharedEnable();
	if (HI_SUCCESS != nRet)
    {
        printf("chip not support or GetVersion err:0x%x\n", nRet);
        return nRet;
    }
#ifdef SLIC_AUDIO_DEVICE_ENABLE    
#define ENA_PRIVATE_CLK
#endif
#ifdef  ENA_PRIVATE_CLK
    AIO_PRIVATECLK_ATTR_S stPrivateClkAttr;
    stPrivateClkAttr.enMclkSel = AIO_MCLK_256_FS; /* 8000*512 = 4096000 or 16000*512 =  8192000 */
    stPrivateClkAttr.enBclkSel = AIO_BCLK_2_DIV;   /* 8000*256 = 2048000 or 16000*256 =  4096000 */
    stPrivateClkAttr.bSampleRiseEdge = HI_FALSE;
    stAttr.pstPrivClkAttr = &stPrivateClkAttr;/* use private clock */
#else
    stAttr.pstPrivClkAttr = HI_NULL; /* use default clock */
#endif

    /*--------------------------  AI ---------------------------------------*/
    stAttr.enBitwidth   = AIAO_BIT_DEPTH;/* should equal to ADC*/
    stAttr.enSamplerate = enSamplerate;/* invalid factly when SLAVE mode*/
    stAttr.enSoundmode  = AIAO_SOUND_MODE;
    stAttr.enWorkmode = AIAO_I2S_INTERFACE_MODE;
    stAttr.u32EXFlag = 1;/* should set 1, ext to 16bit */
    stAttr.u32FrmNum = 4;
    stAttr.u32ChnCnt = 2;
    stAttr.u32PtNumPerFrm = u32SamplePerFrame;
    stAttr.u32ClkSel = 0;

    /* set ai public attr*/
    nRet = HI_MPI_AI_SetPubAttr(g_AiaoDevId, &stAttr);
    if (HI_SUCCESS != nRet)
    {
        printf("set ai %d attr err:0x%x\n", g_AiaoDevId, nRet);
        return nRet;
    }

    /* enable ai device*/
    nRet = HI_MPI_AI_Enable(g_AiaoDevId);
    if (HI_SUCCESS != nRet)
    {
        printf("enable ai dev %d err:0x%x\n", g_AiaoDevId, nRet);
        return nRet;
    }

    /* enable ai chnnel*/
    nRet = HI_MPI_AI_EnableChn(g_AiaoDevId, g_AiCh);
    if (HI_SUCCESS != nRet)
    {
        printf("enable ai chn %d err:0x%x\n", g_AiCh, nRet);
        return nRet;
    }

    /*---------------------------AO----------------------------------*/
    stAttr.enBitwidth   = AIAO_BIT_DEPTH;/* should equal to ADC*/
    stAttr.enSamplerate = enSamplerate;/* invalid factly when SLAVE mode*/
    stAttr.enSoundmode  = AIAO_SOUND_MODE;
    stAttr.enWorkmode = AIAO_I2S_INTERFACE_MODE;
    stAttr.u32EXFlag = 1;/* should set 1, ext to 16bit */
    stAttr.u32FrmNum = 4;
    stAttr.u32ChnCnt = 2;
    stAttr.u32PtNumPerFrm = u32SamplePerFrame;
    stAttr.u32ClkSel = 0;

    /* set ao public attr*/
    nRet = HI_MPI_AO_SetPubAttr(g_AiaoDevId, &stAttr);
    if (HI_SUCCESS != nRet)
    {
        printf("set ao %d attr err:0x%x\n", g_AiaoDevId, nRet);
        return nRet;
    }

    /* enable ao device*/
    nRet = HI_MPI_AO_Enable(g_AiaoDevId);
    if (HI_SUCCESS != nRet)
    {
        printf("enable ao dev %d err:0x%x\n", g_AiaoDevId, nRet);
        return nRet;
    }

    /* enable ao chnnel*/
    nRet = HI_MPI_AO_EnableChn(g_AiaoDevId, g_AoCh);
    if (HI_SUCCESS != nRet)
    {
        printf("enable ao chn %d err:0x%x\n", g_AoCh, nRet);
        return nRet;
    }

    return 0;
}

static HI_VOID HIADP_AiAo_Close(HI_VOID)
{
    HI_MPI_AI_DisableChn((AUDIO_DEV)g_AiaoDevId, g_AiCh);
    HI_MPI_AI_Disable((AUDIO_DEV)g_AiaoDevId);
    HI_MPI_AO_DisableChn((AUDIO_DEV)g_AiaoDevId, g_AoCh);
    HI_MPI_AO_Disable((AUDIO_DEV)g_AiaoDevId);
}

HI_S32 HIADP_AIAO_Init(HI_S32 DevId, HI_S32 AI_Ch, HI_S32 AO_Ch, HI_UNF_SAMPLE_RATE_E enSamplerate,
                       HI_U32 u32SamplePerFrame)
{
    HI_S32 Ret;

    g_AiaoDevId = DevId;
    g_AiCh = AI_Ch;
    g_AoCh = AO_Ch;

#if defined (TLV320_AUDIO_DEVICE_ENABLE)
    Ret = HIADP_TLV320_Open(AIAO_I2S_INTERFACE_MODE);
    if (HI_SUCCESS != Ret)
    {
        printf("TLV320 open err:0x%x\n", Ret);
        return Ret;
    }
#endif

    Ret = HIADP_Dma_Open();
    if (HI_SUCCESS != Ret)
    {
        printf("dma open err:0x%x\n", Ret);
        return Ret;
    }

    Ret = HIADP_AiAo_Open(enSamplerate, u32SamplePerFrame);
    if (HI_SUCCESS != Ret)
    {
        printf(" aiao(%d) open err:0x%x\n", g_AiaoDevId, Ret);
        return Ret;
    }

#if defined (SLIC_AUDIO_DEVICE_ENABLE)
    Ret = HIADP_SLIC_Open();
    if (HI_SUCCESS != Ret)
    {
        printf("Le89116 open err:0x%x\n", Ret);
        return Ret;
    }
#endif

    
    printf("HIADP_AIAO_Init OK\n");

    return HI_SUCCESS;
}

HI_S32 HIADP_AIAO_DeInit(HI_VOID)
{
 
#if defined (SLIC_AUDIO_DEVICE_ENABLE)
    HIADP_SLIC_Close();
#endif

    HIADP_AiAo_Close();
    HIADP_Dma_Close();

#if defined (TLV320_AUDIO_DEVICE_ENABLE)
    HIADP_TLV320_Close();
#endif

    return HI_SUCCESS;
}
/* END AIAO_DeInit: */
#endif


/***************************** Macro Definition ******************************/
#ifdef SLIC_AUDIO_DEVICE_ENABLE
	#define SLIC_USE_SPI 
	/* if use gpio_spi(not spi), should as follow:
		#undef SLIC_USE_SPI
		#undef VP890_USE_SPI (source\msp\ecs\drv\drv_slic_v0.6\slac_private.h)
	 */
#endif

#ifdef TLV320_AUDIO_DEVICE_ENABLE
	#undef SLIC_USE_SPI
#endif

#define PERI_CTRL_CONFIG_BASE	0x10200008
//#define MULIT_GPIO_CONFIG_BASE	0x10203000

/********************** Global Variable declaration **************************/

/******************************* API declaration *****************************/

HI_S32 AudioSIOPinSharedEnable()
{
	HI_SYS_VERSION_S stVersion;
	HI_S32 Ret;

	Ret = HI_SYS_GetVersion(&stVersion);
	if (HI_SUCCESS == Ret)
	{
		if(HI_CHIP_TYPE_HI3716M == stVersion.enChipTypeHardWare && HI_CHIP_VERSION_V101 == stVersion.enChipVersion)
	    {
			Ret = HI_FAILURE;
	    }
	}

/* please make sure sio0_gpio_vga0vs_ao_aio/sio0_gpio_ao_aio OR tsi0_vi0_gpio_ao/tsi0_vi0_gpio/tsi0_vi0_gpio_ad OR
   tsi0_vi0_gpio_ao/tsi0_sio_gpio/tsi0_vi0_gpio_ad/tsi0_vi0_gpio config correct:

	BOARD_TYPE_hi3716cdmoverb/BOARD_TYPE_hi3716hdmoverb:
		himm 0x10203000 0x1
		himm 0x10203004 0x1
	BOARD_TYPE_hi3716mstbverc:
		himm 0x10203154 0x6
		himm 0x10203158 0x6
		himm 0x1020315c 0x6
	BOARD_TYPE_hi3716mtst3avera:
		himm 0x10203158 0x6
		himm 0x10203160 0x6
		himm 0x10203164 0x6
		himm 0x10203168 0x6	
*/

	return Ret;
}

#ifdef SLIC_USE_SPI
static HI_VOID SPI_CSX_RST()
{	
#if defined(BOARD_TYPE_hi3716cdmoverb) || defined(BOARD_TYPE_hi3716hdmoverb)
	HI_U32 u32TmpVal;
	//HI_REG(PERI_CTRL) &= ~(0x01<<7);    /*SPI_CS0*/
	HI_SYS_ReadRegister(PERI_CTRL_CONFIG_BASE, &u32TmpVal);
	u32TmpVal &= ~(0x01<<7);
	HI_SYS_WriteRegister(PERI_CTRL_CONFIG_BASE, u32TmpVal);
#elif defined(BOARD_TYPE_hi3716mstbverc) || defined(BOARD_TYPE_hi3716mtst3avera)\
      || defined(BOARD_TYPE_hi3716mdmo3avera) || defined(BOARD_TYPE_hi3716mdmo3bvera)//tst
	HI_U32 u32TmpVal;
	//HI_REG(PERI_CTRL) |= (0x01<<7);    /*SPI_CS1*/
	HI_SYS_ReadRegister(PERI_CTRL_CONFIG_BASE, &u32TmpVal);
	u32TmpVal |= (0x01<<7);
	HI_SYS_WriteRegister(PERI_CTRL_CONFIG_BASE, u32TmpVal);
#else
	
#endif
}
#endif

HI_VOID AudioSlicTlv320RST()
{
/* please make sure spi_gpio15_vga0hs OR tsi0_vi0_tsi1_gpio_ao config correct:

	BOARD_TYPE_hi3716cdmoverb(GPIO1_4)/BOARD_TYPE_hi3716hdmoverb(GPIO1_4):
		himm 0x10203010 0x0
	BOARD_TYPE_hi3716mstbverc(GPIO11_5):
		himm 0x10203150 0x6(not 0x3) 
	BOARD_TYPE_hi3716mtst3avera(GPIO8_5):
		himm 0x10203150 0x3

*/

#ifdef SLIC_AUDIO_DEVICE_ENABLE
	printf("SLIC Reset\n"); 
#else
	printf("TLV320 Reset\n"); 
#endif

#if defined(BOARD_TYPE_hi3716cdmoverb) || defined(BOARD_TYPE_hi3716hdmoverb) || defined(BOARD_TYPE_hi3716mstbverc)\
    || defined(BOARD_TYPE_hi3716mtst3avera) || defined(BOARD_TYPE_hi3716mdmo3avera) || defined(BOARD_TYPE_hi3716mdmo3bvera)//tst
    HI_UNF_GPIO_Init();
    HI_UNF_GPIO_SetDirBit(SLAC_RESET_GPIO_GNUM * 8 + SLAC_RESET_GPIO_PNUM, HI_FALSE);
    HI_UNF_GPIO_WriteBit(SLAC_RESET_GPIO_GNUM * 8 + SLAC_RESET_GPIO_PNUM, HI_FALSE);
    usleep(100*1000);
    HI_UNF_GPIO_WriteBit(SLAC_RESET_GPIO_GNUM * 8 + SLAC_RESET_GPIO_PNUM, HI_TRUE);
    HI_UNF_GPIO_DeInit();
#endif	
}

HI_VOID AudioSPIPinSharedEnable()
{	
#ifdef SLIC_USE_SPI
	SPI_CSX_RST();
#endif

/* please make sure spi_gpio/spi_gpio_i2c3 OR tsi0_vi0_tsi1_gpio_ao/tsi0_vi0_gpio_ao config correct:

#ifdef SLIC_USE_SPI
	BOARD_TYPE_hi3716cdmoverb/BOARD_TYPE_hi3716hdmoverb:
		himm 0x10203008 0x1
		himm 0x1020300c 0x1
	BOARD_TYPE_hi3716mstbverc/BOARD_TYPE_hi3716mtst3avera:
		himm 0x10203150 0x6
		himm 0x10203154 0x6
#else
	BOARD_TYPE_hi3716cdmoverb(GPIO1_1/GPIO1_2 for Tlv320)/BOARD_TYPE_hi3716hdmoverb(GPIO1_1/GPIO1_2 for Tlv320):
		himm 0x10203008 0x0
		himm 0x1020300c 0x0
	BOARD_TYPE_hi3716mstbverc(GPIO12_0/GPIO12_1 for Tlv320)/BOARD_TYPE_hi3716mtst3avera(GPIO9_0/GPIO9_1 for Tlv320):
		himm 0x10203150 0x3
		himm 0x10203154 0x3
#endif	
*/

}
