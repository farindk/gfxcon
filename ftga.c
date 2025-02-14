
/********************************************************************************
 *
 * modul name:  ftga.c
 *
 * contents:    routines to load, save and check TGA (Targa) files
 *
 *
 * v1.0 (02.01.95)
 *   basic load-routine
 *
 *
 * Copyright (C) 1995  Dirk Farin  <dirk.farin@gmail.com>
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


static struct TGAFooter
{
  ULONG ExtensionOffset;
  ULONG DeveloperOffset;
  char  signature[16];
  char  dotchar;
  char  zero;
};

static struct TGAFooter footer;

BOOL LoadFooter(void)
{
  SeekPosLoad(-26,FromEnd);
  footer.ExtensionOffset=GetLong(0,TRUE);
  footer.DeveloperOffset=GetLong(0,TRUE);
  if (!ReadBlock(&footer.signature[0],26-8)) return FALSE;
  return TRUE;
}

BOOL IsNewFormat(void)
{
  if (strcmp(footer.signature,"TRUEVISION-XFILE.")==0) return TRUE;
  else                                                 return FALSE;
}

/**------------------------------------------------------------------------**
 **  CHECK  ** for TGA-format
 **------------------------------------------------------------------------**/

void CheckForTGA(form fo)
{
  LoadFooter();
  if (IsNewFormat())
  {
    fo->not_valid_format = FALSE;
    return;
  }


  /* --- Altes Format; sichere Erkennung nicht möglich. --- */


  /* ColorMap-Type auf Konsistenz prüfen */

  SeekPosLoad(1,Absolute);

  switch(ReadByte())
  {
    case 0:
    case 1:
              break;
    default:
                fo->not_valid_format = TRUE;
                return;
  }


  /* Image-Type auf Konsistenz prüfen */

  switch(ReadByte())
  {
    case 0:
    case 1:
    case 2:
    case 3:
    case 9:
    case 10:
    case 11:
                break;

    default:
                fo->not_valid_format = TRUE;
                return;
  }

  SeekPosLoad(5+4,Relative);

  if (GetWord(0,TRUE)==0 || GetWord(0,TRUE)==0)
  {
    fo->not_valid_format = TRUE;
    return;
  }

  if (ReadByte()==0)
  {
    fo->not_valid_format = TRUE;
    return;
  }

  fo->not_valid_format = FALSE;
}


/**------------------------------------------------------------------------**
 **  PROPS  **
 **------------------------------------------------------------------------**/

void PropsTGA(void)
{
  char  c;

  SeekPosLoad(2,Absolute);
  c=ReadByte();

  Output_Mode = GFXMOD_CLUT;

  if (c==3 || c==11) Output_nColors=2;
  if (c==2 || c==10) { Output_Mode=GFXMOD_24BIT; Output_nColors=256; } // TrueColor
  if (c==1 || c==9)
  {
    SeekPosLoad(16,Absolute);
    c=ReadByte();

    assert(c<=8);
    Output_nColors = 1<<c;
  }
  if (c==0)
  {
    Output_Width  =0;
    Output_Height =0;
    Output_nColors=0;
  }

  SeekPosLoad(8+2+2,Absolute);

  Output_Width =GetWord(0,TRUE);
  Output_Height=GetWord(0,TRUE);
}


/**------------------------------------------------------------------------**
 **  INFO  **
 **------------------------------------------------------------------------**/

