
/********************************************************************************
 *
 * modul name:	fbmp.c
 *
 * contents:	routines to load and check BMP files with RLE4,RLE8 or no
 *		compression; with 2,16 or 256 colors
 *
 *
 * to do:
 *
 * v1.8b (27.06.97)
 *   24bit-BMP loader
 *
 * v1.8 (07.04.97)
 *   Bug fixes in the RLE4 / RLE8 and monochrome part
 *
 * v1.1 (13.10.93)
 *   Major revision. Module entirely rewritten. Major bug fixes.
 *
 * v1.0 (08.10.93)
 *   added file-info and file-properties load
 *
 * v0.9 (00.00.93)
 *   basic load- and save-routines
 *
 *
 * Copyright (C) 1997  Dirk Farin  <dirk.farin@gmail.com>
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

struct Bitmap_Header
{
  UWORD bfType;
  ULONG bfSize;
  UWORD reserved0;
  UWORD reserved1;
  ULONG bfOffs;
};

struct Bitmap_Info
{
  ULONG biSize;
  ULONG biWidth;
  ULONG biHeight;
  UWORD biPlanes;
  UWORD biBitCnt;
  ULONG biCompr;
  ULONG biSizeIm;
  ULONG biXPels;
  ULONG biYPels;
  ULONG biClrUsed;
  ULONG biClrImp;
};

static void ReadHeader(struct Bitmap_Header *hd)
{
  hd->bfType = GetByte(0);
  hd->bfType <<= 8;
  hd->bfType |= GetByte(0);

  hd->bfSize = GetLong(0,1);
  hd->reserved0 = GetWord(0,1);
  hd->reserved1 = GetWord(0,1);
  hd->bfOffs = GetLong(0,1);
}



static void ReadBMInfo(struct Bitmap_Info *info)
{
  memset(info,0,sizeof(struct Bitmap_Info));

  info->biSize	  = GetLong(0,1);

  switch (info->biSize)
  {
    case 0x28:
      info->biWidth   = GetLong(0,1);
      info->biHeight  = GetLong(0,1);
      info->biPlanes  = GetWord(0,1);
      info->biBitCnt  = GetWord(0,1);
      info->biCompr   = GetLong(0,1);
      info->biSizeIm  = GetLong(0,1);
      info->biXPels   = GetLong(0,1);
      info->biYPels   = GetLong(0,1);
      info->biClrUsed = GetLong(0,1);
      info->biClrImp  = GetLong(0,1);
      break;

    case 0x0C:			     /* I'm only guessing the next entries !!! */
      info->biWidth   = GetWord(0,1);
      info->biHeight  = GetWord(0,1);
      info->biPlanes  = GetWord(0,1);
      info->biBitCnt  = GetWord(0,1);
      break;
  }
}

static void CheckForAllBMPFormats(form fo,int Compression)
{
  struct Bitmap_Header	Header;
  struct Bitmap_Info	Info;

  ReadHeader(&Header);
  ReadBMInfo(&Info);

  if (Header.bfType != (('B'<<8) | 'M')) { fo->not_valid_format=TRUE; return; }
  if (Info.biCompr  != Compression)      { fo->not_valid_format=TRUE; return; }

  fo->not_valid_format=FALSE;
}

void CheckForBMP (form fo) { CheckForAllBMPFormats(fo,0); }
void CheckForRLE8(form fo) { CheckForAllBMPFormats(fo,1); }
void CheckForRLE4(form fo) { CheckForAllBMPFormats(fo,2); }

void PropsBMP(void)
{
  struct Bitmap_Header Header;
  struct Bitmap_Info   Info;

  ReadHeader(&Header);
  ReadBMInfo(&Info);

  if (Info.biBitCnt==24)
  {
    Output_nColors = 256;
    Output_Mode    = GFXMOD_24BIT;
  }
  else
  {
    Output_nColors = 1 << Info.biBitCnt;
    Output_Mode    = GFXMOD_CLUT;
  }

  Output_Width	 = Info.biWidth;
  Output_Height  = Info.biHeight;
}

