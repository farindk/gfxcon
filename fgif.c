
/********************************************************************************
 *
 * modul name:	fgif.c
 *
 * contents:	routines to load and check GIF files and
 *
 *
 * to do:
 *
 *
 * v1.3 (26.05.95)
 *   background-color support
 *
 * v1.2 (01.01.95)
 *   Save-module
 *
 * v1.1 (13.10.93)
 *   Major revision. Module entirely rewritten. Major bug fixes.
 *
 * v1.0 (08.10.93)
 *   added file-info and file-properties
 *
 * v0.9 (00.00.93)
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

extern short NewBackgroundColor;


void CheckForGIF(form fo)
{
  char signatur[4];

  signatur[0] = GetByte(0);
  signatur[1] = GetByte(0);
  signatur[2] = GetByte(0);
  signatur[3] = 0;

  if (strcmp(signatur,"GIF") != 0) { fo->not_valid_format=TRUE; return; }


  fo->not_valid_format=FALSE;
}

/*===================== Logical Screen Descriptor Block ====================*/

struct LogScrDescr
{
  UWORD width;
  UWORD height;

  UBYTE ResolutionFlag;
  UBYTE BackgroundColor;
  UBYTE AspectRatio;
} LSD;

static BOOL LoadLogScrDescrBlock(void)
{
  LSD.width  = GetWord(0,1);
  LSD.height = GetWord(0,1);
  LSD.ResolutionFlag  = GetByte(0);
  LSD.BackgroundColor = GetByte(0);
  LSD.AspectRatio     = GetByte(0);

  return TRUE;
}

/*------------------------------ resolution flag ------------------------*/

static BOOL GlobalMap(void)
{
  return (BOOL)(LSD.ResolutionFlag & 0x80);
}

static int ColorResolution(void)
{
  return ((LSD.ResolutionFlag & 0x70)>>4)+1;
}

static int BitsPerPixel(void)
{
  return (LSD.ResolutionFlag & 0x07)+1;
}

/*------------------------------ aspect ratio -----------------------------*/

static BOOL SortedGlobalMap(void)
{
  return (BOOL)(LSD.AspectRatio & 0x80);
}

static int AspectRatio(void)
{
  return LSD.AspectRatio & 0x7F;
}


/*========================== Global/Local Color Map ==========================*/

static void LoadColorMap(struct ColorLookupTable *clut)
{
  UBYTE r,g,b;
  int	i;

  for (i=0;i< (1<<BitsPerPixel()) ; i++)
  {
    r=GetByte(0);
    g=GetByte(0);
    b=GetByte(0);

    if (clut) SetColor(clut,i, r,g,b);
  }
}

/*=========================== Extension Block ================================*/

static BOOL	     transparentexists;
static unsigned char transparentcolor;

static BOOL LoadExtensionBlock(void)
{
  char FunctionCode;
  char Length;
  char c;


  FunctionCode = GetByte(0);

  while ((Length = GetByte(0)) != 0)
  {
    switch (FunctionCode)
    {
      case 0xF9:  // ---------------------- Graphic Control Block ----------

	if (Length != 4) { return FALSE; }

	c = GetByte(0);         // <Packed Fields>

	GetByte(0); GetByte(0); // eat "Time Delay"

	if (c & 1)
	{
	  transparentexists = TRUE;
	  transparentcolor  = (unsigned char)GetByte(0);
	}
	else
	{
	  c = GetByte(0);
	}

	break;

      default:
	while (Length) { GetByte(0); Length--; } /* eat data-block */
	break;
    }
  }

  return TRUE;
}


/*========================= Image Description Block ==========================*/

struct ImageDescr
{
  UWORD LeftPos;
  UWORD TopPos;
  UWORD Width;
  UWORD Height;
  UBYTE Flag;
} IDB;

static BOOL LoadImageDescriptionBlock(void)
{
  IDB.LeftPos = GetWord(0,1);
  IDB.TopPos  = GetWord(0,1);
  IDB.Width   = GetWord(0,1);
  IDB.Height  = GetWord(0,1);
  IDB.Flag    = GetByte(0);

  return TRUE;
}

/*--------------------------- Image Description Flags ------------------------*/

static BOOL LocalColorMap(void)
{
  return (BOOL)(IDB.Flag & 0x80);
}

