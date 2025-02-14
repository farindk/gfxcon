
/********************************************************************************
 *
 * modul name:  fpcx.c
 *
 * contents:    routines to load, save and check PCX files
 *
 *
 * to do:
 *   better user-interaction when number of colors do not match PCX modes
 *   (user wants more than 256 colors...)
 *
 *
 * v1.8 (07.04.97)
 *   Bug fixed in the 256-color part.
 *
 * v1.2 (05.03.94)
 *   Serious bugs fixed. BytesPerRow was incorrectly saved and images
 *   with odd number of bytes per row were incorrectly read!
 *
 * v1.1 (10.10.93)
 *   Major revision. Module entirely rewritten. Major bug fixes. Now supports
 *   all available PCX-versions.
 *
 * v1.0 (08.10.93)
 *   added file-info and file-properties load
 *
 * v0.9 (30.00.93)
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


#include "global.h"

struct PCX_Header
{
  UBYTE ID_Byte;
  UBYTE Version;
  UBYTE Compression;
  UBYTE BitsPerPixel;
  UWORD XMin,YMin,XMax,YMax;
  UWORD Xdpi,Ydpi;
  UBYTE Colors[16][3];
  UBYTE reserved;
  UBYTE nColorPlanes;
  UWORD BytesPerRow;
  UWORD ColorMode;
  UBYTE pad[58];
};


/**-------------------------------------------------------------------------------**
 **  Transforms all Intel-word entries in the header to Motorola (or vice versa)
 **-------------------------------------------------------------------------------**/

static void SwapAllIntelWords(struct PCX_Header *hd)
{
  IntelSwapWord(&hd->XMin);
  IntelSwapWord(&hd->YMin);
  IntelSwapWord(&hd->XMax);
  IntelSwapWord(&hd->YMax);
  IntelSwapWord(&hd->Xdpi);
  IntelSwapWord(&hd->Ydpi);
  IntelSwapWord(&hd->BytesPerRow);
  IntelSwapWord(&hd->ColorMode);
}


/**------------------------------------------------------------------------**
 **  Read the header. FilePtr must point to beginning of the file.
 **------------------------------------------------------------------------**/

static BOOL ReadHeader(struct PCX_Header *hd)
{
  if (!ReadBlock(hd,sizeof(struct PCX_Header))) return FALSE;
  SwapAllIntelWords(hd);
  return TRUE;
}


/**------------------------------------------------------------------------**
 **  CHECK  ** for PCX-format
 **------------------------------------------------------------------------**/

void CheckForPCX(form fo)
{
  struct PCX_Header  Header;

  if (!ReadHeader(&Header))       { fo->not_valid_format=TRUE; return; }

  if (Header.ID_Byte != 0x0A)     { fo->not_valid_format=TRUE; return; }
  if (Header.Compression > 1)     { fo->not_valid_format=TRUE; return; }

  switch (Header.Version)
  {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
      break;

    default:
      fo->not_valid_format=TRUE;
      return;
  }

  if (Header.XMin >= Header.XMax) { fo->not_valid_format=TRUE; return; }
  if (Header.YMin >= Header.YMax) { fo->not_valid_format=TRUE; return; }
  if (Header.BitsPerPixel > 8)    { fo->not_valid_format=TRUE; return; }
  if (Header.nColorPlanes > 4)    { fo->not_valid_format=TRUE; return; }

  fo->not_valid_format=FALSE;
}


/**------------------------------------------------------------------------**
 **  PROPS  **
 **------------------------------------------------------------------------**/

void PropsPCX(void)
{
  struct PCX_Header Header;

  if (!ReadHeader(&Header)) return;

  Output_nColors = 1<<(Header.BitsPerPixel * Header.nColorPlanes);
  Output_Mode = GFXMOD_CLUT;
  if (Output_nColors>256) { Output_nColors=256; Output_Mode = GFXMOD_24BIT; } // TrueColors

  Output_Width   = Header.XMax-Header.XMin+1;
  Output_Height  = Header.YMax-Header.YMin+1;
}


/**------------------------------------------------------------------------**
 **  INFO  **
 **------------------------------------------------------------------------**/

