/******************************************************************************
 * cortone.c
 * User-facing portions of CoreTone. These include playback requests,
 * parameter adjustments, and the grand rendering update routine.
 *-----------------------------------------------------------------------------
 * Version 1.2.2cz, November 26th, 2016
 * Copyright (C) 2015 - 2016 Osman Celimli
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 ******************************************************************************/
#include <stdint.h>
#include <string.h>

#include "../types.h"
#include "coretone.h"

#include "sample.h"
#include "channel.h"
#include "music.h"

/******************************************************************************
 * !!!!----                 CHANNEL AND SYSTEM STATE                 ----!!!!
 ******************************************************************************/
int32_t iCoreReady;

volatile int32_t iAmPaused;
volatile int32_t iAllStopReq;


struct {
	uint8_t *pSFX;
	int8_t cPriority;

	int8_t cVol_Left,cVol_Right;
} aDispatchQueue[CORETONE_DISPATCH_DEPTH],
  aBatchQueue[CORETONE_DISPATCH_DEPTH];
volatile uint32_t uiDispatchIn,uiDispatchOut;
volatile uint32_t uiBatchIn,uiBatchOut;


typedef enum CoreReq_e
{
	eREQUEST_STOP_SFX = 0,

	eREQUEST_FOOTER
} CoreReq_t;

struct {
	int8_t cTarget;

	CoreReq_t eAction;
	uint32_t uiArg_A,uiArgB,uiArg_C;
} aReqQueue[CORETONE_REQUEST_DEPTH];
volatile uint32_t uiReqIn,uiReqOut;


volatile uint8_t *pMusTrack;
volatile int8_t cMusVol;

volatile int32_t iMusPlaying,iMusMood;
volatile int32_t iMusPlayReq,iMusStopReq,iMusAttenReq;

volatile ct_renderCall_t pRenderCall;

CoreChannel_t aCoreChannels[CORETONE_CHANNELS];
CorePatch_t aCorePatches[CORETONE_CHANNELS];
CoreTrack_t aCoreTracks[CORETONE_CHANNELS];


/******************************************************************************
 * !!!!----            UPDATE TICK AND WAVEFORM RENDERING            ----!!!!
 ******************************************************************************/
/* void ct_update(int16_t *pBuffer)
 *  Update patch scripts of any currently decoding music and sound effects,
 * then render the summed output of all channels to the supplied buffer .
 * Render length is fixed at CORETONE_BUFFER_LEN stereo samples, and this
 * function should be called at a rate of CORETONE_DECODE_RATE Hz.
 *----------------------------------------------------------------------------*/
