/******************************************************************************
 * sample.c
 * Audio sample management.
 *-----------------------------------------------------------------------------
 * Copyright (C) 2015 - 2016 Osman Celimli
 * For conditions of distribution and use, see copyright notice in coretone.c
 ******************************************************************************/
#include <stdint.h>
#include <string.h>
#include <math.h>

#include "../types.h"
#include "coretone.h"

#include "sample.h"

/******************************************************************************
 * !!!!----   ACTIVE SAMPLE PACKAGE and the FREQUENCY RATIO TABLE    ----!!!!
 ******************************************************************************/
/*  Once a sample package has been verified and setup by this library, its
 * base address, directory start address, and sample count will be cached
 * for later use. However, we'll also be generating a 32.32 table to speed 
 * up calculations later on. This is the FREQUENCY RATIO TABLE and to fully
 * understand its purpose we must first go over PHASE INCREMENT CALCULATIONs.
 *
 *  Calculating playback phase increment (Pi) of a particular sample based
 * upon a desired frequency requires the knowledge of four components:
 * Rendering Frequency (Rf), Playback Frequency (Pf), Sample Frequency (Sf),
 * and the Sampled Frequency (Bf).
 *
 *  The naive formula is: Pi = (Sf / Rf) * (Pf / Bf), but the two divides
 * aren't desirable. Noting that Sf, Rf, and Bf are fixed once the sample
 * package is loaded, we can reformat the equation :
 * 
 *  Pi = Pf * Fr, where Fr = (Sf / (Rf * Bf)) which will be precalculated and
 * stored in 32.32 precision which should give tolerable accuracy when
 * multiplied by a 16.16 frequency to yield the final 16.16 phase increment.
 */
const char szCoreSample_Magic[] = CORETONE_SMPPAK_HEAD_MAGICWORD;
const int8_t acCoreSample_Dummy[] = {0,0,0,0};

uint8_t *pCoreSample_PackBase = NULL;
uint32_t *pCoreSample_DirBase = NULL;
uint32_t uiCoreSample_Count = 0;

int32p32_t aCoreSample_Fr[CORETONE_SAMPLES_MAXENTRIES];


/* void ct_sample_get(uint32_t uiSample, int8_t **ppData, uint32_t *puiLen)
 *  Fetch the base address and length of sample number uiSample, if the sample
 * is outside the range of the current pack a dummy sample will be provided.
 *----------------------------------------------------------------------------*/
void ct_sample_get(uint32_t uiSample, int8_t **ppData, uint32_t *puiLen)
{
	uint32_t *pEntry;

	if(uiSample < uiCoreSample_Count)
	{
		pEntry = pCoreSample_DirBase + (CORETONE_SMPPAK_ENTRY_SIZE * uiSample);
		*ppData = (int8_t*)(pCoreSample_PackBase + pEntry[CORETONE_SMPPAK_ENTRY_OFF]);
		*puiLen = pEntry[CORETONE_SMPPAK_ENTRY_SLEN];
	}
	else
	{
		*ppData = (int8_t*)acCoreSample_Dummy;
		*puiLen = 0;
	}
}

/* int16p16_t ct_sample_calcPhase(uint32_t uiSample, int16p16_t freq)
 *  Calculate the phase adjustment value for the given sample number based
 * upon a desired playback frequency in Hz.
 *----------------------------------------------------------------------------*/
int16p16_t ct_sample_calcPhase(uint32_t uiSample, int16p16_t freq)
{
	int32p32_t phaseAdj,freqBase,freqRatio;
	int16p16_t phaseTrim;

	/**
	 * Mixed precision multiply to yield a 16.16 phase increment from a
	 * 32.32 Frequency Ratio (R) * 16.16 Frequency (F) in the form of a
	 * 64 x 32 = 64-Bit multiply as :
	 *
	 *  RRRR RRRR.RRRR RRRR    The top 32-Bits of our 64-Bit result will
	 *  x         FFFF.FFFF    be the 16.16 phase increment (P), and the
	 *  -------------------    lower 32-Bits can be discarded.
	 *  PPPP.PPPP DDDD DDDD
	 *
	 * The frequency ratio has been predoubled to remove the requirement
	 * to shift the result leftward.
	 */
	if(uiSample < uiCoreSample_Count)
	{
		freqBase.lWhole = freq.iWhole;
		freqRatio.lWhole = aCoreSample_Fr[uiSample].lWhole;
		phaseAdj.ulWhole = freqBase.ulWhole * freqRatio.ulWhole;

		phaseTrim.uiWhole = phaseAdj.uiPair.uiHi;
	}
	else
	{
		phaseTrim.uiWhole = 0;
	}

	return phaseTrim;
}




