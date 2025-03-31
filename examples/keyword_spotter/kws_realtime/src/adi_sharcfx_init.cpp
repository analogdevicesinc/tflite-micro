/*********************************************************************************
Copyright(c) 2025 Analog Devices, Inc. All Rights Reserved.
This software is proprietary. By using this software you agree
to the terms of the associated Analog Devices License Agreement.
*********************************************************************************/

/*****************************************************************************
 * adi_sharcfx_init.cpp
 *****************************************************************************/

#include "adi_sharcfx_init.h"

struct Config_Table
{
    short Reg_Add;
    char  Value;
};

char Config_read_DAC[28];
char Config_read_ADC[16];
int count;
struct Config_Table Config_array_DAC[28] = {
                   {     ADAU1962_PDN_CTRL_1,        0x00},
                   {     ADAU1962_PDN_CTRL_2,        0xff},
                   {     ADAU1962_PDN_CTRL_3,        0x0f},
                   {     ADAU1962_DAC_CTRL0,        0x01},
                   {     ADAU1962_DAC_CTRL1,        0x01},
                   {     ADAU1962_DAC_CTRL2,        0x00},
                   {     ADAU1962_DAC_MUTE1,        0x0},
                   {     ADAU1962_DAC_MUTE2,        0x00},
                   {     ADAU1962_MSTR_VOL,        0x00},
                {     ADAU1962_DAC1_VOL,        0x00},
                {     ADAU1962_DAC2_VOL,        0x00},
                {     ADAU1962_DAC3_VOL,        0x00},
                {     ADAU1962_DAC4_VOL,        0x00},
                {     ADAU1962_DAC5_VOL,        0x00},
                {     ADAU1962_DAC6_VOL,        0x00},
                {     ADAU1962_DAC7_VOL,        0x00},
                {     ADAU1962_DAC8_VOL,        0x00},
                {     ADAU1962_DAC9_VOL,        0x00},
                {     ADAU1962_DAC10_VOL,        0x00},
                {     ADAU1962_DAC11_VOL,        0x00},
                {     ADAU1962_DAC12_VOL,        0x00},
                {     ADAU1962_PAD_STRGTH,        0x00},
                {     ADAU1962_DAC_PWR1,        0xaa},
                {     ADAU1962_DAC_PWR2,        0xaa},
                {     ADAU1962_DAC_PWR3,        0xaa},
                {     ADAU1962_PDN_CTRL_2,        0x00},
                {     ADAU1962_PDN_CTRL_3,        0x00},
                {     ADAU1962_DAC_CTRL0,        0x00}

};

struct Config_Table Config_array_ADC[16] = {
         {ADAU1979_REG_BOOST            ,    0x00},
         {ADAU1979_REG_MICBIAS            ,    0x00},
         {ADAU1979_REG_BLOCK_POWER_SAI    ,    0x30},
         {ADAU1979_REG_SAI_CTRL0        ,    0x02},
         {ADAU1979_REG_SAI_CTRL1        ,    0x00},
         {ADAU1979_REG_CMAP12            ,    0x01},
         {ADAU1979_REG_CMAP34            ,    0x00},
         {ADAU1979_REG_SAI_OVERTEMP        ,    0x30},
         {ADAU1979_REG_POST_ADC_GAIN1     ,    0xA0},
         {ADAU1979_REG_POST_ADC_GAIN2     ,    0xA0},
         {ADAU1979_REG_POST_ADC_GAIN3     ,    0xA0},
         {ADAU1979_REG_POST_ADC_GAIN4     ,    0xA0},
         {ADAU1979_REG_ADC_CLIP            ,    0x00},
         {ADAU1979_REG_DC_HPF_CAL        ,    0x00},
         {ADAU1979_REG_BLOCK_POWER_SAI    ,    0x33},
         {ADAU1979_REG_MISC_CONTROL        ,    0x00}
};


/*=============  D A T A  =============*/

/* Buffers */
__attribute__((section(".L1.data")))
ADI_CACHE_ALIGN static int   int_SP0ABuffer1[AUDIO_COUNT];
__attribute__((section(".L1.data")))
ADI_CACHE_ALIGN static int   int_SP0ABuffer2[AUDIO_COUNT];

