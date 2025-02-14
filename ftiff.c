
/********************************************************************************
 *
 * modul name:	ftiff.c
 *
 * contents:	routines to load and check TIFF files which may be uncompressed
 *		or compressed with CCITT2, PackBit or LZW
 *
 *
 * v1.8 (06.04.97)
 *   LZW decompression
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

/* New V1.8 : LZW decompression */

struct CodeTable
{
  struct CodeTable* prefix;
  UBYTE  character;
  int	 stringlength;
  UBYTE  firstchar;
} *codetable;
int nextentry;

#define CLEARCODE 256
#define ENDOFINFORMATION 257


static UBYTE bitbuffer[10];
static int curbit,lastbit,lastbyte;

static void InitRead(void)
{
  curbit=0;
  lastbyte=-1;
  lastbit=-1;
}

static int ReadCode(int codesize)
{
  int code;
  int i;

  // ggf. Buffer auffuellen

  while ((curbit+codesize) >= lastbit)
  {
    while (curbit>8)
    {
      int i;
      for (i=0;i<lastbyte;i++)
	bitbuffer[i] = bitbuffer[i+1];
      curbit-=8;
      lastbit-=8;
      lastbyte--;
    }

    lastbyte++;
    bitbuffer[lastbyte]=GetByte(0);
//    printf("read byte %p in [%d]\n",bitbuffer[lastbyte],lastbyte);
    lastbit+=8;
  }

/*
  printf("bitbuffer: ");
  for (i=0;i<=lastbyte;i++)
    printf("[%d]:%p ",i,bitbuffer[i]);
  printf("\n");
*/

  // Code extrahieren

  code=0;
  for (i=0;i<codesize;i++,curbit++)
  {
    code <<= 1;
//    printf("curbit: %d  %p mask %p ",curbit,bitbuffer[curbit/8],0x80>>(curbit%8));
    if (bitbuffer[curbit/8] & (0x80>>(curbit%8)))
    {
      code |= 1;
//	printf("1\n");
    }
    else
    {
//	printf("0\n");
    }
  }

//  printf("code (%d): %d\n",codesize,code);
  return code;
}

static void InitCodeTable(void)
{
  int i;

//  printf("InitCodeTable\n");

  for (i=0;i<256;i++)
  {
    codetable[i].character=i;
    codetable[i].firstchar=i;
    codetable[i].prefix = NULL;
    codetable[i].stringlength = 1;
  }
  nextentry = 258;
}

static int CodeToBuffer(int code,UBYTE* buffer,UBYTE* bufferend)
{
  struct CodeTable* tab = &codetable[code];
  int totallength = tab->stringlength;

  if (&buffer[totallength-1] > bufferend)
  { printf("buffer overflow\n"); return totallength; }


  while(tab)
  {
    //printf("%3d ",tab->character);
    buffer[tab->stringlength-1] = tab->character;
    tab = tab->prefix;
  }
  //printf("\n");

  return totallength;
}

static BOOL LoadLZW(UBYTE* buffer,int buffersize)
{
  int codesize=9;
  int code,oldcode=-1;

//  UBYTE* bufferstart=buffer;
  UBYTE* bufferend  =&buffer[buffersize];

  InitRead();

  for(;;)
  {
    code = ReadCode(codesize);

    if (code==CLEARCODE)
    {
//	printf("CLEAR\n");
      InitCodeTable();
      codesize=9;

      code = ReadCode(codesize);
      if (code==ENDOFINFORMATION)
	return TRUE;
      buffer = &buffer[CodeToBuffer(code,buffer,bufferend)];
      if (buffer>bufferend) { printf("buffer overflow\n"); return TRUE; }
      oldcode=code;

      continue;
    }
    if (code==ENDOFINFORMATION)
    {
//	printf("EOI\n");
      return TRUE;
    }

    //printf("%p/%p ",buffer,bufferend);

    if (code < nextentry)
    {
//	printf("in table insert:[%d]\n",nextentry);

      buffer = &buffer[CodeToBuffer(code,buffer,bufferend)];
      if (buffer>bufferend) { printf("buffer overflow\n"); return TRUE; }
      codetable[nextentry].prefix	= &codetable[oldcode];
      codetable[nextentry].character	= codetable[code].firstchar;
      codetable[nextentry].stringlength = codetable[oldcode].stringlength+1;
      codetable[nextentry].firstchar	= codetable[oldcode].firstchar;
      nextentry++;
      oldcode = code;
    }
    else
    {
//	printf("not in table insert:[%d]\n",nextentry);

      buffer = &buffer[CodeToBuffer(oldcode,buffer,bufferend)];
      if (buffer>bufferend) { printf("buffer overflow\n"); return TRUE; }
      *buffer++ = codetable[oldcode].firstchar;
      if (buffer>bufferend) { printf("buffer overflow\n"); return TRUE; }

      codetable[nextentry].prefix	= &codetable[oldcode];
      codetable[nextentry].character	= codetable[oldcode].firstchar;
      codetable[nextentry].stringlength = codetable[oldcode].stringlength+1;
      codetable[nextentry].firstchar	= codetable[oldcode].firstchar;
      nextentry++;
      oldcode = code;
    }

    if (nextentry == (1<<codesize)-1)
      codesize++;
  }

  return TRUE;
}

struct TiffTag
{
  UWORD TagType;
  UWORD DataType;
  ULONG DataLength;
  ULONG Value;

  ULONG DataPos;

  UWORD Numerator;
  UWORD Denominator;
};

#define TAG_NewSubfileType	0x0FE
#define TAG_SubfileType 	0x0FF
#define TAG_ImageWidth		0x100
#define TAG_ImageLength 	0x101
#define TAG_XResolution 	0x11A
#define TAG_YResolution 	0x11B
#define TAG_ResolutionUnit	0x128
#define TAG_Orientation 	0x112
#define TAG_PlanarConfiguration 0x11C

#define TAG_StripOffset 	0x111
#define TAG_StripByteCounts	0x117
#define TAG_RowsPerStrip	0x116

#define TAG_SamplesPerPixel	0x115
#define TAG_BitsPerSample	0x102
#define TAG_Tresholding 	0x107
#define TAG_CellWidth		0x108
#define TAG_CellLength		0x109
#define TAG_MinSampleValue	0x118
#define TAG_MaxSampleValue	0x119
#define TAG_PhotometricInterpretation 0x106
#define TAG_GrayResponseCurve	0x123
#define TAG_GrayResponseUnit	0x122
#define TAG_ColorResponseCurve	0x12D
#define TAG_ColorResponseUnit	0x12C
#define TAG_Predictor		0x13D
#define TAG_ColorMap		0x140

#define TAG_FillOrder		0x10A

#define TAG_Compression 	0x103
#define TAG_Group3Options	0x124
#define TAG_Group4Options	0x125

#define TAG_DocumentName	0x10D
#define TAG_PageName		0x11D
#define TAG_XPosition		0x11E
#define TAG_YPosition		0x11F
#define TAG_ImageDescription	0x10E
#define TAG_ScannerMake 	0x10F
#define TAG_ScannerModel	0x110
#define TAG_PageNumber		0x129
#define TAG_Software		0x131
#define TAG_DateTime		0x132
#define TAG_Artist		0x13B
#define TAG_HostComputer	0x13C

#define TAG_FreeOffsets 	0x120
#define TAG_FreeByteCounts	0x121



void CheckForTIFF(form fo)
{
  char byte_order1;
  char byte_order2;

  byte_order1 = GetByte(0);
  byte_order2 = GetByte(0);

  if (byte_order1 != byte_order2) { fo->not_valid_format=TRUE; return; }

  if (byte_order1 != 'I' &&
      byte_order1 != 'M')         { fo->not_valid_format=TRUE; return; }

  fo->not_valid_format=FALSE;
}

static BOOL IntelOrder;

static ULONG TGetLong(void) { return GetLong(0,IntelOrder); }
static UWORD TGetWord(void) { return GetWord(0,IntelOrder); }
static UBYTE TGetByte(void) { return GetByte(0);            }