void InfoPCX(void)
{
  char buffer[100];
  struct PCX_Header Header;

  if (!ReadHeader(&Header)) return;

  ShowInfo(Txt(TXT_PCXPICTURE));
  ShowInfo(" ");

  BufShowInfo(Txt(TXT_WIDTH_D)          ,Header.XMax-Header.XMin+1);
  BufShowInfo(Txt(TXT_HEIGHT_D)         ,Header.YMax-Header.YMin+1);
  BufShowInfo(Txt(TXT_COLORS_D)         ,1<<(Header.BitsPerPixel * Header.nColorPlanes));
  ShowInfo(" ");
  BufShowInfo(Txt(TXT_VERSION_S)        ,Header.Version==0 ? "2.5" :
                                         Header.Version==2 ? Txt(TXT_VER25_WITH_PAL) :
                                         Header.Version==3 ? Txt(TXT_VER25_WITHOUT_PAL) :
                                         Header.Version==4 ? Txt(TXT_VER_WIN_WITHOUT_PAL) :
                                         Header.Version==5 ? "3.0" : Txt(TXT_UNKNOWN) );
  BufShowInfo(Txt(TXT_COMPRESSION_S)    ,Header.Compression==1 ? "RunLength (RLE)" :
                                         Header.Compression==0 ? Txt(TXT_NONE) : "");
  BufShowInfo(Txt(TXT_XRESOLUTION_D_DPI),Header.Xdpi);
  BufShowInfo(Txt(TXT_YRESOLUTION_D_DPI),Header.Ydpi);
  BufShowInfo(Txt(TXT_COLORPLANES_D)    ,Header.nColorPlanes);
  BufShowInfo(Txt(TXT_BITSPERPIXEL_D)   ,Header.BitsPerPixel);
  BufShowInfo(Txt(TXT_COLORMODE_S)      ,Header.ColorMode==1 ? Txt(TXT_ITS_COLOR) :
                                                               Txt(TXT_GREYSCALE) );
  ShowInfo(" ");
}


static void GetRLE(UBYTE *count,UBYTE *pattern)
{
  UBYTE readin;

  readin = ReadByte();

  if ( (readin & 0xC0) == 0xC0 )
  {
    *count   = readin & 0x3f;
    *pattern = ReadByte();
  }
  else
  {
    *count   = 1;
    *pattern = readin;
  }
}

static struct { UBYTE r,g,b; } NormColor[] =
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

