
/********************************************************************************
 *
 * modul name:	filbm.c
 *
 * contents:	routines to load, save and check ILBM files and
 *		routines to load and check LBM files
 *
 *
 * to do:
 *
 *
 * v1.8 (06.04.97)
 *   Added support for OFFSET-command.
 *
 * v1.2 (21.01.95)
 *   HAM8-support
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



#include <graphics/displayinfo.h>
#include <graphics/display.h>
#include "iff/ilbm.h"
#undef DOS_ERROR

#include "global.h"


BOOL FindChunk(ULONG type)
{
  ULONG chunkID=0;
  ULONG chunklength;

  RewindInput();

  GetLong(0,0); /* eat FORM   */
  GetLong(0,0); /* eat length */
  GetLong(0,0); /* eat ILBM   */

  while (CheckEOF() == FALSE && (chunkID=GetLong(0,0)) != type)
  {
    chunklength=GetLong(0,0);
    chunklength++; chunklength&=~1;
    if (!SeekPosLoad(chunklength,Relative)) return FALSE;  /* skip chunk */
  }

  if (chunkID == type) { GetLong(0,0); /* eat length */ return TRUE; }
  else		       return FALSE;
}


void CheckForILBM(form fo)
{
  if (GetLong(0,0) != FORM)     { fo->not_valid_format = TRUE; return; }

  GetLong(0,0); /* eat a long-word */

  if (GetLong(0,0) != ID_ILBM)  { fo->not_valid_format = TRUE; return; }

  if (!FindChunk(ID_BMHD)) { fo->not_valid_format = TRUE; return; }
  if (!FindChunk(ID_BODY)) { fo->not_valid_format = TRUE; return; }

  fo->not_valid_format=FALSE;
}

#define ID_PBM MakeID('P','B','M',' ')

void CheckForLBM(form fo)
{
  if (GetLong(0,0) != FORM)     { fo->not_valid_format = TRUE; return; }

  GetLong(0,0); /* eat a long-word */

  if (GetLong(0,0) != ID_PBM)  { fo->not_valid_format = TRUE; return; }

  if (!FindChunk(ID_BMHD)) { fo->not_valid_format = TRUE; return; }
  if (!FindChunk(ID_BODY)) { fo->not_valid_format = TRUE; return; }

  fo->not_valid_format=FALSE;
}


void PropsILBM(void)
{
  BitMapHeader BMHD;

  if (!FindChunk(ID_BMHD)) return;
  if (!ReadBlock(&BMHD,sizeof(BitMapHeader))) return;

  Output_Width	= BMHD.w;
  Output_Height = BMHD.h;

  if (BMHD.nPlanes==24)
  {
    Output_Mode = GFXMOD_24BIT;
  }
  else
  {
    Output_Mode = GFXMOD_CLUT;
  }

  if (FindChunk(ID_CMAP))
  {
    SeekPosLoad(-4,Relative);
    Output_nColors = GetLong(0,0) / 3;
  }
  else
  {
    Output_nColors = 256;
  }
}

#define ID_RGB8 MakeID('R','G','B','8')
#define ID_RGBN MakeID('R','G','B','N')

