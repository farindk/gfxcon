/*******************************************************************************
 *
 * Copyright (C) 1993  Dirk Farin  <dirk.farin@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 ********************************************************************************/

#include "global.h"
#include "options.h"

#include <libraries/asl.h>
#include <proto/asl.h>

static char LoadPath[MAX_PATH_LENGTH];
static char SavePath[MAX_PATH_LENGTH];
static char LoadFile[MAX_FILE_LENGTH];
static char SaveFile[MAX_FILE_LENGTH];

#define ASSERT_NO_EOF SetError(INPUT_FILE_ERROR);


extern BOOL Load_is_RGB,Save_is_RGB;


FILE *file_load;
FILE *file_load_g;
FILE *file_load_b;
FILE *file_save;
FILE *file_save_g;
FILE *file_save_b;

#define file_load_r file_load
#define file_save_r file_save


BOOL OpenLoadFile(void)
{
  if (Load_is_RGB)
  {
    if (!*Loadname_r ||
        !*Loadname_g ||
        !*Loadname_b) return FALSE;

    file_load_r = fopen(Loadname_r,"r");
    if (!file_load_r) { SetError(DOS_ERROR); return FALSE; }

    file_load_g = fopen(Loadname_g,"r");
    if (!file_load_g)
    { fclose(file_load_r);
      SetError(DOS_ERROR); return FALSE;
    }

    file_load_b = fopen(Loadname_b,"r");
    if (!file_load_b)
    { fclose(file_load_r);
      fclose(file_load_g);
      SetError(DOS_ERROR); return FALSE;
    }
  }
  else
  {
    if (!*Loadname) return FALSE;

    file_load = fopen(Loadname,"r");
    if (!file_load) { SetError(DOS_ERROR);return FALSE; }
  }
  return TRUE;
}

void CloseLoadFile(void)
{
  if (Load_is_RGB)
  {
    fclose(file_load_r);
    fclose(file_load_g);
    fclose(file_load_b);
  }
  else
    fclose(file_load);
}

BOOL OpenSaveFile(void)
{
  if (Save_is_RGB)
  {
    file_save_r = fopen(Savename_r,"w");
    if (!file_save_r) { SetError(DOS_ERROR); return FALSE; }

    file_save_g = fopen(Savename_g,"w");
    if (!file_save_g)
    { fclose(file_save_r);
      SetError(DOS_ERROR); return FALSE;
    }

    file_save_b = fopen(Savename_b,"w");
    if (!file_save_b)
    { fclose(file_save_r);
      fclose(file_save_g);
      SetError(DOS_ERROR); return FALSE;
    }
  }
  else
  {
    file_save = fopen(Savename,"w");
    if (!file_save) { SetError(DOS_ERROR); return FALSE; }
  }
  return TRUE;
}

void CloseSaveFile(void)
{
  if (Save_is_RGB)
  {
    fclose(file_save_r);
    fclose(file_save_g);
    fclose(file_save_b);
  }
  else
    fclose(file_save);
}

UBYTE GetByte(enum FileSpec file)
{
  int byte_val;
  FILE *fp;

  switch (file)
  {
    case R: fp=file_load_r; break;
    case G: fp=file_load_g; break;
    case B: fp=file_load_b; break;
  }

  byte_val=getc(fp);
  if (byte_val==-1) { ASSERT_NO_EOF; return 0; }

  return (UBYTE)byte_val;
}

UWORD GetWord(enum FileSpec file,BOOL intel)
{
  UWORD word_val;
  FILE *fp;

  switch (file)
  {
    case R: fp=file_load_r; break;
    case G: fp=file_load_g; break;
    case B: fp=file_load_b; break;
  }

  if (intel)
  {
    UWORD b1,b2;

    b1=GetByte(file);
    b2=GetByte(file);

    word_val = (b2<<8) | b1;
  }
  else
    if (fread((APTR)&word_val,2,1,fp) == 0) { ASSERT_NO_EOF; return 0; }

  return word_val;
}

ULONG GetLong(enum FileSpec file,BOOL intel)
{
  ULONG long_val;
  FILE *fp;

  switch (file)
  {
    case R: fp=file_load_r; break;
    case G: fp=file_load_g; break;
    case B: fp=file_load_b; break;
  }

  if (intel)
  {
    ULONG b1,b2,b3,b4;

    b1=GetByte(file);
    b2=GetByte(file);
    b3=GetByte(file);
    b4=GetByte(file);

    long_val = (b4<<24) | (b3<<16) | (b2<<8) | b1;
  }
  else
    if (fread((APTR)&long_val,4,1,fp) == 0) { ASSERT_NO_EOF; return 0; }

  return long_val;
}

BOOL WriteByte(UBYTE byte_val,enum FileSpec file)
{
  FILE *fp;

  switch (file)
  {
    case R: fp=file_save_r; break;
    case G: fp=file_save_g; break;
    case B: fp=file_save_b; break;
  }

  if (putc(byte_val,fp)==EOF) { SetError(DOS_ERROR); return FALSE; }
  else                        return TRUE;
}

BOOL WriteWord(UWORD word_val,enum FileSpec file,BOOL intel)
{
  FILE *fp;

  switch (file)
  {
    case R: fp=file_save_r; break;
    case G: fp=file_save_g; break;
    case B: fp=file_save_b; break;
  }

  if (intel)
  {
    if (!WriteByte( word_val     & 0xff,file)) { SetError(DOS_ERROR); return FALSE; }
    if (!WriteByte((word_val>>8) & 0xff,file)) { SetError(DOS_ERROR); return FALSE; }

    return TRUE;
  }

  if (fwrite((APTR)&word_val,2,1,fp) == 1) return TRUE;
  else                                     { SetError(DOS_ERROR); return FALSE; }
}