BOOL LoadPCX(void)
{
  struct PCX_Header  Header;
  struct ImageData  *image=NULL;
  int    SavedWidth,SavedHeight;
  int    ImageWidth,ImageHeight;
  int    BytesPerLine;
  int    x,y,p;
  int    nPlanes,nColors;
  UBYTE *buffer[8];
  BOOL   HaveLineBuffer=FALSE;
  int    i;
  UBYTE  count;
  UBYTE  pattern;
  UBYTE  color;
  struct ColorLookupTable *clut;
  BOOL   success=FALSE;
  enum   { BPP8_PL1    ,BPP1_PL4    ,BPP1_PL1    ,BPP8_PL3,
           BPP8_PL1_RLE,BPP1_PL4_RLE,BPP1_PL1_RLE,BPP8_PL3_RLE } mode;
  UBYTE* ChunkyLineBuffer = NULL;

  ShowMessage(Txt(TXT_READING_PCX));


  /* get properties */

  if (!ReadHeader(&Header)) goto errexit;


  /* select load-mode */

       if (Header.BitsPerPixel == 8 && Header.nColorPlanes==1) mode=BPP8_PL1;
  else if (Header.BitsPerPixel == 1 && Header.nColorPlanes==4) mode=BPP1_PL4;
  else if (Header.BitsPerPixel == 1 && Header.nColorPlanes==1) mode=BPP1_PL1;
  else if (Header.BitsPerPixel == 8 && Header.nColorPlanes==3) mode=BPP8_PL3;
  else
  { SetError(FORMAT_NOT_SUPPORTED); goto errexit; }

  if (Header.Compression == 1)  /* RLE - compression */
  {
    switch (mode)
    {
      case BPP8_PL1: mode=BPP8_PL1_RLE; break;
      case BPP1_PL4: mode=BPP1_PL4_RLE; break;
      case BPP1_PL1: mode=BPP1_PL1_RLE; break;
      case BPP8_PL3: mode=BPP8_PL3_RLE; break;
    }
  }


  ImageWidth  = Header.XMax - Header.XMin +1;
  ImageHeight = Header.YMax - Header.YMin +1;

  SavedWidth  =  Header.BytesPerRow * 8;
  SavedHeight =  ImageHeight;

  if (SavedWidth<ImageWidth)
  {
    SetError(INPUT_FILE_ERROR);
    goto errexit;
  }

  BytesPerLine = Header.BytesPerRow;

  nPlanes = Header.BitsPerPixel * Header.nColorPlanes;
  nColors = 1<<nPlanes;


  /* get image,clut-buffer */

  image=GetBuffer(ImageWidth,ImageHeight);
  if (!image) goto errexit;


  if (mode != BPP8_PL3_RLE)
  {
    SetBufferMode(image,CLUT);

    if (!(clut=GetCLUT(nColors))) goto errexit;
    AttachCLUT(clut,image);
  }
  else
  {
    SetBufferMode(image,RGB);
  }


  /* get line-buffer if we need it */

  switch (mode)
  {
    case BPP1_PL4:
    case BPP1_PL4_RLE:
    case BPP1_PL1:
    case BPP1_PL1_RLE:

      if (!(HaveLineBuffer=GetLineBuffer( SavedWidth , nPlanes , buffer )) )
        goto errexit;
      break;

    case BPP8_PL1:
    case BPP8_PL1_RLE:

   /* HaveLineBuffer=FALSE;    (redundant) */
      break;

    case BPP8_PL3_RLE:
      ChunkyLineBuffer=cu_calloc(Header.BytesPerRow*3,1);
      break;
  }


  /* read image */

  for (y=0;y<ImageHeight;y++)
  {
    ShowProgress(y,SavedHeight-1);

    switch (mode)
    {
      case BPP8_PL1:     /*---------------- 8 Bits per pixel / 1 ColorPlane --------*/

        for (x=0;x<ImageWidth;x++)
        {
          color=ReadByte();
          if (!SetCLUTPixel(image,x,y,color)) goto errexit;
        }
        for ( ; x<SavedWidth;x++) ReadByte(); /* skip additional bytes */

        break;

      case BPP8_PL3_RLE:
        SavedWidth = Header.BytesPerRow;

        for (x=0;x<SavedWidth*3;)
        {
          GetRLE(&count,&color);

//          printf("x:%d SavedWidth:%d count:%d color:%d\n",x,SavedWidth,count,color);

          if (x+count > 3*SavedWidth)
          { SetError(INPUT_FILE_ERROR); goto errexit; }

          for ( ; count>0 ; count--,x++ )
          {
            ChunkyLineBuffer[x] = color;
          }
        }

        for (x=0;x<ImageWidth;x++)
        {
          if (!SetRGBPixel(image,x,y,ChunkyLineBuffer[x],
                                     ChunkyLineBuffer[x+SavedWidth],
                                     ChunkyLineBuffer[x+2*SavedWidth]
                         )) { goto errexit; }
        }
        break;

      case BPP8_PL1_RLE:

        SavedWidth = Header.BytesPerRow;

        for (x=0;x<SavedWidth;)
        {
          GetRLE(&count,&color);

//          printf("x:%d SavedWidth:%d count:%d color:%d\n",x,SavedWidth,count,color);

          if (x+count > SavedWidth) { SetError(INPUT_FILE_ERROR); goto errexit; }

          for ( ; count>0 ; count--,x++ )
          {
            if (x<ImageWidth)
            {
              if (!SetCLUTPixel(image,x,y,color)) { goto errexit; }
            }
          }
        }

        break;

      case BPP1_PL1:    /*----------------- 1 Bit per pixel / 1 ColorPlane ---------*/

        if (!ReadBlock(buffer[0],SavedWidth/8)) goto errexit;
        if (!SetCLUTRow(image,y,buffer,1)) goto errexit;
        break;

      case BPP1_PL1_RLE:

        for (i=0;i<BytesPerLine;)
        {
          GetRLE(&count,&pattern);

          if (i+count > BytesPerLine) { SetError(INPUT_FILE_ERROR);
                                        goto errexit;
                                      }

          for ( ; count>0 ; count-- )
            buffer[0][i++] = pattern;
        }
        if (!SetCLUTRow(image,y,buffer,1)) goto errexit;

        break;

      case BPP1_PL4:    /*----------------- 1 Bit per pixel / 4 ColorPlanes --------*/

        for (p=0;p<4;p++)
          if (!ReadBlock(buffer[p],BytesPerLine)) goto errexit;
        if (!SetCLUTRow(image,y,buffer,4))        goto errexit;

        break;

      case BPP1_PL4_RLE:

        for (p=0;p<4;p++)
          for (i=0;i<BytesPerLine;)
          {
            GetRLE(&count,&pattern);

            if (i+count > BytesPerLine) { SetError(INPUT_FILE_ERROR);
                                          goto errexit;
                                        }

            for ( ; count>0 ; count-- )
              buffer[p][i++] = pattern;
          }
          if (!SetCLUTRow(image,y,buffer,4)) goto errexit;

        break;
    }
  }


  if (mode != BPP8_PL3_RLE)
  {
  /* read colors */

  if (nColors == 256)
  {
    if (ReadByte() != 0x0C) { SetError(INPUT_FILE_ERROR); goto errexit; }

    {
      UBYTE r,g,b;

      for (i=0;i<256;i++)
      {
        r=ReadByte();
        g=ReadByte();
        b=ReadByte();

        SetColor(clut,i, r,g,b);
      }
    }
  }
  else
  {
    if (Header.Version==3 || Header.Version==4)
    {
      for (i=0;i<nColors;i++)
        SetColor(clut,i,NormColor[i].r,
                        NormColor[i].g,
                        NormColor[i].b);
    }
    else
      for (i=0;i<nColors;i++)
        SetColor(clut,i, Header.Colors[i][0],
                         Header.Colors[i][1],
                         Header.Colors[i][2] );
  }
  }

  success=TRUE;

errexit:

  if (HaveLineBuffer)      FreeLineBuffer(nPlanes,buffer);
  if (!success) if (image) FreeBuffer(image);
  if (ChunkyLineBuffer)    cu_free(ChunkyLineBuffer);

  if (success)             SetDefaultBuffer(image);

  return success;
}