void InfoILBM(void)
{
  BitMapHeader BMHD;
  char buffer[100];
  ULONG FileLength;
  ULONG FormType;

  GetLong(0,0);
  FileLength=GetLong(0,0);
  FormType  =GetLong(0,0);

  if (!FindChunk(ID_BMHD)) return;
  if (!ReadBlock(&BMHD,sizeof(BitMapHeader))) return;

  BufShowInfo(Txt(TXT_IFF_S_IMAGE),(FormType == ID_ILBM) ? "ILBM" :
				   (FormType == ID_PBM ) ? "PBM"  :
				   (FormType == ID_RGBN) ? "RGBN" :
				   (FormType == ID_RGB8) ? "RGB8" :
							   Txt(TXT_UNKNOWN) );
  ShowInfo(" ");

  BufShowInfo(Txt(TXT_WIDTH_D) ,BMHD.w);
  BufShowInfo(Txt(TXT_HEIGHT_D),BMHD.h);
  BufShowInfo4(Txt(TXT_BITPLANES_DSS),BMHD.nPlanes,
				      BMHD.nPlanes >= 24 ? Txt(TXT_TRUECOLOR) : "",
				      (FormType==ID_RGBN||FormType==ID_RGB8) ?
				      Txt(TXT_INCL_GENLOCKPLANE) : "");
  if (FindChunk(ID_CMAP))
  {
    SeekPosLoad(-4,Relative);
    BufShowInfo(Txt(TXT_COLORS_D),GetLong(0,0) / 3);
  }

  if (FindChunk(ID_CAMG)) ShowInfo(Txt(TXT_CAMG_EXISTS));
  else			  ShowInfo(Txt(TXT_CAMG_DOESNT_EXIST));

  BufShowInfo(Txt(TXT_COMPRESSION_S),BMHD.compression == cmpByteRun1 ? "ByteRun1" :
				     BMHD.compression == cmpNone     ? Txt(TXT_NONE) :
				     BMHD.compression == 4	     ? Txt(TXT_IMAGINE_COMPRESSION) :
								       Txt(TXT_UNKNOWN) );

  ShowInfo(" ");

  BufShowInfo3(Txt(TXT_POSITION_DD),BMHD.x,BMHD.y);
  BufShowInfo(Txt(TXT_MASKING_S),BMHD.masking == mskNone ? Txt(TXT_NONE) :
				 BMHD.masking == mskHasTransparentColor ? Txt(TXT_TRANSPARENT_COLOR) :
				 BMHD.masking == mskHasMask		? Txt(TXT_MASKPLANE) :
				 BMHD.masking == mskLasso		? Txt(TXT_LASSOMASK) :
									  Txt(TXT_UNKNOWN)      );
  BufShowInfo3(Txt(TXT_ASPECT_DD)  ,BMHD.xAspect,BMHD.yAspect);
  BufShowInfo3(Txt(TXT_PAGESIZE_DD),BMHD.pageWidth,BMHD.pageHeight);

  ShowInfo(" ");
}


/**------------------------------------------------------------------------**
 **  LOAD  **  ILBM  **
 **------------------------------------------------------------------------**/