__attribute__((section(".L1.data")))
ADI_CACHE_ALIGN static int   int_SP0ABuffer4[AUDIO_COUNT];
__attribute__((section(".L1.data")))
ADI_CACHE_ALIGN static int   int_SP0ABuffer5[AUDIO_COUNT];

/* Destination Sport PDMA list */
__attribute__((section(".L1.data")))
ADI_PDMA_DESC_LIST iDESC_LIST_1_SP4A;
__attribute__((section(".L1.data")))
ADI_PDMA_DESC_LIST iDESC_LIST_2_SP4A;

/* Source Sport PDMA list */
__attribute__((section(".L1.data")))
ADI_PDMA_DESC_LIST iSRC_LIST_1_SP4B;
__attribute__((section(".L1.data")))
ADI_PDMA_DESC_LIST iSRC_LIST_2_SP4B;

/* Count to track the number of callBacks for SPORT transfer */
volatile uint8_t TxCallbackCount = 0;
volatile uint8_t RxCallbackCount = 0;

volatile uint32_t TestCallbackCount = 0;

/* Memory required for SPORT */
static uint8_t SPORTMemory4A[ADI_SPORT_MEMORY_SIZE];
static uint8_t SPORTMemory4B[ADI_SPORT_MEMORY_SIZE];

/* SPORT Handle */
static ADI_SPORT_HANDLE hSPORTDev4ATx;//TX
static ADI_SPORT_HANDLE hSPORTDev4BRx;//RX

/* Memory required for TWI */
uint8_t TwideviceMemory[ADI_TWI_MEMORY_SIZE];

/* TWI driver handle */
static ADI_TWI_HANDLE hTwiDevice;

/* SPU handle */
static ADI_SPU_HANDLE      ghSpu;

/* Memory required for the SPU operation */
uint8_t             SpuMemory[ADI_SPU_MEMORY_SIZE];

/* Dev buffer for configuring ADC-DAC through TWI*/
static uint8_t devBuffer[BUFFER_SIZE];

//call back when new data is recieved
static void SPORTRxCallback(
    void        *pAppHandle,
    uint32_t     nEvent,
    void        *pArg
)
{
    int i;

    /* CASEOF (event type) */
    switch (nEvent)
    {
        /* CASE (buffer processed) */
        case ADI_SPORT_EVENT_RX_BUFFER_PROCESSED:
                //copy next buffer to input buffer
                RxCallbackCount +=1;
                if(RxCallbackCount==1)
                {
                    int nReadLocation = (pReadPtr) % NUM_HOPS;//circular buffer loopback to beginning
                    memcpy(g_audio_data_input+(nReadLocation)*AUDIO_COUNT,int_SP0ABuffer4,AUDIO_COUNT*sizeof(int));
                    pReadPtr++;//updated read buffer location
                }
                else if(RxCallbackCount==2)
                {
                    int nReadLocation = (pReadPtr) % NUM_HOPS;//circular buffer loopback to beginning
                    memcpy(g_audio_data_input+(nReadLocation)*AUDIO_COUNT,int_SP0ABuffer5,AUDIO_COUNT*sizeof(int));
                    pReadPtr++;//updated read buffer location
                    RxCallbackCount=0;
                }
    break;
    default:
    break;
    }
    /* return */
    }