void ct_update(int16_t *pBuffer)
{
	uint32_t uiX,uiZ;
	int32_t iY;

	/**
	 * ---- STOP REQUESTS ----
	 * For music, sound effects, or both. This is performed first in the
	 * update procedure in order to ensure we'll free up any channels
	 * which will no longer be needed before advancing to the more
	 * expensive decode, dispatch, and render steps.
	 */
	if(0 != (iMusStopReq || iAllStopReq))
	{
		for(uiX = 0; uiX < CORETONE_CHANNELS; uiX++)
		{
			aCoreTracks[uiX].iPriority = 0;
			aCoreTracks[uiX].ucNote = CORETONE_MUSIC_NOTE_INVALID;

			if(aCorePatches[uiX].iInstrument || iAllStopReq)
			{
				aCorePatches[uiX].iInstrument = 0;
				aCorePatches[uiX].iPriority = 0;

				aCoreChannels[uiX].eMode = eCHANNEL_MODE_OFF;
			}
		}

		iMusPlaying = 0;
		iMusMood = 0;
	}
	iAllStopReq = 0;
	iMusStopReq = 0;


	/**
	 * ---- MUSIC DISPATCH AND DECODE ----
	 * If the playback of a new music track has been requested, kick
	 * it off. If one is already playing, continue its decoding unless
	 * we're in a paused state..
	 */
	if(iMusPlayReq)
	{
		iMusPlaying = !ct_music_setup((uint8_t*)pMusTrack);
		iMusMood = 0;
	}
	iMusPlayReq = 0;

	if(iMusAttenReq)
	{
		for(uiX = 0; uiX < CORETONE_CHANNELS; uiX++)
		{
			aCoreTracks[uiX].iRecalcVol = -1;
			aCoreTracks[uiX].cVolMain = cMusVol;
		}

		iMusAttenReq = 0;
	}

	if(iMusPlaying && !iAmPaused)
	{
		uiZ = 0;
		for(uiX = 0; uiX < CORETONE_CHANNELS; uiX++)
		{
			ct_music_decode(&(aCoreTracks[uiX]));

			if(0 != aCoreTracks[uiX].iPriority)
			{
				uiZ++;
			}
		}

		if(0 == uiZ)
		{
			iMusPlaying = 0;
			iMusMood = 0;
		}
	}

	/**
	 * ---- SOUND EFFECT DISPATCH ----
	 * Check the sound effect dispatch queue for any new noises to play,
	 * assigning each of their patches to channels which are either free
	 * or playing a macro of lower priority.
	 *
	 * Sound effects begin dispatch from the last channel while music
	 * traditionally plays relative to the first in order to reduce
	 * contention between the two.
	 */
	if(uiDispatchIn != uiDispatchOut)
	{
		uiZ = uiDispatchOut;
		while(uiZ != uiDispatchIn)
		{
			ct_sfx_dispatch(aDispatchQueue[uiZ].pSFX,
				aDispatchQueue[uiZ].cPriority,
				aDispatchQueue[uiZ].cVol_Left,
				aDispatchQueue[uiZ].cVol_Right);

			uiZ = (uiZ + 1) % CORETONE_DISPATCH_DEPTH;
		}

		uiDispatchOut = uiZ;
	}

	/**
	 * ---- ACTION REQUESTS ---
	 * General effects such as stopping the playback of a sound effect.
	 */
	if(uiReqIn != uiReqOut)
	{
		uiZ = uiReqOut;
		while(uiZ != uiReqIn)
		{
			switch(aReqQueue[uiZ].eAction)
			{
				case eREQUEST_STOP_SFX:
					ct_sfx_stop(aReqQueue[uiZ].cTarget);
					break;

				default:
					break;
			}

			uiZ = (uiZ + 1) % CORETONE_REQUEST_DEPTH;
		}

		uiReqOut = uiZ;
	}

	/**
	 * ---- PATCH DECODE AND WAVEFORM RENDERING ----
	 * Walk through each of the active patches (nonzero priority) and their
	 * respective channels, in order. The first active channel gets to write
	 * directly to the new outgoing buffer, "stamping" it. Others will just
	 * accumulate their rendered output with the previous buffer value.
	 *
	 * If we're paused, this entire step will be skipped and we'll fall into
	 * the buffer clear routine below.
	 */
	iY = -1;
	if(!iAmPaused)
	{
		for(uiX = 0; uiX < CORETONE_CHANNELS; uiX++)
		{
			if(aCorePatches[uiX].iPriority)
			{
				ct_patch_decode(&(aCorePatches[uiX]));
				ct_patch_recalc(&(aCorePatches[uiX]));
			}

			if(eCHANNEL_MODE_OFF != aCoreChannels[uiX].eMode)
			{
				ct_channel_render(&(aCoreChannels[uiX]), pBuffer, iY);
				iY = 0;
			}
		}
	}

	/**
	 * If none of the channels were enabled (complete silence) just
	 * zero out the buffer before we leave.
	 */
	if(0 != iY)
	{
		for(uiX = 0; uiX < CORETONE_BUFFER_LEN; uiX++)
		{
			pBuffer[uiX] = CORETONE_BUFFER_CENTER;
		}
	}

	/**
	 * Supply our finished buffer to any post-render callbacks if
	 * they're currently active. The callback will be automatically
	 * disabled if it returns zero.
	 */
	if(NULL != pRenderCall)
	{
		if(0 == pRenderCall(pBuffer,
			CORETONE_RENDER_RATE, CORETONE_BUFFER_SAMPLES,
			iAmPaused))
		{
			pRenderCall = NULL;
		}
	}
}