static BOOL InterlacedImage(void)
{
  return (BOOL)(IDB.Flag & 0x40);
}

static BOOL Sorted(void)
{
  return (BOOL)(IDB.Flag & 0x20);
}

static int PixelSize(void)
{
  return (IDB.Flag & 0x07)+1;
}

/*================================ Load Image ================================*/

//BOOL DecoderLine(struct ImageData *,int w,int h,BOOL interlaced);

BOOL ReadRaster    (struct ImageData *,int  ,int );
BOOL ReadInterlaced(struct ImageData *,int  ,int );

static BOOL LoadImage(struct ColorLookupTable *clut,BOOL *clut_hanging)
{
  struct ImageData *image;

  image=GetBuffer(IDB.Width,IDB.Height);
  if (!image) return FALSE;

  SetBufferMode(image,CLUT);
  AttachCLUT(clut,image);
  *clut_hanging=FALSE;

  SetBackgroundColor(image,LSD.BackgroundColor);

  if (InterlacedImage())
  {
    ShowMessage(Txt(TXT_GIF_INTERLACED_IMAGE));
    if(!ReadInterlaced(image,IDB.Width,IDB.Height)) return FALSE;
  }
  else
  {
    ShowMessage(Txt(TXT_GIF_RASTER_IMAGE));
    if(!ReadRaster(image,IDB.Width,IDB.Height)) return FALSE;
  }

  SetDefaultBuffer(image);
  return TRUE;
}

/*================================= LOAD GIF =================================*/

BOOL LoadGIF(void)
{
  int i;
  struct ColorLookupTable *clut;
  BOOL	 CLUT_still_hanging=FALSE;
  char	 Signature;
  BOOL	 success=FALSE;

  transparentexists=FALSE;

  for (i=0;i<6;i++) GetByte(0);  /* eat signature */

  if (!LoadLogScrDescrBlock()) goto errexit;

  if (!(clut=GetCLUT(1<<BitsPerPixel()))) goto errexit;
  CLUT_still_hanging=TRUE;

  if (GlobalMap())
  {
    ShowMessage(Txt(TXT_LOADING_GLOBAL_COLORMAP));
    LoadColorMap(clut);
  }

  for(;;)
  {
    Signature=GetByte(0);

    switch (Signature)
    {
      case '!': // printf("Extension Block!!!\n");
		if (!LoadExtensionBlock())   goto errexit;
		break;

      case ',': if (!LoadImageDescriptionBlock()) goto errexit;

		if (LocalColorMap())
		  LoadColorMap(clut);

		if (!LoadImage(clut,&CLUT_still_hanging)) goto errexit;
		goto finished;
		break;

      case ';': SetError(NO_PICTURE_IN_FILE);
		goto errexit;
    }
  }

finished:
  success=TRUE;

  if (transparentexists)
  {
    struct ImageData* image = GetDefaultBuffer();
    SetBackgroundColor(image,transparentcolor);
  }

errexit:
  if (CLUT_still_hanging) FreeCLUT(clut);
  return success;
}

/*******************************************************************************

			      LZW - decoding

 *******************************************************************************/


#define MAX_LWZ_BITS		12

#define ReadOK(file,buffer,len) (fread(buffer,len,1,file)!=0)

#define LM_to_uint(a,b)                 (((b)<<8)|(a))


int GetCode(int code_size, int flag)   // -1 -> ERROR
{
  static unsigned char buf[280];
  static int	  curbit, lastbit, done, last_byte;
  int		  i, j, ret;
  unsigned char   count;

  if (flag)
  {
    curbit = 0;
    lastbit = 0;
    done = FALSE;
    return 0;
  }

  if ((curbit + code_size) >= lastbit)
  { if (done)
    { if (curbit >= lastbit)
      { ShowMessage("EOF reached, file seems to be too short");
	return -1;
      }
    }
    buf[0] = buf[last_byte - 2];
    buf[1] = buf[last_byte - 1];

    count = GetByte(0);

    if (count == 0)
    { done = TRUE; }

    else
      if (!ReadBlock(&buf[2],count))
      { ShowMessage("EOF reached, could not fill buffer");
	return -1;
      }

    last_byte = 2 + count;
    curbit = (curbit - lastbit) + 16;
    lastbit = (2 + count) * 8;
  }

  ret = 0;

  for (i = curbit, j = 0; j < code_size; i++, j++)
    ret |= ((buf[i / 8] & (1 << (i % 8))) != 0) << j;

  curbit += code_size;

  return ret;
}

  static int	   fresh = FALSE;
  int		   code, incode;
  static int	   code_size, set_code_size;
  static int	   max_code, max_code_size;
  static int	   firstcode, oldcode;
  static int	   clear_code, end_code;
	 short	   table[2][(1 << MAX_LWZ_BITS)];
	 short far stack[(1 << (MAX_LWZ_BITS)) * 2], *sp;

