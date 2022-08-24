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
// BupChip.cpp
// ----------------------------------------------------------------------------
#include "BupChip.h"
#include "Cartridge.h"
#include <iostream>
#include <sstream>
#include <vector>

#define BUPCHIP_FLAGS_PLAYING   1
#define BUPCHIP_FLAGS_PAUSED    2

struct BupchipFileContents {
  uint8_t *data;
  size_t size;
};

byte *bupchip_sample_data;
byte *bupchip_instrument_data;
BupchipFileContents bupchip_songs[32];
byte bupchip_song_count;

byte bupchip_flags;
byte bupchip_volume;
byte bupchip_current_song;

short bupchip_buffer[CORETONE_BUFFER_LEN * 4];

// ----------------------------------------------------------------------------
// Init
// ----------------------------------------------------------------------------
bool bupchip_Init() {
  size_t songIndex = 0;
  std::vector<BupchipFileContents> fileData;

  // TODO: Load song list from CDF.

  if(fileData.size( ) < 2) {
    goto err;
  }
  bupchip_sample_data = fileData[0].data;
  bupchip_instrument_data = fileData[1].data;
  if(ct_init(bupchip_sample_data, bupchip_instrument_data) != 0) {
    goto err;
  }
  for(songIndex = 0; songIndex < fileData.size( ) - 2; songIndex++) {
    bupchip_songs[songIndex] = fileData[songIndex + 2];
  }
  bupchip_song_count = byte(fileData.size( ) - 2);
  return true;

err:
  for(size_t fileIndex = 0; fileIndex < fileData.size( ); fileIndex++) {
    delete [ ] fileData[fileIndex].data;
    fileData[fileIndex].data = NULL;
  }
  bupchip_song_count = 0;
  bupchip_instrument_data = NULL;
  bupchip_sample_data = NULL;
  return false;
}

// ----------------------------------------------------------------------------
// Stop
// ----------------------------------------------------------------------------
void bupchip_Stop( ) {
  bupchip_flags &= ~BUPCHIP_FLAGS_PLAYING;
  ct_stopMusic( );
}

// ----------------------------------------------------------------------------
// Play
// ----------------------------------------------------------------------------
void bupchip_Play(unsigned char song) {
  if(song >= bupchip_song_count) {
    bupchip_Stop( );
    return;
  }
  bupchip_flags |= BUPCHIP_FLAGS_PLAYING;
  bupchip_current_song = song;
  ct_playMusic(bupchip_songs[bupchip_current_song].data);
}

// ----------------------------------------------------------------------------
// Pause
// ----------------------------------------------------------------------------
void bupchip_Pause( ) {
  bupchip_flags |= BUPCHIP_FLAGS_PAUSED;
  ct_pause( );
}

// ----------------------------------------------------------------------------
// Resume
// ----------------------------------------------------------------------------
void bupchip_Resume( ) {
  bupchip_flags &= ~BUPCHIP_FLAGS_PAUSED;
  ct_resume( );
}

// ----------------------------------------------------------------------------
// SetVolume
// ----------------------------------------------------------------------------
void bupchip_SetVolume(byte volume) {
  bupchip_volume = volume & 0x1f;
  // This matches BupSystem.
  int attenuation = volume << 2;
  if((volume & 1) != 0) {
    attenuation += 0x3;
  }
  ct_attenMusic(attenuation);
}

// ----------------------------------------------------------------------------
// ProcessAudioCommand
// ----------------------------------------------------------------------------
void bupchip_ProcessAudioCommand(unsigned char data) {
  switch(data & 0xc0) {
  case 0:
    switch(data) {
    case 0:
      bupchip_flags = 0;
      bupchip_volume = 0x1f;
      ct_stopAll( );
      ct_resume( );
      ct_attenMusic(127);
      return;
    case 2:
      bupchip_Resume( );
      return;
    case 3:
      bupchip_Pause( );
      return;
    }
    return;
  case 0x40:
    bupchip_Stop( );
    return;
  case 0x80:
    bupchip_Play(data & 0x1f);
    return;
  case 0xc0:
    bupchip_SetVolume(data);
    return;
  }
}

// ----------------------------------------------------------------------------
// Process
// ----------------------------------------------------------------------------
void bupchip_Process(unsigned tick) {
  ct_update(&bupchip_buffer[tick * CORETONE_BUFFER_LEN]);
}

// ----------------------------------------------------------------------------
// Release
// ----------------------------------------------------------------------------
void bupchip_Release( ) {
  for(int i = 0; i < bupchip_song_count; i++) {
    delete [ ] bupchip_songs[i].data;
    bupchip_songs[i].data = NULL;
  }
  delete [ ] bupchip_instrument_data;
  bupchip_instrument_data = NULL;
  delete [ ] bupchip_sample_data;
  bupchip_sample_data = NULL;
}

// ----------------------------------------------------------------------------
// StateLoaded
// ----------------------------------------------------------------------------
void bupchip_StateLoaded( ) {
  ct_stopAll( );
  if((bupchip_flags & BUPCHIP_FLAGS_PLAYING) == 0) {
    return;
  }
  ct_playMusic(bupchip_songs[bupchip_current_song].data);
  if((bupchip_flags & BUPCHIP_FLAGS_PAUSED) != 0) {
    ct_pause( );
  }
  else {
    ct_resume( );
  }
  bupchip_SetVolume(bupchip_volume);
}