//call back when data is txed
static void SPORTTxCallback(
    void        *pAppHandle,
    uint32_t     nEvent,
    void        *pArg
)
{
    int i;

    /* CASEOF (event type) */
    switch (nEvent)
    {
        /* CASE (buffer processed) */
        case ADI_SPORT_EVENT_TX_BUFFER_PROCESSED:
                //playback next buffer to output buffer
                TxCallbackCount +=1;
                if(TxCallbackCount==1)
                {
                    //ensure there is new data to write
                    if(pWritePtr < pProcessWritePtr) {
                        int nWriteLocation = (pWritePtr) % NUM_HOPS;//circular buffer loopback to beginning
                        memcpy(int_SP0ABuffer1,g_audio_data_output+(nWriteLocation)*AUDIO_COUNT,AUDIO_COUNT*sizeof(int));
                        pWritePtr++;//updated write buffer location
                    }
                }
                else if(TxCallbackCount==2)
                {
                    //ensure there is new data to write
                    if(pWritePtr < pProcessWritePtr) {
                        int nWriteLocation = (pWritePtr) % NUM_HOPS;//circular buffer loopback to beginning
                        memcpy(int_SP0ABuffer2,g_audio_data_output+(nWriteLocation)*AUDIO_COUNT,AUDIO_COUNT*sizeof(int));
                        pWritePtr++;//updated write buffer location
                        TxCallbackCount=0;
                    }
                }
                break;
        default:
             break;
    }
    /* return */
}

/*
 * Prepares SPU configuration.
 *
 * Parameters
 *  None
 *
 * Returns
 *  None
 *
 */
int SPU_init(void)
{
    if(adi_spu_Init(0, SpuMemory, NULL, NULL, &ghSpu) != ADI_SPU_SUCCESS)
    {
        REPORT_ERROR("Failed to initialize SPU service\n");
        return FAILED;
    }

    /* Make SPORT 0A to generate secure transactions */
    if(adi_spu_EnableMasterSecure(ghSpu, SPORT_4A_SPU, true) != ADI_SPU_SUCCESS)
    {
        REPORT_ERROR("Failed to enable Master secure for SPORT0A\n");
        return FAILED;
    }

    /* Make SPORT 0B to generate secure transactions */
    if(adi_spu_EnableMasterSecure(ghSpu, SPORT_4B_SPU, true) != ADI_SPU_SUCCESS)
    {
        REPORT_ERROR("Failed to enable Master secure for SPORT0B\n");
        return FAILED;
    }

    return SUCCESS;
}


void Switch_Configurator()
{
    int delay11=0xffff;


    /* Software Switch Configuration for Enabling ADC-DAC */
    ConfigSoftSwitches_ADC_DAC();

    while(delay11--)
    {
        asm("nop;");
    }

    /* Software Switch Configuration for Re-Setting ADC-DAC  */
    ConfigSoftSwitches_ADAU_Reset();


    /* wait for Codec to up */
    delay11=0xffff;
    while(delay11--)
    {
        asm("nop;");
    }
}


/*
 * Prepares SRU configuration.
 *
 * Parameters
 *  None
 *
 * Returns
 *  None
 *
 */
void SRU_Init(void)
{
    *pREG_PADS0_DAI0_IE=0xfffff;
    *pREG_PADS0_DAI1_IE=0xfffff;

    SRU2(LOW,DAI1_PBEN05_I);

    SRU2(DAI1_PB05_O,SPT4_ACLK_I); /*DAC clock to SPORT 4A*/
    SRU2(DAI1_PB05_O,SPT4_BCLK_I); /*DAC clock to SPORT 4B*/

    SRU2(DAI1_PB04_O,SPT4_AFS_I);  /*DAC FS to SPORT 4A*/
    SRU2(DAI1_PB04_O,SPT4_BFS_I);  /*DAC FS to SPORT 4B*/
    SRU2(LOW,DAI1_PBEN04_I);

    SRU2(SPT4_AD0_O,DAI1_PB01_I); /* SPORT 4A to DAC*/
    SRU2(HIGH,DAI1_PBEN01_I);

    SRU2(DAI1_PB05_O,DAI1_PB12_I);  /*DAC clock to ADC */
    SRU2(HIGH,DAI1_PBEN12_I);

    SRU2(DAI1_PB04_O,DAI1_PB20_I);  /*DAC FS to ADC */
    SRU2(HIGH,DAI1_PBEN20_I);

    SRU2(DAI1_PB06_O,SPT4_BD0_I);
    SRU2(LOW,DAI1_PBEN06_I);

}