void InfoTGA(void)
{
  char buffer[100];
  char c;
  char *s;
  BOOL hascolormap;

  ShowInfo("Targa (TGA) infos");
  ShowInfo(" ");

  LoadFooter();
  if (IsNewFormat()) ShowInfo("New TGA Format");
  else               ShowInfo("Original TGA Format");

  SeekPosLoad(1,Absolute);

  switch(ReadByte())
  {
    case 0: ShowInfo(Txt(TXT_TARGA_NO_CMAP));    hascolormap=FALSE; break;
    case 1: ShowInfo(Txt(TXT_TARGA_CMAP_INCL));  hascolormap=TRUE;  break;
    default: ShowInfo(Txt(TXT_TARGA_UNKN_CMAP)); hascolormap=FALSE; break;
  }

  switch(ReadByte())
  {
    case  0: ShowInfo(Txt(TXT_TARGA_NO_IMAGE));        break;
    case  1: ShowInfo(Txt(TXT_TARGA_UNCOMPR_CMAP));    break;
    case  2: ShowInfo(Txt(TXT_TARGA_UNCOMPR_TRUECOL)); break;
    case  3: ShowInfo(Txt(TXT_TARGA_UNCOMPR_BW));      break;
    case  9: ShowInfo(Txt(TXT_TARGA_RLE_CMAP));        break;
    case 10: ShowInfo(Txt(TXT_TARGA_RLE_TRUECOL));     break;
    case 11: ShowInfo(Txt(TXT_TARGA_RLE_BW));          break;
  }

  if (hascolormap)
  {
    BufShowInfo(Txt(TXT_TARGA_FIRST_CMAP_ENTRY), GetWord(0,TRUE));
    BufShowInfo(Txt(TXT_TARGA_CMAP_ENTRIES)    , GetWord(0,TRUE));
    BufShowInfo(Txt(TXT_TARGA_BITS_PER_ENTRY)  , ReadByte());
  }
  else
  {
    SeekPosLoad(5,Relative);
  }

  BufShowInfo(Txt(TXT_TARGA_XORIGIN), GetWord(0,TRUE));
  BufShowInfo(Txt(TXT_TARGA_YORIGIN), GetWord(0,TRUE));
  BufShowInfo(Txt(TXT_TARGA_WIDTH)  , GetWord(0,TRUE));
  BufShowInfo(Txt(TXT_TARGA_HEIGHT) , GetWord(0,TRUE));
  BufShowInfo(Txt(TXT_TARGA_PIXELDEPTH), ReadByte());

  c=ReadByte();

  switch(c & 0x30)
  {
    case 0x00: s=Txt(TXT_TARGA_BOTLEFT);  break;
    case 0x10: s=Txt(TXT_TARGA_BOTRIGHT); break;
    case 0x20: s=Txt(TXT_TARGA_TOPLEFT);  break;
    case 0x30: s=Txt(TXT_TARGA_TOPRIGHT); break;
  }

  BufShowInfo(Txt(TXT_TARGA_ORIGIN),s);
  BufShowInfo(Txt(TXT_TARGA_ALPHA_BITS),c&0xF);
}


/**------------------------------------------------------------------------**
 **  LOAD  **
 **------------------------------------------------------------------------**/

static enum { TrueColor,ColorMapped,BlackWhite } ColorType;
static unsigned char IDlength;
static char ColorMapType;
static char ImageType;
static enum { Uncompressed,RunLength } Compression;
static BOOL ImageIncluded;

static int   FirstColorMapEntry;
static int   ColorMapLength;
static short BitsPerCMEntry;

static UWORD imagewidth;
static UWORD imagewidth;
static UWORD imageheight;
static UBYTE pixeldepth;
static UBYTE alphabits;
static enum { BL=0,BR=1,TL=2,TR=3 } ImageOrigin;

static struct ImageData* inputimage;

static BOOL DrawPixel8(int x,int y,char* data)
{
  return SetCLUTPixel(inputimage,x,y,*data);
}

static BOOL DrawPixel24(int x,int y,char* data)
{
  return SetRGBPixel(inputimage,x,y,data[2],data[1],data[0]);
}

static BOOL DrawPixel32(int x,int y,char* data)
{
  return SetRGBPixel(inputimage,x,y,data[2],data[1],data[0]);
}

static BOOL DrawPixel16(int x,int y,char* data)
{
  USHORT w;
  UBYTE r,g,b;

  w =   data[1];
  w <<= 8;
  w |=  data[0];

  b = w&0x1F; w>>=5;
  g = w&0x1F; w>>=5;
  r = w&0x1F;

  r<<=3;
  g<<=3;
  b<<=3;

  return SetRGBPixel(inputimage,x,y,r,g,b);
}

typedef BOOL (*drawpixelfunc)(int,int,char*);

static drawpixelfunc drawpixel;