static void StartupRead(void)
{
  char	order;
  ULONG pos;

  SeekPosLoad(0,Absolute);

  order=GetByte(0);
	GetByte(0);
  if (order=='I') IntelOrder=TRUE; else IntelOrder=FALSE;

  TGetWord(); /* eat version */

  SeekPosLoad(4,Absolute);
  pos=TGetLong();
  SeekPosLoad(pos,Absolute);
}


/**------------------------------------------------------------------------**
 **  Read in a Tag
 **  If the value will fit in one ULONG, it can be found in tag->Value.
 **  The file-position of the value will be in tag->DataPos in EVERY case.
 **------------------------------------------------------------------------**/
static void ReadTag(struct TiffTag *tag)
{
  tag->TagType	  = TGetWord();
  tag->DataType   = TGetWord();
  tag->DataLength = TGetLong();
  tag->Value	  = TGetLong();

  if (tag->DataType < 5 && tag->DataLength==1)
    tag->DataPos = GetLoadPos()-4;
  else
    tag->DataPos = tag->Value;

  if (tag->DataType == 5) /* rational */
  {
    int oldpos=GetLoadPos();

    SeekPosLoad(tag->Value,Absolute);
    tag->Numerator   = TGetLong();
    tag->Denominator = TGetLong();
    SeekPosLoad(oldpos,Absolute);
  }

  if ( ! IntelOrder )
  {
    switch (tag->DataType)
    {
      case 1: tag->Value >>= 24; break;
      case 2: tag->Value >>= 24; break;
      case 3: tag->Value >>= 16; break;
    }
  }
}

static char *TGetString(int pos)
{
  int oldpos=GetLoadPos();
  static char buffer[100];
  int i;

  SeekPosLoad(pos,Absolute);

  for (i=0; ;i++)
  {
    buffer[i] = TGetByte();
    if (buffer[i]==0) break;
    if (i==99) { buffer[99]=0; break; }
  }

  SeekPosLoad(oldpos,Absolute);

  return buffer;
}

#define Case break; case

/*=============================================================================
  ***********************************  INFO  **********************************
  =============================================================================*/

static BOOL InfoTag(struct TiffTag *tag)
{
  char buffer[100];
  ULONG value=tag->Value;

  switch (tag->TagType)
  {
    case TAG_NewSubfileType	 :
	 ShowInfo("NewSubfileType :");
	 if(value & (1<<0)) ShowInfo(Txt(TXT_REDUCED_RESOLUTION));
	 if(value & (1<<1)) ShowInfo(Txt(TXT_MULTI_PAGE_PICTURE));
	 if(value & (1<<2)) ShowInfo(Txt(TXT_TRANSPARENT_MASK)  );
	 if((value & 0x07)==0) ShowInfo(Txt(TXT_NOTHING_SPECIAL));

    Case TAG_SubfileType	 :
	 BufShowInfo("SubfileType : %s",value==1 ? Txt(TXT_FULL_RES_PICTURE)    :
					value==2 ? Txt(TXT_REDU_RES_PICTURE)    :
					value==3 ? Txt(TXT_SINGLE_PAGE_PICTURE) :
						   Txt(TXT_UNKNOWN));

    Case TAG_ImageWidth 	 :
	 BufShowInfo("ImageWidth : %d",tag->Value);

    Case TAG_ImageLength	 :
	 BufShowInfo("ImageLength : %d",tag->Value);

    Case TAG_BitsPerSample	 :
	 BufShowInfo("BitsPerSample : %d",value);

    Case TAG_Compression	 :
	 BufShowInfo("Compression : %s",value == 1     ? Txt(TXT_UNCOMPRESSED) :
					value == 2     ? "CCITT/3 1-D"  :
					value == 3     ? "FAX CCITT Group 3" :
					value == 4     ? "FAX CCITT Group 4" :
					value == 5     ? "LZW" :
					value == 32771 ? Txt(TXT_UNCOMPRESSED_WORDALIGN) :
					value == 32773 ? "PackBit" :
							 Txt(TXT_UNKNOWN));

    Case TAG_PhotometricInterpretation :
	 BufShowInfo("PhotometricInterpretation : %s",value == 0 ? Txt(TXT_MINSAMPLE_WHITE) :
						      value == 1 ? Txt(TXT_MINSAMPLE_BLACK) :
						      value == 2 ? Txt(TXT_MINSAMPLE_RGB_DARK) :
						      value == 3 ? Txt(TXT_PALETTE_COLOR) :
						      value == 4 ? Txt(TXT_TRANSPARENCY_MASK) :
								   Txt(TXT_UNKNOWN) );

    Case TAG_Tresholding	 :
	 BufShowInfo("Tresholding : %s",value == 1 ? Txt(TXT_BILEVEL)         :
					value == 2 ? Txt(TXT_GREYSCALE_TO_BW) :
					value == 3 ? Txt(TXT_GREYSCALE_TO_BW_ERRDIF) :
						     Txt(TXT_UNKNOWN) );

    Case TAG_CellWidth		 :
	 BufShowInfo("CellWidth : %d",value);

    Case TAG_CellLength 	 :
	 BufShowInfo("CellLength : %d",value);

    Case TAG_FillOrder		 :
	 BufShowInfo("FillOrder : %s",value == 1 ? Txt(TXT_MSB_LSB) :
				      value == 2 ? Txt(TXT_LSB_MSB) :
						   Txt(TXT_UNKNOWN)   );

    Case TAG_DocumentName	 :
	 BufShowInfo("DocumentName : %s",TGetString(value));

    Case TAG_ImageDescription	 :
	 BufShowInfo("ImageDescription : %s",TGetString(value));

    Case TAG_ScannerMake	 :
	 BufShowInfo("ScannerMake : %s",TGetString(value));

    Case TAG_ScannerModel	 :
	 BufShowInfo("ScannerModel : %s",TGetString(value));

    Case TAG_StripOffset	 :
	 BufShowInfo3("StripOffset : %s at $%x",tag->DataLength == 1 ? "bitmap" : "pointer array",
						value);

    Case TAG_Orientation	 :
	 BufShowInfo(Txt(TXT_ORIENTATION_S),value == 1 ? Txt(TXT_EDGE_TLR) :
					    value == 2 ? Txt(TXT_EDGE_TRL) :
					    value == 3 ? Txt(TXT_EDGE_BRL) :
					    value == 4 ? Txt(TXT_EDGE_BLR) :
					    value == 5 ? Txt(TXT_EDGE_LTB) :
					    value == 6 ? Txt(TXT_EDGE_RTB) :
					    value == 7 ? Txt(TXT_EDGE_RBT) :
					    value == 8 ? Txt(TXT_EDGE_LBT) :
							 Txt(TXT_UNKNOWN)  );


    Case TAG_SamplesPerPixel	 :
	 BufShowInfo("SamplesPerPixel : %d",value);

    Case TAG_RowsPerStrip	 :
	 BufShowInfo("RowsPerStrip : %d",value);

    Case TAG_StripByteCounts	 :
	 BufShowInfo3("StripByteCounts :%s $%x",tag->DataLength==1 ? "" : " length table at",value);

    Case TAG_MinSampleValue	 :
	 BufShowInfo("MinSampleValue : %d",value);

    Case TAG_MaxSampleValue	 :
	 BufShowInfo("MaxSampleValue : %d",value);

    Case TAG_XResolution	 :
	 BufShowInfo3("XResolution : %d/%d",tag->Numerator,tag->Denominator);

    Case TAG_YResolution	 :
	 BufShowInfo3("YResolution : %d/%d",tag->Numerator,tag->Denominator);

    Case TAG_PlanarConfiguration :
	 BufShowInfo("PlanarConfiguration : %s",value == 1 ? Txt(TXT_PLANAR_NOBITPLANES) :
						value == 2 ? Txt(TXT_PLANAR_BITPLANES) :
							     Txt(TXT_UNKNOWN));

    Case TAG_PageName		 :
	 BufShowInfo("PageName : %s",TGetString(value));

    Case TAG_XPosition		 :
	 BufShowInfo("XPosition : %d",value);

    Case TAG_YPosition		 :
	 BufShowInfo("YPosition : %d",value);

    Case TAG_FreeOffsets	 :
	 BufShowInfo("FreeOffsets : table at $%x",value);

    Case TAG_FreeByteCounts	 :
	 BufShowInfo("FreeByteCounts : table at $%x",value);

    Case TAG_GrayResponseUnit	 :
	 BufShowInfo("GrayResponseUnit : 1/%d",value == 1 ? 10 :
					       value == 2 ? 100 :
					       value == 3 ? 1000 :
					       value == 4 ? 10000 :
					       value == 5 ? 100000 : 0);
    Case TAG_GrayResponseCurve	 :
	 BufShowInfo("GrayResponseCurve : table at $%x",value);

    Case TAG_Group3Options	 :
	 ShowInfo("Group3Options :");
	 if (value & 0x1) ShowInfo("    2 dimensional coding");
	 else		  ShowInfo("    1 dimensional coding");
	 if (value & 0x2) ShowInfo(Txt(TXT_4SPC_UNCOMPRESSED));
	 if (value & 0x4) ShowInfo(Txt(TXT_4SPC_FILLBITSATEND));

    Case TAG_Group4Options	 :
	 ShowInfo("Group4Options :");
	 if (value & 0x2) ShowInfo(Txt(TXT_4SPC_UNCOMPRESSED));
	 if ((value & 0x2)==0) ShowInfo(Txt(TXT_4SPC_NONE));

    Case TAG_ResolutionUnit	 :
	 BufShowInfo("ResolutionUnit : %s",value==1 ? Txt(TXT_NO_INFORMATION) :
					   value==2 ? "inch" :
					   value==3 ? "cm" : Txt(TXT_UNKNOWN));

    Case TAG_PageNumber 	 :
	 BufShowInfo3("PageNumber : number %d of a total of %d",value & 0xffff,
							       (value & 0xffff0000) >> 16);

    Case TAG_ColorResponseUnit	 :
	 BufShowInfo("ColorResponseUnit : 1/%d",value==1 ? 10 :
						value==2 ? 100 :
						value==3 ? 1000 :
						value==4 ? 10000 :
						value==5 ? 100000 : 0);

    Case TAG_ColorResponseCurve :
	 BufShowInfo("ColorResponseCurve : %d",value);

    Case TAG_Software		 :
	 BufShowInfo("Software : %s",TGetString(value));

    Case TAG_DateTime		 :
	 BufShowInfo("DateTime : %s",TGetString(value));

    Case TAG_Artist		 :
	 BufShowInfo("Artist : %s",TGetString(value));

    Case TAG_HostComputer	 :
	 BufShowInfo("HostComputer : %s",TGetString(value));

    Case TAG_Predictor		 :
	 BufShowInfo("Predictor : %s",value == 1 ? Txt(TXT_NO_PREDICTION) :
				      value == 2 ? "horizontal" :
						   Txt(TXT_UNKNOWN));

    Case TAG_ColorMap		 :
	 BufShowInfo("ColorMap : table at $%x",value);

	 break;

    default:
	 BufShowInfo3("Unknown Tag: %x Value: %x",tag->TagType,tag->Value);
  }

  return TRUE;
}