BOOL LoadILBM(void)
{
  BitMapHeader BMHD;
  int	       nColors;
  UBYTE        r,g,b;
  UBYTE *buffer[24];
  int	 x,y;
  int	 plane;
  struct ImageData *image=NULL;
  int	 SavedWidth;
  LONG	 CAMG=0;
  BOOL	 HaveCAMG=FALSE;
  enum	 PixelMode pixmode;
  UBYTE  nBufPlanes;
  BOOL	 HaveLineBuffer=FALSE;
  BOOL	 success=FALSE;
  int	 BytesPerRow;
  BOOL	 IsHAM,IsEHB;

  int	      i;
  signed char readin;
  int	      count;
  UBYTE       pattern;


  ShowMessage(Txt(TXT_LOADING_ILBM));


  /* load properties */

  if (!FindChunk(ID_BMHD)) { SetError(INPUT_FILE_ERROR); goto errexit; }
  if (!ReadBlock(&BMHD,sizeof(BitMapHeader))) { SetError(INPUT_FILE_ERROR); goto errexit; }

  SavedWidth  = (BMHD.w+15) & ~15;
  BytesPerRow = SavedWidth/8;

  IsHAM=IsEHB=FALSE;

  if (FindChunk(ID_CAMG))
  { CAMG=GetLong(0,0);
    HaveCAMG=TRUE;
    if (CAMG & EXTRAHALFBRITE_KEY) IsEHB=TRUE;
    if (CAMG & HAM_KEY)            IsHAM=TRUE;
  }
  else
  {
    if (BMHD.nPlanes == 6) IsHAM=TRUE;
  }

  if (IsEHB && IsHAM) { SetError(INPUT_FILE_ERROR); goto errexit; }
  if (IsHAM && (BMHD.nPlanes!=6 && BMHD.nPlanes!=8)) { SetError(INPUT_FILE_ERROR); goto errexit; }

  pixmode = (BMHD.nPlanes == 24) ? RGB : CLUT;
  if (CAMG & HAM_KEY || CAMG & EXTRAHALFBRITE_KEY) pixmode=RGB;

  nBufPlanes = BMHD.nPlanes + (BMHD.masking==mskHasMask ? 1 : 0);


  /* get buffer */

  if (!(image=GetBuffer(BMHD.w,BMHD.h))) { SetError(INPUT_FILE_ERROR); goto errexit; }

  SetBufferMode(image,pixmode);

  if (HaveCAMG) SetCAMG( image,CAMG );


  /* load colors */

  if (FindChunk(ID_CMAP))
  {
    struct ColorLookupTable *clut;

    SeekPosLoad(-4,Relative);
    nColors = GetLong(0,0) / 3;

    if (!(clut=GetCLUT(nColors))) goto errexit;
    AttachCLUT(clut,image);

    for (i=0;i<nColors;i++)
    {
      r=ReadByte();
      g=ReadByte();
      b=ReadByte();

      SetColor(clut,i, r,g,b);
    }
  }

  SetBackgroundColor(image,BMHD.transparentColor);


  /* get line-buffer */

  if (!(HaveLineBuffer=GetLineBuffer( SavedWidth , nBufPlanes , buffer )) )
    goto errexit;


  /* load image */

  if (!FindChunk(ID_BODY)) { SetError(INPUT_FILE_ERROR); goto errexit; }

  for (y=0;y<BMHD.h;y++)
  {
    ShowProgress(y,BMHD.h);

    if (BMHD.compression == cmpByteRun1)
    {
	for (plane=0;plane<nBufPlanes;plane++)
	  for (x=0 ; x<BytesPerRow ; )
	  {
	    readin=(signed char)ReadByte();

	    if (readin>=0)          /***** if (readin>=0 && readin<=127) *****/
	    {
	      count = readin+1;

	      if (x+count > BytesPerRow) { SetError(INPUT_FILE_ERROR); goto errexit; }

	      if (!ReadBlock(&buffer[plane][x],count)) { SetError(INPUT_FILE_ERROR); goto errexit; }
	      x += count;
	    }
	    else if (readin>=-127)  /***** (readin>=-127 && readin<=-1)  *****/
	    {
	      pattern=ReadByte();
	      count  =-readin+1;

	      if (x+count > BytesPerRow) { SetError(INPUT_FILE_ERROR); goto errexit; }

	      for (i=0;i<count;i++)
		buffer[plane][x++]=pattern;
	    }
	  }
    }
    else /* uncompressed */
    {
	for (plane=0;plane<nBufPlanes;plane++)
	  if (!ReadBlock(buffer[plane],BytesPerRow)) { SetError(INPUT_FILE_ERROR); goto errexit; }
    }

	      /* order of next lines is important !!! */

	 if (IsHAM)   {
			     if (BMHD.nPlanes==6) { if (!SetHAMRow (image,y,buffer)) goto errexit; }
			else if (BMHD.nPlanes==8) { if (!SetHAM8Row(image,y,buffer)) goto errexit; }
		      }
    else if (IsEHB)   { if (!SetEHBRow (image,y,buffer)) goto errexit; }
    else if (pixmode == RGB)   { if (!SetRGBRow (image,y,buffer))              goto errexit; }
    else		       { if (!SetCLUTRow(image,y,buffer,BMHD.nPlanes)) goto errexit; }
  }


  success=TRUE;

errexit:

  if (HaveLineBuffer)      FreeLineBuffer(nBufPlanes,buffer);
  if (!success) if (image) FreeBuffer(image);

  if (success) SetDefaultBuffer(image);

  return success;
}


