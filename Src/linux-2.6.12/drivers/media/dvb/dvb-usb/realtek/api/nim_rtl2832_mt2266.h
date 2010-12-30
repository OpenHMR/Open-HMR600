#ifndef __NIM_RTL2832_MT2266
#define __NIM_RTL2832_MT2266

/**

@file

@brief   RTL2832 MT2266 NIM module declaration

One can manipulate RTL2832 MT2266 NIM through RTL2832 MT2266 NIM module.
RTL2832 MT2266 NIM module is derived from DVB-T NIM module.



@par Example:
@code

// The example is the same as the NIM example in dvbt_nim_base.h except the listed lines.



#include "nim_rtl2832_mt2266.h"


...



int main(void)
{
	DVBT_NIM_MODULE *pNim;
	DVBT_NIM_MODULE DvbtNimModuleMemory;
	RTL2832_EXTRA_MODULE Rtl2832ExtraModuleMemory;
	MT2266_EXTRA_MODULE Mt2266ExtraModuleMemory;

	...



	// Build RTL2832 MT2266 NIM module.
	BuildRtl2832Mt2266Module(
		&pNim,
		&DvbtNimModuleMemory,

		9,								// Maximum I2C reading byte number is 9.
		8,								// Maximum I2C writing byte number is 8.
		CustomI2cRead,					// Employ CustomI2cRead() as basic I2C reading function.
		CustomI2cWrite,					// Employ CustomI2cWrite() as basic I2C writing function.
		CustomWaitMs,					// Employ CustomWaitMs() as basic waiting function.

		&Rtl2832ExtraModuleMemory,		// Employ RTL2832 extra module for RTL2832 module.
		0x20,							// The RTL2832 I2C device address is 0x20 in 8-bit format.
		CRYSTAL_FREQ_28800000HZ,		// The RTL2832 crystal frequency is 28.8 MHz.
		RTL2832_APPLICATION_STB,		// The RTL2832 application mode is STB mode.
		50,								// The RTL2832 update function reference period is 50 millisecond
		ON,								// The RTL2832 Function 1 enabling status is on.

		&Mt2266ExtraModuleMemory,		// Employ MT2266 extra module for MT2266 module.
		0xc0							// The MT2266 I2C device address is 0xc0 in 8-bit format.
		);



	// See the example for other NIM functions in dvbt_nim_base.h

	...


	return 0;
}


@endcode

*/


#include "demod_rtl2832.h"
#include "tuner_mt2266.h"
#include "dvbt_nim_base.h"





// Definitions
#define RTL2832_MT2266_ADDITIONAL_INIT_REG_TABLE_LEN		21

#define RTL2832_MT2266_IF_AGC_MIN_BIT_NUM		8
#define RTL2832_MT2266_IF_AGC_MIN_INT_MAX		36
#define RTL2832_MT2266_IF_AGC_MIN_INT_MIN		-64
#define RTL2832_MT2266_IF_AGC_MIN_INT_STEP		0





// RTL2832 MT2266 extra module
typedef struct RTL2832_MT2266_EXTRA_MODULE_TAG RTL2832_MT2266_EXTRA_MODULE;
struct RTL2832_MT2266_EXTRA_MODULE_TAG
{
	// Extra variables
	unsigned char LnaConfig;
	unsigned char UhfSens;
	unsigned char AgcCurrentState;
	unsigned long LnaGainOld;
};





// Builder
void
BuildRtl2832Mt2266Module(
	DVBT_NIM_MODULE **ppNim,								// DVB-T NIM dependence
	DVBT_NIM_MODULE *pDvbtNimModuleMemory,
	RTL2832_MT2266_EXTRA_MODULE *pRtl2832Mt2266ExtraModuleMemory,

	unsigned char I2cReadingByteNumMax,						// Base interface dependence
	unsigned char I2cWritingByteNumMax,
	BASE_FP_I2C_READ I2cRead,
	BASE_FP_I2C_WRITE I2cWrite,
	BASE_FP_WAIT_MS WaitMs,

	RTL2832_EXTRA_MODULE *pRtl2832ExtraModuleMemory,		// Demod dependence
	unsigned char DemodDeviceAddr,
	unsigned long DemodCrystalFreqHz,
	int DemodAppMode,
	int DemodTsInterfaceMode,
	unsigned long DemodUpdateFuncRefPeriodMs,
	int DemodIsFunc1Enabled,

	MT2266_EXTRA_MODULE *pMt2266ExtraModuleMemory,			// Tuner dependence
	unsigned char TunerDeviceAddr
	);





// RTL2832 MT2266 NIM manipulaing functions
int
rtl2832_mt2266_Initialize(
	DVBT_NIM_MODULE *pNim
	);

int
rtl2832_mt2266_SetParameters(
	DVBT_NIM_MODULE *pNim,
	unsigned long RfFreqHz,
	int BandwidthMode
	);

int
rtl2832_mt2266_UpdateFunction(
	DVBT_NIM_MODULE *pNim
	);





// The following context is source code provided by Microtune.





// Additional definition for mt_control.c
//typedef unsigned char    uint8_t;
//typedef unsigned short   uint16_t;
//typedef unsigned long    uint32_t;
typedef void *           handle_t;

#define MT2266_DEMOD_ASSUMED_AGC_REG_BIT_NUM		16



// Microtune source code - mt_control.c



/*  $Id: mt_control.c,v 1.6 2008/01/02 12:04:39 tune\tpinz Exp $ */
/*! 
 * \file mt_control.c 
 * \author Thomas Pinz, Microtune GmbH&Co KG
 * \author Marie-Curie-Str. 1, 85055 Ingolstadt
 * \author E-Mail: thomas.pinz@microtune.com
 */


#define LNAGAIN_MIN 0
#define LNAGAIN_MAX 14

#define AGC_STATE_START 0
#define AGC_STATE_LNAGAIN_BELOW_MIN 1
#define AGC_STATE_LNAGAIN_ABOVE_MAX 2
#define AGC_STATE_NORMAL 3
#define AGC_STATE_NO_INTERFERER 4
#define AGC_STATE_MEDIUM_INTERFERER 5
#define AGC_STATE_GRANDE_INTERFERER 6
#define AGC_STATE_MEDIUM_SIGNAL 7
#define AGC_STATE_GRANDE_SIGNAL 8
#define AGC_STATE_MAS_GRANDE_SIGNAL 9
#define AGC_STATE_MAX_SENSITIVITY 10
#define AGC_STATE_SMALL_SIGNAL 11


UData_t
demod_get_pd(
	handle_t demod_handle,
	uint16_t *pd_value
	);

UData_t
demod_get_agc(
	handle_t demod_handle,
	uint16_t *rf_level,
	uint16_t *bb_level
	);

UData_t
demod_set_bbagclim(
	handle_t demod_handle,
	int on_off_status
	);

UData_t
tuner_set_bw_normal(
	handle_t tuner_handle,
	handle_t demod_handle
	);

UData_t
tuner_set_bw_narrow(
	handle_t tuner_handle,
	handle_t demod_handle
	);

UData_t
demod_pdcontrol_reset(
	handle_t demod_handle,
	handle_t tuner_handle,
	uint8_t *agc_current_state
	);

UData_t
demod_pdcontrol(
	handle_t demod_handle,
	handle_t tuner_handle,
	uint8_t* lna_config,
	uint8_t* uhf_sens,
	uint8_t *agc_current_state,
	uint32_t *lna_gain_old
	);







#endif