int LWZReadByte(int flag, int input_code_size)  // -2 : end / -1 : ERROR
{
  int	 i;

  if (flag)
  {
    set_code_size = input_code_size;
    code_size = set_code_size + 1;
    clear_code = 1 << set_code_size;
    end_code = clear_code + 1;
    max_code_size = 2 * clear_code;
    max_code = clear_code + 2;

    GetCode(0, TRUE);

    fresh = TRUE;

    for (i = 0; i < clear_code; i++)
    {
      table[0][i] = 0;
      table[1][i] = i;
    }

    for (; i < (1 << MAX_LWZ_BITS); i++)
      table[0][i] = table[1][0] = 0;

    sp = stack;
    return 0;
  }
  else if (fresh)
  { fresh = FALSE;
    do
    { firstcode = oldcode =
	GetCode(code_size, FALSE);
    } while (firstcode == clear_code);

    return firstcode;
  }

  if (sp > stack)
    return *--sp;

  while ((code = GetCode(code_size, FALSE)) >= 0)
  { if (code == clear_code)
    { for (i = 0; i < clear_code; i++)
      {table[0][i] = 0;
	table[1][i] = i;
      }

      for (; i < (1 << MAX_LWZ_BITS); i++)
	table[0][i] = table[1][i] = 0;

      code_size = set_code_size + 1;
      max_code_size = 2 * clear_code;
      max_code = clear_code + 2;
      sp = stack;
      firstcode = oldcode =
	GetCode(code_size, FALSE);
      return firstcode;
    }
    else if (code == end_code)
    { unsigned char   count;
      unsigned char   junk;

      while ((count=GetByte(0)) != 0)
	while (count-- != 0) junk=GetByte(0);

      if (count != 0)
      { ShowMessage("EOD missing");
	return -1;
      }

      return -2;
    }

    incode = code;

    if (code >= max_code)
    { *sp++ = firstcode;
      code = oldcode;
    }

    while (code >= clear_code)
    {
      *sp++ = table[1][code];
      if (code == table[0][code])
      { ShowMessage("severe input error");
	return -1;
      }
      code = table[0][code];
    }

    *sp++ = firstcode = table[1][code];

    if ((code = max_code) < (1 << MAX_LWZ_BITS))
    {
      table[0][code] = oldcode;
      table[1][code] = firstcode;
      max_code++;

      if ((max_code >= max_code_size) &&
	  (max_code_size < (1 << MAX_LWZ_BITS)))
      {
	max_code_size *= 2;
	code_size++;
      }
    }
    oldcode = incode;

    if (sp > stack)
      return *--sp;
  }

  return code;
}

BOOL ReadInterlaced(struct ImageData *image,int len,int height)
{
  unsigned char   c;
  int	 v;
  int	 xpos = 0;
  int	 ypos = 0, pass = 0;
  int	 maxypos = 0;

  int	 nLinesRead=0;


  c=GetByte(0);
  if (LWZReadByte(TRUE, c) < 0)
    return FALSE;

  while ((v = LWZReadByte(FALSE, c)) >= 0)
  {
    SetCLUTPixel(image,xpos,ypos,v);

    if (++xpos == len)
    {
      ShowProgress(++nLinesRead,height);

      xpos = 0;
      switch (pass)
      {
       case 0: case 1:		ypos += 8; break;
       case 2:			ypos += 4; break;
       case 3:			ypos += 2; break;
      }

      if (ypos > maxypos)       maxypos = ypos;

      if (ypos >= height)
      {
	switch (++pass)
	{
	 case 1:		ypos = 4; break;
	 case 2:		ypos = 2; break;
	 case 3:		ypos = 1; break;
	}
      }
    }
  }

  if (maxypos >= height) return TRUE;

  if (v == (-2))
    return TRUE;
  return FALSE;
}