/******************************************************************************
 * !!!!----                GLOBAL PLAYBACK CONTROL                   ----!!!!
 ******************************************************************************/
/* void ct_pause(void)
 *  Pause (and silence) the decoding of all active music and sound effects,
 * post-render callbacks will still be allowed to run and will be notified
 * regarding the current pause status.
 *----------------------------------------------------------------------------*/
void ct_pause(void)
{
	iAmPaused = -1;
}

/* void ct_resume(void)
 *  Resume audio playback from a paused state.
 *----------------------------------------------------------------------------*/
void ct_resume(void)
{
	iAmPaused = 0;
}

/* int32_t ct_isPaused(void)
 *  Indicates if CoreTone is currently paused (nonzero) or unpaused (zero).
 *----------------------------------------------------------------------------*/
int32_t ct_isPaused(void)
{
	return iAmPaused;
}

/* void ct_stopAll(void)
 *  Halt the decoding and playback off all sound effects and music.
 *----------------------------------------------------------------------------*/
void ct_stopAll(void)
{
	if(iCoreReady)
	{
		iAllStopReq = -1;
	}
}

/* void ct_setRenderCall(ct_renderCall_t pCall)
 *  Set the current render callback to pCall.
 *----------------------------------------------------------------------------*/
void ct_setRenderCall(ct_renderCall_t pCall)
{
	if(iCoreReady)
	{
		pRenderCall = pCall;
	}
}




/******************************************************************************
 * !!!!----                 MUSIC PLAYBACK CONTROL                   ----!!!!
 ******************************************************************************/
/* void ct_playMusic(uint8_t *pMusic)
 *  Reqest the playback of the music track whose base address is at pMusic
 * on the next update tick. Will "prestop" the current playing track (if any).
 *----------------------------------------------------------------------------*/
void ct_playMusic(uint8_t *pMusic)
{
	if(iCoreReady && (NULL != pMusic))
	{
		ct_stopMusic();

		pMusTrack = pMusic;
		iMusPlayReq = -1;
	}
}

/* void ct_stopMusic(void)
 *  Request any currently playing music to cease on the next update tick.
 *----------------------------------------------------------------------------*/
void ct_stopMusic(void)
{
	if(iCoreReady)
	{
		iMusStopReq = -1;
	}
}

/* void ct_attenMusic(int8_t cVol)
 *  Request a change in volume for the currently playing music (if any) on
 * the next update tick, zero is silent and 127 is loudest.
 *----------------------------------------------------------------------------*/
void ct_attenMusic(int8_t cVol)
{
	if(iCoreReady)
	{
		cMusVol = cVol;
		iMusAttenReq = -1;
	}
}

/* int32_t ct_checkMusic(void)
 *  Check to see whether the music is playing (nonzero return) or not (zero),
 * useful to determine if a single-shot track has finished or if a stop
 * request has completed.
 *----------------------------------------------------------------------------*/
int32_t ct_checkMusic(void)
{
	if(iCoreReady)
	{
		return iMusPlaying;
	}

	return 0;
}

/* int32_t ct_getMood(void)
 *  Get the mood flag of the currently playing music track. If no music is
 * playing, the mood flag will be zero (neutral).
 *----------------------------------------------------------------------------*/
int32_t ct_getMood(void)
{
	return iMusMood;
}




/******************************************************************************
 * !!!!----                  SFX PLAYBACK CONTROL                    ----!!!!
 ******************************************************************************/
/* void ct_playSFX(uint8_t *pSFX, int8_t cPriority,
 *                 int8_t cVol_Left, int8_t cVol_Right)
 *  Request the playback of the sound effect pSFX on the next update tick with
 * priority ucPriority and panning cVol_Left and cVol_Right.
 *----------------------------------------------------------------------------*/
