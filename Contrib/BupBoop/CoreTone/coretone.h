/******************************************************************************
 * coretone.h
 * Software wavetable synthesizer.
 *-----------------------------------------------------------------------------
 * Copyright (C) 2015 - 2016 Osman Celimli
 * For conditions of distribution and use, see copyright notice in coretone.c
 ******************************************************************************/
#ifndef CORETONE
#define CORETONE
/******************************************************************************
 * Operating Parameters
 ******************************************************************************/
/* CORETONE_CHANNELS is the total number of channels available for rendering
 * music and sound effects into. Increasing this number will only drastically
 * increase render time when more channels are active, but memory usage will
 * always go up.
 *
 * CORETONE_DEFAULT_VOLUME is how loud music will be played when CoreTone is
 * first initialized.
 *
 * CORETONE_DISPATCH_DEPTH and CORETONE_REQUEST_DEPTH specify the depth of the
 * dispatch and request queues for sound effects. Increasing these allows more
 * sound effects to be queued up for playback or manipulation by the user in
 * between driver ticks.
 */
#ifndef CORETONE_CHANNELS
	#define CORETONE_CHANNELS			16
#endif

#ifndef CORETONE_DEFAULT_VOLUME
	#define CORETONE_DEFAULT_VOLUME		127
#endif

#ifndef CORETONE_DISPATCH_DEPTH
	#define CORETONE_DISPATCH_DEPTH		32
#endif

#ifndef CORETONE_REQUEST_DEPTH
	#define CORETONE_REQUEST_DEPTH		32
#endif

#ifdef _WIN32
	#ifdef CORETONE_EXPORTS
		#define CORETONE_API __declspec(dllexport)
	#else
		#define CORETONE_API __declspec(dllimport)
	#endif
#else
	#define CORETONE_API 
#endif


/* CORETONE_RENDER_RATE defines the samplerate of the output audio while
 * CORETONE_DECODE_RATE indicates the rate at which the buffer is rendered and
 * all decoding operations (music, instruments, sfx) are performed. In short
 * this is the base driver tick.
 * 
 * For proper operation ensure CORETONE_RENDER_RATE is evenly divisible
 * by CORETONE_DECODE_RATE.
 */
#ifndef CORETONE_RENDER_RATE
	#define CORETONE_RENDER_RATE		48000
#endif

#ifndef CORETONE_DECODE_RATE
	#define CORETONE_DECODE_RATE		240
#endif


/* CORETONE_BUFFER_LEN is the buffer length in MONO SAMPLES, which are
 * interleaved as LEFT, RIGHT, LEFT, RIGHT, etc in order to create the stereo
 * output stream while CORETONE_BUFFER_SAMPLES is the stereo sample count.
 *
 * This should be an even number for what I hope are obvious reasons.
 *
 * CORETONE_BUFFER_CENTER is the center (silent) value to stamp the buffer
 * with, this will usually be zero for signed output but can be adjusted up
 * or down for unsigned platforms.
 */
#define CORETONE_BUFFER_SAMPLES		(CORETONE_RENDER_RATE / CORETONE_DECODE_RATE)
#define CORETONE_BUFFER_LEN			(CORETONE_BUFFER_SAMPLES * 2)
#define CORETONE_BUFFER_CENTER		0


/* Functions of type ct_renderCall_t may be configured as a post-render
 * callback through ct_setRenderCall(). These are called each time CoreTone
 * completes a rendering update and are supplied with the raw render buffer
 * for any additional mixing or post processing.
 *
 * A post-render callback will remain active until it returns zero, upon
 * which it will be disabled. If the post-render callback returns zero
 * on its first update, it is effetively one-shot.
 *
 * Note that uiLen is the count of STEREO samples in the buffer.
 */
typedef int32_t (*ct_renderCall_t)(void *pBuffer,
								   uint32_t uiFreq, uint32_t uiLen,
								   int32_t iAmPaused);


/******************************************************************************
 * Function Defines (User-Facing)
 ******************************************************************************/
CORETONE_API void ct_update(int16_t *pBuffer);

CORETONE_API void ct_pause(void);
CORETONE_API void ct_resume(void);
CORETONE_API int32_t ct_isPaused(void);
CORETONE_API void ct_stopAll(void);
CORETONE_API void ct_setRenderCall(ct_renderCall_t pCall);

CORETONE_API void ct_playMusic(uint8_t *pMusic);
CORETONE_API void ct_stopMusic(void);
CORETONE_API void ct_attenMusic(int8_t cVol);
CORETONE_API int32_t ct_checkMusic(void);
CORETONE_API int32_t ct_getMood(void);

CORETONE_API void ct_playSFX(uint8_t *pSFX, int8_t cPriority, int8_t cVol_Left, int8_t cVol_Right);
CORETONE_API void ct_stopSFX(int8_t cPriority);
CORETONE_API void ct_addSFX(uint8_t *pSFX, int8_t cPriority, int8_t cVol_Left, int8_t cVol_Right);
CORETONE_API void ct_dumpSFX(void);

CORETONE_API int32_t ct_getMutex(void);
CORETONE_API int32_t ct_giveMutex(void);

CORETONE_API int32_t ct_init(uint8_t *pSamplePak, uint8_t *pInstrPak);

CORETONE_API void ct_getState(void **ppaChannels, void **ppaPatches, void **ppaTracks);
CORETONE_API void ct_getInfo(uint32_t *puiChannels, uint32_t *puiRenderFreq,
                             uint32_t *puiDecodeRate,
                             uint32_t *puiSamples, uint32_t *puiSampleLen);
#endif