void InfoTIFF(void)
{
  int nTags;
  int i;
  struct TiffTag tag;
  int NextIFDPos;

  StartupRead();

  do
  {
    nTags = TGetWord();
    for (i=0;i<nTags;i++)
    {
      ReadTag(&tag);
      if (!(InfoTag(&tag))) return;
    }

    NextIFDPos = TGetLong();
    if (NextIFDPos) if (!SeekPosLoad(NextIFDPos,Absolute)) return;
  } while(NextIFDPos);
}


/*=============================================================================
  **********************************  PROPS  **********************************
  =============================================================================*/

static BOOL FindTag(UWORD tagtype,struct TiffTag *tag)
{
  int nTags;
  int i;
  int NextIFDPos;

  StartupRead();

  do
  {
    nTags = TGetWord();
    for (i=0;i<nTags;i++)
    {
      ReadTag(tag);
      if (tag->TagType == tagtype) return TRUE;
    }

    NextIFDPos = TGetLong();
    if (NextIFDPos) if (!SeekPosLoad(NextIFDPos,Absolute)) return FALSE;
  } while(NextIFDPos);

  return FALSE;
}

void PropsTIFF(void)
{
  struct TiffTag tag;
  int	 SamplesPerPixel;
  int	 BitsPerSample;

  if (!FindTag(TAG_ImageWidth,&tag))  { DoError(INPUT_FILE_ERROR); return ; }
  Output_Width=tag.Value;

  if (!FindTag(TAG_ImageLength,&tag)) { DoError(INPUT_FILE_ERROR); return ; }
  Output_Height=tag.Value;

  if (!FindTag(TAG_SamplesPerPixel,&tag)) SamplesPerPixel=1;
  else					  SamplesPerPixel=tag.Value;

  if (!FindTag(TAG_BitsPerSample,&tag)) BitsPerSample=1;
  else					BitsPerSample=tag.Value;

  if (SamplesPerPixel==1)
    Output_nColors=1<<BitsPerSample;
  else
  {
    Output_nColors=256; /* TrueColor (24bit) */
    Output_Mode = GFXMOD_24BIT;
  }
}

/*=============================================================================
  ******************************  LOAD - SUPPORT ******************************
  =============================================================================*/

/**********************------ local block begin ------**********************/

static int nStripesLeft;
static int CurrentStripe;
static int NextFilePosStripe;
static BOOL only1Pos;
static char PosAdvance;

static BOOL NextStripe(int *stripepos)
{
  int oldpos;

  if (nStripesLeft==0) return FALSE;

  oldpos=GetLoadPos();

  if (!only1Pos)
  {
    SeekPosLoad(NextFilePosStripe,Absolute);
    NextFilePosStripe +=  PosAdvance;
    CurrentStripe      = (PosAdvance==4 ? TGetLong() : TGetWord());
  }

  nStripesLeft--;

  SeekPosLoad(oldpos,Absolute);

  *stripepos   =CurrentStripe;

  return TRUE;
}

static BOOL InitStripe(void)
{
  struct TiffTag tag;

  if (!FindTag(TAG_StripOffset,&tag)) { SetError(INPUT_FILE_ERROR); return FALSE; }
  if (tag.DataLength==1)
  {
    nStripesLeft=1;
    only1Pos=TRUE;
    CurrentStripe=tag.Value;
  }
  else
  {
    only1Pos=FALSE;
    nStripesLeft     =tag.DataLength;
    NextFilePosStripe=tag.Value;
    PosAdvance = (tag.DataType==3) ? 2 : 4;
  }

  return TRUE;
}

/**********************------- local block end -------**********************/

static void LoadColorMap(struct ColorLookupTable *clut,int filepos,int nColors,BOOL invers)
{
  int oldpos;
  int i;
  UBYTE r[256],g[256],b[256];

  oldpos=GetLoadPos();
  SeekPosLoad(filepos,Absolute);

  for (i=0;i<nColors;i++)   r[i] = TGetWord() >> 8;
  for (i=0;i<nColors;i++)   g[i] = TGetWord() >> 8;
  for (i=0;i<nColors;i++) { b[i] = TGetWord() >> 8;
			    SetColor(clut,i,r[i],g[i],b[i]);
			  }

  SeekPosLoad(oldpos,Absolute);
}

#define CCITT3_EOF 65000

struct {
  USHORT  Length;
  UBYTE   IsBlack;
  UBYTE   IsMakeUp;
  UBYTE   Pattern;
  UBYTE   PatternLength;
} Codes[] =