BOOL ReadRaster (struct ImageData *image,int len,int height)
{
  unsigned char   c;
  int	 v;
  int	 xpos = 0;
  int	 ypos = -1;
  union Pixel *pixrow;

  BOOL Locked=FALSE;

  c=GetByte(0);

  if (LWZReadByte(TRUE, c) < 0)
    return FALSE;

  /* Read the raster data and dump it into the FBM bitmap */
  while ((v = LWZReadByte(FALSE, c)) >= 0)
  {
    if (xpos==0)
    {
      if (Locked) UnlockLine(image,ypos);

      ypos++;

      pixrow=LockAndGetLine(image,ypos);
      if (pixrow==NULL) return FALSE;
      Locked=TRUE;

      ShowProgress(ypos+1,height);
    }

    pixrow->color = v;

    pixrow++;
    xpos++;

    if (xpos == len) /* if (xpos == len) */ xpos = 0;
  }

  if (Locked) UnlockLine(image,ypos);

  if (ypos >= height) return TRUE;

  if (v == (-2))
    return TRUE;

  return FALSE;
}

/******************** Load Properties / Show Informations *********************/

static BOOL ShowExtensionBlockInfos(void)
{
  char FunctionCode;
  char Length;
  char c;
  char buffer[100];


  FunctionCode = GetByte(0);

//  printf("FunctionCode: %x\n",FunctionCode);

  while ((Length = GetByte(0)) != 0)
  {
    switch (FunctionCode)
    {
      case 0xF9:  // ---------------------- Graphic Control Block ----------

	ShowInfo(Txt(TXT_GRAPHIC_CONTROL_EXISTS));

	if (Length != 4) { ShowInfo(Txt(TXT_GRAPHIC_CONTROL_CORRUPT)); return FALSE; }

	c = GetByte(0);         // <Packed Fields>

	GetByte(0); GetByte(0); // eat "Time Delay"

	if (c & 1)
	{
	  c = GetByte(0);
	  BufShowInfo(Txt(TXT_GRAPHIC_CONTROL_TRANSCOLOR_D),(unsigned char)c);
	}
	else
	{
	  c = GetByte(0);
	}

	break;

      default:
	while (Length) { GetByte(0); Length--; } /* eat data-block */
	break;
    }
  }

  return TRUE;
}


static BOOL SkipExtensionBlocks(void)
{
  char FunctionCode;
  char Length;

  FunctionCode = GetByte(0);

  while ((Length = GetByte(0)) != 0)
  {
    while (Length) { GetByte(0); Length--; } /* eat data-block */
  }

  return TRUE;
}

void PropsGIF(void)
{
  int i;
  char Signature;

  Output_Mode = GFXMOD_CLUT; // new 1.8b

  for (i=0;i<6;i++) GetByte(0);  /* eat signature */

  if (!LoadLogScrDescrBlock()) return;

  Output_nColors = 1<<BitsPerPixel();

  if (GlobalMap()) LoadColorMap(NULL);

  for(;;)
  {
    Signature=GetByte(0);

    switch (Signature)
    {
      case '!': if (!SkipExtensionBlocks())   return;
		break;

      case ',': if (!LoadImageDescriptionBlock()) return;

		Output_Width  = IDB.Width;
		Output_Height = IDB.Height;
		return;
		break;

      case ';': return;
    }
  }
}


