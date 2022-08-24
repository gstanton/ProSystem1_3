/******************************************************************************
 * music.h
 * Music script decoding and playback control.
 *-----------------------------------------------------------------------------
 * Copyright (C) 2015 - 2016 Osman Celimli
 * For conditions of distribution and use, see copyright notice in coretone.c
 ******************************************************************************/
#ifndef CORETONE_MUSIC
#define CORETONE_MUSIC
/******************************************************************************
 * Operating Parameters
 ******************************************************************************/
/* This indicates the stack depth used to track music script loops,
 * the default of four should be adequate for most users.
 */
#ifndef CORETONE_MUSIC_STACKDEPTH
	#define CORETONE_MUSIC_STACKDEPTH		4
#endif

/******************************************************************************
 * Instrument Package Format
 ******************************************************************************/
/* Instrument Packages are composed of three major regions:
 *
 *  The HEADER, which contains an identifier string and the number of
 *		instruments included within the package.
 *
 *  The DIRECTORY, which contains the SAMPLE ID, SCRIPT OFFSET, and NOTE OFF
 *		OFFSET (relative to the script start) of each instrument in the package.
 *		All of these values are 32-Bits long, yielding 12-Bytes per entry.
 *
 *  The DATA AREA, which contains the patch scripts for all instruments
 *		in the package.
 *
 * It is expected that the instrument package starts at a 32-Bit aligned address
 * and will remain at said address for the duration of its use by the SoftSynth.
 */
#define CORETONE_INSPAK_HEAD_MAGICWORD	"CINS"
#define CORETONE_INSPAK_HEAD_MAGICLEN	4
#define CORETONE_INSPAK_HEAD_COUNT		4
#define CORETONE_INSPAK_HEAD_SIZE		8

#define CORETONE_INSPAK_DIR_BASE		8

/* Note that the entry offsets below are in uint32_t counts intead of bytes */
#define CORETONE_INSPAK_ENTRY_SAMPLE	0
#define CORETONE_INSPAK_ENTRY_SCRIPT	1
#define CORETONE_INSPAK_ENTRY_NOTE_OFF	2
#define CORETONE_INSPAK_ENTRY_LEN		3

/******************************************************************************
 * Music Binary Format
 ******************************************************************************/
/* Music Binaries also use the traditional three-region structure:
 *
 *  The HEADER, which contains an identifier string and the number of
 *		tracks in the piece of music.
 *
 *  The DIRECTORY, which contains the INITIAL PRIORITY (8-Bit) and
 *		TRACK SCRIPT OFFSET (32-Bit), yielding 5-Bytes per entry.
 *
 *  The DATA AREA, which contains the track scripts.
 *
 * Unlike Sample and Instrument packages, no alignment restrictions are
 * imposed on Music Binaries.
 */
#define CORETONE_MUSPAK_HEAD_MAGICWORD	"CMUS"
#define CORETONE_MUSPAK_HEAD_MAGICLEN	4
#define CORETONE_MUSPAK_HEAD_TRACKS		4
#define CORETONE_MUSPAK_HEAD_SIZE		8

#define CORETONE_MUSPAK_DIR_BASE		8

#define CORETONE_MUSPAK_ENTRY_PRIORITY	0
#define CORETONE_MUSPAK_ENTRY_OFFSET	1
#define CORETONE_MUSPAK_ENTRY_LEN		5

/******************************************************************************
 * Track Script Commands and Decode Descriptors
 ******************************************************************************/
#define CORETONE_MUSIC_SET_PRIORITY		0
#define CORETONE_MUSIC_SET_PANNING		1
#define CORETONE_MUSIC_SET_INSTRUMENT	2
#define CORETONE_MUSIC_NOTE_ON			3
#define CORETONE_MUSIC_NOTE_OFF			4
#define CORETONE_MUSIC_PITCH			5
#define CORETONE_MUSIC_LOOP_START		6
#define CORETONE_MUSIC_LOOP_END			7
#define CORETONE_MUSIC_CALL				8
#define CORETONE_MUSIC_RETURN			9
#define CORETONE_MUSIC_BREAK			10
#define CORETONE_MUSIC_NOP				11
#define CORETONE_MUSIC_SET_MOOD			12

#define CORETONE_MUSIC_FOOTER			13

/* Wait commands work identically for music and patch scripts, varlength delay
 * in driver ticks where the MSB of each byte is used to indicate the value is
 * either the start or continuation of a delay.
 *
 * See "channel.h" for a full description.
 */
#define CORETONE_MUSIC_WAIT				0x80
#define CORETONE_MUSIC_WAIT_MASK		0x7F

/* The loop stack isn't just used for LOOPs, CALLs and RETURNs will also push
 * and pop their decode addresses to it. However, they use a special count
 * value in order to allow backwards traversal of the stack for BREAKs.
 */
#define CORETONE_MUSIC_CALL_TAG			-128

/* The last dispatched note will be cached in each track's ucNote, but when
 * nothing is playing this will be tagged with the value below.
 */
#define CORETONE_MUSIC_NOTE_INVALID		0x80

typedef struct CoreTrack_s
{
	CoreChannel_t *pChannel;
	CorePatch_t *pPatch;

	int32_t iPriority,iRecalcVol;

	uint32_t uiInstSel;
	uint8_t ucNote;

	uint8_t *pScript;
	uint32_t uiOffset;
	uint32_t uiDel;

	int8_t cVolMain;
	int8_t cPanLeft,cPanRight;
	int8_t cVolLeft,cVolRight;

	uint32_t uiStackPos;
	int32_t aiLoopStack[CORETONE_PATCH_STACKDEPTH];
	uint32_t auiAddrStack[CORETONE_PATCH_STACKDEPTH];
} CoreTrack_t;

/******************************************************************************
 * Function Defines
 ******************************************************************************/
int32_t ct_instr_setup(uint8_t *pInstrPak);

void ct_music_recalcVol(CoreTrack_t *pTrack);
void ct_music_decode(CoreTrack_t *pTrack);
int32_t ct_music_setup(uint8_t *pMusic);
#endif