/**------------------------------------------------------------------------**
 **  LOAD  **  LBM  **
 **------------------------------------------------------------------------**/

BOOL LoadLBM(void)
{
  BitMapHeader BMHD;
  int	       nColors;
  int	       i;
  UBYTE        r,g,b;
  int	 x,y;
  struct ImageData *image=NULL;
  BOOL		    success=FALSE;


  ShowMessage(Txt(TXT_LOADING_ILBM_PBM));


  /* load properties */

  if (!FindChunk(ID_BMHD))                    { SetError(INPUT_FILE_ERROR); goto errexit; }
  if (!ReadBlock(&BMHD,sizeof(BitMapHeader))) { SetError(INPUT_FILE_ERROR); goto errexit; }


  if (!(image=GetBuffer(BMHD.w,BMHD.h))) { goto errexit; }

  SetBufferMode(image,CLUT); /* LBM-format only supports CLUT images */


  /* load colors */

  if (FindChunk(ID_CMAP))
  {
    struct ColorLookupTable *clut;

    SeekPosLoad(-4,Relative);
    nColors = GetLong(0,0) / 3;

    if (!(clut=GetCLUT(nColors))) { goto errexit; }
    AttachCLUT(clut,image);

    for (i=0;i<nColors;i++)
    {
      r=ReadByte();
      g=ReadByte();
      b=ReadByte();

      SetColor(clut,i, r,g,b);
    }
  }


  /* load file */

  if (!FindChunk(ID_BODY)) { SetError(INPUT_FILE_ERROR); goto errexit; }

  if (BMHD.compression == cmpByteRun1)
  {
    signed char readin;
    int  count;
    UBYTE pattern,pattern2;
    UBYTE color;
    enum { CopyBlock,RunLength,NoOperation } state;

    for (y=0;y<BMHD.h;y++)
    {
      ShowProgress(y,BMHD.h);

      for (x=0 ; x<BMHD.w ; )
      {

	/* read RunLength-input */


	readin=(signed char)ReadByte();

	if (readin>=0)          /***** if (readin>=0 && readin<=127) *****/
	{
	  count = readin+1;


	  state=CopyBlock;
	}
	else if (readin>=-127)  /***** (readin>=-127 && readin<=-1)  *****/
	{
	  pattern=ReadByte();
	  count  =-readin+1;

	  state=RunLength;
	}
	else
	  state=NoOperation;


	/* write into bitplanes */

	if (BMHD.nPlanes == 8)
	{
	  switch (state)
	  {
	    case CopyBlock:
	      for (i=0;i<count;i++)
	      {
		color=ReadByte();
		if (!SetCLUTPixel(image,x,y,color)) goto errexit;
		x++;
	      }
	      break;

	    case RunLength:
	      for (i=0;i<count;i++)
	      {
		if (!SetCLUTPixel(image,x,y,pattern)) goto errexit;
		x++;
	      }
	      break;

	    case NoOperation:
	      break;
	  }
	}
	else
	if (BMHD.nPlanes == 4)
	{
	  switch (state)
	  {
	    case CopyBlock:
	      for (i=0;i<count;i++)
	      {
		color=ReadByte();

			      if (!SetCLUTPixel(image,x,y,(color & 0xF0)>>4)) goto errexit;
		x++;
		if (x<BMHD.w) if (!SetCLUTPixel(image,x,y,(color & 0x0F)>>0)) goto errexit;
		x++;
	      }
	      break;

	    case RunLength:
	      pattern2 = (pattern & 0x0F) >> 0;
	      pattern  = (pattern & 0xF0) >> 4;

	      for (i=0;i<count;i++)
	      {
			      if (!SetCLUTPixel(image,x,y,pattern )) goto errexit;
		x++;
		if (x<BMHD.w) if (!SetCLUTPixel(image,x,y,pattern2)) goto errexit;
		x++;
	      }
	      break;

	    case NoOperation:
	      break;
	  }
	}
	else
	{
	  SetError(FORMAT_NOT_SUPPORTED);
	  goto errexit;
	}
      }
    }
  }
  else /* uncompressed */
  {
    if (BMHD.nPlanes == 8)
    {
      UBYTE color;

      for (y=0;y<BMHD.h;y++)
      for (x=0;x<BMHD.w;x++)
      {
	color=ReadByte();
	if (!SetCLUTPixel(image,x,y,color)) goto errexit;
      }
    }
    else
    if (BMHD.nPlanes == 4)
    {
      UBYTE color1,color2;

      for (y=0;y<BMHD.h;y++)
      for (x=0;x<BMHD.w; )
      {
	color1=ReadByte();
	color2= color1 & 0x0F;
	color1=(color1 & 0xF0) >> 4;
	if (!SetCLUTPixel(image,x,y,color1)) goto errexit;
	x++;
	if (!SetCLUTPixel(image,x,y,color2)) goto errexit;
	x++;
      }
    }
    else
    { SetError(FORMAT_NOT_SUPPORTED); goto errexit; }
  }

  success=TRUE;

errexit:

  if (!success) if (image) FreeBuffer(image);

  if (success) SetDefaultBuffer(image);

  return success;
}