void InfoGIF(void)
{
  int i;
  char Signature;
  char GIF_Sig[7];
  char buffer[100];

  for (i=0;i<6;i++) GIF_Sig[i]=GetByte(0);  /* eat signature */
  GIF_Sig[6]=0;

  ShowInfo(GIF_Sig);
  ShowInfo(" ");

  if (!LoadLogScrDescrBlock()) return;

  if (GlobalMap()) LoadColorMap(NULL);

  for(;;)
  {
    Signature=GetByte(0);

    switch (Signature)
    {
      case '!': if (!ShowExtensionBlockInfos())        return;
		break;

      case ',': if (!LoadImageDescriptionBlock()) return;

  ShowInfo("Image Description Block:");

  BufShowInfo(Txt(TXT_WIDTH_D) ,IDB.Width);
  BufShowInfo(Txt(TXT_HEIGHT_D),IDB.Height);
  BufShowInfo(Txt(TXT_S_IMAGE) ,InterlacedImage() ? "Interlaced" : "Raster");
  BufShowInfo3(Txt(TXT_POSITION_DD),IDB.LeftPos,IDB.TopPos);

  if (LocalColorMap()) ShowInfo(Txt(TXT_LOCAL_COLORMAP_EXISTS));
  else		       ShowInfo(Txt(TXT_NO_LOCAL_COLORMAP));

  ShowInfo(" ");

  ShowInfo("Logical Screen Descriptor Block:");

  BufShowInfo(Txt(TXT_SCREENWIDTH_D) ,LSD.width);
  BufShowInfo(Txt(TXT_SCREENHEIGHT_D),LSD.height);
  BufShowInfo(Txt(TXT_BITSPERPIXEL_D),BitsPerPixel());
  BufShowInfo(Txt(TXT_COLORS_D)      ,1<<BitsPerPixel());

  if (LocalColorMap()) ShowInfo(Txt(TXT_GLOBAL_COLORMAP_EXISTS));
  else		       ShowInfo(Txt(TXT_NO_GLOBAL_COLORMAP));

		return;
		break;

      case ';': return;
    }
  }
}


/*==============================================================================================
				      GIF - Write
  ==============================================================================================*/

static char* WriteBuffer;
static int   BytesInWriteBuffer;

static BOOL FlushBuffer(void)
{
  if (BytesInWriteBuffer==0) return TRUE;

  if (!WriteByte(BytesInWriteBuffer,0)) return FALSE;
  if (!WriteBlock(WriteBuffer,BytesInWriteBuffer)) return FALSE;

  BytesInWriteBuffer=0;
  return TRUE;
}

static BOOL WriteByteToBuffer(char c)
{
  WriteBuffer[BytesInWriteBuffer] = c;
  BytesInWriteBuffer++;

  if (BytesInWriteBuffer==255)
  {
    if (!FlushBuffer()) return FALSE;
  }

  return TRUE;
}

static char  buildchar;
static char  bitsleft;

static BOOL FlushByte(void)
{
  if (bitsleft == 8) return TRUE;

  if (!WriteByteToBuffer(buildchar)) return FALSE;

  bitsleft=8;
  return TRUE;
}

static BOOL WriteCode(short code,int codelength)
{
  int codeleft	  = codelength;
  int codetowrite = code;

  assert(bitsleft>0);


  while (codeleft>0)
  {
    int bitstowrite;
    int mask;
    short cuttedbits;


    bitstowrite = min(bitsleft,codeleft);


    /* Bits ausschneiden, so daß sich die Bits nachher ganz rechts befinden */

    mask = ((1<<bitstowrite)-1);

    cuttedbits	  = codetowrite & mask;
    codetowrite >>= bitstowrite;


    /* Bits zu dem Byte links hinzufügen */

    cuttedbits <<= 8-bitsleft;
    buildchar  |= cuttedbits;

    bitsleft -= bitstowrite;
    codeleft -= bitstowrite;


    /* wenn Byte voll, in den Buffer speichern und leeren */

    if (bitsleft==0)
    {
      if (!WriteByteToBuffer(buildchar)) return FALSE;
      bitsleft=8;
      buildchar=0;
    }
  }

  return TRUE;
}


static int InputX,InputY,FinalX,FinalY;
static struct ImageData* InputImage;

static void InitInputStream(void)
{
  InputImage = GetDefaultBuffer();
  InputX=InputY=0;

  FinalX = GetImageWidth (InputImage);
  FinalY = GetImageHeight(InputImage);
}

static BOOL HaveMoreInput(void)
{
  if (InputY<FinalY) return TRUE;
  else		     return FALSE;
}

static BOOL GetNextInputByte(char* code)
{
  UWORD w;
  if (!GetCLUTPixel(InputImage,InputX,InputY,&w))
  {
//  printf("GetNextInputByte-ERROR\n");
    return FALSE;
  }

  *code = w;

  InputX++;
  if (InputX==FinalX)
  {
    InputX=0;
    InputY++;

    ShowProgress(InputY,FinalY);
  }

  return TRUE;
}

