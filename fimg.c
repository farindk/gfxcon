
/********************************************************************************
 *
 * modul name:  fimg.c
 *
 * contents:    routines to load and check IMG files (no save at this time)
 *
 * to do:
 *   is there really no palette information available ?
 *
 *
 * v1.1 (10.10.93)
 *   Major revision. Module entirely rewritten. Major bug fixes. Now supports
 *   all available IMG-versions.
 *
 * v1.0 (08.10.93)
 *   added file-info and file-properties load
 *
 * v0.9 (30.00.93)
 *   basic load-routine
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



#include "global.h"

struct IMG_Header
{
  UWORD Version;
  UWORD HeadLength;
  UWORD BitsPerPixel;
  UWORD PatternLength;
  UWORD PixelWidth;
  UWORD PixelHeight;
  UWORD Width;
  UWORD Height;
};


static BOOL LoadHeader(struct IMG_Header *hd)
{
  if (!ReadBlock(hd,sizeof(struct IMG_Header))) return FALSE;
  else                                          return TRUE;
}

void CheckForIMG(form fo) /* well actually, we CAN'T recognize if this is a IMG */
{
  struct IMG_Header Head;

  if (!LoadHeader(&Head))      { fo->not_valid_format=TRUE; return; }

  if (Head.HeadLength    != 8) { fo->not_valid_format=TRUE; return; }
  if (Head.Version       != 1) { fo->not_valid_format=TRUE; return; }
  if (Head.PatternLength  > 8) { fo->not_valid_format=TRUE; return; }

  fo->not_valid_format = FALSE;
}

void PropsIMG(void)
{
  struct IMG_Header  head;

  if (!LoadHeader(&head)) return;

  Output_nColors = 1 << head.BitsPerPixel;

  Output_Width  = head.Width;
  Output_Height = head.Height;

  Output_Mode   = GFXMOD_CLUT;
}

void InfoIMG(void)
{
  struct IMG_Header  head;
  char buffer[100];

  if (!LoadHeader(&head)) return;

  ShowInfo(Txt(TXT_GEM_IMAGE));
  ShowInfo(" ");
  BufShowInfo(Txt(TXT_WIDTH_D) ,head.Width);
  BufShowInfo(Txt(TXT_HEIGHT_D),head.Height);
  BufShowInfo(Txt(TXT_COLORS_D),1<<head.BitsPerPixel);
  ShowInfo(" ");
  BufShowInfo(Txt(TXT_VERSION_D),head.Version);
  BufShowInfo(Txt(TXT_BITSPERPIXEL_D),head.BitsPerPixel);
  BufShowInfo(Txt(TXT_PATTERNLENGTH_D) ,head.PatternLength);
  BufShowInfo(Txt(TXT_PIXELWIDTH_D_YM) ,head.PixelWidth);
  BufShowInfo(Txt(TXT_PIXELHEIGHT_D_YM),head.PixelHeight);
  ShowInfo(" ");
}

static struct { UBYTE r,g,b; } NormColor16[16] =
{
    0,  0,  0,
    0,  0,170,
    0,170,  0,
    0,170,170,
  170,  0,  0,
  170,  0,170,
  170,170,  0,
  170,170,170,
    0,  0,  0,
    0,  0,255,
    0,255,  0,
    0,255,255,
  255,  0,  0,
  255,  0,255,
  255,255,  0,
  255,255,255
};

/**------------------------------------------------------------------------**
 **  LOAD  **
 **------------------------------------------------------------------------**/

