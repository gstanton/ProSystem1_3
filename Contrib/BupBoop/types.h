/******************************************************************************
 * types.h
 * The usual gang of pairings for the 8.8, 16.16, and 32.32 fixed precision
 * math we love oh-so-very-much on platforms that aren't bouyant.
 *-----------------------------------------------------------------------------
 * Copyright (C) 2015 - 2016 Osman Celimli
 * For conditions of distribution and use, see copyright notice in bupboop.h
 ******************************************************************************/
#ifndef CORE_TYPES
#define CORE_TYPES

typedef struct int8p_s
{
	int8_t cLo;
	int8_t cHi;
} int8p_t;
typedef struct uint8p_s
{
	uint8_t ucLo;
	uint8_t ucHi;
} uint8p_t;

typedef struct int16p_s
{
	int16_t sLo;
	int16_t sHi;
} int16p_t;
typedef struct uint16p_s
{
	uint16_t usLo;
	uint16_t usHi;
} uint16p_t;

typedef struct int32p_s
{
	int32_t iLo;
	int32_t iHi;
} int32p_t;
typedef struct uint32p_s
{
	uint32_t uiLo;
	uint32_t uiHi;
} uint32p_t;


typedef union int8p8_u
{
	int16_t sWhole;
	uint16_t usWhole;
	int8p_t cPair;
	uint8p_t ucPair;
} int8p8_t;

typedef union int16p16_u
{
	int32_t iWhole;
	uint32_t uiWhole;
	int16p_t sPair;
	uint16p_t usPair;
} int16p16_t;

typedef union int32p32_u
{
	int64_t lWhole;
	uint64_t ulWhole;
	int32p_t iPair;
	uint32p_t uiPair;
} int32p32_t;

#endif
