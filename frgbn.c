
/********************************************************************************
 *
 * modul name:  frgbn.c
 *
 * contents:    routines to load and check RGBN and RGB8 files
 *
 *
 * to do:
 *   switch output-mode popup-gadget to 24bit
 *
 *
 * v1.1 (11.10.93)
 *   Major revision. Module entirely rewritten. Major bug fixes.
 *
 * v1.0 (08.10.93)
 *   added file-info and file-properties load
 *
 * v0.9 (00.00.93)
 *   basic load- and save-routines
 *
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



#include "iff/ilbm.h"
#undef DOS_ERROR

#include "global.h"

#define ID_RGB8 MakeID('R','G','B','8')
#define ID_RGBN MakeID('R','G','B','N')


void CheckForRGB8(form fo)
{
  ULONG longval;


  longval=GetLong(0,0);
  if (longval != FORM) { fo->not_valid_format = TRUE; return; }

  GetLong(0,0); /* eat a long-word */

  longval=GetLong(0,0);
  if (longval != ID_RGB8) { fo->not_valid_format = TRUE; return; }


  fo->not_valid_format=FALSE;
}

void CheckForRGBN(form fo)
{
  ULONG longval;


  longval=GetLong(0,0);
  if (longval != FORM) { fo->not_valid_format = TRUE; return; }

  GetLong(0,0); /* eat a long-word */

  longval=GetLong(0,0);
  if (longval != ID_RGBN) { fo->not_valid_format = TRUE; return; }


  fo->not_valid_format=FALSE;
}

extern BOOL FindChunk(ULONG);


void PropsRGBN(void)
{
  BitMapHeader BMHD;

  if (!FindChunk(ID_BMHD)) return;
  if (!ReadBlock(&BMHD,sizeof(BitMapHeader))) return;

  Output_Width  = BMHD.w;
  Output_Height = BMHD.h;


  if (FindChunk(ID_CMAP))
  {
    SeekPosLoad(-4,Relative);
    Output_nColors = GetLong(0,0) / 3;
  }

  Output_Mode = GFXMOD_24BIT;
}


/**------------------------------------------------------------------------**
 **  LOAD  **
 **------------------------------------------------------------------------**/

static BOOL LoadImagine(BOOL IsRGB8)
{
  BitMapHeader   BMHD;
  struct         ImageData *image=NULL;
  int            SavedWidth;
  BOOL           success=TRUE;


  ShowMessage(Txt(TXT_LOADING_RGBN8));


  /* get properties */

  if (!FindChunk(ID_BMHD)) { SetError(INPUT_FILE_ERROR); goto errexit; }
  if (!ReadBlock(&BMHD,sizeof(BitMapHeader))) { SetError(INPUT_FILE_ERROR); goto errexit; }

  SavedWidth = (BMHD.w+15) & ~15;


  /* get image-buffer */

  if (!(image=GetBuffer(BMHD.w,BMHD.h))) goto errexit;

  SetBufferMode(image, RGB );  /* RGBN/RGB8 is always TrueColor */


  /* get CAMG if existing */

  if (FindChunk(ID_CAMG))
    SetCAMG( image,GetLong(0,0) );


  if (!FindChunk(ID_BODY)) { SetError(INPUT_FILE_ERROR); goto errexit; }

  {
    int   x,y;
    int   count;
    UBYTE readin;
    UBYTE r,g,b;

    for (y=0;y<BMHD.h;y++)
    {
      ShowProgress(y,BMHD.h);

      for (x=0;x<SavedWidth; )
      {
        if (IsRGB8)
        {
          r = ReadByte();
          g = ReadByte();
          b = ReadByte();

          count = ReadByte() & 0x7F;
        }
        else
        {
          readin = ReadByte();

          r =  readin & 0xf0;
          g = (readin & 0x0f) << 4;

          readin = ReadByte();

          b =  readin & 0xf0;

          count = readin & (1+2+4);
        }

        if (count == 0) count = ReadByte();
        if (count == 0) count = GetWord(0,0);
        if (count == 0) { SetError(INPUT_FILE_ERROR); goto errexit; }

        if (x+count > SavedWidth) { SetError(INPUT_FILE_ERROR); goto errexit; }

        for ( ; count>0 ; count-- , x++)
          if (x<BMHD.w) if (!SetRGBPixel(image,x,y,r,g,b)) goto errexit;
      }
    }
  }

  skip
  {
errexit: success=FALSE;
  }

  if (!success) if (image) FreeBuffer(image);

  if (success)  SetDefaultBuffer(image);

  return success;
}

static BOOL SaveImagine(BOOL IsRGBN)
{
  return TRUE;
}

BOOL LoadRGBN(void) { return LoadImagine(FALSE); }
BOOL SaveRGBN(void) { return SaveImagine(FALSE); }
BOOL LoadRGB8(void) { return LoadImagine(TRUE); }
BOOL SaveRGB8(void) { return SaveImagine(TRUE); }