/**************************************** SAVE ILBM ****************************/

static int file_pos[2];

static BOOL BeginChunk(ULONG id,int level)
{
  if (!WriteLong(id,0,0)) return FALSE;

  if (!WriteLong(0,0,0)) return FALSE;  /* write a meaningless length */
  file_pos[level]=GetSavePos();

  return TRUE;
}

static BOOL EndChunk(int level)
{
  int currentpos;
  int chunksize;

  currentpos=GetSavePos();
  chunksize=currentpos-file_pos[level];

  if (chunksize & 1) { if (!WriteByte(0,0)) return FALSE;
		       currentpos++;
		     }

  if (!SeekPosSave(file_pos[level]-4,Absolute)) return FALSE;
  if (!WriteLong(chunksize,0,0))                return FALSE;
  if (!SeekPosSave(currentpos,Absolute))        return FALSE;

  return TRUE;
}

/*----------------------------------------------------------------------
		 THIS FILE HAS BEEN MODIFIED BY DiFa !!!
 *----------------------------------------------------------------------*
 * packer.c Convert data to "cmpByteRun1" run compression.     11/15/85
 *
 * By Jerry Morrison and Steve Shaw, Electronic Arts.
 * This software is in the public domain.
 *
 *	control bytes:
 *	 [0..127]   : followed by n+1 bytes of data.
 *	 [-1..-127] : followed by byte to be repeated (-n)+1 times.
 *	 -128	    : NOOP.
 *
 * This version for the Commodore-Amiga computer.
 *----------------------------------------------------------------------*/

#define DUMP	0
#define RUN	1

#define MinRun 3
#define MaxRun 128
#define MaxDat 128

#define PackerGetByte()       (*bufptr++)
#define PackerPutByte(c)      if (SaveByte(c)==EOF) return FALSE

static char buf[256];  /* [TBD] should be 128?	on stack?*/

static BOOL PutDump(int nn) {
	int i;

	PackerPutByte(nn-1);
	for(i = 0;  i < nn;  i++)   PackerPutByte(buf[i]);
	return TRUE;
	}

static BOOL PutRun(int nn, int cc)   {
	PackerPutByte(-(nn-1));
	PackerPutByte(cc);
	return TRUE;
	}

#define OutDump(nn)   PutDump(nn)
#define OutRun(nn,cc) PutRun(nn, cc)

/*----------- PackRow --------------------------------------------------*/