{
     0,0,0,0x35, 8,	    0,1,0,0x37,10,
     1,0,0,0x07, 6,	    1,1,0,0x02, 3,
     2,0,0,0x07, 4,	    2,1,0,0x03, 2,
     3,0,0,0x08, 4,	    3,1,0,0x02, 2,
     4,0,0,0x0b, 4,	    4,1,0,0x03, 3,
     5,0,0,0x0c, 4,	    5,1,0,0x03, 4,
     6,0,0,0x0e, 4,	    6,1,0,0x02, 4,
     7,0,0,0x0f, 4,	    7,1,0,0x03, 5,
     8,0,0,0x13, 5,	    8,1,0,0x05, 6,
     9,0,0,0x14, 5,	    9,1,0,0x04, 6,
    10,0,0,0x07, 5,	   10,1,0,0x04, 7,
    11,0,0,0x08, 5,	   11,1,0,0x05, 7,
    12,0,0,0x08, 6,	   12,1,0,0x07, 7,
    13,0,0,0x03, 6,	   13,1,0,0x04, 8,
    14,0,0,0x34, 6,	   14,1,0,0x07, 8,
    15,0,0,0x35, 6,	   15,1,0,0x18, 9,
    16,0,0,0x2a, 6,	   16,1,0,0x17,10,
    17,0,0,0x2b, 6,	   17,1,0,0x18,10,
    18,0,0,0x27, 7,	   18,1,0,0x08,10,
    19,0,0,0x0c, 7,	   19,1,0,0x67,11,
    20,0,0,0x08, 7,	   20,1,0,0x68,11,
    21,0,0,0x17, 7,	   21,1,0,0x6c,11,
    22,0,0,0x03, 7,	   22,1,0,0x37,11,
    23,0,0,0x04, 7,	   23,1,0,0x28,11,
    24,0,0,0x28, 7,	   24,1,0,0x17,11,
    25,0,0,0x2b, 7,	   25,1,0,0x18,11,
    26,0,0,0x13, 7,	   26,1,0,0xca,12,
    27,0,0,0x24, 7,	   27,1,0,0xcb,12,
    28,0,0,0x18, 7,	   28,1,0,0xcc,12,
    29,0,0,0x02, 8,	   29,1,0,0xcd,12,
    30,0,0,0x03, 8,	   30,1,0,0x68,12,
    31,0,0,0x1a, 8,	   31,1,0,0x69,12,
    32,0,0,0x1b, 8,	   32,1,0,0x6a,12,
    33,0,0,0x12, 8,	   33,1,0,0x6b,12,
    34,0,0,0x13, 8,	   34,1,0,0xd2,12,
    35,0,0,0x14, 8,	   35,1,0,0xd3,12,
    36,0,0,0x15, 8,	   36,1,0,0xd4,12,
    37,0,0,0x16, 8,	   37,1,0,0xd5,12,
    38,0,0,0x17, 8,	   38,1,0,0xd6,12,
    39,0,0,0x28, 8,	   39,1,0,0xd7,12,
    40,0,0,0x29, 8,	   40,1,0,0x6c,12,
    41,0,0,0x2a, 8,	   41,1,0,0x6d,12,
    42,0,0,0x2b, 8,	   42,1,0,0xda,12,
    43,0,0,0x2c, 8,	   43,1,0,0xdb,12,
    44,0,0,0x2d, 8,	   44,1,0,0x54,12,
    45,0,0,0x04, 8,	   45,1,0,0x55,12,
    46,0,0,0x05, 8,	   46,1,0,0x56,12,
    47,0,0,0x0a, 8,	   47,1,0,0x57,12,
    48,0,0,0x0b, 8,	   48,1,0,0x64,12,
    49,0,0,0x52, 8,	   49,1,0,0x65,12,
    50,0,0,0x53, 8,	   50,1,0,0x52,12,
    51,0,0,0x54, 8,	   51,1,0,0x53,12,
    52,0,0,0x55, 8,	   52,1,0,0x24,12,
    53,0,0,0x24, 8,	   53,1,0,0x37,12,
    54,0,0,0x25, 8,	   54,1,0,0x38,12,
    55,0,0,0x58, 8,	   55,1,0,0x27,12,
    56,0,0,0x59, 8,	   56,1,0,0x28,12,
    57,0,0,0x5a, 8,	   57,1,0,0x58,12,
    58,0,0,0x5b, 8,	   58,1,0,0x59,12,
    59,0,0,0x4a, 8,	   59,1,0,0x2b,12,
    60,0,0,0x4b, 8,	   60,1,0,0x2c,12,
    61,0,0,0x32, 8,	   61,1,0,0x5a,12,
    62,0,0,0x33, 8,	   62,1,0,0x66,12,
    63,0,0,0x34, 8,	   63,1,0,0x67,12,

    64,0,1,0x1b, 5,	   64,1,1,0x0f,10,
   128,0,1,0x12, 5,	  128,1,1,0xc8,12,
   192,0,1,0x17, 6,	  192,1,1,0xc9,12,
   256,0,1,0x37, 7,	  256,1,1,0x5b,12,
   320,0,1,0x36, 8,	  320,1,1,0x33,12,
   384,0,1,0x37, 8,	  384,1,1,0x34,12,
   448,0,1,0x64, 8,	  448,1,1,0x35,12,
   512,0,1,0x65, 8,	  512,1,1,0x6c,13,
   576,0,1,0x68, 8,	  576,1,1,0x6d,13,
   640,0,1,0x67, 8,	  640,1,1,0x4a,13,
   704,0,1,0xcc, 9,	  704,1,1,0x4b,13,
   768,0,1,0xcd, 9,	  768,1,1,0x4c,13,
   832,0,1,0xd2, 9,	  832,1,1,0x4d,13,
   896,0,1,0xd3, 9,	  896,1,1,0x72,13,
   960,0,1,0xd4, 9,	  960,1,1,0x73,13,
  1024,0,1,0xd5, 9,	 1024,1,1,0x74,13,
  1088,0,1,0xd6, 9,	 1088,1,1,0x75,13,
  1152,0,1,0xd7, 9,	 1152,1,1,0x76,13,
  1216,0,1,0xd8, 9,	 1216,1,1,0x77,13,
  1280,0,1,0xd9, 9,	 1280,1,1,0x52,13,
  1344,0,1,0xda, 9,	 1344,1,1,0x53,13,
  1408,0,1,0xdb, 9,	 1408,1,1,0x54,13,
  1472,0,1,0x98, 9,	 1472,1,1,0x55,13,
  1536,0,1,0x99, 9,	 1536,1,1,0x5a,13,
  1600,0,1,0x9a, 9,	 1600,1,1,0x5b,13,
  1664,0,1,0x18, 6,	 1664,1,1,0x64,13,
  1728,0,1,0x9b, 9,	 1728,1,1,0x65,13,
  CCITT3_EOF,0,1,0x01,12,	 CCITT3_EOF,1,1,0x01,12,

  1792,0,1,0x08,11,	 1792,1,1,0x08,11,
  1856,0,1,0x0c,11,	 1856,1,1,0x0c,11,
  1920,0,1,0x0d,11,	 1920,1,1,0x0d,11,
  1984,0,1,0x12,12,	 1984,1,1,0x12,12,
  2048,0,1,0x13,12,	 2048,1,1,0x13,12,
  2112,0,1,0x14,12,	 2112,1,1,0x14,12,
  2176,0,1,0x15,12,	 2176,1,1,0x15,12,
  2240,0,1,0x16,12,	 2240,1,1,0x16,12,
  2304,0,1,0x17,12,	 2304,1,1,0x17,12,
  2368,0,1,0x1c,12,	 2368,1,1,0x1c,12,
  2432,0,1,0x1d,12,	 2432,1,1,0x1d,12,
  2496,0,1,0x1e,12,	 2496,1,1,0x1e,12,
  2560,0,1,0x1f,12,	 2560,1,1,0x1f,12
};