void ct_playSFX(uint8_t *pSFX, int8_t cPriority,
                int8_t cVol_Left, int8_t cVol_Right)
{
	uint32_t uiX,uiY;

	if(iCoreReady && (0 != cPriority) && (NULL != pSFX))
	{
		uiX = uiDispatchIn;
		uiY = (uiX + 1) % CORETONE_DISPATCH_DEPTH;
		if(uiY != uiDispatchOut)
		{
			aDispatchQueue[uiX].pSFX = pSFX;

			aDispatchQueue[uiX].cPriority = cPriority;
			aDispatchQueue[uiX].cVol_Left = cVol_Left;
			aDispatchQueue[uiX].cVol_Right = cVol_Right;
			uiDispatchIn = uiY;
		}
	}
}

/* void ct_stopSFX(uint8_t ucPriority)
 *  Request all currently decoding sound effects with the priority ucPriority
 * to cease playback on the next update tick.
 *----------------------------------------------------------------------------*/
void ct_stopSFX(int8_t cPriority)
{
	uint32_t uiX,uiY;

	if(iCoreReady && (0 != cPriority))
	{
		uiX = uiReqIn;
		uiY = (uiX + 1) % CORETONE_REQUEST_DEPTH;
		if(uiY != uiReqOut)
		{
			aReqQueue[uiX].cTarget = cPriority;

			aReqQueue[uiX].eAction = eREQUEST_STOP_SFX;
			uiReqIn = uiY;
		}
	}
}

/* void ct_addSFX(uint8_t *pSFX, int8_t cPriority,
 *                int8_t cVol_Left, int8_t cVol_Right)
 *  Request the playback of the sound effect pSFX on the next batch dump with
 * priority ucPriority and panning cVol_Left and cVol_Right. This can be used
 * to synchronize the start of multiple sound effects.
 *----------------------------------------------------------------------------*/
void ct_addSFX(uint8_t *pSFX, int8_t cPriority,
               int8_t cVol_Left, int8_t cVol_Right)
{
	uint32_t uiX,uiY;

	if(iCoreReady && (0 != cPriority) && (NULL != pSFX))
	{
		uiX = uiBatchIn;
		uiY = (uiX + 1) % CORETONE_DISPATCH_DEPTH;
		if(uiY != uiBatchOut)
		{
			aBatchQueue[uiX].pSFX = pSFX;

			aBatchQueue[uiX].cPriority = cPriority;
			aBatchQueue[uiX].cVol_Left = cVol_Left;
			aBatchQueue[uiX].cVol_Right = cVol_Right;
			uiBatchIn = uiY;
		}
	}
}

/* void ct_dumpSFX(void)
 *  Play all sound effects in the current batch set using ct_playSFX(), this
 * can be used to synchronize the start of multiple sound effects.
 *----------------------------------------------------------------------------*/
void ct_dumpSFX(void)
{
	uint32_t uiX,uiY;

	if(iCoreReady)
	{
		while(uiBatchIn != uiBatchOut)
		{
			uiX = uiBatchOut;
			uiY = (uiX + 1) % CORETONE_DISPATCH_DEPTH;

			ct_playSFX(aBatchQueue[uiX].pSFX,
				aBatchQueue[uiX].cPriority,
                aBatchQueue[uiX].cVol_Left, aBatchQueue[uiX].cVol_Right);
			uiBatchOut = uiY;
		}
	}
}




/******************************************************************************
 * !!!!----                       MUTEX ACCESS                       ----!!!!
 ******************************************************************************/
/* int32_t ct_getMutex(void)
 *  Aquire the CoreTone access mutex, upon which the caller will be guaranteed
 * that CoreTone will not perform an update until the mutex is released. This
 * is expected to be implemented on the platform-specific layer on top of
 * CoreTone, and therefore this function will always return nonzero (failure).
 *----------------------------------------------------------------------------*/
int32_t ct_getMutex(void)
{
	return -1;
}

/* int32_t ct_giveMutex(void)
 *  Release the CoreTone access mutex. This is expected to be implemented on
 * the platform-specific layer on top of CoreTone, and therefore this function
 * will always return nonzero (failure).
 *----------------------------------------------------------------------------*/
int32_t ct_giveMutex(void)
{
	return -1;
}