BOOL WriteLong(ULONG long_val,enum FileSpec file,BOOL intel)
{
  FILE *fp;

  switch (file)
  {
    case R: fp=file_save_r; break;
    case G: fp=file_save_g; break;
    case B: fp=file_save_b; break;
  }

  if (intel)
  {
    if (!WriteByte((long_val>> 0) & 0xff , file)) { SetError(DOS_ERROR); return FALSE; }
    if (!WriteByte((long_val>> 8) & 0xff , file)) { SetError(DOS_ERROR); return FALSE; }
    if (!WriteByte((long_val>>16) & 0xff , file)) { SetError(DOS_ERROR); return FALSE; }
    if (!WriteByte((long_val>>24) & 0xff , file)) { SetError(DOS_ERROR); return FALSE; }

    return TRUE;
  }

  if (fwrite((APTR)&long_val,4,1,fp) == 1) return TRUE;
  else                                     { SetError(DOS_ERROR); return FALSE; }
}

BOOL SeekPosLoad(ULONG pos,enum SeekMode mode)
{
  if (Load_is_RGB)
  {
    fseek(file_load_r,pos,mode);
    fseek(file_load_g,pos,mode);
    fseek(file_load_b,pos,mode);
  }
  else
    if (fseek(file_load,pos,mode) == -1) { SetError(DOS_ERROR); return FALSE; }

  return TRUE;
}

BOOL SeekPosSave(ULONG pos,enum SeekMode mode)
{
  if (Save_is_RGB)
  {
    fseek(file_save_r,pos,mode);
    fseek(file_save_g,pos,mode);
    fseek(file_save_b,pos,mode);
  }
  else
    if (fseek(file_save,pos,mode) == -1) { SetError(DOS_ERROR); return FALSE; }

  return TRUE;
}

int GetLoadFileLength(void)
{
  int pos;
  int length;

  pos=ftell(file_load);
  fseek(file_load,0,2);
  length=ftell(file_load);
  fseek(file_load,pos,0);

  return length;
}

void RewindInput(void)
{
  if (!SeekPosLoad(0,Absolute)) assert(1==2);
}

BOOL CheckEOF(void)
{
  BOOL ismore;
  char c;

  ismore = fread(&c,1,1,file_load);
  fseek(file_load,-1,1);
  return (BOOL)(!ismore);
}

BOOL ReadBlock(APTR ptr,int size)
{
  assert(Load_is_RGB==FALSE);

  if (fread(ptr,size,1,file_load) != 1) { SetError(DOS_ERROR); return FALSE; }
  else                                  return TRUE;
}

BOOL WriteBlock(APTR ptr,int size)
{
  assert(Save_is_RGB==FALSE);

  if (fwrite(ptr,size,1,file_save) == 1) return TRUE;
  else                                   { SetError(DOS_ERROR); return FALSE; }
}

int GetSavePos(void)
{
  return ftell(file_save);
}

int GetLoadPos(void)
{
  return ftell(file_load);
}

void IntelSwapWord(UWORD *p)
{
  UBYTE *bytep;
  UBYTE lsb,msb;

  bytep = (UBYTE *)p;

  lsb = *bytep++;
  msb = *bytep;

  bytep = (UBYTE *)p;

  *bytep++ = msb;
  *bytep   = lsb;
}


void SetLoadPath(char *path) { strcpy(LoadPath,path); }
void SetSavePath(char *path) { strcpy(SavePath,path); }

void GetLoadFilename(char *dest)
{
  struct FileRequester *request;

  request=AllocAslRequestTags(ASL_FileRequest,
                              ASL_Hail      ,Txt(TXT_LOADREQ_TITLE),
                              ASL_OKText    ,Txt(TXT_REQUESTER_OK),
                              ASL_CancelText,Txt(TXT_REQUESTER_CANCEL),
                              ASL_File      ,LoadFile,
                              ASL_Dir       ,LoadPath,
                              ASL_Window    ,GetWindow(h),
                              ASL_Height    ,400,
                              TAG_DONE);
  if (request)
  {
    if (AslRequest(request,NULL))
    {
      strcpy(LoadPath,request->rf_Dir);
      strcpy(LoadFile,request->rf_File);

      strcpy (dest,request->rf_Dir);
      AddPart(dest,request->rf_File,MAX_FILENAME_LENGTH);
    }
    FreeAslRequest(request);
  }
}

void GetSaveFilename(char *dest)
{
  struct FileRequester *request;

  request=AllocAslRequestTags(ASL_FileRequest,
                              ASL_Hail      ,Txt(TXT_SAVEREQ_TITLE),
                              ASL_OKText    ,Txt(TXT_REQUESTER_OK),
                              ASL_CancelText,Txt(TXT_REQUESTER_CANCEL),
                              ASL_File      ,SaveFile,
                              ASL_Dir       ,SavePath,
                              ASL_Window    ,GetWindow(h),
                              ASL_FuncFlags ,FILF_SAVE,
                              ASL_Height    ,300,
                              TAG_DONE);
  if (request)
  {
    if (AslRequest(request,NULL))
    {
      strcpy(SavePath,request->rf_Dir);
      strcpy(SaveFile,request->rf_File);

      strcpy (dest,request->rf_Dir);
      AddPart(dest,request->rf_File,MAX_FILENAME_LENGTH);
    }
    FreeAslRequest(request);
  }
}

