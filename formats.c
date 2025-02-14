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

extern void CheckForILBM(form fo);
extern void CheckForLBM (form fo);  /* ILBM with PBM - FORM ( PC-DeluxePaint ) */
extern void CheckForRGB (form fo);
extern void CheckForPCX (form fo);
extern void CheckForIMG (form fo);
extern void CheckForBMP (form fo);
extern void CheckForRLE4(form fo);
extern void CheckForRLE8(form fo);
extern void CheckForGEM (form fo);
extern void CheckForGIF (form fo);
extern void CheckForTIFF(form fo);
extern void CheckForCGM (form fo);
extern void CheckForWGP (form fo);
extern void CheckForDXF (form fo);
extern void CheckForEPS (form fo);
extern void CheckForJPEG(form fo);
extern void CheckForRGB8(form fo);
extern void CheckForRGBN(form fo);
extern void CheckForTGA (form fo);
extern void CheckForCVP (form fo);

extern BOOL LoadILBM(void);
extern BOOL LoadLBM (void);
extern BOOL LoadRGB8(void);
extern BOOL LoadRGBN(void);
extern BOOL LoadPCX (void);
extern BOOL LoadIMG (void);
extern BOOL LoadBMP (void);
// extern BOOL LoadRLE4(void);   /* use LoadBMP instead */
// extern BOOL LoadRLE8(void);   /*  "   """      """   */
extern BOOL LoadGEM (void);
extern BOOL LoadGIF (void);
extern BOOL LoadTIFF(void);
extern BOOL LoadWGP (void);
extern BOOL LoadRGB (void);
extern BOOL LoadPS  (void);
extern BOOL LoadJPEG(void);
extern BOOL LoadTGA (void);
extern BOOL LoadCVP (void);

extern BOOL SaveILBM(void);
extern BOOL SaveLBM (void);
extern BOOL SaveRGB8(void);
extern BOOL SaveRGBN(void);
extern BOOL SavePCX (void);
extern BOOL SaveIMG (void);
extern BOOL SaveBMP (void);
extern BOOL SaveRLE4(void);
extern BOOL SaveRLE8(void);
extern BOOL SaveGEM (void);
extern BOOL SaveGIF (void);
extern BOOL SaveTIFF(void);
extern BOOL SaveWGP (void);
extern BOOL SaveRGB (void);
extern BOOL SavePS  (void);
extern BOOL SaveJPEG(void);
extern BOOL SaveTGA (void);
extern BOOL SaveCVP (void);

extern void PropsILBM(void);
extern void PropsLBM (void);
extern void PropsRGB8(void);
extern void PropsRGBN(void);
extern void PropsPCX (void);
extern void PropsIMG (void);
extern void PropsBMP (void);
extern void PropsRLE4(void);
extern void PropsRLE8(void);
extern void PropsGEM (void);
extern void PropsGIF (void);
extern void PropsTIFF(void);
extern void PropsWGP (void);
extern void PropsRGB (void);
extern void PropsPS  (void);
extern void PropsJPEG(void);
extern void PropsTGA (void);
extern void PropsCVP (void);

extern void InfoILBM(void);
extern void InfoLBM (void);
extern void InfoRGB8(void);
extern void InfoRGBN(void);
extern void InfoPCX (void);
extern void InfoIMG (void);
extern void InfoBMP (void);
extern void InfoRLE4(void);
extern void InfoRLE8(void);
extern void InfoGEM (void);
extern void InfoGIF (void);
extern void InfoTIFF(void);
extern void InfoWGP (void);
extern void InfoRGB (void);
extern void InfoPS  (void);
extern void InfoJPEG(void);
extern void InfoTGA (void);
extern void InfoCVP (void);


void CheckValid(form fo)
{
  fo->not_valid_format=FALSE;
}

