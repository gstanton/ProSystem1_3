/******************************************************************************
 * sample.h
 * Audio sample management.
 *-----------------------------------------------------------------------------
 * Copyright (C) 2015 - 2016 Osman Celimli
 * For conditions of distribution and use, see copyright notice in coretone.c
 ******************************************************************************/
#ifndef CORETONE_SAMPLE
#define CORETONE_SAMPLE

/******************************************************************************
 * Operating Parameters
 ******************************************************************************/
#ifndef CORETONE_SAMPLES_MAXENTRIES
	#define CORETONE_SAMPLES_MAXENTRIES		256
#endif

#ifndef CORETONE_SAMPLES_MAXLENGTH
	#define CORETONE_SAMPLES_MAXLENGTH		32768
#endif

/******************************************************************************
 * Sample Package Format
 ******************************************************************************/
/* Sample Packages are composed of three major regions:
 *
 *  The HEADER, which contains an identifier string and the number of samples
 *      included within the package.
 *
 *  The DIRECTORY, which contains the DATA OFFSET, LENGTH, SAMPLE FREQUENCY (Sf),
 *      and CONTENT FREQUENCY (Bf) of each sample in the package. Each of these
 *      values is 32-Bits yielding 16-Bytes per entry.
 *
 *  The DATA AREA, which contains the 8-Bit signed PCM data for all of the
 *      samples in the package.
 *
 * It is expected that the sample package starts at a 32-Bit aligned address
 * and will remain at said address for the duration of its use by the SoftSynth.
 */
#define CORETONE_SMPPAK_HEAD_MAGICWORD	"CSMP"
#define CORETONE_SMPPAK_HEAD_MAGICLEN	4
#define CORETONE_SMPPAK_HEAD_COUNT		4
#define CORETONE_SMPPAK_HEAD_SIZE		8

#define CORETONE_SMPPAK_DIR_BASE		8

/* Note that the entry offsets below are in uint32_t counts intead of bytes */
#define CORETONE_SMPPAK_ENTRY_OFF		0
#define CORETONE_SMPPAK_ENTRY_SLEN		1
#define CORETONE_SMPPAK_ENTRY_SFREQ		2
#define CORETONE_SMPPAK_ENTRY_BFREQ		3
#define CORETONE_SMPPAK_ENTRY_SIZE		4

/******************************************************************************
 * Function Defines
 ******************************************************************************/
void ct_sample_get(uint32_t uiSample, int8_t **ppData, uint32_t *puiLen);
int16p16_t ct_sample_calcPhase(uint32_t uiSample, int16p16_t freq);

int32_t ct_sample_setup(uint8_t *pSamplePak);
#endif
