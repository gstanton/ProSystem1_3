/******************************************************************************
 * channel.h
 * Waveform rendering and patch script decoding.
 *-----------------------------------------------------------------------------
 * Copyright (C) 2015 - 2016 Osman Celimli
 * For conditions of distribution and use, see copyright notice in coretone.c
 ******************************************************************************/
#ifndef CORETONE_CHANNEL
#define CORETONE_CHANNEL
/******************************************************************************
 * Operating Parameters
 ******************************************************************************/
/* This indicates the stack depth used to track patch script loops,
 * the default of four should be adequate for most users.
 */
#ifndef CORETONE_PATCH_STACKDEPTH
	#define CORETONE_PATCH_STACKDEPTH		4
#endif

/******************************************************************************
 * Channel Descriptors
 ******************************************************************************/
typedef enum CoreChannel_Mode_e
{
	eCHANNEL_MODE_OFF			= 0,
	eCHANNEL_MODE_SINGLESHOT,
	eCHANNEL_MODE_LOOP,

	eCHANNEL_MODE_FOOTER
} CoreChannel_Mode_t;

typedef struct CoreChannel_s
{
	CoreChannel_Mode_t eMode;

	int8_t *pSample;
	uint16_t usSampleLen;

	int8_t cVolMain;
	int8_t cVolLeft,cVolRight;

	int16p16_t phaseAcc,phaseAdj;
	int16_t usLoopStart,usLoopEnd;
} CoreChannel_t;

/******************************************************************************
 * Sound Effect Format
 ******************************************************************************/
/* Sound Effect Binaries are composed of three major regions:
 *
 *  The HEADER, which contains an identifier string and the number of patch
 *		channels present in the sound effect data.
 *
 *  The DIRECTORY, which contains the SAMPLE ID and SCRIPT OFFSET of each
 *		channel, in order. Both of these values are 32-Bits, yielding
 *		8-Bytes per directory entry.
 *
 *  The DATA AREA, which contains the patch scripts for all channels.
 *
 * Sound effects should be located at a 32-Bit aligned address and not fiddled
 * with while they're in use. Playback priorities and panning are assigned
 * during dispatch requests using ct_playSFX().
 */
#define CORETONE_SFXPAK_HEAD_MAGICWORD	"CSFX"
#define CORETONE_SFXPAK_HEAD_MAGICLEN	4
#define CORETONE_SFXPAK_HEAD_COUNT		4
#define CORETONE_SFXPAK_HEAD_SIZE		8

#define CORETONE_SFXPAK_DIR_BASE		8

/* Note that the entry offsets below are in uint32_t counts intead of bytes */
#define CORETONE_SFXPAK_ENTRY_SAMPLE	0
#define CORETONE_SFXPAK_ENTRY_SCRIPT	1
#define CORETONE_SFXPAK_ENTRY_LEN		2

/******************************************************************************
 * Patch Script Commands and Decode Descriptors
 ******************************************************************************/
#define	CORETONE_PATCH_END				0
#define CORETONE_PATCH_MODE_SINGLESHOT	1
#define CORETONE_PATCH_MODE_LOOP		2
#define CORETONE_PATCH_VOLUME			3
#define CORETONE_PATCH_FREQUENCY		4
#define CORETONE_PATCH_LOOP_START		5
#define CORETONE_PATCH_LOOP_END			6
#define CORETONE_PATCH_NOP				7

#define CORETONE_PATCH_FOOTER			8

/* Wait commands are a special case which is somewhat borrowed from MIDI,
 * the delay (in ticks) is variable length up to four bytes long. The MSB
 * of each byte indicates whether or not to extend the delay count and the
 * seven remaining bits are placed into bits 7-0, 14-8, 21-15, or 28-22
 * of said accumulated count.
 * All other patch commands are below 0x7F and have their MSB clear, so
 * there shouldn't be any concern of crossover.
 */
#define CORETONE_PATCH_WAIT				0x80
#define CORETONE_PATCH_WAIT_MASK		0x7F

typedef struct CorePatch_s
{
	CoreChannel_t *pChannel;

	int32_t iInstrument,iPriority;

	uint8_t *pScript;
	uint32_t uiOffset,uiNoteOff;
	uint32_t uiDel;

	int16p16_t freqBase;
	int16p16_t freqPitch,pitchAdj;
	int16p16_t freqOffset,offsetAdj;

	int8p8_t volCur,volAdj;

	uint32_t uiStackPos;
	int32_t aiLoopStack[CORETONE_PATCH_STACKDEPTH];
	uint32_t auiAddrStack[CORETONE_PATCH_STACKDEPTH];
} CorePatch_t;

/******************************************************************************
 * Function Defines
 ******************************************************************************/
void ct_channel_render(CoreChannel_t *pChannel, int16_t *pBuffer, const int32_t iStamp);

void ct_patch_recalc(CorePatch_t *pPatch);
void ct_patch_keyOn(CorePatch_t *pPatch);
void ct_patch_keyOff(CorePatch_t *pPatch);
void ct_patch_decode(CorePatch_t *pPatch);

void ct_sfx_dispatch(uint8_t *pSFX, int8_t cPriority, int8_t cVol_Left, int8_t cVol_Right);
void ct_sfx_stop(int8_t cPriority);
#endif
