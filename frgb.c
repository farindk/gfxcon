
/********************************************************************************
 *
 * modul name:	frgb.c
 *
 * contents:	routines to load, save and check raw RGB files
 *
 *
 * to do:
 *
 *
 * v1.1 (11.10.93)
 *   Major revision. Module entirely rewritten. Major bug fixes.
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
#include "options.h"

extern ULONG inputwidth;

void CheckForRGB(form fo)
{
  fo->not_valid_format = FALSE;
}

#define Guess(x) if ((FileLength % x)==0) GuessW=x; else


static short NormDims[][2] =
{
  1280,1024,
  1024, 768,
   800, 600,
   640, 512,
   640, 480,
   640, 400,
   512, 512,
   320, 256,
   320, 240,
   320, 128,
   128, 128,
    64,  64,
    32,  32,
    16,  16,
     0,   0
};

static ULONG EndWindow(void) { return EXIT_EXITWINDOW; }

static void AskWidth(ULONG *w,ULONG *height)
{
  struct Handle *ha;

  if (BeginNewHandleTree())
  {
    ha=CrSmallWindow(
        CrSpaceBox(
          CrHBox(
            CrText(Txt(TXT_WIDTH_COL_SPC),TAG_DONE),
            CrGadget(GAGA_Kind,INTEGER_KIND,
                     GAGA_CallFunc,&EndWindow,
                     GAGA_IN_Ptr,w,
                     GAGA_ID    ,1,
                     GAGA_LowerBound,1,
                     GAGA_UpperBound,99999,
                     TAG_DONE,
                     TAG_DONE),
            HANDLE_END
          )
        ),
        WAWA_ActiveGad,1,
        WAWA_Centered ,h,
        TAG_DONE,
        WA_Activate   ,TRUE,
        TAG_DONE
      );
  }
  else
    ShowError();

  if (ha != HANDLE_ERR)
  {
    if (ComputeGadgets(ha))
    {
      if (OpenHandle(ha))
      {
        HandleHandle(ha,NULL,NULL);

        CloseHandle(ha);

        FreeHandleTree(ha);
      }
      else
        ShowError();
    }
    else
      ShowError();
  }
}

BOOL LoadRGB(void)
{
  ULONG GuessW,GuessH;
  int width;
  int FileLength;
  int i;
  struct ImageData *image=NULL;
  int x,y;
  UBYTE r,g,b;
  BOOL  success=FALSE;

  FileLength=GetLoadFileLength();

  GuessW=0;

  for (i=0;NormDims[i][0];i++)
    if (FileLength == (NormDims[i][0]*NormDims[i][1]))
    {
      GuessW = NormDims[i][0];
      GuessH = NormDims[i][1];
      break;
    }

  if (!GuessW)
  {
    Guess(1280)
    Guess(1024)
    Guess( 960)
    Guess( 800)
    Guess( 640)
    Guess( 600)
    Guess( 480)
    Guess( 320)
      ;

    for (width=1280;width>=32 && !GuessW;width-=32)
      Guess(width);

    for (width=1280;width>=16 && !GuessW;width-=8)
      Guess(width);

    for (width=8192;width>=16 && !GuessW;width-=1)
      Guess(width);
  }

  if (!GuessW) return FALSE;

  if (!CLImode) AskWidth(&GuessW,&GuessH);
  else          { if (inputwidth) GuessW=inputwidth; }

  GuessH = FileLength/GuessW;

  image=GetBuffer(GuessW,GuessH);
  if (!image) goto errexit;

  SetBufferMode(image,RGB);

  ShowMessage(Txt(TXT_LOADING_RGBFILES));

  for (y=0;y<GuessH;y++)
  {
    for (x=0;x<GuessW;x++)
    {
      r=GetByte(R);
      g=GetByte(G);
      b=GetByte(B);

      if (!SetRGBPixel(image,x,y,r,g,b)) goto errexit;
    }

    ShowProgress(y,GuessH-1);
  }

  success=TRUE;

errexit:
  if (!success) if (image) FreeBuffer(image);

  if (success) SetDefaultBuffer(image);
  return success;
}

BOOL SaveRGB(void)
{
  struct ImageData *image;
  int x,y;
  UBYTE r,g,b;
  int h,w;

  if (!ConvertToMode(0,(GetOutputFlags() & ~MODE_FLAGS) | GFXMOD_24BIT)) return FALSE;

  image=GetDefaultBuffer();

  ShowMessage(Txt(TXT_SAVING_RGBFILES));

  h=GetImageHeight(image);
  w=GetImageWidth (image);

  for (y=0;y<h;y++)
  {
    for (x=0;x<w;x++)
    {
      if (!GetRGBPixel(image,x,y,&r,&g,&b)) return FALSE;

      if (!WriteByte(r,R)) return FALSE;
      if (!WriteByte(g,G)) return FALSE;
      if (!WriteByte(b,B)) return FALSE;
    }
    ShowProgress(y,GetImageHeight(image)-1);

    ForgetLine(image,y);
  }
  return TRUE;
}

void InfoRGB(void)
{
  ShowInfo(" ");
}