BOOL LoadTGA(void)
{
  struct ColorLookupTable* clut=NULL;
  int   x;
  int   y;
  char  pixellength;
  short repcount;
  UBYTE input[4];

  ShowMessage(Txt(TXT_LOADING_TARGA));

  inputimage=NULL;

  SeekPosLoad(0,Absolute);
  IDlength    =ReadByte();
  ColorMapType=ReadByte();
  ImageType   =ReadByte();

  if (ImageType>=1 && ImageType<= 3) Compression=Uncompressed;
  if (ImageType>=9 && ImageType<=11) Compression=RunLength;
  switch (ImageType)
  {
    case  1:
    case  9: ColorType = ColorMapped;
             break;
    case  2:
    case 10: ColorType = TrueColor;
             break;
    case  3:
    case 11: ColorType = BlackWhite;
             break;
    default:
      SetError(FORMAT_NOT_SUPPORTED);
      return FALSE;
  }

  ImageIncluded = (ImageType!=0);

  FirstColorMapEntry = GetWord(0,TRUE);
  ColorMapLength     = GetWord(0,TRUE);
  BitsPerCMEntry     = ReadByte();

  GetWord(0,TRUE);
  GetWord(0,TRUE);
  imagewidth  = GetWord(0,TRUE);
  imageheight = GetWord(0,TRUE);
  pixeldepth  = ReadByte();

  {
    char c;
    c=ReadByte();
    alphabits = c&0x0F;
    ImageOrigin = (c>>4)&3;
  }


  inputimage=GetBuffer(imagewidth,imageheight);
  if (!inputimage) goto errexit;

  SetBufferMode(inputimage,(ColorType==TrueColor) ? RGB : CLUT);

  if (ColorMapType!=0)
  {
    if (!(clut=GetCLUT(1<<pixeldepth))) goto errexit;
    AttachCLUT(clut,inputimage);
  }


  /* ImageID-Feld überspringen */

  SeekPosLoad(IDlength,Relative);


  /* ColorMap ggf. einlesen */

  if (ColorMapType==0)
  {
    /* nothing */
  }
  else if (ColorMapType==1)
  {
    int  i;
    char r,g,b;

    for (i=0;i<ColorMapLength;i++)
    {
      switch(BitsPerCMEntry)
      {
        case 24:
          b = ReadByte();
          g = ReadByte();
          r = ReadByte();
          break;

        case 32:
          b = ReadByte();
          g = ReadByte();
          r = ReadByte();
              ReadByte();  /* alpha-channel */
          break;

        default:
          SetError(FORMAT_NOT_SUPPORTED);
          goto errexit;
      }

      SetColor(clut,FirstColorMapEntry+i, r,g,b);
    }
  }
  else
  {
    SetError(FORMAT_NOT_SUPPORTED);
    return FALSE;
  }


  /* Bild einlesen */

  ShowProgress(0,1);

  switch(pixeldepth)
  {
    case  8: drawpixel = DrawPixel8;  break;
    case 16: drawpixel = DrawPixel16; break;
    case 24: drawpixel = DrawPixel24; break;
    case 32: drawpixel = DrawPixel32; break;
    default: SetError(FORMAT_NOT_SUPPORTED);
             return FALSE;
  }

  y=imageheight-1;
  x=0;
  pixellength = pixeldepth/8;

  if (Compression==RunLength)
  {
    for (;y>=0;)
    {
      repcount=ReadByte();

      if (repcount & 0x80)
      {
        repcount = repcount-0x80+1;
        if (!ReadBlock(input,pixellength)) return FALSE;

        while (repcount > 0)
        {
          if (!drawpixel(x,y,input)) return FALSE;
          x++;
          if (x==imagewidth)
          {
            x=0; y--;
            ShowProgress(imageheight-1-y,imageheight-1);
          }
          repcount--;
        }
      }
      else
      {
        repcount++;

        while (repcount > 0)
        {
          if (!ReadBlock(input,pixellength)) return FALSE;
          if (!drawpixel(x,y,input)) return FALSE;
          x++;
          if (x==imagewidth)
          {
            x=0; y--;
            ShowProgress(imageheight-1-y,imageheight-1);
          }
          repcount--;
        }
      }
    }

  }



  if (Compression==Uncompressed)
  {
    for (y=imageheight-1;y>=0;y--)
    {
      ShowProgress(imageheight-1-y,imageheight-1);

      for (x=0;x<imagewidth;x++)
      {
        ReadBlock(input,pixellength);
        drawpixel(x,y,input);
      }
    }
  }

  SetDefaultBuffer(inputimage);

  return TRUE;

errexit:
  if (inputimage) FreeBuffer(inputimage);
  return FALSE;
}

