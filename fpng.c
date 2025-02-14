
/********************************************************************************
 *
 * modul name:  fpng.c
 *
 * contents:    routines to load and save PNG files
 *              (needs libpng)
 *
 *
 * to do:
 *
 *
 * v1.0 (10.07.99)
 *   PNG file loading
 *
 *
 * © Dirk Farin
 *
 ********************************************************************************/


#include "global.h"
#include "options.h"
// #include <setjmp.h>



#include "png:png.h"

void CheckForPNG(form fo)
{
  BOOL     isPNG;
  png_byte sigbuf[8];

  ReadBlock(sigbuf,8);
  isPNG = !png_sig_cmp(sigbuf,0,8);

  fo->not_valid_format = !isPNG;
}


void InfoPNG(void)
{
  png_structp pngptr  = NULL;
  png_infop   pnginfo = NULL;
  png_infop   endinfo = NULL;

  pngptr  = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if (!pngptr)  goto infoexit;
  pnginfo = png_create_info_struct(pngptr);
  if (!pnginfo) goto infoexit;
  endinfo = png_create_info_struct(pngptr);
  if (!endinfo) goto infoexit;

  if (setjmp(pngptr->jmpbuf))
  {
    goto infoexit;
  }

  png_init_io(pngptr,file_load);
  png_read_info(pngptr,pnginfo);

  {
  char buffer[100];
  png_uint_32 w,h;
  int depth,colortype,interlacetype,compressiontype,filtertype;

  png_get_IHDR(pngptr,pnginfo,&w,&h,&depth,&colortype,&interlacetype,&compressiontype,&filtertype);

  sprintf(buffer,"image dimensions: %dx%d",w,h); ShowInfo(buffer);
  sprintf(buffer,"bit depth: %d",depth); ShowInfo(buffer);

  strcpy(buffer,"colormode: ");
  switch (colortype)
  {
  case PNG_COLOR_TYPE_GRAY:       strcat(buffer,"gray"); break;
  case PNG_COLOR_TYPE_GRAY_ALPHA: strcat(buffer,"gray + alpha"); break;
  case PNG_COLOR_TYPE_PALETTE:    strcat(buffer,"palette"); break;
  case PNG_COLOR_TYPE_RGB:        strcat(buffer,"rgb"); break;
  case PNG_COLOR_TYPE_RGB_ALPHA:  strcat(buffer,"rgb + alpha"); break;
  }
  ShowInfo(buffer);

  strcpy(buffer,"interlace: ");
  switch (interlacetype)
  {
  case PNG_INTERLACE_NONE:   strcat(buffer,"none"); break;
  case PNG_INTERLACE_ADAM7:  strcat(buffer,"adam-7"); break;
  default: strcat(buffer,"unknown"); break;
  }
  ShowInfo(buffer);
  }

  //png_read_end(pngptr,pnginfo);

infoexit:
  png_destroy_read_struct(&pngptr,&pnginfo,&endinfo);
}


/**------------------------------------------------------------------------**
 **  LOAD  **  PNG  **
 **------------------------------------------------------------------------**/