static void PrepareDescriptors (void)
{
    iDESC_LIST_1_SP4A.pStartAddr    = (int *)int_SP0ABuffer1;
    iDESC_LIST_1_SP4A.pNxtDscp        = &iDESC_LIST_2_SP4A;
    iDESC_LIST_1_SP4A.Config        = ENUM_DMA_CFG_XCNT_INT ;
    iDESC_LIST_1_SP4A.XCount        = AUDIO_COUNT;
    iDESC_LIST_1_SP4A.XModify        = 4;
    iDESC_LIST_1_SP4A.YCount        = 0;
    iDESC_LIST_1_SP4A.YModify        = 0;

    iDESC_LIST_2_SP4A.pStartAddr    = (int *)int_SP0ABuffer2;
    iDESC_LIST_2_SP4A.pNxtDscp        = &iDESC_LIST_1_SP4A;
    iDESC_LIST_2_SP4A.Config        = ENUM_DMA_CFG_XCNT_INT ;
    iDESC_LIST_2_SP4A.XCount        = AUDIO_COUNT;
    iDESC_LIST_2_SP4A.XModify        = 4;
    iDESC_LIST_2_SP4A.YCount        = 0;
    iDESC_LIST_2_SP4A.YModify        = 0;

    iSRC_LIST_1_SP4B.pStartAddr        =(int *)int_SP0ABuffer4;
    iSRC_LIST_1_SP4B.pNxtDscp        = &iSRC_LIST_2_SP4B;
    iSRC_LIST_1_SP4B.Config            = ENUM_DMA_CFG_XCNT_INT ;
    iSRC_LIST_1_SP4B.XCount            = AUDIO_COUNT;
    iSRC_LIST_1_SP4B.XModify        = 4;
    iSRC_LIST_1_SP4B.YCount            = 0;
    iSRC_LIST_1_SP4B.YModify        = 0;

    iSRC_LIST_2_SP4B.pStartAddr        =(int *)int_SP0ABuffer5;
    iSRC_LIST_2_SP4B.pNxtDscp        = &iSRC_LIST_1_SP4B;
    iSRC_LIST_2_SP4B.Config            = ENUM_DMA_CFG_XCNT_INT;
    iSRC_LIST_2_SP4B.XCount            = AUDIO_COUNT;
    iSRC_LIST_2_SP4B.XModify        = 4;
    iSRC_LIST_2_SP4B.YCount            = 0;
    iSRC_LIST_2_SP4B.YModify        = 0;

}

int Sport_Init(void)
{
    /* SPORT return code */
    ADI_SPORT_RESULT    eResult;

    /* Open the SPORT Device 4A */
    eResult = adi_sport_Open(SPORT_DEVICE_4A,ADI_HALF_SPORT_A,ADI_SPORT_DIR_TX, ADI_SPORT_I2S_MODE, SPORTMemory4A,ADI_SPORT_MEMORY_SIZE,&hSPORTDev4ATx);
    CHECK_RESULT(eResult);
    /* Open the SPORT Device 4B*/
    eResult = adi_sport_Open(SPORT_DEVICE_4B,ADI_HALF_SPORT_B,ADI_SPORT_DIR_RX, ADI_SPORT_I2S_MODE, SPORTMemory4B,ADI_SPORT_MEMORY_SIZE,&hSPORTDev4BRx);
    CHECK_RESULT(eResult);

    /* Register SPORT Callback function */
    eResult = adi_sport_RegisterCallback(hSPORTDev4BRx,SPORTRxCallback,NULL);
    CHECK_RESULT(eResult);

    eResult = adi_sport_RegisterCallback(hSPORTDev4ATx,SPORTTxCallback,NULL);
    CHECK_RESULT(eResult);

    /* Prepare descriptors */
    PrepareDescriptors();

    /* Submit the first buffer for Rx.  */
    eResult = adi_sport_DMATransfer(hSPORTDev4BRx,&iSRC_LIST_1_SP4B,(DMA_NUM_DESC),ADI_PDMA_DESCRIPTOR_LIST, ADI_SPORT_CHANNEL_PRIM);
    CHECK_RESULT(eResult);
    /* Submit the first buffer for Tx.  */
    eResult = adi_sport_DMATransfer(hSPORTDev4ATx,&iDESC_LIST_1_SP4A,(DMA_NUM_DESC),ADI_PDMA_DESCRIPTOR_LIST, ADI_SPORT_CHANNEL_PRIM);
    CHECK_RESULT(eResult);

    /*Enable the Sport Device 4B */
    eResult = adi_sport_Enable(hSPORTDev4BRx,true);
    CHECK_RESULT(eResult);
    /*Enable the Sport Device 4A */
    eResult = adi_sport_Enable(hSPORTDev4ATx,true);
    CHECK_RESULT(eResult);

    return eResult;

}