/* ---- the table above has been computed using this original table and the
   ---- function below !

{
     0,"00110101-----",0,0,0,0,     0,"0000110111---",1,0,0,0,
     1,"000111-------",0,0,0,0,     1,"010----------",1,0,0,0,
     2,"0111---------",0,0,0,0,     2,"11-----------",1,0,0,0,
     3,"1000---------",0,0,0,0,     3,"10-----------",1,0,0,0,
     4,"1011---------",0,0,0,0,     4,"011----------",1,0,0,0,
     5,"1100---------",0,0,0,0,     5,"0011---------",1,0,0,0,
     6,"1110---------",0,0,0,0,     6,"0010---------",1,0,0,0,
     7,"1111---------",0,0,0,0,     7,"00011--------",1,0,0,0,
     8,"10011--------",0,0,0,0,     8,"000101-------",1,0,0,0,
     9,"10100--------",0,0,0,0,     9,"000100-------",1,0,0,0,
    10,"00111--------",0,0,0,0,    10,"0000100------",1,0,0,0,
    11,"01000--------",0,0,0,0,    11,"0000101------",1,0,0,0,
    12,"001000-------",0,0,0,0,    12,"0000111------",1,0,0,0,
    13,"000011-------",0,0,0,0,    13,"00000100-----",1,0,0,0,
    14,"110100-------",0,0,0,0,    14,"00000111-----",1,0,0,0,
    15,"110101-------",0,0,0,0,    15,"000011000----",1,0,0,0,
    16,"101010-------",0,0,0,0,    16,"0000010111---",1,0,0,0,
    17,"101011-------",0,0,0,0,    17,"0000011000---",1,0,0,0,
    18,"0100111------",0,0,0,0,    18,"0000001000---",1,0,0,0,
    19,"0001100------",0,0,0,0,    19,"00001100111--",1,0,0,0,
    20,"0001000------",0,0,0,0,    20,"00001101000--",1,0,0,0,
    21,"0010111------",0,0,0,0,    21,"00001101100--",1,0,0,0,
    22,"0000011------",0,0,0,0,    22,"00000110111--",1,0,0,0,
    23,"0000100------",0,0,0,0,    23,"00000101000--",1,0,0,0,
    24,"0101000------",0,0,0,0,    24,"00000010111--",1,0,0,0,
    25,"0101011------",0,0,0,0,    25,"00000011000--",1,0,0,0,
    26,"0010011------",0,0,0,0,    26,"000011001010-",1,0,0,0,
    27,"0100100------",0,0,0,0,    27,"000011001011-",1,0,0,0,
    28,"0011000------",0,0,0,0,    28,"000011001100-",1,0,0,0,
    29,"00000010-----",0,0,0,0,    29,"000011001101-",1,0,0,0,
    30,"00000011-----",0,0,0,0,    30,"000001101000-",1,0,0,0,
    31,"00011010-----",0,0,0,0,    31,"000001101001-",1,0,0,0,
    32,"00011011-----",0,0,0,0,    32,"000001101010-",1,0,0,0,
    33,"00010010-----",0,0,0,0,    33,"000001101011-",1,0,0,0,
    34,"00010011-----",0,0,0,0,    34,"000011010010-",1,0,0,0,
    35,"00010100-----",0,0,0,0,    35,"000011010011-",1,0,0,0,
    36,"00010101-----",0,0,0,0,    36,"000011010100-",1,0,0,0,
    37,"00010110-----",0,0,0,0,    37,"000011010101-",1,0,0,0,
    38,"00010111-----",0,0,0,0,    38,"000011010110-",1,0,0,0,
    39,"00101000-----",0,0,0,0,    39,"000011010111-",1,0,0,0,
    40,"00101001-----",0,0,0,0,    40,"000001101100-",1,0,0,0,
    41,"00101010-----",0,0,0,0,    41,"000001101101-",1,0,0,0,
    42,"00101011-----",0,0,0,0,    42,"000011011010-",1,0,0,0,
    43,"00101100-----",0,0,0,0,    43,"000011011011-",1,0,0,0,
    44,"00101101-----",0,0,0,0,    44,"000001010100-",1,0,0,0,
    45,"00000100-----",0,0,0,0,    45,"000001010101-",1,0,0,0,
    46,"00000101-----",0,0,0,0,    46,"000001010110-",1,0,0,0,
    47,"00001010-----",0,0,0,0,    47,"000001010111-",1,0,0,0,
    48,"00001011-----",0,0,0,0,    48,"000001100100-",1,0,0,0,
    49,"01010010-----",0,0,0,0,    49,"000001100101-",1,0,0,0,
    50,"01010011-----",0,0,0,0,    50,"000001010010-",1,0,0,0,
    51,"01010100-----",0,0,0,0,    51,"000001010011-",1,0,0,0,
    52,"01010101-----",0,0,0,0,    52,"000000100100-",1,0,0,0,
    53,"00100100-----",0,0,0,0,    53,"000000110111-",1,0,0,0,
    54,"00100101-----",0,0,0,0,    54,"000000111000-",1,0,0,0,
    55,"01011000-----",0,0,0,0,    55,"000000100111-",1,0,0,0,
    56,"01011001-----",0,0,0,0,    56,"000000101000-",1,0,0,0,
    57,"01011010-----",0,0,0,0,    57,"000001011000-",1,0,0,0,
    58,"01011011-----",0,0,0,0,    58,"000001011001-",1,0,0,0,
    59,"01001010-----",0,0,0,0,    59,"000000101011-",1,0,0,0,
    60,"01001011-----",0,0,0,0,    60,"000000101100-",1,0,0,0,
    61,"00110010-----",0,0,0,0,    61,"000001011010-",1,0,0,0,
    62,"00110011-----",0,0,0,0,    62,"000001100110-",1,0,0,0,
    63,"00110100-----",0,0,0,0,    63,"000001100111-",1,0,0,0,

    64,"11011--------",0,1,0,0,    64,"0000001111---",1,1,0,0,
   128,"10010--------",0,1,0,0,   128,"000011001000-",1,1,0,0,
   192,"010111-------",0,1,0,0,   192,"000011001001-",1,1,0,0,
   256,"0110111------",0,1,0,0,   256,"000001011011-",1,1,0,0,
   320,"00110110-----",0,1,0,0,   320,"000000110011-",1,1,0,0,
   384,"00110111-----",0,1,0,0,   384,"000000110100-",1,1,0,0,
   448,"01100100-----",0,1,0,0,   448,"000000110101-",1,1,0,0,
   512,"01100101-----",0,1,0,0,   512,"0000001101100",1,1,0,0,
   576,"01101000-----",0,1,0,0,   576,"0000001101101",1,1,0,0,
   640,"01100111-----",0,1,0,0,   640,"0000001001010",1,1,0,0,
   704,"011001100----",0,1,0,0,   704,"0000001001011",1,1,0,0,
   768,"011001101----",0,1,0,0,   768,"0000001001100",1,1,0,0,
   832,"011010010----",0,1,0,0,   832,"0000001001101",1,1,0,0,
   896,"011010011----",0,1,0,0,   896,"0000001110010",1,1,0,0,
   960,"011010100----",0,1,0,0,   960,"0000001110011",1,1,0,0,
  1024,"011010101----",0,1,0,0,  1024,"0000001110100",1,1,0,0,
  1088,"011010110----",0,1,0,0,  1088,"0000001110101",1,1,0,0,
  1152,"011010111----",0,1,0,0,  1152,"0000001110110",1,1,0,0,
  1216,"011011000----",0,1,0,0,  1216,"0000001110111",1,1,0,0,
  1280,"011011001----",0,1,0,0,  1280,"0000001010010",1,1,0,0,
  1344,"011011010----",0,1,0,0,  1344,"0000001010011",1,1,0,0,
  1408,"011011011----",0,1,0,0,  1408,"0000001010100",1,1,0,0,
  1472,"010011000----",0,1,0,0,  1472,"0000001010101",1,1,0,0,
  1536,"010011001----",0,1,0,0,  1536,"0000001011010",1,1,0,0,
  1600,"010011010----",0,1,0,0,  1600,"0000001011011",1,1,0,0,
  1664,"011000-------",0,1,0,0,  1664,"0000001100100",1,1,0,0,
  1728,"010011011----",0,1,0,0,  1728,"0000001100101",1,1,0,0,
    ~0,"000000000001-",0,1,0,0,    ~0,"000000000001-",1,1,0,0,

  1792,"00000001000--",0,1,0,0,  1792,"00000001000--",1,1,0,0,
  1856,"00000001100--",0,1,0,0,  1856,"00000001100--",1,1,0,0,
  1920,"00000001101--",0,1,0,0,  1920,"00000001101--",1,1,0,0,
  1984,"000000010010-",0,1,0,0,  1984,"000000010010-",1,1,0,0,
  2048,"000000010011-",0,1,0,0,  2048,"000000010011-",1,1,0,0,
  2112,"000000010100-",0,1,0,0,  2112,"000000010100-",1,1,0,0,
  2176,"000000010101-",0,1,0,0,  2176,"000000010101-",1,1,0,0,
  2240,"000000010110-",0,1,0,0,  2240,"000000010110-",1,1,0,0,
  2304,"000000010111-",0,1,0,0,  2304,"000000010111-",1,1,0,0,
  2368,"000000011100-",0,1,0,0,  2368,"000000011100-",1,1,0,0,
  2432,"000000011101-",0,1,0,0,  2432,"000000011101-",1,1,0,0,
  2496,"000000011110-",0,1,0,0,  2496,"000000011110-",1,1,0,0,
  2560,"000000011111-",0,1,0,0,  2560,"000000011111-",1,1,0,0
};

*/