void InfoBMP(void)
{
  struct Bitmap_Header Header;
  struct Bitmap_Info   Info;
  char buffer[100];

  ReadHeader(&Header);
  ReadBMInfo(&Info);

  ShowInfo(Txt(TXT_WIN3X_BMP));
  ShowInfo(" ");

  BufShowInfo(Txt(TXT_WIDTH_D) ,Info.biWidth);
  BufShowInfo(Txt(TXT_HEIGHT_D),Info.biHeight);
  BufShowInfo(Txt(TXT_COLORS_D),1<<(Info.biBitCnt*Info.biPlanes));
  ShowInfo(" ");

  if (Info.biSize==0x28)
  {
    BufShowInfo(Txt(TXT_COMPRESSION_S),Info.biCompr == 0 ? Txt(TXT_NONE) :
				       Info.biCompr == 1 ? "RLE8" :
				       Info.biCompr == 2 ? "RLE4" :
							   Txt(TXT_UNKNOWN));
    BufShowInfo(Txt(TXT_XRESOLUTION_D_PIXM),Info.biXPels);
    BufShowInfo(Txt(TXT_YRESOLUTION_D_PIXM),Info.biYPels);
    BufShowInfo(Txt(TXT_COLORPLANES_D)     ,Info.biPlanes);
    BufShowInfo(Txt(TXT_BITSPERPIXEL_D)    ,Info.biBitCnt);
    BufShowInfo(Txt(TXT_COLORSUSED_D)      ,Info.biClrUsed);
    BufShowInfo(Txt(TXT_IMPORTANTCOLORS_D) ,Info.biClrImp);
    ShowInfo(" ");
  }
}


/**------------------------------------------------------------------------**
 **  LOAD  **
 **------------------------------------------------------------------------**/

