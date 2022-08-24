// ----------------------------------------------------------------------------
//   ___  ___  ___  ___       ___  ____  ___  _  _
//  /__/ /__/ /  / /__  /__/ /__    /   /_   / |/ /
// /    / \  /__/ ___/ ___/ ___/   /   /__  /    /  emulator
//
// ----------------------------------------------------------------------------
// Copyright 2005 Greg Stanton
// 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
// ----------------------------------------------------------------------------
// BupChip.h
// ----------------------------------------------------------------------------
#ifndef BUPCHIP_H
#define BUPCHIP_H

#include <stdint.h>
extern "C" {
#include "../Contrib/BupBoop/types.h"
#include "../Contrib/BupBoop/CoreTone/coretone.h"
}
#include <sstream>

extern unsigned char bupchip_flags;
extern unsigned char bupchip_volume;
extern unsigned char bupchip_current_song;
extern short bupchip_buffer[CORETONE_BUFFER_LEN * 4];

bool bupchip_InitFromCDF(std::istringstream &cdf, const char *workingDir);
void bupchip_ProcessAudioCommand(unsigned char data);
void bupchip_Process(unsigned tick);
void bupchip_Release( );
void bupchip_StateLoaded( );

#endif