/*

static void InitCCITTCodes(void)
{
  int i;
  int bit;
  UWORD pattern;

  for (i=0;i<sizeof(Codes)/sizeof(Codes[0]);i++)
  {
    pattern=0;

    for (bit=0;bit<13;bit++)
    {
      if (Codes[i].Bits[bit]=='-') break;

      pattern *= 2;
      if (Codes[i].Bits[bit]=='1') pattern += 1;
    }

    Codes[i].Pattern	   = pattern;
    Codes[i].PatternLength = bit;

    printf("  %4d,%d,%d,0x%02x,%2d,\n",Codes[i].Length,Codes[i].IsBlack,
				       Codes[i].IsMakeUp,
				       Codes[i].Pattern,
				       Codes[i].PatternLength);
  }
}
*/

static UBYTE WorkByte;
static	BYTE WorkBitNr;

static UBYTE bitmasks[8] = { 0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80 };

void InitBitRead(void)
{
  WorkBitNr=-1;
}

int ReadBit(void)
{
  int result;

  if (WorkBitNr == -1)
  {
    WorkByte =GetByte(0);
    WorkBitNr=7;
  }

  result = (WorkByte & bitmasks[WorkBitNr]) ? 1 : 0;

  WorkBitNr--;

  return result;
}

static BOOL FoundMatch(UWORD pattern,int patternlength,BOOL IsBlack , int *length,BOOL *SwitchColor,int x,int y)
{
  int i;

  for (i=IsBlack ? 1 : 0;i<sizeof(Codes)/sizeof(Codes[0]);i+=2)
  {
    if (Codes[i].Pattern==pattern &&
	Codes[i].PatternLength == patternlength &&
	((Codes[i].IsBlack && IsBlack) || (!Codes[i].IsBlack && !IsBlack)) )
    {
      *length	   =  Codes[i].Length;
      *SwitchColor = !Codes[i].IsMakeUp;
      return TRUE;
    }
  }

  return FALSE;
}


/*=============================================================================
  ***********************************  LOAD  **********************************
  =============================================================================*/

int    Compression;
int    width,height;
int    nStripes;
int    RowsPerStripe;
int    PlanarConfiguration;
int    SamplesPerPixel;
int    BitsPerSample;
BOOL   invers;
int    predictor;

BOOL LoadTIFFInputSpecs(void)
{
  struct TiffTag tag;

  /* read props */

  if (!FindTag(TAG_Compression,&tag)) { SetError(INPUT_FILE_ERROR); return FALSE; }
  Compression=tag.Value;

  if (!FindTag(TAG_ImageWidth,&tag))  { SetError(INPUT_FILE_ERROR); return FALSE; }
  width=tag.Value;

  if (!FindTag(TAG_ImageLength,&tag)) { SetError(INPUT_FILE_ERROR); return FALSE; }
  height=tag.Value;

  if (!FindTag(TAG_StripOffset,&tag)) { SetError(INPUT_FILE_ERROR); return FALSE; }
  nStripes=tag.DataLength;

  if (!FindTag(TAG_RowsPerStrip,&tag)) RowsPerStripe=height;
  else				       RowsPerStripe=tag.Value;

  if (!FindTag(TAG_PlanarConfiguration,&tag)) PlanarConfiguration=1;
  else					      PlanarConfiguration=tag.Value;

  if (!InitStripe()) { SetError(INPUT_FILE_ERROR); return FALSE; }

  if (!FindTag(TAG_SamplesPerPixel,&tag)) SamplesPerPixel=1;
  else					  SamplesPerPixel=tag.Value;

  if (!FindTag(TAG_BitsPerSample,&tag))   BitsPerSample=1;
  else					  BitsPerSample=tag.Value;

  if (FindTag(TAG_PhotometricInterpretation,&tag) && tag.Value==0) invers=TRUE;
  else								   invers=FALSE;

  if (FindTag(TAG_Predictor,&tag)) predictor=tag.Value;
  else				   predictor=1;
}


static struct ImageData *image=NULL;
static struct ColorLookupTable *clut;
static char  *linebuffer[24];
static BOOL   HaveLineBuffer=FALSE;
static int    LineBufferPlanes=0;



BOOL LoadUncompressed(void)
{
  int	 x,y;
  int	 i;

  int	 stripe;
  int	 StripeDataPos;
  struct TiffTag tag;

  if (SamplesPerPixel==1 && BitsPerSample==1)     /*----------- BW -----------*/
  {
    int  SavedWidth;
    int  BytesWidth;


    ShowMessage(Txt(TXT_LOADING_TIFF_BW));


    /* get image-buffer */

    SavedWidth=(width+7)&~7;
    BytesWidth=SavedWidth/8;

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,CLUT);

    if (!(clut=GetCLUT(2))) return FALSE;
    AttachCLUT(clut,image);


    /* set colors */

    SetColor(clut,invers ? 1 : 0,  0,  0,  0);
    SetColor(clut,invers ? 0 : 1,255,255,255);


    /* get line-buffer */

    LineBufferPlanes=1;
    if (!(HaveLineBuffer=GetLineBuffer( SavedWidth , LineBufferPlanes , linebuffer )) )
      return FALSE;

    y=0;

    for (stripe=0;stripe<nStripes;stripe++)
    {
      if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

      SeekPosLoad(StripeDataPos,Absolute);

      for (i=0;i<RowsPerStripe;i++)
      {
	if (!ReadBlock(&linebuffer[0][0],BytesWidth)) { return FALSE; }
	if (!SetCLUTRow(image,y,linebuffer,1))        { assert(0); }

	ShowProgress(y,height-1);
	y++;
	if (y==height)
	  break;
      }
    }

    return TRUE;
  }




  if (BitsPerSample == 8 && SamplesPerPixel==1) /*----------- 256 colors ------*/
  {
    ShowMessage(Txt(TXT_LOADING_TIFF_256));


    /* get image-buffer */

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,CLUT);

    if (!(clut=GetCLUT(256))) return FALSE;
    AttachCLUT(clut,image);


    /* load colors */

    if (FindTag(TAG_ColorMap,&tag))    /* picture with color map  */
      LoadColorMap(clut,tag.Value,256,invers);
    else			       /* picture with grey-scale */
    {
      x=invers ? 255 : 0;
      for (i=0;i<256;i++,x+=invers ? -1 : 1)
      {
	SetColor(clut,x,i,i,i);
      }
    }

    if (PlanarConfiguration == 1)  /*--------------- no bitplanes -------------*/
    {
      UBYTE color;
      y=0;

      for (stripe=0;stripe<nStripes;stripe++)
      {
	if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

	SeekPosLoad(StripeDataPos,Absolute);

	for (i=0;i<RowsPerStripe;i++)
	{
	  for (x=0;x<width;x++)
	  {
	    color=GetByte(0);
	    if (!SetCLUTPixel(image,x,y,color)) { assert(0); }
	  }
	  ShowProgress(y,height-1);
	  y++;
	if (y==height)
	  break;
	}
      }

      return TRUE;
    }
    else   /*-------------------------------------------- bitplanes -----------*/
    {
      /* not supported yet */
    }
  }



  if (BitsPerSample == 4 && SamplesPerPixel == 1) /*----------- 16 colors -----*/
  {
    ShowMessage(Txt(TXT_LOADING_TIFF_16));


    /* get image-buffer */

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,CLUT);

    if (!(clut=GetCLUT(16))) return FALSE;
    AttachCLUT(clut,image);


    /* load colors */

    if (FindTag(TAG_ColorMap,&tag))    /* picture with color map  */
      LoadColorMap(clut,tag.Value,16,invers);
    else			       /* picture with grey-scale */
    {
      x=invers ? 255 : 0;
      for (i=0;i<16;i++,x+=invers ? -1 : 1)
      {
	SetColor(clut,x,i,i,i);
      }
    }

    if (PlanarConfiguration == 1) /*---- without bitplanes -----*/
    {
      UBYTE color;
      y=0;

      for (stripe=0;stripe<nStripes;stripe++)
      {
	if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

	SeekPosLoad(StripeDataPos,Absolute);

	for (i=0;i<RowsPerStripe;i++)
	{
	  for (x=0;x<width;)
	  {
	    color=GetByte(0);
	    if (!SetCLUTPixel(image,x,y,(color >> 4  )   )) { assert(0); }
	    x++;

	    if (x==width) break;

	    if (!SetCLUTPixel(image,x,y,(color & 0x0F)   )) { assert(0); }
	    x++;
	  }
	  ShowProgress(y,height-1);
	  y++;
	if (y==height)
	  break;
	}
      }

      return FALSE;
    }
    else			    /*---- with bitplanes -----*/
    {
      ;  /* not supported yet */
    }
  }

  if (SamplesPerPixel==3)
  {
    ShowMessage(Txt(TXT_LOADING_TIFF_RGB));

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,RGB);

    if (PlanarConfiguration == 1)
    {
      UBYTE r,g,b;

      for (stripe=0;stripe<nStripes;stripe++)
      {
	if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

	SeekPosLoad(StripeDataPos,Absolute);

	for (i=0;i<RowsPerStripe;i++)
	{
	  for (x=0;x<width;x++)
	  {
	    r=GetByte(0);
	    g=GetByte(0);
	    b=GetByte(0);

	    if (!SetRGBPixel(image,x,y,r,g,b)) { assert(0); }
	  }
	  ShowProgress(y,height-1);
	  y++;
	if (y==height)
	  break;
	}
      }

      return TRUE;
    }
    else
    {
      ;
    }
  }

  SetError(FORMAT_NOT_SUPPORTED);
  return FALSE;
}