BOOL LoadIMG(void)
{
  struct IMG_Header         head;
  struct ImageData        * image=NULL;
  char                    * buffer[8];
  UBYTE                     readin;
  UBYTE                     count;
  UBYTE                     solidbyte;
  struct ColorLookupTable * clut;
  int                       x,y,p,i;
  UBYTE                     pattern[32];
  UBYTE                     n_line_copies;
  int                       nColors;
  int                       SavedWidth,BytesPerLine;
  BOOL                      success=FALSE;
  BOOL                      HaveLineBuffer=FALSE;


  /* show message-line */

  ShowMessage(Txt(TXT_LOADING_IMG));


  /* load properties */

  if (!LoadHeader(&head)) goto errexit;

  if (head.PatternLength > 32) { SetError(FORMAT_NOT_SUPPORTED);
                                 goto errexit; }

  SavedWidth   = (head.Width+7) & ~7;
  BytesPerLine = SavedWidth/8;
  nColors      = 1 << head.BitsPerPixel;


  /* get image-buffer */

  image=GetBuffer(head.Width,head.Height);
  if (!image) goto errexit;

  SetBufferMode(image,CLUT);

  if (!(clut=GetCLUT(nColors))) goto errexit;
  AttachCLUT(clut,image);



  /* set colors */

  switch (nColors)
  {
    case 2:
      SetColor(clut,0,255,255,255);
      SetColor(clut,1,  0,  0,  0);
      break;

    case 16:
      for (i=0;i<nColors;i++)
      {
        SetColor(clut,i,NormColor16[i].r,
                        NormColor16[i].g,
                        NormColor16[i].b);
      }
      break;

    case 256:                        /* ?????? default palette ???????? */
      for (i=0;i<nColors;i++)
        SetColor(clut,i, i,i,i);
      break;

    default:
        SetError(FORMAT_NOT_SUPPORTED);
        goto errexit;
      break;
  }


  /* get line-buffer */

  if (!(HaveLineBuffer=GetLineBuffer( SavedWidth , head.BitsPerPixel , buffer )) )
    goto errexit;


  /* load image */

  for (y=0;y<head.Height;)
  {
    n_line_copies=1;

    for (p=0;p<head.BitsPerPixel;p++)
      for (x=0;x<BytesPerLine;)
      {
        readin=GetByte(0);

        if (readin == 0x00)
        {
          count = GetByte(0);

          if (count==0) /*------ Vertical Replication Count ------------*/
          {
            if (GetByte(0) != 0xFF) { SetError(INPUT_FILE_ERROR);
                                      goto errexit; }
            n_line_copies = GetByte(0);
          }
          else /*--------------- Pattern Run Format --------------------*/
          {
            if (x + count*head.PatternLength > BytesPerLine)
            { SetError(INPUT_FILE_ERROR); goto errexit; }

            /* read pattern */

            if (!ReadBlock(pattern,head.PatternLength)) goto errexit;

            /* write pattern 'count' times into buffer */

            for ( ; count>0 ; count-- )
              for (i=0;i<head.PatternLength;i++)
                buffer[p][x++]=pattern[i];
          }
        }
        else
        if (readin == 0x80) /*----------- Bit String Format ------------*/
        {
          count = GetByte(0);

          if (x+count > BytesPerLine)
          { SetError(INPUT_FILE_ERROR); goto errexit; }

          if (!ReadBlock(&buffer[p][x],count))
          { SetError(INPUT_FILE_ERROR); goto errexit; }

          x+=count;
        }
        else /*-------------------------- Solid Run Format -------------*/
        {
          count = readin & ~0x80;

          if (x+count > BytesPerLine)
          { SetError(INPUT_FILE_ERROR); goto errexit; }

          if (readin & 0x80) solidbyte=0xFF;
          else               solidbyte=0x00;

          for (i=0;i<count;i++)
            buffer[p][x++] = solidbyte;
        }
      }

    /* copy line-buffer to line(s) */

    {
      int i;

      if (y+n_line_copies > head.Height) { SetError(INPUT_FILE_ERROR); goto errexit; }

      for (i=0;i<n_line_copies;i++)
      {
        ShowProgress(y,head.Height-1);
        if (!SetCLUTRow(image,y,buffer,head.BitsPerPixel)) goto errexit;
        y++;
      }
    }
  }

  success=TRUE;

errexit:

  if (HaveLineBuffer)      FreeLineBuffer(head.BitsPerPixel,buffer);
  if (!success) if (image) FreeBuffer(image);

  if (success) SetDefaultBuffer(image);

  return success;
}


BOOL SaveIMG(void)
{
  return FALSE;
}

