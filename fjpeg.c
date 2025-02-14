
/********************************************************************************
 *
 * modul name:  fjpeg.c
 *
 * contents:    routines to load, save and check JPEG files
 *              (needs external source)
 *
 *
 * to do:
 *
 *
 * v1.9 (23.May.99)
 *   - JPEG part is now based on IJG-JPEGlib V6b.
 *
 * v0.9 (12.12.93)
 *   basic check- and props-routines
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
// #include <setjmp.h>



#undef GLOBAL /* suppress warning */
#include "jpeg:jpeglib.h"

void CheckForJPEG(form fo)
{
  BOOL isJPEG=TRUE;

  if (GetByte(0) != 0xFF) isJPEG=FALSE;
  if (GetByte(0) != 0xD8) isJPEG=FALSE;

/* Macintosh-Header ueberspringen. Geht aber nicht mit dem JPEG-Decoder.
  if (isJPEG==FALSE)
  {
    isJPEG=TRUE;
    SeekPosLoad(128,Absolute);

    if (GetByte(0) != 0xFF) isJPEG=FALSE;
    if (GetByte(0) != 0xD8) isJPEG=FALSE;
  }
*/

/*
  if (GetByte(0) != 0xFF) isJPEG=FALSE;
  if (GetByte(0) != 0xE0) isJPEG=FALSE;
  GetByte(0);
  GetByte(0);
  if (GetByte(0) != 'J' ) isJPEG=FALSE;
  if (GetByte(0) != 'F' ) isJPEG=FALSE;
  if (GetByte(0) != 'I' ) isJPEG=FALSE;
  if (GetByte(0) != 'F' ) isJPEG=FALSE;
  if (GetByte(0) != 0x00) isJPEG=FALSE;
*/
  fo->not_valid_format = !isJPEG;
}


/* V1.8d: Akzeptiert nun auch nicht-JFIF-Dateien.
 */
void InfoJPEG(void)
{
  char buffer[100];
  short Version;
  char  units;
  int   xdensity,ydensity;


  // FFC0-Marker (SOF0) suchen

  char c;
  BOOL quit=FALSE;
  while (!CheckEOF() && !quit)
  {
    c=GetByte(0);
    if (c==0xFF)
    {
      c=GetByte(0);

      switch(c)
      {
      case 0x00:
        break;

      case 0xC0:
      case 0xC1:
      case 0xC2:
      case 0xC3:
      case 0xC5:
      case 0xC6:
      case 0xC7:
      case 0xC9:
      case 0xCA:
      case 0xCB:
      case 0xCD:
      case 0xCE:
      case 0xCF:
        {
        int Nf,i;

        GetWord(0,0); // frame header length
        sprintf(buffer,"sample precision: %d bits",GetByte(0)); ShowInfo(buffer);  // sample precision
        sprintf(buffer,"height: %d",GetWord(0,0)); ShowInfo(buffer);
        sprintf(buffer,"width:  %d",GetWord(0,0)); ShowInfo(buffer);
        sprintf(buffer,"number of components: %d",Nf=GetByte(0)); ShowInfo(buffer);
        for (i=1;i<=Nf;i++)
        {
          int sf;
          sprintf(buffer,"component %d - ID: %d",i,GetByte(0)); ShowInfo(buffer);
          sf = GetByte(0);
          sprintf(buffer,"component %d - h-sampling-factor: %d",i,sf>>4);  ShowInfo(buffer);
          sprintf(buffer,"component %d - v-sampling-factor: %d",i,sf&0xF); ShowInfo(buffer);
          sprintf(buffer,"component %d - quantization-table-ID: %d",i,GetByte(0)); ShowInfo(buffer);
        }
        }

        quit=TRUE;

        break;

      case 0xE0:
        GetWord(0,0);
        GetLong(0,0); // JFIF
        GetByte(0);   // 0x00
        Version=GetWord(0,0);
        units  =GetByte(0);
        xdensity=GetWord(0,0);
        ydensity=GetWord(0,0);

        BufShowInfo4(Txt(TXT_VERSION_DDD),Version/256,(Version>>4)&0x0F,(Version&0x0F));
        switch (units)
        {
          case 0:  sprintf(buffer,"aspect: %dx%d",xdensity,ydensity); ShowInfo(buffer); break;
          case 1:  sprintf(buffer,"%dx%d dpi"    ,xdensity,ydensity); ShowInfo(buffer); break;
          case 2:  sprintf(buffer,"%dx%d dots/cm",xdensity,ydensity); ShowInfo(buffer); break;
          default: ShowInfo("unknown density"); break;
        }
        break;
      }
    }
  }

}


/**------------------------------------------------------------------------**
 **  LOAD  **  JPEG  **
 **------------------------------------------------------------------------**/