BOOL LoadCCITT3(void)
{
    UWORD pattern;
    int   patternlength;
    BOOL  StatusIsBlack;
    BOOL  SwitchColors=0;  /* calm down Lattice (this is not really needed) */
    int   length;

  int	 x,y;
  int	 i;

  int	 stripe;
  int	 StripeDataPos;
//  struct TiffTag tag;


    ShowMessage(Txt(TXT_LOADING_TIFF_BW_CCITT3));


    /* get image-buffer */

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,CLUT);

    if (!(clut=GetCLUT(2))) return FALSE;
    AttachCLUT(clut,image);


    /* set colors */

    SetColor(clut,invers ? 1 : 0,  0,  0,  0);
    SetColor(clut,invers ? 0 : 1,255,255,255);


    /* load image */

//    SeekPosLoad(bitmappos,Absolute);

    y=0;
    for (stripe=0;stripe<nStripes;stripe++)
    {
      if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

      SeekPosLoad(StripeDataPos,Absolute);

      for (i=0;i<RowsPerStripe;i++,y++)
      {
	if (y==height)
	  break;
	InitBitRead();
	StatusIsBlack=FALSE;

	for (x=0;x<width || !SwitchColors; )
	{
	  pattern      =0;
	  patternlength=0;

	  do {
	    pattern *= 2;
	    pattern += ReadBit();
	    patternlength++;
	  } while(!FoundMatch(pattern,patternlength,StatusIsBlack , &length,&SwitchColors,x,y));

	  if (x+length > width)
	  {
	    SetError(INPUT_FILE_ERROR);
	    return FALSE;
	  }

	  while (length)
	  {
	    if (!SetCLUTPixel(image,x,y,StatusIsBlack ? 1 : 0)) { assert(0); }
	    length--;
	    x++;
	  }

	  if (SwitchColors) StatusIsBlack = !StatusIsBlack;
	}
	ShowProgress(y,height-1);
      }
    }

    return TRUE;
}


BOOL LoadRUN32773(void)
{
  int	 x,y;
  int	 i;

  int	 stripe;
  int	 StripeDataPos;
  struct TiffTag tag;


  if (SamplesPerPixel == 1 && BitsPerSample == 1) /*------- black / white -----*/
  {
    BYTE  count;
    UBYTE pattern;
    int   SavedWidth;
    int   BytesPerRow;

    ShowMessage(Txt(TXT_LOADING_TIFF_BW_PACKBIT));


    /* get image-buffer */

    SavedWidth =(width+7)&~7;
    BytesPerRow=SavedWidth/8;

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,CLUT);

    if (!(clut=GetCLUT(2))) return FALSE;
    AttachCLUT(clut,image);


    /* set colors */

    SetColor(clut,invers ? 1 : 0,  0,  0,  0);
    SetColor(clut,invers ? 0 : 1,255,255,255);


    /* get line-buffer */

    if (!(HaveLineBuffer=GetLineBuffer( SavedWidth , LineBufferPlanes=1 , linebuffer )) )
      return FALSE;


    /* load image */

    y=0;

    for (stripe=0;stripe<nStripes;stripe++)
    {
      if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

      SeekPosLoad(StripeDataPos,Absolute);

      for (i=0;i<RowsPerStripe;i++)
      {
	for (x=0;x<BytesPerRow; )
	{
	  count=GetByte(0);
	  if (count>=0 && count <= 0x7F)   /*=== copy run ===*/
	  {
	    count++;

	    if (x+count > BytesPerRow) { SetError(INPUT_FILE_ERROR); return FALSE; }

	    if (!ReadBlock(&linebuffer[0][x],count)) return FALSE;
	    x+=count;
	  }
	  else if (count != 0x80)          /*=== pattern run ===*/
	  {
	    count    = -count+1;
	    pattern  =	GetByte(0);

	    if (x+count > BytesPerRow) { SetError(INPUT_FILE_ERROR); return FALSE; }

	    while (count)
	    {
	      linebuffer[0][x++]=pattern;
	      count--;
	    }
	  }
	}

	if (!SetCLUTRow(image,y,linebuffer,1))     { assert(0); }

	ShowProgress(y,height-1);
	y++;
	if (y==height)
	  break;
      }
    }

    return TRUE;
  }


  if (SamplesPerPixel == 1 && BitsPerSample == 8) /*------- 256 colors -----*/
  {
    BYTE  count;
    UBYTE pattern;

    ShowMessage(Txt(TXT_LOADING_TIFF_256_PACKBIT));


    /* get image-buffer */

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,CLUT);

    if (!(clut=GetCLUT(256))) return FALSE;
    AttachCLUT(clut,image);


    /* load colors */

    if (FindTag(TAG_ColorMap,&tag))    /* picture with color map  */
      LoadColorMap(clut,tag.Value,256,invers);
    else			       /* picture with grey-scale */
    {
      x=invers ? 255 : 0;
      for (i=0;i<256;i++,x+=invers ? -1 : 1)
      {
	SetColor(clut,x,i,i,i);
      }
    }



    /* get line-buffer */
