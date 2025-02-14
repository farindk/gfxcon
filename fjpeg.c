
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
#include <setjmp.h>

#undef GLOBAL /* suppress warning */
#include "jinclude.h"

extern short JPEGQuality;   // only CLI-input !!!

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

FILE *tmpfile(void)
{
//  return fopen("sys:gfxtmpfile","w");
  FILE* dummy=0;
  assert(0);
  return dummy;
}

/**------------------------------------------------------------------------**
 **  LOAD  **  JPEG  **
 **------------------------------------------------------------------------**/

static jmp_buf load_JPEG_jmpbuf;
static FirstRow_NextTime;
static struct ImageData *LoadImage;
static external_methods_ptr emethods;

METHODDEF void TraceMessageToNIL(const char *msgtext)
{
  ;  /* do nothing (we don't like trace-messages) ! */
}

METHODDEF void error_exit(const char *msgtext)
{
  if (LastError() == OK) SetError(COULDNT_CONVERT_FROM_JPEG);

  (*emethods->free_all)();
  longjmp(load_JPEG_jmpbuf,1);
}

METHODDEF void output_init(decompress_info_ptr cinfo)
{
  LoadImage=GetBuffer(cinfo->image_width,cinfo->image_height);
  if (!LoadImage) longjmp(load_JPEG_jmpbuf,1);

  SetBufferMode(LoadImage,RGB);
}

METHODDEF void WeDONT_need_color_reduction(decompress_info_ptr d1,int d2,JSAMPARRAY d3)
{
  assert(0);
}

METHODDEF void put_pixel_rows(decompress_info_ptr cinfo,
                              int                 num_rows,
                              JSAMPIMAGE          pixel_data)
{
  int row,col;
  JSAMPROW ptr0,ptr1,ptr2;

  union Pixel *pixrow;

  for (row=0;row<num_rows;row++)
  {
    ptr0=pixel_data[0][row];
    ptr1=pixel_data[1][row];
    ptr2=pixel_data[2][row];

    pixrow=LockAndGetLine(LoadImage,FirstRow_NextTime+row);
    if (! pixrow)
    {
      ERREXIT(cinfo->emethods,NULL);
    }

    /* progress indication */

    ShowProgress(FirstRow_NextTime+row,cinfo->image_height);

    for (col=0;col<cinfo->image_width;col++)
    {
      pixrow[col].rgb.r = (*ptr0);
      pixrow[col].rgb.g = (*ptr1);
      pixrow[col].rgb.b = (*ptr2);

      ptr0++;
      ptr1++;
      ptr2++;
    }

    UnlockLine(LoadImage,FirstRow_NextTime+row);
  }

  FirstRow_NextTime += num_rows;
}

METHODDEF void output_term(decompress_info_ptr cinfo)
{
  ;
}

METHODDEF void d_ui_method_selection(decompress_info_ptr cinfo)
{
  ;
}

BOOL LoadJPEG(void)
{
  struct Decompress_info_struct cinfo;
  struct Decompress_methods_struct dc_methods;
  struct External_methods_struct e_methods;


  ShowMessage(Txt(TXT_LOADING_JPEG));

  cinfo.input_file  = file_load;
  cinfo.output_file = NULL;

  cinfo.methods  = &dc_methods;
  cinfo.emethods = &e_methods;

  emethods = &e_methods;
  e_methods.error_exit = error_exit;
  e_methods.trace_message = TraceMessageToNIL;
  e_methods.trace_level = 0;
  e_methods.num_warnings = 0;
  e_methods.first_warning_level = 0;
  e_methods.more_warning_level = 3;

  cinfo.methods->output_init    = output_init;
  cinfo.methods->put_color_map  = WeDONT_need_color_reduction;
  cinfo.methods->put_pixel_rows = put_pixel_rows;
  cinfo.methods->output_term    = output_term;

  LoadImage=NULL;
  FirstRow_NextTime = 0;


  if (setjmp(load_JPEG_jmpbuf))
  {
    if (LoadImage) FreeBuffer(LoadImage);
    return FALSE;
  }

  jselmemmgr(&e_methods);
  dc_methods.d_ui_method_selection = d_ui_method_selection;
  j_d_defaults(&cinfo,TRUE);
  jselrjfif(&cinfo);

  jpeg_decompress(&cinfo);

  SetDefaultBuffer(LoadImage);
  return TRUE;
}

/*---------------------------------------- get Image-Props -------------------*/

static jmp_buf Props_JPEG_jmpbuf;
METHODDEF void DummyFunc1(unsigned char const* dummy) { }
METHODDEF void DummyFunc2(decompress_info_ptr d1,int d2,JSAMPARRAY d3) { }
METHODDEF void DummyFunc3(decompress_info_ptr d1,int d2,JSAMPIMAGE d3) { }
METHODDEF void DummyFunc4(decompress_info_ptr d1) { }
METHODDEF void DummyFunc5(unsigned char const* d1) { }

METHODDEF void Throw_Props_Error(decompress_info_ptr cinfo)
{
  ERREXIT(cinfo->emethods,NULL);
}

METHODDEF void Props_Error_exit(unsigned char const* dummyarg)
{
  (*emethods->free_all)();
  longjmp(Props_JPEG_jmpbuf,1);
}