/* int32_t ct_sample_setup(uint8_t *pSamplePak)
 *  Verify the integrity of the supplied sample package, setting it up as our
 * active sample pack if it checks out. The frequency ratio table is also
 * calculated during this time.
 *  Will return a nonzero value if any issues are encountered during the
 * verification or precalculation phase.
 *----------------------------------------------------------------------------*/
int32_t ct_sample_setup(uint8_t *pSamplePak)
{
	double dFr,dSf,dRf,dBf;
	double dFr_int,dFr_frac;
	int16p16_t iSf,iBf;
	int32p32_t lFr;

	uint32_t *pEntry;
	uint32_t uiX,uiY;

	/**
	 * We'll consider a sample package valid for use as long as its magic
	 * word matches what we expect, the sample count is below our limit,
	 * and it's living at a 32-Bit aligned address.
	 */
	uiY = (uint32_t)pSamplePak;
	if(0 != (uiY % sizeof(uint32_t)))
	{
		return -1;
	}

	for(uiX = 0; uiX < CORETONE_SMPPAK_HEAD_MAGICLEN; uiX++)
	{
		if(szCoreSample_Magic[uiX] != pSamplePak[uiX])
		{
			return -1;
		}
	}

	memcpy(&uiY, (pSamplePak + CORETONE_SMPPAK_HEAD_COUNT), sizeof(uint32_t));
	if(uiY > CORETONE_SAMPLES_MAXENTRIES)
	{
		return -1;
	}
	
	pCoreSample_PackBase = pSamplePak;
	pCoreSample_DirBase = (uint32_t*)(pSamplePak + CORETONE_SMPPAK_DIR_BASE);
	uiCoreSample_Count = uiY;

	/**
	 * We can now precalculate the frequency ratio table for each sample
	 * within the package using Fr = (Sf / (Rf * Bf)). Each of these results
	 * is then doubled prior to insertion in the table in order to remove
	 * a post-multiply shift requirement during phase increment calculations.
	 *
	 * I'm using floats for the basic math here since this isn't time
	 * critical and (as of writing anyway) most microcontrollers in the
	 * $5 and below bin actually include a somewhat functional FPU.
	 *
	 * For the phase increment calculations later on, we'll have to keep
	 * things purely in the integer realm.
	 */
	pEntry = pCoreSample_DirBase;
	dRf = CORETONE_RENDER_RATE;
	for(uiX = 0; uiX < uiY; uiX++)
	{
		iSf.uiWhole = pEntry[CORETONE_SMPPAK_ENTRY_SFREQ];
		iBf.uiWhole = pEntry[CORETONE_SMPPAK_ENTRY_BFREQ];

		dFr = iSf.usPair.usLo;
		dSf = iSf.usPair.usHi;
		dSf = (dFr / 65536.0) + dSf;
		dFr = iBf.usPair.usLo;
		dBf = iBf.usPair.usHi;
		dBf = (dFr / 65536.0) + dBf;

		dFr = (dSf / (dRf * dBf)) * 2.0;

		dFr_int = floor(dFr);
		dFr_frac = dFr - dFr_int;
		dFr_frac *= 4294967296.0;
		lFr.iPair.iHi = (int32_t)dFr_int;
		lFr.uiPair.uiLo = (uint32_t)dFr_frac;

		aCoreSample_Fr[uiX].lWhole = lFr.lWhole;
		pEntry += CORETONE_SMPPAK_ENTRY_SIZE;
	}
	return 0;
}