BOOL LoadPNG(void)
{
  png_structp pngptr  = NULL;
  png_infop   pnginfo = NULL;
  png_infop   endinfo = NULL;
  BOOL success=FALSE;
  png_uint_32 w,h;
  int depth,colortype,interlacetype,compressiontype,filtertype;
  int passes;
  struct ImageData *LoadImage;

  pngptr  = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if (!pngptr)  goto loadexit;
  pnginfo = png_create_info_struct(pngptr);
  if (!pnginfo) goto loadexit;
  endinfo = png_create_info_struct(pngptr);
  if (!endinfo) goto loadexit;

  if (setjmp(pngptr->jmpbuf))
  {
printf("asd\n");
    goto loadexit;
  }

  png_init_io(pngptr,file_load);
  png_read_info(pngptr,pnginfo);

  png_get_IHDR(pngptr,pnginfo,&w,&h,&depth,&colortype,&interlacetype,&compressiontype,&filtertype);



  /*if (depth<8)*/ png_set_expand(pngptr);

printf("a1\n");
  if (depth==16) png_set_strip_16(pngptr);


printf("a2\n");
  if (colortype & PNG_COLOR_MASK_ALPHA)
    png_set_strip_alpha(pngptr);

printf("a3\n");
  if (depth==8 &&
      (colortype==PNG_COLOR_TYPE_RGB ||
       colortype==PNG_COLOR_TYPE_RGB_ALPHA))
    png_set_filler(pngptr,0,PNG_FILLER_AFTER);

printf("a4\n");
  if (colortype == PNG_COLOR_TYPE_GRAY ||
      colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(pngptr);

printf("a5\n");
  //passes=png_set_interlace_handling(pngptr);

printf("a6\n");
  png_read_update_info(pngptr,pnginfo);

printf("a7\n");
  LoadImage=GetBuffer(w,h);
  if (!LoadImage) { SetError(INPUT_FILE_ERROR); goto loadexit; }

printf("a8\n");
  if (colortype==PNG_COLOR_TYPE_PALETTE)
  {
    int bpl = png_get_rowbytes(pngptr,pnginfo);
    png_bytep* rows = malloc(h*sizeof(png_bytep));
    png_bytep  image = malloc(h*bpl*4);
    int x,y;
    int i;
    png_colorp col;
    int nCol;
    struct ColorLookupTable* clut;

printf("a9 %d %d %d\n",h,bpl,h*bpl);
    for (y=0;y<h;y++)
      rows[y] = malloc(bpl);

printf("a10\n");
    SetBufferMode(LoadImage,CLUT);

printf("PALETTE\n");

    png_get_PLTE(pngptr,pnginfo,&col,&nCol);
    clut = GetCLUT(nCol);
    AttachCLUT(clut,LoadImage);

    for (i=0;i<nCol;i++)
    {
      printf("%d: %d %d %d\n",i,col[i].red,col[i].green,col[i].blue);
      SetColor(clut,i,col[i].red,col[i].green,col[i].blue);
    }

printf("qwe1\n");

    png_read_image(pngptr,rows);

printf("qwe2\n");

    for (y=0;y<h;y++)
      for (x=0;x<w;x++)
        {
        printf("%d\n",rows[y][x]);
        SetPixel(LoadImage,x,y,rows[y][x]);
        }

    free(image);
    free(rows);
  }
  else
  {
    png_bytep* rows = malloc(h*sizeof(png_bytep));
    int i;
    int p;

    SetBufferMode(LoadImage,RGB);

    for (i=0;i<h;i++)
      rows[i] = (unsigned char*)LockAndGetLine(LoadImage,i);

    png_read_image(pngptr,rows);

    free(rows);

    for (i=0;i<h;i++)
      UnlockLine(LoadImage,i);
  }

  //png_read_end(pngptr,pnginfo);

  SetDefaultBuffer(LoadImage);
  success=TRUE;
loadexit:
  png_destroy_read_struct(&pngptr,&pnginfo,&endinfo);

  return success;


#if 0
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
#endif
}

/*---------------------------------------- get Image-Props -------------------*/

void PropsPNG(void)
{
  png_structp pngptr  = NULL;
  png_infop   pnginfo = NULL;
  png_infop   endinfo = NULL;

  pngptr  = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
  if (!pngptr)  goto propexit;
  pnginfo = png_create_info_struct(pngptr);
  if (!pnginfo) goto propexit;
  endinfo = png_create_info_struct(pngptr);
  if (!endinfo) goto propexit;

  if (setjmp(pngptr->jmpbuf))
  {
    goto propexit;
  }

  png_init_io(pngptr,file_load);
  png_read_info(pngptr,pnginfo);

  {
  png_uint_32 w,h;
  int depth,colortype,interlacetype,compressiontype,filtertype;

  png_get_IHDR(pngptr,pnginfo,&w,&h,&depth,&colortype,&interlacetype,&compressiontype,&filtertype);

  Output_Width   = w;
  Output_Height  = h;

  Output_nColors = 1<<depth;

  switch (colortype)
  {
  case PNG_COLOR_TYPE_GRAY:
  case PNG_COLOR_TYPE_GRAY_ALPHA:
  case PNG_COLOR_TYPE_PALETTE:
    Output_Mode    = GFXMOD_CLUT;
    break;
  case PNG_COLOR_TYPE_RGB:
  case PNG_COLOR_TYPE_RGB_ALPHA:
    Output_Mode    = GFXMOD_24BIT;
    break;
  }
  }

  //png_read_end(pngptr,pnginfo);

propexit:
  png_destroy_read_struct(&pngptr,&pnginfo,&endinfo);
}


/**------------------------------------------------------------------------**
 **  SAVE  **  PNG  **
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




BOOL SavePNG(void)
{
#if 0
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
#endif
}