int Sport_Stop(void)
{
    /* SPORT return code */
    ADI_SPORT_RESULT    eResult;

    /*Stop the DMA transfer of  Sport Device 4B */
    eResult = adi_sport_StopDMATransfer(hSPORTDev4BRx);
    CHECK_RESULT(eResult);
    /*Stop the DMA transfer of  Sport Device 4A */
    eResult = adi_sport_StopDMATransfer(hSPORTDev4ATx);
    CHECK_RESULT(eResult);
    /*Close Sport Device 4B */
    eResult = adi_sport_Close(hSPORTDev4BRx);
    CHECK_RESULT(eResult);
    /*Close Sport Device 4A */
    eResult = adi_sport_Close(hSPORTDev4ATx);
    CHECK_RESULT(eResult);

    return eResult;
}

void Write_TWI_8bit_Reg(unsigned char Reg_ID, unsigned char Tx_Data)
{
    devBuffer[0] = Reg_ID;
    devBuffer[1] = Tx_Data;
    adi_twi_Write(hTwiDevice, devBuffer, 2u, false);
}

unsigned char Read_TWI_8bit_Reg(unsigned char Reg_ID)
{
    ADI_TWI_RESULT eResult;
    unsigned char Rx_Data;

    /* write register address */
    devBuffer[0] = Reg_ID;
    eResult = adi_twi_Write(hTwiDevice, devBuffer, 1u, true);
    if(eResult!=ADI_TWI_SUCCESS)
    {
        REPORT_ERROR("TWI write failed 0x%08X\n", eResult);
    }

    /* read register value */
    eResult = adi_twi_Read(hTwiDevice, &Rx_Data, 1u, false);
    if(eResult!=ADI_TWI_SUCCESS)
    {
        REPORT_ERROR("TWI Read failed 0x%08X\n", eResult);
    }

    return Rx_Data;
}

int Init_TWI(void)
{
    ADI_TWI_RESULT eResult;

    eResult = adi_twi_Open(TWIDEVNUM, ADI_TWI_MASTER, &TwideviceMemory[0],
                   ADI_TWI_MEMORY_SIZE, &hTwiDevice);
    if(eResult!=ADI_TWI_SUCCESS)
    {
        REPORT_ERROR("TWI Open failed 0x%08X\n", eResult);
    }

    eResult = adi_twi_SetPrescale(hTwiDevice, PRESCALEVALUE);
    if(eResult!=ADI_TWI_SUCCESS)
    {
        REPORT_ERROR("TWI Set Prescale failed 0x%08X\n", eResult);
    }

    eResult = adi_twi_SetBitRate(hTwiDevice, BITRATE);
    if(eResult!=ADI_TWI_SUCCESS)
    {
        REPORT_ERROR("TWI Set Bitrate failed 0x%08X\n", eResult);
    }


    eResult = adi_twi_SetDutyCycle(hTwiDevice, DUTYCYCLE);
    if(eResult!=ADI_TWI_SUCCESS)
    {
        REPORT_ERROR("TWI Set Duty cycle failed 0x%08X\n", eResult);
    }

    eResult = adi_twi_SetHardwareAddress(hTwiDevice, TARGETADDR);
    if(eResult!=ADI_TWI_SUCCESS)
    {
        REPORT_ERROR("TWI Set Hw address failed 0x%08X\n", eResult);
    }

    return eResult;
}

int Stop_TWI(void)
{
    ADI_TWI_RESULT eResult;

    eResult = adi_twi_Close(hTwiDevice);
    CHECK_RESULT(eResult);

    return eResult;
}

/*****************************************************************************************************************************/

