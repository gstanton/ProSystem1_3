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
// Archive.cpp
// ----------------------------------------------------------------------------
#include "Archive.h"

// ----------------------------------------------------------------------------
// GetUncompressedFileSize
// ----------------------------------------------------------------------------
uint archive_GetUncompressedFileSize(std::string filename) {
  if((filename.empty( ) || filename.size( ) == 0)) {
    logger_LogError(IDS_ZIP1,"");
    return 0;
  }
  
  unzFile file = unzOpen(filename.c_str( ));
  if(file == NULL) {
    logger_LogInfo(IDS_ZIP2,filename);
    return 0;
  }

  int result = unzGoToFirstFile(file);
  if(result != UNZ_OK) {
    logger_LogInfo(IDS_ZIP3, "");
    unzClose(file);
    return 0;
  }
  
  unz_file_info_s zipInfo = {0};
  char buffer[_MAX_PATH] = {0};
  result = unzGetCurrentFileInfo(file, &zipInfo, buffer, _MAX_PATH, NULL, 0, NULL, 0);
  if(result != UNZ_OK) {
    logger_LogInfo(IDS_ZIP5, "");
    unzClose(file);
    return 0;
  }
  
  uint size = zipInfo.uncompressed_size;
  unzClose(file);
  return size;
}

// ----------------------------------------------------------------------------
// Uncompress
// ----------------------------------------------------------------------------
bool archive_Uncompress(std::string filename, byte* data, uint size) {
  if(filename.empty( ) || filename.size( ) == 0) {
    logger_LogError(IDS_ZIP1,"");
    return false;
  }
  if(data == NULL) {
    logger_LogError(IDS_ZIP6,"");
    return false;  
  }

  unzFile file = unzOpen(filename.c_str( ));
  if(file == NULL) {
    logger_LogInfo(IDS_ZIP2,filename);
    return false;
  }

  int result = unzGoToFirstFile(file);
  if(result != UNZ_OK) {
    logger_LogInfo(IDS_ZIP3 ,filename);
    unzClose(file);
    return false;
  }

  result = unzOpenCurrentFile(file);
  if(result != UNZ_OK) {
    logger_LogInfo(IDS_ZIP3 ,filename);
    unzClose(file);
    return false;  
  }

  result = unzReadCurrentFile(file, data, size);
  if(result != size) {
    logger_LogInfo(IDS_ZIP8 ,filename);
    unzCloseCurrentFile(file);
    unzClose(file);
    return false;
  }

  unzCloseCurrentFile(file);
  unzClose(file);
  return true;
}

// ----------------------------------------------------------------------------
// Compress
// ----------------------------------------------------------------------------
bool archive_Compress(std::string zipFilename, std::string filename, const byte* data, uint size) {
  if(zipFilename.empty( ) || zipFilename.size( ) == 0) {
    logger_LogError(IDS_ZIP1,"");
    return false;
  }  
  if(filename.empty( ) || filename.size( ) == 0) {
    logger_LogError(IDS_ZIP9,"");
    return false;
  }
  if(data == NULL) {
    logger_LogError(IDS_ZIP6,"");
    return false;  
  }
  
  zipFile file = zipOpen(zipFilename.c_str( ), APPEND_STATUS_CREATE);
  if(file == NULL) {
    logger_LogInfo(IDS_ZIP10, zipFilename);
    return false;
  }
  
  zip_fileinfo fileInfo = {0};
  fileInfo.dosDate = 1;
  
  int result = zipOpenNewFileInZip(file, filename.c_str( ), &fileInfo, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_BEST_COMPRESSION);
  if(result != ZIP_OK) {
    logger_LogInfo(IDS_ZIP11, filename);
    zipClose(file, "Failed to compress.");
    return false;  
  }
  
  result = zipWriteInFileInZip(file, data, size);
  if(result != ZIP_OK) {
    logger_LogInfo(IDS_ZIP12, filename);
    zipCloseFileInZip(file);
    zipClose(file, "Failed to compress.");
    return false;
  }
 
  zipCloseFileInZip(file);
  zipClose(file, "Comment");
  return true;
}