static BOOL PackRow(UBYTE *bufptr,int rowSize)
{
    char c,lastc = '\0';
    BOOL mode = DUMP;
    short nbuf = 0;		/* number of chars in buffer */
    short rstart = 0;		/* buffer index current run starts */

    buf[0] = lastc = c = PackerGetByte();  /* so have valid lastc */
    nbuf = 1;	rowSize--;	/* since one byte eaten.*/


    for (;  rowSize;  --rowSize) {
	buf[nbuf++] = c = PackerGetByte();
	switch (mode) {
		case DUMP:
			/* If the buffer is full, write the length byte,
			   then the data */
			if (nbuf>MaxDat) {
				if (!OutDump(nbuf-1)) return FALSE;
				buf[0] = c;
				nbuf = 1;   rstart = 0;
				break;
				}

			if (c == lastc) {
			    if (nbuf-rstart >= MinRun) {
				if (rstart > 0) if (!OutDump(rstart)) return FALSE;
				mode = RUN;
				}
			    else if (rstart == 0)
				mode = RUN;	/* no dump in progress,
				so can't lose by making these 2 a run.*/
			    }
			else  rstart = nbuf-1;		/* first of run */
			break;

		case RUN: if ( (c != lastc)|| ( nbuf-rstart > MaxRun)) {
			/* output run */
			if (!OutRun(nbuf-1-rstart,lastc)) return FALSE;
			buf[0] = c;
			nbuf = 1; rstart = 0;
			mode = DUMP;
			}
			break;
		}

	lastc = c;
	}

    switch (mode) {
	case DUMP: if (!OutDump(nbuf)) return FALSE; break;
	case RUN:  if (!OutRun(nbuf-rstart,lastc)) return FALSE; break;
	}

  return TRUE;
}



/**------------------------------------------------------------------------**
 **  SAVE  **  ILBM  **
 **------------------------------------------------------------------------**/

extern short NewBackgroundColor;