struct FormatObject FormatObjs[] =
{
  { "ILBM"       ,"iff", 9 , FALSE , FORM_ILBM    , &CheckForILBM , &LoadILBM , &SaveILBM , &PropsILBM , &InfoILBM },
  { "LBM"        ,"lbm", 9 , FALSE , FORM_LBM     , &CheckForLBM  , &LoadLBM  , NULL      , &PropsILBM , &InfoILBM },
  { "RGB8"       ,"iff", 9 , FALSE , FORM_RGB8    , &CheckForRGB8 , &LoadRGB8 , NULL      , &PropsILBM , &InfoILBM },
  { "RGBN"       ,"iff", 9 , FALSE , FORM_RGBN    , &CheckForRGBN , &LoadRGBN , NULL      , &PropsRGBN , &InfoILBM },
  { "PCX"        ,"pcx", 2 , FALSE , FORM_PCX     , &CheckForPCX  , &LoadPCX  , &SavePCX  , &PropsPCX  , &InfoPCX  },
  { "IMG"        ,"img", 1 , FALSE , FORM_IMG     , &CheckForIMG  , &LoadIMG  , NULL      , &PropsIMG  , &InfoIMG  },
  { "BMP"        ,"bmp", 2 , FALSE , FORM_BMP     , &CheckForBMP  , &LoadBMP  , NULL      , &PropsBMP  , &InfoBMP  },
  { "RLE4"       ,"bmp", 2 , FALSE , FORM_RLE4    , &CheckForRLE4 , &LoadBMP  , NULL      , &PropsBMP  , &InfoBMP  },
  { "RLE8"       ,"bmp", 2 , FALSE , FORM_RLE8    , &CheckForRLE8 , &LoadBMP  , NULL      , &PropsBMP  , &InfoBMP  },
  { "GIF"        ,"gif", 2 , FALSE , FORM_GIF     , &CheckForGIF  , &LoadGIF  , &SaveGIF  , &PropsGIF  , &InfoGIF  },
  { "TIFF"       ,"tif", 2 , FALSE , FORM_TIFF    , &CheckForTIFF , &LoadTIFF , NULL      , &PropsTIFF , &InfoTIFF },
  { "JPEG"       ,"jpg", 2 , FALSE , FORM_JPEG    , &CheckForJPEG , &LoadJPEG , &SaveJPEG , &PropsJPEG , &InfoJPEG },
  { "Targa"      ,"tga", 2 , FALSE , FORM_TGA     , &CheckForTGA  , &LoadTGA  , NULL      , &PropsTGA  , &InfoTGA  },
  { "CVP"        ,"cvp", 1 , FALSE , FORM_CVP     , &CheckForCVP  , &LoadCVP  , &SaveCVP  , &PropsCVP  , &InfoCVP  },
  { "RGB-Raw"    ,""   , 0 , TRUE  , FORM_RGB     , &CheckForRGB  , &LoadRGB  , &SaveRGB  , NULL       , &InfoRGB  },
  { "Postscript" ,"ps",  0 , FALSE , FORM_PS      ,  NULL         ,  NULL     , &SavePS   , NULL       , &InfoPS   },

/*
  { "GEM"      , 1 , FALSE , FORM_GEM     , &CheckForGEM  },
  { "WGP"      , 1 , FALSE , FORM_WGP     , &CheckForWGP  },
*/

//  { "CGM"     , 1 , FALSE , FORM_CGM     , &CheckValid },
//  { "DXF"     , 1 , FALSE , FORM_DXF     , &CheckValid },
//  { "EPS"     , 1 , FALSE , FORM_EPS     , &CheckValid },

  { NULL }
};

struct FormatObject *NextFormatObject(ULONG *state)
{
  struct FormatObject *format;

  format = &FormatObjs[*state];

  if (format->FormatName == NULL) return NULL;

  (*state)++;

  return format;
}

ULONG nFormats(void)
{
  return sizeof(FormatObjs)/sizeof(struct FormatObject) -1;
}

BOOL CallFormatMethod(struct FormatObject *fo,enum Method method)
{
  switch (method)
  {
    case Method_CheckFormat:
      RewindInput();
      if (fo->CheckFormat)
         (*fo->CheckFormat)(fo);
      break;

    case Method_Information:
      RewindInput();
      if (fo->Information)
        (*fo->Information)();
      break;

    case Method_GetProperties:
      RewindInput();
      if (fo->GetProperties)
        (*fo->GetProperties)();
      break;

    case Method_LoadPict:
      RewindInput();
      if (fo->LoadPict)
        if ((*fo->LoadPict)() == FALSE)
        {
          if (LastError() == OK) SetError(INPUT_FILE_ERROR);
          ShowError();
          return FALSE;
        }
      break;

    case Method_SavePict:
      if (fo->SavePict)
        if ((*fo->SavePict)() == FALSE)
        {
          if (LastError() == OK) SetError(OUTPUT_FILE_ERROR);
          ShowError();
          return FALSE;
        }
      break;
  }

  return TRUE;
}

void CallFormatMethodAll(enum Method method)
{
  ULONG state;
  struct FormatObject *fo;

  state=NULL;
  while (fo=NextFormatObject(&state))
    CallFormatMethod(fo,method);
}

struct FormatObject *GetFormat(ULONG formatID)
{
  ULONG state;
  struct FormatObject *fo;

  for (state=0 ; fo=NextFormatObject(&state) ; )
    if (fo->FormatID == formatID) return fo;

  SetError(INTERNAL_ERROR);
  return NULL;
}