int ADAU_1962_init(void)
{    int i;
    ADAU_1962_Pllinit();
    for(i=0;i<28;i++)
    {
        /* write value */
        Write_TWI_8bit_Reg(Config_array_DAC[i].Reg_Add,Config_array_DAC[i].Value);
        Config_read_DAC[i]=Read_TWI_8bit_Reg(Config_array_DAC[i].Reg_Add);
        if(Config_array_DAC[i].Value!= Config_read_DAC[i])
        {
            DEBUG_INFORMATION("\n Configuring ADAU_1962 failed");
            return FAILED;
        }
    }
    return SUCCESS;
}

int ADAU_1962_Pllinit(void)
{
    int status,delay1=0xffff;

    ADI_TWI_RESULT eResult;

       eResult = adi_twi_SetHardwareAddress(hTwiDevice, TARGETADDR_1962);
    if(eResult!=ADI_TWI_SUCCESS)
    {
        REPORT_ERROR("TWI Set Hw address failed 0x%08X\n", eResult);
    }

    Write_TWI_8bit_Reg(ADAU1962_PLL_CTL_CTRL0,0x01);
    while(delay1--)
    {
        asm("nop;");
    }

    Write_TWI_8bit_Reg(ADAU1962_PLL_CTL_CTRL0,0x05);

    delay1=0xffff;
    while(delay1--)
    {
        asm("nop;");
    }

    Write_TWI_8bit_Reg(ADAU1962_PLL_CTL_CTRL1,0x22);
    delay1=0xffff;
    while(delay1--)
    {
        asm("nop;");
    }
    status=Read_TWI_8bit_Reg(ADAU1962_PLL_CTL_CTRL1);
    while(!((status & 0x4)>>2)  )
    {
        status=Read_TWI_8bit_Reg(ADAU1962_PLL_CTL_CTRL1);
    }

    return eResult;
}


int ADAU_1979_init(void)
{    int i;

    ADAU_1979_Pllinit();

    for(i=0;i<16;i++)
    {
        Write_TWI_8bit_Reg(Config_array_ADC[i].Reg_Add,Config_array_ADC[i].Value);
        Config_read_ADC[i]=Read_TWI_8bit_Reg(Config_array_ADC[i].Reg_Add);
        if(Config_array_ADC[i].Value!= Config_read_ADC[i])
        {
            DEBUG_INFORMATION("\n Configuring ADAU_1979 failed");
            return FAILED;
        }
    }

    return SUCCESS;
}

int ADAU_1979_Pllinit(void)
{

    int status,delay1=0xffff;

    ADI_TWI_RESULT eResult;

       eResult = adi_twi_SetHardwareAddress(hTwiDevice, TARGETADDR_1979);
    if(eResult!=ADI_TWI_SUCCESS)
    {
        REPORT_ERROR("TWI Set Hw address failed 0x%08X\n", eResult);
    }

       Write_TWI_8bit_Reg(ADAU1979_REG_POWER,0x01);
    Write_TWI_8bit_Reg(ADAU1979_REG_PLL,0x03);
    status=Read_TWI_8bit_Reg(ADAU1979_REG_PLL);
    while(delay1--)
            {
                asm("nop;");
            }
    while(!((status & 0x80)>>7)  )
    {
        status=Read_TWI_8bit_Reg(ADAU1979_REG_PLL);
        asm("nop;");
    }

    return eResult;
}

void Init_Twi1PinMux(void)
{
	/* PORTx_MUX registers */
	*pREG_PORTB_MUX |= TWI1_SCL_PORTB_MUX | TWI1_SDA_PORTB_MUX;

	/* PORTx_FER registers */
	*pREG_PORTB_FER |= TWI1_SCL_PORTB_FER | TWI1_SDA_PORTB_FER;
}

void Init_Twi2PinMux(void)
{
	/* PORTx_MUX registers */
	*pREG_PORTA_MUX |= TWI2_SCL_PORTA_MUX | TWI2_SDA_PORTA_MUX;

	/* PORTx_FER registers */
	*pREG_PORTA_FER |= TWI2_SCL_PORTA_FER | TWI2_SDA_PORTA_FER;
}

