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

#include "glib:mylib.h"
#include "glib:myerrs.h"
#include "glib:mytxts.h"

typedef UBYTE BOOL8;
typedef struct FormatObject *form;

#define skip if(0)
#define ReadByte()  getc(file_load)
#define SaveByte(c) putc(c,file_save)

extern FILE *file_load; /*--> do NOT directly use this <--*/
extern FILE *file_save; /*--> do NOT directly use this <--*/

#define MAX_FORMATS 20

struct FormatObject
{
  char   *FormatName;
  char   *suffix;
  UBYTE   Priority;     /* format recognition priority */
  BOOL8   RGB_files;    /* will be saved in three separate files */
  UWORD   FormatID;     /* ID of the format */


  /* functions */

  void  (*CheckFormat)   (form);     /* check if this file can be this format */
  BOOL  (*LoadPict)      (void);
  BOOL  (*SavePict)      (void);     /* save the buffer */

  void  (*GetProperties) (void);
  void  (*Information)   (void);


  /* volatile variables */

  BOOL8 not_valid_format;
};

enum Method { Method_CheckFormat,
              Method_LoadPict,
              Method_SavePict,

              Method_GetProperties,
              Method_Information
            };

#define REQUIRE_COMPLETE_PICTURE_BUFFERED (1<<0)


#define SAVEFLAG_IFF_DONT_COMPRESS (1<<0)


#define FORM_ILBM   1
#define FORM_LBM   18
#define FORM_RGB    2
#define FORM_PCX    3
#define FORM_IMG    4
#define FORM_GEM    5
#define FORM_GIF    6
#define FORM_TIFF   7
#define FORM_CGM    8
#define FORM_WGP    9
#define FORM_DXF   10
#define FORM_EPS   11
#define FORM_JPEG  12
#define FORM_RGB8  13
#define FORM_RGBN  14
#define FORM_BMP   15
#define FORM_RLE4  16
#define FORM_RLE8  17
#define FORM_PS    19
#define FORM_TGA   20
#define FORM_CVP   21
#define FORM_PNG   22



#define NFILEINFOS       5
#define FILEINFO_ERROR (~0)

extern struct CleanupList *GlobalCleanup;

extern struct Handle *h;

extern char *GetTempPrefix(void);
extern char *GetTempPostfix(void);

#define TMP_FILENAME_LENGTH 200



ULONG GetOutputFlags(void);

/* io - functions */

enum SeekMode { Absolute=0,Relative=1,FromEnd=2 };
enum FileSpec { R=0,G,B };

UBYTE GetByte(enum FileSpec );
UWORD GetWord(enum FileSpec,BOOL intel);
ULONG GetLong(enum FileSpec,BOOL intel);
BOOL  ReadBlock (APTR,int size);

BOOL  WriteByte(UBYTE,enum FileSpec );
BOOL  WriteWord(UWORD,enum FileSpec,BOOL intel);
BOOL  WriteLong(ULONG,enum FileSpec,BOOL intel);
BOOL  WriteBlock(APTR,int size);

BOOL  SeekPosLoad(ULONG pos,enum SeekMode mode);
BOOL  SeekPosSave(ULONG pos,enum SeekMode mode);
int   GetSavePos (void);
int   GetLoadPos (void);
int   GetLoadFileLength(void);
void  RewindInput(void);

BOOL   CheckEOF(void);

BOOL   OpenLoadFile(void);
void   CloseLoadFile(void);
BOOL   OpenSaveFile(void);
void   CloseSaveFile(void);

void   GetLoadFilename(char *);
void   GetSaveFilename(char *);
void   SetLoadPath(char *);
void   SetSavePath(char *);

int   wbstart (int argc,char **argv);
int   clistart(int argc,char **argv);
void  Interactive(void);
struct FormatObject *NextFormatObject(ULONG *state);
ULONG                nFormats(void);
BOOL CallFormatMethod(struct FormatObject *,enum Method);
void CallFormatMethodAll(enum Method);
struct FormatObject *GetFormat(ULONG formatID);

void ShowProgress(int,int);
void ShowMessage (char*);
void ShowInfo    (char*);

/* next #defines only work, is you've a char buffer[...] declared in your function */

#define BufShowInfo(x,y)      sprintf(buffer,x,y);     ShowInfo(buffer)
#define BufShowInfo3(x,y,z)   sprintf(buffer,x,y,z);   ShowInfo(buffer)
#define BufShowInfo4(x,y,z,q) sprintf(buffer,x,y,z,q); ShowInfo(buffer)

void IntelSwapWord(UWORD *);


/* gfx-buffer */

enum PixelMode { RGB,CLUT };

union Pixel   /* each pixel is defined with its RGB-value or its CLUT-index */
{
  struct { UBYTE r,g,b;
           UBYTE HAM_Byte;
                        } rgb;
  UWORD                   color;
  ULONG                   everything; /* this must be a field which holds all data */
};

#ifndef CREATING_GFXPROTO
#  include "gfxbuffer.h"
#endif


#define GFXMOD_CLUT     (1<< 0)  /* normal color-table output */
#define GFXMOD_HAM      (1<< 1)  /* prepare for HAM           */
#define GFXMOD_HAM8     (1<<19)  /* prepare for HAM8          */
#define GFXMOD_EHB      (1<< 2)  /* prepare for EHB           */
#define GFXMOD_24BIT    (1<< 3)  /* 24 bit high-color         */

#define GFXMOD_FLOYD     (1<< 4)  /* do dither                 */
#define GFXMOD_FASTFLOYD (1<<17)  /* do dither (faster but not that accurate) */
#define GFXMOD_HALFTONE  (1<< 5)  /*--- not yet ---------------*/
#define GFXMOD_GRAYSCALE (1<<18)  /* gleichmäßige Graustufen   */

#define GFXMOD_RESIZE   (1<< 6)
#define GFXMOD_FLIPX    (1<< 7)
#define GFXMOD_FLIPY    (1<< 8)
#define GFXMOD_FLIPXY   (GFXMOD_FLIPX | GFXMOD_FLIPY)
#define GFXMOD_ROTATE_R (1<<10)
#define GFXMOD_ROTATE_L (1<<11)

#define GFXMOD_BW       (1<<12)
#define GFXMOD_NO_R     (1<<13)
#define GFXMOD_NO_G     (1<<14)
#define GFXMOD_NO_B     (1<<15)
#define GFXMOD_INVERS   (1<<16)


#define MODE_FLAGS    ( GFXMOD_CLUT | GFXMOD_HAM | GFXMOD_EHB | GFXMOD_24BIT )

#define DITHER_FLAGS  ( GFXMOD_FLOYD | GFXMOD_FASTFLOYD | GFXMOD_HALFTONE )

#define EFFECTS_FLAGS ( GFXMOD_RESIZE | GFXMOD_FLIPX | GFXMOD_FLIPY |  \
                        GFXMOD_ROTATE_R | GFXMOD_ROTATE_L | GFXMOD_FLIPXY );

#define COLOR_FLAGS   ( GFXMOD_BW | GFXMOD_NO_R | GFXMOD_NO_G | GFXMOD_NO_B | GFXMOD_GRAYSCALE )