static struct CodeTableEntry
{
  UBYTE  code;
  USHORT tableidx;
  struct CodeTableEntry* next;
  struct CodeTableEntry* childs;
};

static struct CodeTableEntry* codetable;
static int		      nexttableentry;

static void InitCodeTable(int codesize)
{
  int i = (1<<codesize)-1;

  assert(codesize<=8);
  assert(codesize>=2);

  nexttableentry = (1<<codesize)+2;

  for ( ; i>=0 ; i-- )
  {
    codetable[i].code	  = i;
    codetable[i].childs   = NULL; /* noch leer */
 // codetable[i].next wird bei den ersten 1<<codesize Einträgen nie benutzt */
  }
}


/*
  Algorithmus:
    Die ersten 1<<codesize Einträge werden ganz normal als Tabelle benutzt.
    Alle weiteren Tabelleneinträge, die x als Prefix haben, sind durch
    x->childs erreichbar, verkettet durch ->next.

    Die beiden Einträge bei den Spezial-Codes bleiben unbenutzt.
*/


static BOOL WriteLZW(int nPlanes,int InitialCodeSize)
{
  struct CodeTableEntry* currentprefix;
  struct CodeTableEntry* ptr;
  char	 inputcode;
  BOOL	 inTabelle;
  int	 codesize;
  int	 ClearCode = 1<<InitialCodeSize;
  int	 EOICode   = ClearCode+1;

  InitInputStream();

  InitCodeTable(InitialCodeSize);
  codesize = InitialCodeSize+1;

  WriteCode(ClearCode,codesize);



  if (!GetNextInputByte(&inputcode)) return FALSE;
  currentprefix = &codetable[inputcode];

  while (HaveMoreInput())
  {
    if (!GetNextInputByte(&inputcode)) return FALSE;

    /* suchen, ob (prefix)inputcode in der Tabelle enthalten ist. */

    inTabelle=FALSE;

    for (ptr=currentprefix->childs;ptr!=NULL;ptr=ptr->next)
    {
      if ( ptr->code == inputcode )
      {
	/* ja, dann ist der neue Prefix der gesammt-String */

	currentprefix = ptr;
	inTabelle=TRUE;

	break;
      }
    }


    if (!inTabelle)
    {
      WriteCode( currentprefix->tableidx , codesize );

      if ( nexttableentry >= (1<<codesize) )
      {
	codesize++;
      }

      /* neuen Tabelleneintrag erstellen */

      if (nexttableentry<4096)
      {
	codetable[nexttableentry].code	   = inputcode;
	codetable[nexttableentry].next	   = currentprefix->childs;
	codetable[nexttableentry].childs   = NULL;
	currentprefix->childs		   = &codetable[nexttableentry];
	nexttableentry++;
      }
      else
      {
	WriteCode(ClearCode,12);
	codesize = InitialCodeSize+1;
	InitCodeTable(InitialCodeSize);
      }

      currentprefix = &codetable[inputcode];
    }
  }

  WriteCode( currentprefix->tableidx, codesize );

  WriteCode( EOICode, codesize );

  return TRUE;
}

static BOOL InitGIFWrite(void)
{
  int i;

  /* Schreib-Buffer initialisieren */

  WriteBuffer = cu_calloc( 255,1 );
  if (WriteBuffer==NULL) goto errexit;

  BytesInWriteBuffer=0;

  bitsleft=8;
  buildchar=0;


  /* Code-Tabelle erstellen */

  codetable = cu_calloc( (1<<12),sizeof(struct CodeTableEntry) );
  if (codetable==NULL) goto errexit;

  for (i=0;i<4096;i++)
  {
    codetable[i].tableidx = i;
  }

  return TRUE;

errexit:
  if (codetable)   cu_free(codetable);
  if (WriteBuffer) cu_free(WriteBuffer);

  return FALSE;
}


static void CleanupGIFWrite(void)
{
  cu_free(codetable);
  assert(WriteBuffer!=NULL);
  cu_free(WriteBuffer);
  WriteBuffer=NULL;
}


