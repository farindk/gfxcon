
/********************************************************************************
 *
 * modul name:  fcvp.c
 *
 * contents:    routines to load, save and check CVP files
 *
 *
 * v1.0 (27.07.98)
 *   basic load-routine
 *
 *
 * Copyright (C) 1998  Dirk Farin  <dirk.farin@gmail.com>
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


void CheckForCVP(form fo)
{
  int length = GetLoadFileLength();

  if (length != 786432)
  {
    fo->not_valid_format = TRUE;
    return;
  }

  fo->not_valid_format = FALSE;
}


void PropsCVP(void)
{
  Output_Mode=GFXMOD_24BIT;
  Output_nColors=256;

  Output_Width =512;
  Output_Height=512;
}


/**------------------------------------------------------------------------**
 **  INFO  **
 **------------------------------------------------------------------------**/

void InfoCVP(void)
{
  ShowInfo("CVP infos");
}


/**------------------------------------------------------------------------**
 **  LOAD  **
 **------------------------------------------------------------------------**/

BOOL LoadCVP(void)
{
  int   x;
  int   y;
  struct ImageData* inputimage=NULL;

  ShowMessage("loading CVP");

  inputimage=GetBuffer(512,512);
  if (!inputimage) { return FALSE; }

  SetBufferMode(inputimage,RGB);


  /* Bild einlesen */

  ShowProgress(0,1);


  for (y=0;y<512;y++)
  {
    for (x=0;x<512;x++)
    {
      if (!SetRGBPixel(inputimage,x,y,ReadByte(),0,0))
      { FreeBuffer(inputimage); return FALSE; }
    }

    ShowProgress(y,512*3);
  }

  for (y=0;y<512;y++)
  {
    for (x=0;x<512;x++)
    {
      char r,g,b;
      if (!GetRGBPixel(inputimage,x,y,&r,&g,&b) ||
          !SetRGBPixel(inputimage,x,y, r,ReadByte(),0))
      { FreeBuffer(inputimage); return FALSE; }
    }

    ShowProgress(512+y,512*3);
  }

  for (y=0;y<512;y++)
  {
    for (x=0;x<512;x++)
    {
      char r,g,b;
      if (!GetRGBPixel(inputimage,x,y,&r,&g,&b) ||
          !SetRGBPixel(inputimage,x,y, r,g,ReadByte()))
      { FreeBuffer(inputimage); return FALSE; }
    }

    ShowProgress(1024+y,512*3);
  }

  SetDefaultBuffer(inputimage);

  return TRUE;
}





BOOL SaveCVP(void)
{
  struct ImageData *image;
  int x,y;
  int h,w;

  if (!ConvertToMode(0,(GetOutputFlags() & ~MODE_FLAGS) | GFXMOD_24BIT)) return FALSE;

  image=GetDefaultBuffer();

  ShowMessage("saving CVP file");

  h=GetImageHeight(image);
  w=GetImageWidth (image);

  ShowProgress(0,1);


  for (y=0;y<h;y++)
  {
    for (x=0;x<w;x++)
    {
      char r,g,b;
      if (!GetRGBPixel(image,x,y,&r,&g,&b)) return FALSE;
      SaveByte(r);
    }

    ShowProgress(y,512*3);
  }

  for (y=0;y<h;y++)
  {
    for (x=0;x<w;x++)
    {
      char r,g,b;
      if (!GetRGBPixel(image,x,y,&r,&g,&b)) return FALSE;
      SaveByte(g);
    }

    ShowProgress(512+y,512*3);
  }

  for (y=0;y<h;y++)
  {
    for (x=0;x<w;x++)
    {
      char r,g,b;
      if (!GetRGBPixel(image,x,y,&r,&g,&b)) return FALSE;
      SaveByte(b);
    }

    ForgetLine(image,y);
    ShowProgress(1024+y,512*3);
  }

  return TRUE;
}