BOOL LoadJPEG(void)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;
  struct ImageData *LoadImage;

  JSAMPROW linebuf = NULL;
  int y;

  BOOL success=FALSE;


  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo,file_load);
  jpeg_read_header(&cinfo,TRUE);

  cinfo.out_color_space = JCS_RGB;

  ShowMessage(Txt(TXT_LOADING_JPEG));

  jpeg_start_decompress(&cinfo);

  linebuf=malloc(cinfo.output_width*3);

  LoadImage=GetBuffer(cinfo.output_width,cinfo.output_height);
  if (!LoadImage) { SetError(INPUT_FILE_ERROR); goto errexit; }

  SetBufferMode(LoadImage,RGB);


  for (y=0;y<cinfo.output_height;y++)
  {
    union Pixel *pixrow;
    int x;
    JSAMPROW p = linebuf;

    pixrow=LockAndGetLine(LoadImage,y);
    if (! pixrow)
    {
      SetError(INPUT_FILE_ERROR);
      goto errexit;
    }

    /* progress indication */

    ShowProgress(y+1,cinfo.output_height);

    jpeg_read_scanlines(&cinfo,&linebuf,1);

    for (x=0;x<cinfo.output_width;x++)
    {
      pixrow[x].rgb.r = (*p++);
      pixrow[x].rgb.g = (*p++);
      pixrow[x].rgb.b = (*p++);
    }

    UnlockLine(LoadImage,y);
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);

  SetDefaultBuffer(LoadImage);
  success=TRUE;

errexit:
  if (linebuf) free(linebuf);

  return success;
}

/*---------------------------------------- get Image-Props -------------------*/

void PropsJPEG(void)
{
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  jpeg_stdio_src(&cinfo,file_load);
  jpeg_read_header(&cinfo,TRUE);

  jpeg_calc_output_dimensions(&cinfo);

  Output_Width   = cinfo.output_width;
  Output_Height  = cinfo.output_height;

  Output_nColors = 256;  /* well, actually, it's 24bit */
  Output_Mode    = GFXMOD_24BIT;

  jpeg_destroy_decompress(&cinfo);
}


/**------------------------------------------------------------------------**
 **  SAVE  **  JPEG  **
 **------------------------------------------------------------------------**/

static struct Handle *ha;

#define ID_JPEG_QUALITY 1

static ULONG LeaveWindow(void)
{
  return EXIT_EXITWINDOW;
}

static void GetOutputSettings(void)
{
  if (CLImode) { return; }

  if (BeginNewHandleTree())
  {
    ha=CrSmallWindow(
        CrSpaceBox(
          CrSpaceVBox(
            CrSpaceRaster(3,
              CrText(Txt(TXT_JPEG_QUALITY),TAG_DONE),
              CrGadget(GAGA_Kind       ,STRING_KIND,
                       GAGA_IN_Ptr     ,&Output_JPEG_Quality,
                       GAGA_CharsWidth ,3,
                       GAGA_ID         ,ID_JPEG_QUALITY,
                       GAGA_UpperBound ,100,
                       GAGA_LowerBound ,25,
                       GAGA_NumberBytes,1,
                       GAGA_CallFunc   ,LeaveWindow,
                       TAG_DONE,
                       TAG_DONE),
              CrText("%",TAG_DONE),

              HANDLE_END
            ),
            CrCBGadget("progressive",
                       GAGA_Kind,CHECKBOX_KIND,
                       GAGA_CB_Ptr,&Output_JPEG_Progressive,
                       TAG_DONE,TAG_DONE),

            HANDLE_END
          )
        ),
        WAWA_ActiveGad,ID_JPEG_QUALITY,
        WAWA_Centered ,h,
        TAG_DONE,
        WA_Activate   ,TRUE,
        WA_CloseGadget,TRUE,
        WA_Title      ,Txt(TXT_JPEG_PARAMETERS),
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




BOOL SaveJPEG(void)
{
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;
  int x,y;
  int w,h;
  JSAMPROW linebuf = NULL;
  BOOL success=FALSE;
  struct ImageData *OutputImage;

  OutputImage=GetDefaultBuffer();

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_compress(&cinfo);
  jpeg_stdio_dest(&cinfo,file_save);

  if (!ConvertToMode(0,(GetOutputFlags() & ~MODE_FLAGS) | GFXMOD_24BIT)) return FALSE;

  ShowMessage(Txt(TXT_SAVING_JPEG));

  GetOutputSettings();

  cinfo.image_width  = w = GetImageWidth (OutputImage);
  cinfo.image_height = h = GetImageHeight(OutputImage);
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;


  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo,Output_JPEG_Quality,TRUE);

  if (Output_JPEG_Progressive)
  {
    jpeg_simple_progression(&cinfo);
    // cinfo.optimize_coding = TRUE;
  }

  linebuf = malloc(w*3);


  jpeg_start_compress(&cinfo,TRUE);
  for (y=0;y<h;y++)
  {
    JSAMPROW ptr;

    ptr = linebuf;

    for (x=0;x<w;x++)
    {
      if (!GetRGBPixel(OutputImage,x,y,ptr,ptr+1,ptr+2))
      { goto savejpegexit; }

      ptr+=3;
    }

    jpeg_write_scanlines(&cinfo,&linebuf,1);

    ShowProgress(y,h-1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  success=TRUE;

savejpegexit:
  if (linebuf) free(linebuf);

  return success;
}