BOOL SaveGIF(void)
{
  struct ImageData* image=NULL;
  char		    nPlanes;
  int		    nColors;
  int		    ColorTableSize;
  int		    InitialCodeSize;
  short 	    background;


  /* Bilddaten bestimmen */

  image=GetDefaultBuffer();

  nColors = min(Output_nColors,256);

  for (nPlanes=1 ; (1<<nPlanes) < nColors ; nPlanes++)
  {
    /* nothing to do here */
  }

  assert(nPlanes <= 8);

  ColorTableSize = 1<<nPlanes;

  if (nPlanes==1) InitialCodeSize=2;
  else		  InitialCodeSize=nPlanes;


  /* Bild umwandeln */

  if (!ConvertToMode(nColors,(GetOutputFlags() & ~MODE_FLAGS) | GFXMOD_CLUT))
    return FALSE;

  image=GetDefaultBuffer();


  ShowMessage(Txt(TXT_SAVING_GIF));
  ShowProgress(0,1);

  /* Signatur schreiben */

  if (!WriteByte('G',0)) return FALSE;
  if (!WriteByte('I',0)) return FALSE;
  if (!WriteByte('F',0)) return FALSE;
  if (!WriteByte('8',0)) return FALSE;
  if (!WriteByte('9',0)) return FALSE;
  if (!WriteByte('a',0)) return FALSE;


  /* Log. Screen Desc. Block schreiben */

  if (!WriteWord( GetImageWidth(image)  ,0,1 )) return FALSE;
  if (!WriteWord( GetImageHeight(image) ,0,1 )) return FALSE;

  if (!WriteByte( 0x80 | ((nPlanes-1) << 4) | 0 | (nPlanes-1) ,0 )) return FALSE;


  background = NewBackgroundColor;
  if (background == -1) background = AskBackgroundColor(image);

  if (background == -1) { if (!WriteByte( 0,0 )) return FALSE; }
  else			{ if (!WriteByte( background,0)) return FALSE; }


  if (!WriteByte( 0,0 )) return FALSE;


  /* Global Color Table schreiben */

  {
    int i;
    for (i=0;i<ColorTableSize;i++)
    {
      char r,g,b;

      GetImageColor(image,i,&r,&g,&b);

      if (!WriteByte( r,0 )) return FALSE;
      if (!WriteByte( g,0 )) return FALSE;
      if (!WriteByte( b,0 )) return FALSE;
    }
  }



  /* Graphic Control Extension schreiben, wenn Transparency benötigt wird */

  if (background != -1)
  {
    if (!WriteByte( 0x21,0 ))       return FALSE;   // Extension Introducer
    if (!WriteByte( 0xF9,0 ))       return FALSE;   // Graphic Control Label

    if (!WriteByte( 0x04,0 ))       return FALSE;   // Block Size
    if (!WriteByte( 0x01,0 ))       return FALSE;   // Disposal Method | ... | Transparency Flag
    if (!WriteWord( 0x00,0,TRUE ))  return FALSE;   // Delay Time
    if (!WriteByte( background,0 )) return FALSE;   // Transparency Index

    if (!WriteByte( 0x00,0 ))       return FALSE;   // Block-Terminator
  }



  /* Image Descriptor schreiben */

  if (!WriteByte( 0x2C,0    )) return FALSE;
  if (!WriteWord( 0,0,TRUE  )) return FALSE;
  if (!WriteWord( 0,0,TRUE  )) return FALSE;
  if (!WriteWord( GetImageWidth(image) ,0,TRUE )) return FALSE;
  if (!WriteWord( GetImageHeight(image),0,TRUE )) return FALSE;
  if (!WriteByte( 0 | 0 | 0 | 0 | 0 ,0 )) return FALSE;



  /* Das Bild schreiben */

  if (!InitGIFWrite()) return FALSE;

  if (!WriteByte( InitialCodeSize ,0 )) return FALSE;

  if (!WriteLZW(nPlanes,InitialCodeSize)) return FALSE;

  FlushByte();
  FlushBuffer();

  CleanupGIFWrite();


  /* BlockTerminator */

  if (!WriteByte(0,0)) return FALSE;



  /* Terminator schreiben */

  if (!WriteByte( 0x3B,0 )) return FALSE;

  return TRUE;
}

