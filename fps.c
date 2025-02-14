
/********************************************************************************
 *
 * modul name:  fps.c
 *
 * contents:    routines to save postscript files
 *
 *
 * to do:
 *
 *
 * v1.1 (11.10.93)
 *   Major revision. Module entirely rewritten. Major bug fixes.
 *
 * v0.9 (00.00.93)
 *   basic load-routines
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

#define CM 28.346456
#define WriteSpace()   WriteByte(' ' ,0)
#define WriteNewLine() WriteByte('\n',0)

static BOOL WriteString(char *str)
{
  char *p;

  for (p=str;*p;p++)
    if (!WriteByte(*p,0)) return FALSE;

  return TRUE;
}

static BOOL WriteInt(int x)
{
  char buffer[20];

  sprintf(buffer,"%d",x);
  return WriteString(buffer);
}

static BOOL WriteDouble(double x)
{
  char buffer[30];

  sprintf(buffer,"%lf",x);
  return WriteString(buffer);
}

static BOOL WriteHexByte(UBYTE x)
{
  char buffer[4];

  sprintf(buffer,"%02x",x);
  if (!WriteByte(buffer[0],0)) return FALSE;
  if (!WriteByte(buffer[1],0)) return FALSE;
  return TRUE;
}

#define ID_HEIGHT 1
#define ID_XPOS 2

static double xpos,ypos,width,height;
static ULONG  PicWidth,PicHeight;

static struct Handle *ha;

static ULONG RestoreAspect(void)
{
  char buffer[30];

  height=(width*PicHeight)/PicWidth;

  sprintf(buffer,"%lf",height);
  GT_SetGadgetAttrs(GetGadget(ha,ID_HEIGHT),GetWindow(ha),NULL,
                    GTST_String,buffer,
                    TAG_DONE);


  return 0;
}

static void GetOutputDimensions(void)
{
  if (BeginNewHandleTree())
  {
    ha=CrSmallWindow(
        CrSpaceBox(
          CrSpaceRaster(3,
            CrText(Txt(TXT_POST_XPOS),TAG_DONE),
            CrGadget(GAGA_Kind,STRING_KIND,
                     GAGA_DB_Ptr,&xpos,
                     GAGA_CharsWidth,8,
                     GAGA_ID,ID_XPOS,
                     TAG_DONE,
                     TAG_DONE),
            CrText("cm",TAG_DONE),

            CrText(Txt(TXT_POST_YPOS),TAG_DONE),
            CrGadget(GAGA_Kind,STRING_KIND,
                     GAGA_DB_Ptr,&ypos,
                     GAGA_CharsWidth,8,
                     TAG_DONE,
                     TAG_DONE),
            CrText("cm",TAG_DONE),

            CrText(Txt(TXT_POST_WIDTH),TAG_DONE),
            CrGadget(GAGA_Kind,STRING_KIND,
                     GAGA_DB_Ptr,&width,
                     GAGA_CharsWidth,8,
                     GAGA_CallFunc,&RestoreAspect,
                     TAG_DONE,
                     TAG_DONE),
            CrText("cm",TAG_DONE),

            CrText(Txt(TXT_POST_HEIGHT),TAG_DONE),
            CrGadget(GAGA_Kind,STRING_KIND,
                     GAGA_DB_Ptr,&height,
                     GAGA_CharsWidth,8,
                     GAGA_ID,ID_HEIGHT,
                     TAG_DONE,
                     TAG_DONE),
            CrText("cm",TAG_DONE),

            HANDLE_END
          )
        ),
        WAWA_ActiveGad,ID_XPOS,
        WAWA_Centered ,h,
        TAG_DONE,
        WA_Activate   ,TRUE,
        WA_CloseGadget,TRUE,
        WA_Title      ,Txt(TXT_POST_POSWIN_TITLE),
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


  /* correct values */

  xpos *= CM;
  ypos *= CM;
  width  *= CM;
  height *= CM;
}

BOOL SavePS(void)
{
  struct ImageData *image;
  int x,y;
  UBYTE r,g,b;
  int   CharsOnLine;
  double LineHeight;

  if ( ! ConvertToMode(0,GetOutputFlags() & ~MODE_FLAGS )) return FALSE;

  MoveStripesToFastMem();

  ShowMessage(Txt(TXT_SAVING_POSTSCRIPT));

  image=GetDefaultBuffer();

  PicWidth  = GetImageWidth (image);
  PicHeight = GetImageHeight(image);


  /* zentriert auf Seite plazieren   (DIN A4 ist 21x29.7) */

  xpos   =  1.5;
  width  = 18.0;

  height = (width*PicHeight)/PicWidth;
  ypos   = 29.7-(29.7-height)/2;


  GetOutputDimensions();

  WriteString("gsave\n");

  LineHeight = height/PicHeight;

  ypos -= LineHeight;

  CharsOnLine = 0;

  WriteDouble(xpos);
  WriteSpace();
  WriteDouble(ypos);
  WriteSpace();
  WriteString("translate\n");

  WriteDouble(width);
  WriteSpace();
  WriteDouble(LineHeight);
  WriteString(" scale\n");

  for (y=0;y<GetImageHeight(image);y++)
  {
    WriteInt(GetImageWidth(image));
    WriteSpace();
    WriteInt(1);
    WriteSpace();
    WriteInt(8);   /* BitsPerSample */
    WriteString(" [ ");
    WriteInt(GetImageWidth(image));
    WriteString(" 0 0 1 0 0 ]\n");

    WriteString("{ <\n");

    for (x=0;x<GetImageWidth(image);x++)
    {
      if (!GetRGBPixel(image,x,y,&r,&g,&b)) return FALSE;

      WriteHexByte((r+g+b)/3);
      CharsOnLine+=2;

      if (CharsOnLine >= 80) { WriteNewLine(); CharsOnLine=0; }
    }
    WriteString("\n > } image\n");

    ShowProgress(y,GetImageHeight(image)-1);
    ForgetLine(image,y);

    CharsOnLine=0;

    WriteString("0 -1 translate\n");
  }

  WriteString("grestore\n");
  WriteString("showpage\n");

  return TRUE;
}

void InfoPS(void)
{
  ShowInfo(" ");
}