/******************************************************************************
 * !!!!----                INITIALIZATION & DIAGNOSTICS              ----!!!!
 ******************************************************************************/
/* int32_t ct_init(uint8_t *pSamplePak, uint8_t *pInstrPak)
 *  Initialize the CoreTone library with the supplied sample and instrument
 * packages, should be run once at startup. Will return a nonzero value if
 * any errors occured during the setup procedure.
 *----------------------------------------------------------------------------*/
int32_t ct_init(uint8_t *pSamplePak, uint8_t *pInstrPak)
{
	int32_t iFailed = 0;
	uint32_t uiX;

	iAllStopReq = 0;

	uiDispatchIn = 0;
	uiDispatchOut = 0;
	uiBatchIn = 0;
	uiBatchOut = 0;
	uiReqIn = 0;
	uiReqOut = 0;

	pMusTrack = NULL;
	cMusVol = CORETONE_DEFAULT_VOLUME;
	iMusAttenReq = -1;

	iMusPlaying = 0;
	iMusPlayReq = 0;
	iMusStopReq = 0;

	pRenderCall = NULL;

	for(uiX = 0; uiX < CORETONE_CHANNELS; uiX++)
	{
		memset(&(aCoreChannels[uiX]), 0, sizeof(CoreChannel_t));
		aCoreChannels[uiX].eMode = eCHANNEL_MODE_OFF;

		memset(&(aCorePatches[uiX]), 0, sizeof(CorePatch_t));
		aCorePatches[uiX].iPriority = 0;
		aCorePatches[uiX].pChannel = &(aCoreChannels[uiX]);
		
		memset(&(aCoreTracks[uiX]), 0, sizeof(CoreTrack_t));
		aCoreTracks[uiX].iPriority = 0;
		aCoreTracks[uiX].ucNote = CORETONE_MUSIC_NOTE_INVALID;
		aCoreTracks[uiX].pChannel = &(aCoreChannels[uiX]);
		aCoreTracks[uiX].pPatch = &(aCorePatches[uiX]);
	}

	iFailed |= ct_sample_setup(pSamplePak);
	iFailed |= ct_instr_setup(pInstrPak);

	iCoreReady = !iFailed;
	return iFailed;
}

/* void ct_getState(void **ppaChannels, void **ppaPatches, void **ppaTracks)
 *  Store the base addresses of CoreTone's channel, patch, and track states
 * into *ppaChannels, *ppaPatches, and *ppaTracks respectively which can be
 * used to observe (and only observe) internal activity.
 *----------------------------------------------------------------------------*/
void ct_getState(void **ppaChannels, void **ppaPatches, void **ppaTracks)
{
	if(NULL != ppaChannels)
		*ppaChannels = (void*)&aCoreChannels;
	if(NULL != ppaPatches)
		*ppaPatches = (void*)&aCorePatches;
	if(NULL != ppaTracks)
		*ppaTracks = (void*)&aCoreTracks;
}

/* void ct_getInfo(uint32_t *puiChannels, uint32_t *puiRenderFreq,
 *                 uint32_t *puiDecodeRate,
 *                 uint32_t *puiSamples, uint32_t *puiSampleLen)
 *  Get information about the CoreTone build's capabilities and requirements
 * including channel count, render frequency, decode rate, maximum sample
 * package size, and sample length.
 *----------------------------------------------------------------------------*/
void ct_getInfo(uint32_t *puiChannels, uint32_t *puiRenderFreq,
                uint32_t *puiDecodeRate,
                uint32_t *puiSamples, uint32_t *puiSampleLen)
{
	if(NULL != puiChannels)
		*puiChannels = CORETONE_CHANNELS;
	if(NULL != puiRenderFreq)
		*puiRenderFreq = CORETONE_RENDER_RATE;

	if(NULL != puiDecodeRate)
		*puiDecodeRate = CORETONE_DECODE_RATE;

	if(NULL != puiSamples)
		*puiSamples = CORETONE_SAMPLES_MAXENTRIES;
	if(NULL != puiSampleLen)
		*puiSampleLen = CORETONE_SAMPLES_MAXLENGTH;
}