void PropsJPEG(void)
{
  struct Decompress_info_struct cinfo;
  struct Decompress_methods_struct dc_methods;
  struct External_methods_struct e_methods;


  cinfo.input_file  = file_load;
  cinfo.output_file = NULL;

  cinfo.methods  = &dc_methods;
  cinfo.emethods = &e_methods;

  emethods = &e_methods;
  e_methods.error_exit = Props_Error_exit;
  e_methods.trace_message = DummyFunc1;
  e_methods.trace_level = 0;
  e_methods.num_warnings = 0;
  e_methods.first_warning_level = 0;
  e_methods.more_warning_level = 3;

  cinfo.methods->output_init    = Throw_Props_Error;
  cinfo.methods->put_color_map  = DummyFunc2;
  cinfo.methods->put_pixel_rows = DummyFunc3;
  cinfo.methods->output_term    = DummyFunc4;


  if (setjmp(Props_JPEG_jmpbuf))
  {
    Output_Width   = cinfo.image_width;
    Output_Height  = cinfo.image_height;

    Output_nColors = 256;  /* well, actually, it's 24bit */
    Output_Mode    = GFXMOD_24BIT;
    return;
  }


  jselmemmgr(&e_methods);
  dc_methods.d_ui_method_selection = d_ui_method_selection;
  j_d_defaults(&cinfo,TRUE);
  jselrjfif(&cinfo);


  jpeg_decompress(&cinfo);

  assert(0);
}


/**------------------------------------------------------------------------**
 **  SAVE  **  JPEG  **
 **------------------------------------------------------------------------**/

static struct Handle *ha;
static UBYTE  JPEG_Quality=75;

#define ID_JPEG_QUALITY 1

static ULONG LeaveWindow(void)
{
  return EXIT_EXITWINDOW;
}

static void GetOutputSettings(void)
{
  if (JPEGQuality) { JPEG_Quality=JPEGQuality; return; }

  if (BeginNewHandleTree())
  {
    ha=CrSmallWindow(
        CrSpaceBox(
          CrSpaceRaster(3,
            CrText(Txt(TXT_JPEG_QUALITY),TAG_DONE),
            CrGadget(GAGA_Kind       ,STRING_KIND,
                     GAGA_IN_Ptr     ,&JPEG_Quality,
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

static int NextRowToRead;
static struct ImageData *OutputImage;
static external_methods_ptr emethods_save;
static jmp_buf JPEG_save_jmpbuf;

METHODDEF void input_init(compress_info_ptr cinfo)
{
  OutputImage=GetDefaultBuffer();

  cinfo->image_width  = GetImageWidth (OutputImage);
  cinfo->image_height = GetImageHeight(OutputImage);

  cinfo->input_components = 3;
  cinfo->in_color_space = CS_RGB;
  cinfo->data_precision = 8;

  NextRowToRead = 0;
}


METHODDEF void get_input_row(compress_info_ptr cinfo, JSAMPARRAY pixel_row)
{
  int col;
  JSAMPROW ptr0,ptr1,ptr2;
  UBYTE r,g,b;

  ptr0 = pixel_row[0];
  ptr1 = pixel_row[1];
  ptr2 = pixel_row[2];

  for (col=0;col<GetImageWidth(OutputImage);col++)
  {
    if (!GetRGBPixel(OutputImage,col,NextRowToRead,&r,&g,&b))
    { ERREXIT(cinfo->emethods,NULL); }

    *ptr0++ = r;
    *ptr1++ = g;
    *ptr2++ = b;
  }

  ShowProgress(NextRowToRead,cinfo->image_height-1);

  NextRowToRead++;
}


METHODDEF void input_term(compress_info_ptr cinfo) { }

METHODDEF void c_ui_method_selection(compress_info_ptr cinfo)
{
  jselwjfif(cinfo);
}

METHODDEF void error_exit_save(const char *msgtext)
{
  if (LastError() == OK) SetError(COULDNT_CONVERT_TO_JPEG);

  (*emethods_save->free_all)();
  longjmp(JPEG_save_jmpbuf,1);
}

BOOL SaveJPEG(void)
{
  struct Compress_info_struct cinfo;
  struct Compress_methods_struct c_methods;
  struct External_methods_struct e_methods;

  if (!ConvertToMode(0,(GetOutputFlags() & ~MODE_FLAGS) | GFXMOD_24BIT)) return FALSE;

  ShowMessage(Txt(TXT_SAVING_JPEG));

  cinfo.methods  = &c_methods;
  cinfo.emethods = &e_methods;

  emethods_save = &e_methods;
  e_methods.error_exit          = error_exit_save;
  e_methods.trace_message       = DummyFunc5;
  e_methods.trace_level         = 0;
  e_methods.num_warnings        = 0;
  e_methods.first_warning_level = 0;
  e_methods.more_warning_level  = 3;

  if (setjmp(JPEG_save_jmpbuf))
  {
    return FALSE;
  }

  jselmemmgr(&e_methods);

  c_methods.input_init = input_init;
  c_methods.get_input_row = get_input_row;
  c_methods.input_term = input_term;
  c_methods.c_ui_method_selection = c_ui_method_selection;

  GetOutputSettings();

  j_c_defaults(&cinfo, JPEG_Quality, FALSE);

  cinfo.input_file = NULL;

  cinfo.output_file = file_save;

  jpeg_compress(&cinfo);

  return TRUE;
}