/**--------------------------------------------------------------------------**
 **  Write a line of data RLE-compressed to file. (Length: number of bytes)
 **--------------------------------------------------------------------------**/

static BOOL WriteRLECompressed(UBYTE *buffer,int Length)
{
  int   x,lookahead;
  UBYTE pattern,count;

  for (x=0;x<Length;)
  {
    pattern  =buffer[x];
    lookahead=x;
    count    =0;

    while ( lookahead < Length           &&
            pattern == buffer[lookahead] &&
            count < 0x3F )
    {
      count++;
      lookahead++;
    }

    x += count;


    /* save byte-string */

    if (count>1)
    {
      if (!WriteByte(0xC0 | count,0)) return FALSE;
      if (!WriteByte(pattern     ,0)) return FALSE;
    }
    else /* save uncompressed data */
    {
      if ((pattern & 0xC0) == 0xC0)
        if (!WriteByte(0xC1,0)) return FALSE;

      if (!WriteByte(pattern,0)) return FALSE;
    }
  }

  return TRUE;
}


/**------------------------------------------------------------------------**
 **  SAVE  **
 **------------------------------------------------------------------------**/

BOOL SavePCX(void)
{
  struct PCX_Header   Head;
  UBYTE               ColPlanes,BitsPPix;
  int                 nColors;
  int                 x,y,p;
  struct ImageData  * image=NULL;
  int                 SaveWidth,ImageWidth;
  char              * buffer[4];
  UBYTE             * ColorLine=NULL;
  BOOL                success=FALSE;
  BOOL                HaveLineBuffer=FALSE;
  enum { BPP1_PL1_RLE,BPP1_PL4_RLE,BPP8_PL1_RLE } mode;
  UWORD               color;


  /* calculate number of colors needed to save file */

  if (Output_nColors > 256) Output_nColors=256;   /* todo: something cleaner (show change ? ...) */

       if (Output_nColors <=   2) { nColors=  2; BitsPPix=1; ColPlanes=1; mode=BPP1_PL1_RLE; }
  else if (Output_nColors <=  16) { nColors= 16; BitsPPix=1; ColPlanes=4; mode=BPP1_PL4_RLE; }
  else if (Output_nColors <= 256) { nColors=256; BitsPPix=8; ColPlanes=1; mode=BPP8_PL1_RLE; }


  /* create output-image */

  if (!ConvertToMode(nColors,(GetOutputFlags() & ~MODE_FLAGS) | GFXMOD_CLUT)) goto errexit;


  ShowMessage(Txt(TXT_SAVING_PCX));


  /* write header */

  image=GetDefaultBuffer();

  ImageWidth = GetImageWidth(image);
  SaveWidth  = (ImageWidth+7) & ~7;

  memset(&Head,0,sizeof(struct PCX_Header));

  Head.ID_Byte       = 0x0A;
  Head.Version       = 0x05;
  Head.Compression   = 0x01; /* RLE */
  Head.BitsPerPixel  = BitsPPix;
  Head.XMin          = 0;
  Head.XMax          = ImageWidth-1;
  Head.YMin          = 0;
  Head.YMax          = GetImageHeight(image)-1;
  Head.Xdpi          = 360; /* just a dummy value */
  Head.Ydpi          = 360;
  Head.reserved      = 0;
  Head.nColorPlanes  = ColPlanes;
  Head.BytesPerRow   = SaveWidth/8;
  Head.ColorMode     = 1;


  /* fill color-table in header with colors if there are no more than 16 colors */

  if (nColors<=16)
  {
    int nr;

    for (nr=0;nr<nColors;nr++)
    {
      GetImageColor(image,nr,&Head.Colors[nr][0],
                             &Head.Colors[nr][1],
                             &Head.Colors[nr][2]);
    }
  }

  SwapAllIntelWords(&Head);

  if (!WriteBlock(&Head,sizeof(Head))) goto errexit;


  /* get LineBuffer or equivalent (for 8 bits) */

  switch (mode)
  {
    case BPP1_PL1_RLE:
    case BPP1_PL4_RLE:

        if (!(HaveLineBuffer=GetLineBuffer( SaveWidth , ColPlanes , buffer )) )
          goto errexit;
        break;

    case BPP8_PL1_RLE:

        ColorLine = cu_calloc(SaveWidth,sizeof(UBYTE));
        break;
  }

  /* - - - write image-data - - - */


  for (y=0;y<GetImageHeight(image);y++)
  {
    ShowProgress(y,GetImageHeight(image)-1);

    switch (mode)
    {
      case BPP1_PL1_RLE:
        if (!GetCLUTRow(image,y,buffer,1)) goto errexit;
        if (!WriteRLECompressed(buffer[0],SaveWidth/8)) goto errexit;
        break;

      case BPP1_PL4_RLE:
        if (!GetCLUTRow(image,y,buffer,4)) goto errexit;
        for (p=0;p<4;p++)
          if (!WriteRLECompressed(buffer[p],SaveWidth/8)) goto errexit;
        break;

      case BPP8_PL1_RLE:  /*------------------ 8 Bit per pixel / 1 ColorPlane -----*/

        /* copy a row into a color-line buffer */

        for (x=0;x<ImageWidth;x++)
        {
          if (!GetCLUTPixel(image,x,y,&color)) goto errexit;
          ColorLine[x]=color;
        }

        if (!WriteRLECompressed(ColorLine,SaveWidth)) goto errexit;

        break;
    }

    ForgetLine(image,y);
  }


  /* add color-table if image has more than 16 colors */

  if (nColors>16)
  {
    int nr;
    UBYTE r,g,b;

    assert(nColors == 256);

    if(!WriteByte(0x0C,0)) goto errexit;

    for (nr=0;nr<256;nr++)
    {
      GetImageColor(image,nr,&r,&g,&b);
      if (!WriteByte(r,0) ||
          !WriteByte(g,0) ||
          !WriteByte(b,0)) goto errexit;
    }
  }

  success=TRUE;

errexit:

  if (ColorLine)      cu_free(ColorLine);
  if (HaveLineBuffer) FreeLineBuffer(ColPlanes,buffer);

  return success;
}