BOOL LoadBMP(void)
{
  struct Bitmap_Header Header;
  struct Bitmap_Info   Info;
  struct ImageData  *image=NULL;
  int	 x,y;
  int	 nPlanes,nColors;
  int	 i;
  UBYTE  readin=0; /* don't really need to initialize, but this will calm down Lattice. */
  UBYTE  color;
  struct ColorLookupTable *clut;
  BOOL	 Is24Bit;
  int	 SavedWidth;
  BOOL	 success=FALSE;
  char	*buffer[1];
  BOOL	 HaveLineBuffer=FALSE;


  ShowMessage(Txt(TXT_LOADING_BMP));


  /* read properties */

  ReadHeader(&Header);
  ReadBMInfo(&Info);

		       SavedWidth = Info.biWidth;
  if (Info.biCompr==1) SavedWidth = (Info.biWidth + 1) & ~1;
  if (Info.biCompr==0) SavedWidth = (Info.biWidth + 7) & ~7;

  nPlanes = Info.biBitCnt;
  nColors = 1<<nPlanes;

  Is24Bit = (nPlanes==24);


  /* get image-buffer */

  if (!(image=GetBuffer(Info.biWidth,Info.biHeight))) goto errexit;

  SetBufferMode(image,Is24Bit ? RGB : CLUT);


  /* get Color-Map */

  if (!Is24Bit)
  {
    if (!(clut=GetCLUT(nColors))) goto errexit;
    AttachCLUT(clut,image);

    for (i=0;i<nColors;i++)
    {
      UBYTE r,g,b;

      b=ReadByte();
      g=ReadByte();
      r=ReadByte();

      if (Info.biSize==0x28) ReadByte();

      SetColor(clut,i,r,g,b);
    }
  }

//  printf("Start:%d\n",Header.bfOffs);
  if (!SeekPosLoad(Header.bfOffs,Absolute)) goto errexit;


  /* load image */

  if (Info.biCompr == 0) /* --------------------- no compression ------------ */
  {
	if (Info.biBitCnt == 8)  /*--- 256 colors ---*/
	{
	  UBYTE color;

	  for (y=Info.biHeight-1;y>=0;y--)
	  {
	    ShowProgress(Info.biHeight-1-y,Info.biHeight-1);

	    for (x=0;x<Info.biWidth;x++)
	    {

	      color = ReadByte();

	      if (!SetCLUTPixel(image,x,y,color)) goto errexit;
	    }
	    for ( ; x<SavedWidth;x++) ReadByte();  /* eat additional bytes */
	  }

	  ;

	  goto finished;
	}
	else if (Info.biBitCnt == 4)  /*--- 16 colors ---*/
	{
	  BOOL lowword=FALSE;

	  for (y=Info.biHeight-1;y>=0;y--)
	  {
	    ShowProgress(Info.biHeight-1-y,Info.biHeight-1);

	    for (x=0;x<Info.biWidth;x++)
	    {
	      if (!lowword) { readin  = ReadByte();
			      // printf("Read: %02x\n",readin);
			      color   = readin >> 4;
			    }
	      else	    { color   = readin & 0x0F;
			    }

	      if (!SetCLUTPixel(image,x,y,color)) goto errexit;

	      lowword = !lowword;
	    }

	    for (x=Info.biWidth ; x<SavedWidth;x++)
	    {
	      if (!lowword) { readin=ReadByte(); /*printf("Skip: %02x\n",readin);*/ }
	      lowword = !lowword;
	    }
	  }

	  ;  /* bug in Lattice ??? */

	  goto finished;
	}
	else if (Info.biBitCnt == 1)  /*--- 2 colors ---*/
	{
	  SavedWidth = (SavedWidth+15)/16*16;

	  if (!(HaveLineBuffer=GetLineBuffer( SavedWidth , 1 , buffer )) )
	    goto errexit;

	  for (y=Info.biHeight-1;y>=0;y--)
	  {
	    ShowProgress(Info.biHeight-1-y,Info.biHeight-1);

	    if (!ReadBlock (buffer[0],SavedWidth/8)) goto errexit;
	    if (!SetCLUTRow(image,y,buffer,1))       goto errexit;
	  }

	  ;

	  goto finished;
	}
	else if (Info.biBitCnt == 24) /*--- 24bit  (NEW V1.8b) ---*/
	{
	  for (y=Info.biHeight-1;y>=0;y--)
	  {
	    ShowProgress(Info.biHeight-1-y,Info.biHeight-1);
	    for (x=0;x<Info.biWidth;x++)
	    {
	      UBYTE r,g,b;

	      b=ReadByte();
	      g=ReadByte();
	      r=ReadByte();

	      if (!SetRGBPixel(image,x,y,r,g,b)) goto errexit;
	    }
	    x*=3;
	    while (x%4) { ReadByte(); x++; }
	  }

	  ;

	  goto finished;
	}
  }
  else
  if (Info.biCompr == 1) /* --------------------- RLE 8 --------------------- */
  {
	UBYTE count;
	UBYTE pattern;

	for (y=Info.biHeight-1;y>=0;y--)
	{
	  ShowProgress(Info.biHeight-1-y,Info.biHeight-1);

	  for (x=0;x<SavedWidth;)
	  {
	    count = ReadByte();

	    if (count == 0)
	    {
	      count = ReadByte();

	      if (count == 0 || count == 1)
	      {
		;
	      }
	      else if (count == 2)
	      {
		x += ReadByte();
		y -= ReadByte();
	      }
	      else					 /*=== copy run ===*/
	      {
		UBYTE addone;

		addone = count & 1;

		if (x+count > SavedWidth)
		{
//printf(">1>> x %d count %d   SavedWidth %d\n",x,count,SavedWidth);
		  SetError(INPUT_FILE_ERROR);
		  goto errexit;
		}

		while (count)
		{
		  pattern = ReadByte();
		  if (x<Info.biWidth)
		    if (!SetCLUTPixel(image,x,y,pattern)) goto errexit;
		  count--;
		  x++;
		}

		if (addone) ReadByte();
	      }
	    }
	    else					 /*=== pattern run ===*/
	    {
	      if (x+count > SavedWidth)
	      {
//printf(">2>> x %d count %d   SavedWidth %d\n",x,count,SavedWidth);
		SetError(INPUT_FILE_ERROR);
		goto errexit;
	      }

	      pattern = ReadByte();
	      while (count)
	      {
		if (x<Info.biWidth)
		  if (!SetCLUTPixel(image,x,y,pattern)) goto errexit;
		count--;
		x++;
	      }
	    }

	  }
	}

	;

	goto finished;
  }
  else
  if (Info.biCompr == 2) /* --------------------- RLE 4 --------------------- */
  {
	unsigned char count;
	UBYTE pattern,pattern2;
//printf(">1\n");
	for (y=Info.biHeight-1;y>=0;y--)
	{
	  ShowProgress(Info.biHeight-1-y,Info.biHeight-1);

	  for (x=0;x<SavedWidth;)
	  {
	    count = ReadByte();
	    //printf("count %d\n",count);
	    if (count != 0)                    /*=== pattern run ===*/
	    {
	      if (x+count > SavedWidth)
	      {
//printf(">2 %d %d  %d\n",x,count,SavedWidth);
		SetError(INPUT_FILE_ERROR);
		goto errexit;
	      }

	      pattern  = ReadByte();
	      pattern2 = pattern & 0x0F;
	      pattern  = pattern >> 4;

		//printf("pattern %d %d\n",pattern,pattern2);

	      while (count && x<SavedWidth)
	      {
		if (x<Info.biWidth)
		{
		  if (!SetCLUTPixel(image,x,y,pattern )) goto errexit;
		  //printf("%2d ",pattern);
		}
		x++;
		count--;

		if (count == 0) break;

		if (x<SavedWidth)
		{
		  if (!SetCLUTPixel(image,x,y,pattern2)) goto errexit;
		  //printf("%2d ",pattern2);
		}
		x++;
		count--;
	      }
	    }
	    else				/*=== copy run ===*/
	    {
	      count = ReadByte();
		//printf("copy %d\n",count);
	      if (count >= 3)
	      {
		BOOL align;

//		  align = ( (((count+1)&~1) % 4) == 2);
		align=FALSE;
		if ((count%4)==2) align=TRUE;
		if ((count%4)==1) align=TRUE;

		while (count)
		{
		  readin = ReadByte();

		  if (x<SavedWidth)
		  {
		    if (!SetCLUTPixel(image,x,y,readin  >>  4)) goto errexit;
		    //printf("%2d ",readin>>4);
		  }
		  x++;
		  count--;

		  if (count==0) break;

		  if (x<Info.biWidth)
		  {
		    if (!SetCLUTPixel(image,x,y,readin & 0x0F)) goto errexit;
		    //printf("%2d ",readin&0xf);
		  }
		  x++;
		  count--;
		}
		if (align) ReadByte();
	      }
	      else
	      {
		switch(count)
		{
		  case 0: break;
		  case 1: break;
		  case 2: x += ReadByte();
			  y -= ReadByte();
			  break;
		}
	      }
	    }
	    //printf("x %d SavedWidth %d\n",x,SavedWidth);
	  }
	  //printf("\n");
	}

//printf(">4\n");
	;

	goto finished;
  }


  SetError(FORMAT_NOT_SUPPORTED);
  goto errexit;


finished:
  success=TRUE;

errexit:
  if (!success) if (image) FreeBuffer(image);
  if (HaveLineBuffer)      FreeLineBuffer(nPlanes,buffer);
  if (success)             SetDefaultBuffer(image);

  return success;
}

BOOL SaveBMP(void)
{
  return FALSE;
}