BOOL SaveILBM(void)
{
  struct ImageData * image;
  int		     BytesPerLine;
  BitMapHeader	     BMHD;
  UWORD 	     nPlanes;
  UBYTE 	   * buffer[24];
  BOOL		     HaveLineBuffer=FALSE;
  int		     nColors;
  ULONG 	     ConversionFlags;
  Compression	     compr=cmpByteRun1;
  BOOL		     success=FALSE;
  int width,height;


  /* get propertier */

  ConversionFlags = GetOutputFlags();

  nColors = Output_nColors + Output_ColorOffset;

       if (ConversionFlags & GFXMOD_24BIT)   nPlanes=24;
  else if (ConversionFlags & GFXMOD_HAM  ) { nPlanes= 6; nColors=16; }
  else if (ConversionFlags & GFXMOD_HAM8 ) { nPlanes= 8; nColors=64; }
  else if (ConversionFlags & GFXMOD_CLUT )
  {
    UWORD colors = nColors-1;

    for (nPlanes=0 ; colors ; nPlanes++) colors >>= 1;
  }
  else
    assert(0);



  /* convert to output-mode */

  if (!ConvertToMode(Output_nColors,ConversionFlags)) goto errexit;


  /* show logo */

  ShowMessage(Txt(TXT_SAVING_ILBM));


  /* write header */

  image=GetDefaultBuffer();

  BytesPerLine=((GetImageWidth(image)+15) & ~15)/8;

  if (!BeginChunk(FORM,0)) goto errexit;

    if (!WriteLong(ID_ILBM,0,0)) goto errexit;

  width  = GetImageWidth (image);
  height = GetImageHeight(image);

    if (!BeginChunk(ID_BMHD,1)) goto errexit;
    {
      BMHD.w	   = width;
      BMHD.h	   = height;
      BMHD.x	   = 0;
      BMHD.y	   = 0;
      BMHD.nPlanes = nPlanes;
      BMHD.masking = 0;
      BMHD.compression = compr;
      BMHD.pad1    = 0;
      if (NewBackgroundColor != -1)           { BMHD.transparentColor = NewBackgroundColor; }
      else if (AskBackgroundColor(image)==-1) { BMHD.transparentColor = 0;                  }
	   else 			      { BMHD.transparentColor = AskBackgroundColor(image); }
      BMHD.xAspect = 11;
      BMHD.yAspect = 10;
      BMHD.pageWidth  = BMHD.w;
      BMHD.pageHeight = BMHD.h;

      if (!WriteBlock(&BMHD,sizeof(BMHD))) goto errexit;
    }
    if (!EndChunk(1)) goto errexit;

    /* write CAMG if existing */

    {
      LONG CAMG=0;

      GetCAMG(image,&CAMG);

      if ( BMHD.nPlanes < 6)
      {
	if (BMHD.w > 400) CAMG |= HIRES_KEY; else CAMG &= ~HIRES_KEY;
      }

      if (BMHD.h > 300) CAMG |= INTERLACE; else CAMG &= ~INTERLACE;

      if (!BeginChunk(ID_CAMG,1) ||
	  !WriteLong(CAMG,0,0)   ||
	  !EndChunk(1) )             goto errexit;
    }


    /* write colormap if image isn't 24bit */

    if (!(ConversionFlags & GFXMOD_24BIT))
    {
      if (!BeginChunk(ID_CMAP,1)) goto errexit;
      {
	int nColors;
	int i;
	UBYTE r,g,b;

	for (i=0;i<Output_ColorOffset;i++)
	{
	  if (!WriteByte(EmptyCLUTEntry_R,0)) goto errexit;
	  if (!WriteByte(EmptyCLUTEntry_G,0)) goto errexit;
	  if (!WriteByte(EmptyCLUTEntry_B,0)) goto errexit;
	}

	nColors=GetImageNColors(image);

	for (i=0;i<nColors;i++)
	{
	  GetImageColor(image,i,&r,&g,&b);
	  if (!WriteByte(r,0) ||
	      !WriteByte(g,0) ||
	      !WriteByte(b,0) )   goto errexit;
	}
      }
      if (!EndChunk(1)) goto errexit;
    }


    /* write image */

    if (Output_ColorOffset!=0)
    {
      int x,y;
      UWORD col;
      for (y=0;y<height;y++)
	for (x=0;x<width;x++)
	{
	  GetCLUTPixel(image,x,y,&col);
	  SetCLUTPixel(image,x,y, col+Output_ColorOffset);
	}
    }

    if (!BeginChunk(ID_BODY,1)) goto errexit;
    {
      int y,p;

      if (!(HaveLineBuffer=GetLineBuffer( BytesPerLine*8 , nPlanes , buffer )) )
	goto errexit;


      for (y=0;y<BMHD.h;y++)
      {
	ShowProgress(y,BMHD.h-1);

	if (ConversionFlags & GFXMOD_24BIT)
	 { if (!GetRGBRow(image,y,buffer)) goto errexit; }
	else
	if (ConversionFlags & GFXMOD_HAM)
	 { if (!GetHAMRow(image,y,buffer)) goto errexit; }
	else
	if (ConversionFlags & GFXMOD_HAM8)
	 { if (!GetHAM8Row(image,y,buffer)) goto errexit; }
	else
	{ if (!GetCLUTRow(image,y,buffer,nPlanes)) goto errexit; }


	for (p=0;p<nPlanes;p++)
	{
	  if (compr == cmpNone)
	  {
	    if (!WriteBlock(buffer[p],BytesPerLine)) goto errexit;
	  }
	  else
	  if (compr == cmpByteRun1)
	  {
	    if (!PackRow(buffer[p],BytesPerLine)) goto errexit;
	  }
	  else
	    assert(0);
	}

	ForgetLine(image,y);
      }
    }
    if (!EndChunk(1)) goto errexit;

  if (!EndChunk(0)) goto errexit;


  success=TRUE;

errexit:

  if (HaveLineBuffer) FreeLineBuffer(nPlanes,buffer);

  return success;
}