/*
    if (!(HaveLineBuffer=GetLineBuffer( width , LineBufferPlanes=1 , linebuffer )) )
      return FALSE;
*/

    /* load image */

    y=0;

    for (stripe=0;stripe<nStripes;stripe++)
    {
      if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

      SeekPosLoad(StripeDataPos,Absolute);

      for (i=0;i<RowsPerStripe;i++)
      {
	for (x=0;x<width; )
	{
	  count=GetByte(0);
	  if (count>=0 && count <= 0x7F)   /*=== copy run ===*/
	  {
	    count++;

	    if (x+count > width) { SetError(INPUT_FILE_ERROR); return FALSE; }

	    while (count)
	    {
	      UBYTE c = GetByte(0);
	      if (!SetCLUTPixel(image,x,y,c)) return FALSE;
	      count--;
	      x++;
	    }
	  }
	  else if (count != 0x80)          /*=== pattern run ===*/
	  {
	    count    = -count+1;
	    pattern  =	GetByte(0);

	    if (x+count > width) { SetError(INPUT_FILE_ERROR); return FALSE; }

	    while (count)
	    {
	      if (!SetCLUTPixel(image,x,y,pattern)) return FALSE;
	      x++;
	      count--;
	    }
	  }
	}

	ShowProgress(y,height-1);
	y++;
	if (y==height)
	  break;
      }
    }

    return TRUE;
  }


  SetError(FORMAT_NOT_SUPPORTED);
  return FALSE;
}


/*
FAX3:
FAX4:
JPEG:
  SetError(FORMAT_NOT_SUPPORTED);
  goto errexit;
*/


BOOL LoadLZW_TIFF(void)
{
  int	 x,y;
  int	 i;

  int	 stripe;
  int	 StripeDataPos;
  struct TiffTag tag;



  if (SamplesPerPixel==3)
  {
    UBYTE* lzwbuffer = cu_calloc(RowsPerStripe,width*3);
    codetable = cu_calloc(sizeof(struct CodeTable),1<<12);

    ShowMessage(Txt(TXT_LOADING_TIFF_LZW_RGB));

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,RGB);

    if (PlanarConfiguration == 1)
    {
      UBYTE r,g,b;
      int n;

      y=0;

      for (stripe=0;stripe<nStripes;stripe++)
      {
	if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

	SeekPosLoad(StripeDataPos,Absolute);

	LoadLZW(lzwbuffer,RowsPerStripe*width*3);

	n=0;
	for (i=0;i<RowsPerStripe;i++)
	{
	  r=g=b=0;
	  for (x=0;x<width;x++)
	  {
	    if (predictor==1)
	    {
	      r=lzwbuffer[n++];
	      g=lzwbuffer[n++];
	      b=lzwbuffer[n++];
	    }
	    else if (predictor==2)
	    {
	      r+=(int)lzwbuffer[n++];
	      g+=(int)lzwbuffer[n++];
	      b+=(int)lzwbuffer[n++];
	    }
	    else
	    {
	      return FALSE;
	    }

	    if (!SetRGBPixel(image,x,y,r,g,b)) { assert(0); }
	  }
	  ShowProgress(y,height-1);
	  y++;

	  if (y==height)
	  {
	    cu_free(lzwbuffer);    lzwbuffer=NULL;
	    cu_free(codetable); codetable=NULL;
	    return TRUE;
	  }
	}
      }


      cu_free(lzwbuffer);  lzwbuffer=NULL;
      cu_free(codetable); codetable=NULL;
      SetError(INPUT_FILE_ERROR);
      return FALSE;
    }
    else
    {
      ;
    }
  }

  if (SamplesPerPixel==1 && BitsPerSample==8)
  {
    UBYTE* lzwbuffer = cu_calloc(RowsPerStripe,width);
    codetable = cu_calloc(sizeof(struct CodeTable),1<<12);

    ShowMessage(Txt(TXT_LOADING_TIFF_LZW_256));

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,CLUT);

    if (!(clut=GetCLUT(256))) return FALSE;
    AttachCLUT(clut,image);


    /* load colors */

    if (FindTag(TAG_ColorMap,&tag))    /* picture with color map  */
      LoadColorMap(clut,tag.Value,256,invers);
    else			       /* picture with grey-scale */
    {
      x=invers ? 255 : 0;
      for (i=0;i<256;i++,x+=invers ? -1 : 1)
      {
	SetColor(clut,x,i,i,i);
      }
    }

    if (PlanarConfiguration == 1)
    {
      int n;

      y=0;

      for (stripe=0;stripe<nStripes;stripe++)
      {
	if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

	SeekPosLoad(StripeDataPos,Absolute);

	LoadLZW(lzwbuffer,RowsPerStripe*width);

	n=0;
	for (i=0;i<RowsPerStripe;i++)
	{
	  for (x=0;x<width;x++)
	  {
	    if (!SetCLUTPixel(image,x,y,lzwbuffer[n++])) { assert(0); }
	  }

	  ShowProgress(y,height-1);
	  y++;

	  if (y==height)
	  {
	    cu_free(lzwbuffer);  lzwbuffer=NULL;
	    cu_free(codetable); codetable=NULL;
	    return TRUE;
	  }
	}
      }

      cu_free(lzwbuffer);  lzwbuffer=NULL;
      cu_free(codetable); codetable=NULL;
      SetError(INPUT_FILE_ERROR);
      return FALSE;
    }
    else
    {
      ;
    }
  }

  if (SamplesPerPixel==1 && BitsPerSample==1)
  {
    UBYTE* lzwbuffer;

    int SavedWidth =(width+7)&~7;
    int BytesPerRow=SavedWidth/8;

    lzwbuffer = cu_calloc(RowsPerStripe,BytesPerRow);
    codetable = cu_calloc(sizeof(struct CodeTable),1<<12);

    ShowMessage(Txt(TXT_LOADING_TIFF_LZW_MONO));

    if (!(image=GetBuffer(width,height))) return FALSE;

    SetBufferMode(image,CLUT);

    if (!(clut=GetCLUT(2))) return FALSE;
    AttachCLUT(clut,image);


    /* set colors */

    SetColor(clut,invers ? 1 : 0,  0,  0,  0);
    SetColor(clut,invers ? 0 : 1,255,255,255);


    if (PlanarConfiguration == 1)
    {
      int n;

      y=0;

      for (stripe=0;stripe<nStripes;stripe++)
      {
	if (!(NextStripe(&StripeDataPos))) { SetError(INPUT_FILE_ERROR); return FALSE; }

	SeekPosLoad(StripeDataPos,Absolute);

	LoadLZW(lzwbuffer,RowsPerStripe*BytesPerRow);

	n=0;
	for (i=0;i<RowsPerStripe;i++)
	{
	  for (x=0;x<width;x++)
	  {
	    if (lzwbuffer[x/8+BytesPerRow*i] & (0x80>>(x%8)))
	    {
	      if (!SetCLUTPixel(image,x,y,1)) { assert(0); }
	    }
	    else
	    {
	      if (!SetCLUTPixel(image,x,y,0)) { assert(0); }
	    }
	  }
	  ShowProgress(y,height-1);
	  y++;

	  if (y==height)
	  {
	    cu_free(lzwbuffer);  lzwbuffer=NULL;
	    cu_free(codetable); codetable=NULL;
	    return TRUE;
	  }
	}
      }

      cu_free(lzwbuffer);  lzwbuffer=NULL;
      cu_free(codetable); codetable=NULL;
      SetError(INPUT_FILE_ERROR);
      return FALSE;
    }
    else
    {
      ;
    }
  }

  SetError(FORMAT_NOT_SUPPORTED);
  return FALSE;
}


BOOL LoadTIFF(void)
{
  BOOL success;
  image=NULL;
  clut=NULL;
  HaveLineBuffer=FALSE;

  LoadTIFFInputSpecs();

  switch (Compression)
  {
    case     1: success=LoadUncompressed(); break;
    case     2: success=LoadCCITT3();       break;
    case     5: success=LoadLZW_TIFF();     break;
    case 32773: success=LoadRUN32773();     break;

    default:
      SetError(FORMAT_NOT_SUPPORTED);
      return FALSE;
//    case     3: goto FAX3;
//    case     4: goto FAX4;
//    case     6: goto JPEG;
  }

  if (!success && image)   FreeBuffer(image);
  if (!success && clut)    FreeCLUT(clut);
  if (HaveLineBuffer)      FreeLineBuffer(LineBufferPlanes,linebuffer);
  if (success)             SetDefaultBuffer(image);

  return success;
}



BOOL SaveTIFF(void)
{
  return TRUE;
}

