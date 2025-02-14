/*******************************************************************************
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
#include "options.h"

extern char Loadname  [];
extern char Loadname_g[];
extern char Loadname_b[];
extern char Savename  [];
extern char Savename_g[];
extern char Savename_b[];

extern ULONG loadmode   ,savemode;
extern BOOL  Load_is_RGB,Save_is_RGB;

extern void DoConversionMain(void);
extern void CheckLoadFormats(void);

extern struct FormatObject FormatObjs[];

ULONG inputwidth;
BOOL  noprogress;

short NewBackgroundColor;



static void usage(const char* prgname)
{
  printf("usage: %s inputfile [TO filename] [FORMAT f] [COLORS c] [FLIPX] [...]\n"
         "\n"
         "TO destname   set destination filename\n"
         "FORMAT        output format { ILBM,GIF,PCX,JPEG,Postscript,RGB-Raw }\n"
         "\n"
         "COLORS n      use 'n' colors max.\n"
         "UNUSED r g b  fill empty CLUT entries with (r,g,b)\n"
         "OFFSET n      don't use first 'n' colors of CLUT (only ILBM with CLUT !)\n"
         "SORTDIR n     sort CLUT-color (n=1 darkest to brightest / n=-1 resp.)\n"
         "BACKGROUND n  use color 'n' as background-color (only GIF and ILBM)\n"
         "DITHER        use floyd-steinberg dithering\n"
         "FASTDITHER    use fast floyd dithering\n"
         "\n"
         "CROP x1 y1 x2 y2 crops the image to the specified rectangle\n"
         "RESIZE f         proportionally resize image with factor f (>0)\n"
         "RESIZEH f        resize image horizontally with factor f (>0)\n"
         "RESIZEV f        resize image vertically factor f (>0)\n"
         "SIZE w h         resize image\n"
         "BOXFIT w h       proportionally shrink(!) image to fit into the box specified\n"
         "BOXFITALL w h    same as BOXFIT, but enlarges image, if smaller than the box\n"
         "CENTERBOX width height r g b   centers the image to the box and fills the\n"
         "                               border with the color (r,g,b)\n"
         "\n"
         "QUALITY q     set JPEG-quality\n"
         "PROGRESSIVE   save JPEG in progressive mode\n"
         "CLUT          select IFF-CLUT  mode\\\n"
         "HAM                  IFF-HAM   mode \\_ use only with 'FORMAT ILBM'\n"
         "HAM8                 IFF-HAM8  mode /\n"
         "24BIT                IFF-24bit mode/\n"
         "\n"
         "INVERS        invert colors\n"
         "BW            create black and white only\n"
         "GRAYSCALE     produce a grayscale (not simply BW!) output\n"
         "NORED         remove red\n"
         "NOGREEN       remove green\n"
         "NOBLUE        remove blue\n"
         "\n"
         "BRIGHTNESS n  change brightness by n (255 to shift black to white)\n"
         "CONTRAST n    change contrast by n (percent)\n"
         "\n"
         "FLIPX         flip horizontally\n"
         "FLIPY         flip vertically\n"
         "ROTATELEFT    rotate left (90 degrees)\n"
         "ROTATERIGHT   rotate right (90 degrees)\n"
         "\n"
         "GREEN n       set name of green-RGB-raw-file to n (RGB-Raw only)\n"
         "BLUE n        set name of blue -RGB-raw-file to n (RGB-Raw only)\n"
         "INPUTWIDTH n  force input-width to be 'n' pixels\n"
         "\n"
         "INFO          show image-information only\n"
         "NOPROGRESS    don't show how much the conversion has progressed\n"
         "HELP / -help / -h / ? / -?   this help-page :-)\n"
        );
}


//static int  outputcolors = 0;
static int  newwidth,newheight;
static BOOL showinfo = FALSE;
static BOOL explicit_savename = FALSE;
static BOOL hasgreen,hasblue;

int clistart(int argc,char **argv)
{
  int argcnt;
  int i;
  struct FormatObject* fo;

  printf("----------------------------------------------------------------------------\n"
         " GfxCon V1.9                        © Dirk Farin / farin@ti.uni-mannheim.de\n"
         "----------------------------------------------------------------------------\n");

  if (argc>=2)
  {
    if (stricmp("-h"   ,argv[1])==0 ||
        stricmp("-help",argv[1])==0 ||
        stricmp("help" ,argv[1])==0 ||
        stricmp("?"    ,argv[1])==0 ||
        stricmp("-?"   ,argv[1])==0    )
    {
      usage(argv[0]);
      return 0;
    }
  }

  argcnt = 1;

//  Load_is_RGB = FALSE;
//  Save_is_RGB = FALSE;

  NewBackgroundColor = -1;

  strcpy(Loadname,argv[argcnt]);
  strcpy(Savename,argv[argcnt]);

  argcnt++;

  CheckLoadFormats();  // setzt auch "Load_is_RGB"

  if (loadmode==FORM_RGB) { Load_is_RGB = TRUE; }

  printf("input size: %d x %d\n",Output_Width,Output_Height);


  savemode = loadmode;  // per Voreinstellung

  Output_DoCrop      = FALSE;
  Output_DoCenterBox = FALSE;
  Output_ColorOffset = 0;
  Output_SortCLUT    = 0;


  while (argcnt<argc)
  {
    if (stricmp("FORMAT",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc)
      {
        printf("FORMAT-parameter missing !\n");
        return 100;
      }

      for (i=0;FormatObjs[i].FormatName;i++)
      {
        if (stricmp(argv[argcnt],FormatObjs[i].FormatName)==0)
        {
          fo = &FormatObjs[i];
          if (fo->SavePict == NULL) { printf("can't save this format !\n"); return 100; }
          savemode = fo->FormatID;
          goto formatfound;
        }
      }

      printf("unknown output format !!!\n");
      return 100;

formatfound:
      ;

      // --- neue Endung an Savenamen dranhängen, wenn er nicht explizit geändert wurde

      if (!explicit_savename)
      {
        for (i=strlen(Savename)-1;i>=0;i--)
        {
          if (Savename[i]=='.')
          {
            int j;
            for (j=0;fo->suffix[j];j++)
            {
              Savename[i+1+j] = fo->suffix[j];
            }
            Savename[i+1+j] = 0;
            goto suffixexchanged;
          }
        }

        if (savemode != FORM_RGB) strcat(Savename,".");
                                  strcat(Savename,fo->suffix);
      }

suffixexchanged:
      ;

      argcnt++;
    }
    else
    if (stricmp("COLORS",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("COLORS-parameter missing !\n"); return 100; }

      //outputcolors = atoi(argv[argcnt]);
      Output_nColors = atoi(argv[argcnt]);

      argcnt++;
    }
    else
    if (stricmp("OFFSET",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("OFFSET-parameter missing !\n"); return 100; }

      Output_ColorOffset = atoi(argv[argcnt]);
      Output_nColors -= Output_ColorOffset;

      argcnt++;
    }
    else
    if (stricmp("SORTDIR",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("SORTDIR-parameter missing !\n"); return 100; }

      Output_SortCLUT = atoi(argv[argcnt]);

      argcnt++;
    }
    else
    if (stricmp("BACKGROUND",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("BACKGROUND-parameter missing !\n"); return 100; }

      NewBackgroundColor = atoi(argv[argcnt]);
      if (NewBackgroundColor > 255 || NewBackgroundColor<0) NewBackgroundColor=-1;

      argcnt++;
    }
    else
    if (stricmp("BRIGHTNESS",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("BRIGHTNESS-parameter missing !\n"); return 100; }

      Output_Brightness = atoi(argv[argcnt]);

      argcnt++;
    }
    else
    if (stricmp("CONTRAST",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("CONTRAST-parameter missing !\n"); return 100; }

      Output_Contrast = atoi(argv[argcnt]);

      argcnt++;
    }
    else
    if (stricmp("DITHER",argv[argcnt])==0)
    {
      Output_Dither |= GFXMOD_FLOYD;
      argcnt++;
    }
    else
    if (stricmp("FASTDITHER",argv[argcnt])==0)
    {
      Output_Dither |= GFXMOD_FASTFLOYD;
      argcnt++;
    }
    else
    if (stricmp("GRAYSCALE",argv[argcnt])==0)
    {
      Output_Dither |= GFXMOD_GRAYSCALE;
      argcnt++;
    }
    else
    if (stricmp("CLUT",argv[argcnt])==0)
    {
      Output_Mode = GFXMOD_CLUT;
      argcnt++;
    }
    else
    if (stricmp("HAM",argv[argcnt])==0)
    {
      Output_Mode = GFXMOD_HAM;
      argcnt++;
    }
    else
    if (stricmp("24BIT",argv[argcnt])==0)
    {
      Output_Mode = GFXMOD_24BIT;
      argcnt++;
    }
    else
    if (stricmp("HAM8",argv[argcnt])==0)
    {
      Output_Mode = GFXMOD_HAM8;
      argcnt++;
    }
    else
    if (stricmp("INVERS",argv[argcnt])==0)
    {
      Output_ColorEffects |= GFXMOD_INVERS;
      argcnt++;
    }
    else
    if (stricmp("BW",argv[argcnt])==0)
    {
      Output_ColorEffects |= GFXMOD_BW;
      argcnt++;
    }
    else
    if (stricmp("NORED",argv[argcnt])==0)
    {
      Output_ColorEffects |= GFXMOD_NO_R;
      argcnt++;
    }
    else
    if (stricmp("NOGREEN",argv[argcnt])==0)
    {
      Output_ColorEffects |= GFXMOD_NO_G;
      argcnt++;
    }
    else
    if (stricmp("NOBLUE",argv[argcnt])==0)
    {
      Output_ColorEffects |= GFXMOD_NO_B;
      argcnt++;
    }
    else
    if (stricmp("FLIPX",argv[argcnt])==0)
    {
      Output_FlipFlags |= GFXMOD_FLIPX;
      argcnt++;
    }
    else
    if (stricmp("FLIPY",argv[argcnt])==0)
    {
      Output_FlipFlags |= GFXMOD_FLIPY;
      argcnt++;
    }
    else
    if (stricmp("ROTATELEFT",argv[argcnt])==0)
    {
      Output_RotateFlags |= GFXMOD_ROTATE_L;
      argcnt++;
    }
    else
    if (stricmp("ROTATERIGHT",argv[argcnt])==0)
    {
      Output_RotateFlags |= GFXMOD_ROTATE_R;
      argcnt++;
    }
    else
    if (stricmp("NOPROGRESS",argv[argcnt])==0)
    {
      noprogress=TRUE;
      argcnt++;
    }
    else
    if (stricmp("TO",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("TO-parameter missing !\n"); return 100; }

      strcpy(Savename,argv[argcnt]);
      explicit_savename = TRUE;

      argcnt++;
    }
    else
    if (stricmp("GREEN",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("GREEN-parameter missing !\n"); return 100; }
      if (!Load_is_RGB) { printf("No RGB input, redundant GREEN-parameter !\n"); return 100; }

      strcpy(Loadname_g,argv[argcnt]);
      hasgreen = TRUE;

      argcnt++;
    }
    else
    if (stricmp("BLUE",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("BLUE-parameter missing !\n"); return 100; }
      if (!Load_is_RGB) { printf("No RGB input, redundant BLUE-parameter !\n"); return 100; }

      strcpy(Loadname_b,argv[argcnt]);
      hasblue = TRUE;

      argcnt++;
    }
    else
    if (stricmp("QUALITY",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("QUALITY-parameter missing !\n"); return 100; }

      Output_JPEG_Quality = atoi(argv[argcnt]);
      if (Output_JPEG_Quality<25)  Output_JPEG_Quality=25;
      if (Output_JPEG_Quality>100) Output_JPEG_Quality=100;

      argcnt++;
    }
    else
    if (stricmp("PROGRESSIVE",argv[argcnt])==0)
    {
      Output_JPEG_Progressive = TRUE;
      argcnt++;
    }
    else
    if (stricmp("INPUTWIDTH",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("INPUTWIDTH-parameter missing !\n"); return 100; }

      inputwidth = atoi(argv[argcnt]);

      argcnt++;
    }
    else
    if (stricmp("SIZE",argv[argcnt])==0)
    {
      Output_DoSize = TRUE;

      argcnt++;
      if (argcnt>=argc) { printf("first SIZE-parameter missing !\n"); return 100; }

      Output_NewWidth = atoi(argv[argcnt]);

      argcnt++;
      if (argcnt>=argc) { printf("second SIZE-parameter missing !\n"); return 100; }

      Output_NewHeight = atoi(argv[argcnt]);

      argcnt++;
    }
    else
    if (stricmp("CROP",argv[argcnt])==0)
    {
      Output_DoCrop = TRUE;

      argcnt++; if (argcnt>=argc) { printf("first CROP-parameter missing !\n"); return 100; }
      Output_Crop_x1 = atoi(argv[argcnt]);

      argcnt++; if (argcnt>=argc) { printf("second CROP-parameter missing !\n"); return 100; }
      Output_Crop_y1 = atoi(argv[argcnt]);

      argcnt++; if (argcnt>=argc) { printf("third CROP-parameter missing !\n"); return 100; }
      Output_Crop_x2 = atoi(argv[argcnt]);

      argcnt++; if (argcnt>=argc) { printf("fourth CROP-parameter missing !\n"); return 100; }
      Output_Crop_y2 = atoi(argv[argcnt]);

      argcnt++;

      if (Output_Crop_x1 > Output_Crop_x2)
      {
        ULONG x = Output_Crop_x1;
        Output_Crop_x1 = Output_Crop_x2;
        Output_Crop_x2 = x;
      }

      if (Output_Crop_y1 > Output_Crop_y2)
      {
        ULONG y = Output_Crop_y1;
        Output_Crop_y1 = Output_Crop_y2;
        Output_Crop_y2 = y;
      }

      if (Output_Crop_x1 < 0 || Output_Crop_y1 < 0 ||
          Output_Crop_x2 >= Output_Width || Output_Crop_y2 >= Output_Height)
      {
        printf("Invalid CROP rectangle. Maximum allowed range is: 0,0 - %d,%d\n",Output_Width-1,Output_Height-1);
        return 100;
      }
    }
    else
    if (stricmp("CENTERBOX",argv[argcnt])==0)
    {
      Output_DoCenterBox = TRUE;

      argcnt++; if (argcnt>=argc) { printf("first CENTERBOX-parameter missing !\n"); return 100; }
      Output_CenterBox_Width = atoi(argv[argcnt]);

      argcnt++; if (argcnt>=argc) { printf("second CENTERBOX-parameter missing !\n"); return 100; }
      Output_CenterBox_Height = atoi(argv[argcnt]);

      argcnt++; if (argcnt>=argc) { printf("third CENTERBOX-parameter missing !\n"); return 100; }
      Output_CenterBox_R = atoi(argv[argcnt]);

      argcnt++; if (argcnt>=argc) { printf("fourth CENTERBOX-parameter missing !\n"); return 100; }
      Output_CenterBox_G = atoi(argv[argcnt]);

      argcnt++; if (argcnt>=argc) { printf("fifth CENTERBOX-parameter missing !\n"); return 100; }
      Output_CenterBox_B = atoi(argv[argcnt]);

      argcnt++;
    }
    else
    if (stricmp("BOXFIT",argv[argcnt])==0 || stricmp("BOXFITALL",argv[argcnt])==0)
    {
      BOOL boxfit_all = (stricmp("BOXFITALL",argv[argcnt])==0);

      Output_DoBoxfit = TRUE;

      argcnt++;
      if (argcnt>=argc) { printf("first BOXFIT-parameter missing !\n"); return 100; }

      Output_BoxfitWidth = atoi(argv[argcnt]);

      argcnt++;
      if (argcnt>=argc) { printf("second BOXFIT-parameter missing !\n"); return 100; }

      Output_BoxfitHeight = atoi(argv[argcnt]);

      argcnt++;

      if (boxfit_all)
        Output_Boxfit_MayEnlarge = TRUE;
    }
    else
    if (stricmp("RESIZE",argv[argcnt])==0)
    {
      Output_DoResize = TRUE;
      Output_ResizeH  = TRUE;
      Output_ResizeV  = TRUE;

      argcnt++;
      if (argcnt>=argc) { printf("RESIZE-parameter missing !\n"); return 100; }

      Output_ResizeFactor = atof(argv[argcnt]);

      if (Output_ResizeFactor <= 0.0) { printf("RESIZE-parameter invalid (must be > 0.0)\n"); return 100; }

      argcnt++;
    }
    else
    if (stricmp("RESIZEH",argv[argcnt])==0)
    {
      Output_DoResize = TRUE;
      Output_ResizeH  = TRUE;

      argcnt++;
      if (argcnt>=argc) { printf("RESIZE-parameter missing !\n"); return 100; }

      Output_ResizeFactor = atof(argv[argcnt]);

      if (Output_ResizeFactor <= 0.0) { printf("RESIZE-parameter invalid (must be > 0.0)\n"); return 100; }

      argcnt++;
    }
    else
    if (stricmp("RESIZEV",argv[argcnt])==0)
    {
      Output_DoResize = TRUE;
      Output_ResizeV  = TRUE;

      argcnt++;
      if (argcnt>=argc) { printf("RESIZE-parameter missing !\n"); return 100; }

      Output_ResizeFactor = atof(argv[argcnt]);

      if (Output_ResizeFactor <= 0.0) { printf("RESIZE-parameter invalid (must be > 0.0)\n"); return 100; }

      argcnt++;
    }
    else
    if (stricmp("UNUSED",argv[argcnt])==0)
    {
      argcnt++;
      if (argcnt>=argc) { printf("1st UNUSED-parameter missing !\n"); return 100; }
      EmptyCLUTEntry_R = atoi(argv[argcnt]);

      argcnt++;
      if (argcnt>=argc) { printf("2nd UNUSED-parameter missing !\n"); return 100; }
      EmptyCLUTEntry_G = atoi(argv[argcnt]);

      argcnt++;
      if (argcnt>=argc) { printf("3rd UNUSED-parameter missing !\n"); return 100; }
      EmptyCLUTEntry_B = atoi(argv[argcnt]);

      argcnt++;
    }
    else
    if (stricmp("INFO",argv[argcnt])==0)
    {
      showinfo = TRUE;
      argcnt++;
    }
    else
    {
      printf("unknown command %s !\n",argv[argcnt]);
      return 100;
    }
  }


  if (Load_is_RGB)
  {
    if (!hasgreen || !hasblue) { printf("RGB-Raw input needs GREEN and BLUE parameter !\n"
                                        "Maybe, it's not an image-file?\n"); return 100; }
  }

  if (savemode == FORM_RGB)
  {
    Save_is_RGB = TRUE;

    strcpy(Savename_g,Savename);
    strcpy(Savename_b,Savename);
    strcat(Savename  ,".red");
    strcat(Savename_g,".green");
    strcat(Savename_b,".blue");
  }

  if (showinfo)
  {
    if(OpenLoadFile()) {
      CallFormatMethod(GetFormat(loadmode),Method_Information);
      CloseLoadFile();
    } else DoError(CANT_OPEN_LOAD_FILE);
  }
  else
  {
//    if (outputcolors) Output_nColors=outputcolors;

/*
    if (Output_CustomSize)
    {
      Output_Width  = max(newwidth ,1);
      Output_Height = max(newheight,1);
    }
*/

    printf("%s -> %s\n",Loadname,Savename);

    DoConversionMain();
  }

  return 0;
}

