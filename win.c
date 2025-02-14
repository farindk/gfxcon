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

 #include "global.h"

static char* v = "$VER: GfxCon 1.8f (25.Feb.99)";
extern struct TextAttr bigfont;

#define ID_LOAD_R    10
#define ID_LOAD_G    1
#define ID_LOAD_B    2
#define ID_LOADBUT_G 3
#define ID_LOADBUT_B 4
#define ID_SAVE_R    5
#define ID_SAVE_G    6
#define ID_SAVE_B    7
#define ID_SAVEBUT_G 8
#define ID_SAVEBUT_B 9
#define ID_FIRST_STRINGGAD 10
#define ID_NCOLORS   11
#define ID_INTERPOL  12
#define ID_WIDTH     13
#define ID_HEIGHT    14
#define POPUP_GFXMODE 15

struct Handle *h;
static struct Handle *popup_load_format,*popup_gfxmode;

extern char Loadname  [],Loadname_g[],Loadname_b[];
extern char Savename  [],Savename_g[],Savename_b[];

#define Loadname_r Loadname
#define Savename_r Savename


ULONG loadmode   =FORM_JPEG,savemode   =FORM_ILBM;
BOOL  Load_is_RGB=FALSE    ,Save_is_RGB=FALSE;

 LONG Output_Brightness   = 0;
 LONG Output_Contrast     = 100;

ULONG Output_Width        = 0,Output_Height       = 0;
// ULONG Output_CustomSize   = FALSE;  // rev.V1.8 OBSOLET
ULONG Output_Interpolated = FALSE;

ULONG Output_Mode         = GFXMOD_CLUT;
ULONG Output_nColors      = 16;
ULONG Output_Dither       = 1<<29;
ULONG Output_FlipFlags    = 1<<29;
ULONG Output_RotateFlags  = 1<<29;
ULONG Output_ColorEffects = 1<<29;

static ULONG SetLoadName_r(void);
static ULONG CheckDisableLoadNames(void);

extern BOOL cli;  // ob das Programm vom CLI gestartet wurde


void DoConversionMain(void)
{
  struct CleanupList *convertcleanup;

  if (!(convertcleanup=NewCleanup(GlobalCleanup)))
  {
    ShowError();
    return;
  }

  if( (!Save_is_RGB && *Savename) ||
      ( Save_is_RGB && *Savename_r && *Savename_g && *Savename_b) )
    if(OpenLoadFile())
    {
      if (CallFormatMethod(GetFormat(loadmode),Method_LoadPict))
      {
        CloseLoadFile();

        if(OpenSaveFile())
        {
          CallFormatMethod(GetFormat(savemode),Method_SavePict);
          CloseSaveFile();
          FreeBuffer(GetDefaultBuffer());
        }
        else
          DoError(OUTPUT_FILE_ERROR);
      }
      else
        CloseLoadFile();
    }
    else
      DoError(CANT_OPEN_LOAD_FILE);

  DoCleanupFree(convertcleanup);
}


static struct Handle *conversion_h,*textdisp_h,*progress_h,*info_h,*infodisp_h;
extern BOOL noprogress;

void ShowProgress(int a,int b) { if (conversion_h) DisplayProgress(conversion_h,progress_h,a,b);
                                 else if (!noprogress) printf("%d / %d\r",a,b); fflush(stdout); }
void ShowMessage (char *txt)   { if (conversion_h) WriteTextLine  (conversion_h,textdisp_h,txt,-1);
                                 else              printf("%s\n",txt);       }
void ShowInfo    (char *txt)   { if (!cli)         WriteTextLine  (info_h      ,infodisp_h,txt,-1);
                                 else              printf("%s\n",txt); }

static ULONG DoConversion(void)
{
  conversion_h = HANDLE_ERR;

  if (BeginNewHandleTree())
  {
    conversion_h=CrSmallWindow(
                   CrSpaceBox(
                     CrSpaceVBox(
                       CrNormTextBox(Txt(TXT_PROGRESS_INDICATOR),
                         CrSpaceBox(
                           progress_h=CrProgressDisplay(
                                        GAGA_PROGR_Percent,TRUE,
//                                        ALT_TextAttr,&countfont,
                                        TAG_DONE
                                      )
                         )
                       ),
                       CrNormTextBox(Txt(TXT_OPERATION_MESSAGES),
                         textdisp_h=CrTextDisplay(GAGA_TXDI_MinLines,5,TAG_DONE)
                       ),
                       HANDLE_END
                     )
                   ),
                   WAWA_Centered,h,
                   TAG_DONE,
                   TAG_DONE
                 );
  }
  else
    ShowError();

  if (conversion_h != HANDLE_ERR)
  {
    if (ComputeGadgets(conversion_h))
    {
      if (OpenHandle(conversion_h))
      {
        WriteTextLine(conversion_h,textdisp_h,Txt(TXT_CONVERSION_PROGRESS),-1);
        DoConversionMain();
        CloseHandle(conversion_h);
        FreeHandleTree(conversion_h);
      }
      else
        ShowError();
    }
    else
      ShowError();
  }

  return 0;
}


/***********************************************************************************
 ----------------------------- Information Window ----------------------------------
 ***********************************************************************************/

static ULONG EndInformation(void) { return EXIT_EXITWINDOW; }

static ULONG DoInformation(void)
{
  info_h = HANDLE_ERR;

  if (BeginNewHandleTree())
  {
    info_h=CrSmallWindow(CrSpaceBox(CrSpaceVBox(CrLowBox(
                         infodisp_h=CrTextDisplay(GAGA_TXDI_MinLines,20,TAG_DONE)),
                       CrGadget(GAGA_Text,"OK",
                                GAGA_CallFunc,&EndInformation,
                                TAG_DONE,TAG_DONE),
                       HANDLE_END ) ),
                   WAWA_Centered,h, TAG_DONE, TAG_DONE );
  }
  else
    ShowError();

  if (info_h != HANDLE_ERR) {
    if (ComputeGadgets(info_h)) {
      if (OpenHandle(info_h)) {
        if(OpenLoadFile()) {
          CallFormatMethod(GetFormat(loadmode),Method_Information);
          CloseLoadFile();
        } else DoError(CANT_OPEN_LOAD_FILE);

        HandleHandle(info_h,NULL,NULL);

        CloseHandle(info_h);
        FreeHandleTree(info_h);
      }
      else ShowError();
    }
    else ShowError();
  }
  return 0;
}


/***********************************************************************************
 ----------------------------- Program-Info Window ---------------------------------
 ***********************************************************************************/

static ULONG EndPrgInformation(void) { return EXIT_EXITWINDOW; }

static ULONG DoShowPrgInfos(void)
{
  struct Handle *prginfoh=HANDLE_ERR;

  if (BeginNewHandleTree())
  {
    prginfoh=CrSmallWindow(
                   CrSpaceBox(
                     CrSpaceVBox(
                       CrHBox(
                         NewPri(CrFiller(),1),
                         NewPri(
                           CrLowBox(
                             CrLowBox(
                               CrSpaceBox(
                                 CrSpaceVBox(
                                   CrText("GfxCon",ALT_TextAttr,&bigfont,TAG_DONE),
                                   CrText("Version 1.8f",TAG_DONE),
#ifdef _M68020
     CrText("(68020)",TAG_DONE),
#else
     CrText("(68000)",TAG_DONE),
#endif
                                   HANDLE_END
                                 )
                               )
                             )
                           ),1
                         ),
                         NewPri(CrFiller(),1),
                         HANDLE_END
                       ),
                       CrNormTextBox("Autor",
                         CrSpaceBox(
                           CrVBox(
                             CrText("Dirk Farin"    ,TAG_DONE),
                             CrText("Kapellenweg 15",TAG_DONE),
                             CrText("72070 T�bingen",TAG_DONE),
                             CrText("Germany"       ,TAG_DONE),
                             CrFiller(),
                             CrText("farindk@trick.informatik.uni-stuttgart.de",TAG_DONE),
                             HANDLE_END
                           )
                         )
                       ),
                       CrGadget(GAGA_Text,"OK",
                                GAGA_CallFunc,&EndPrgInformation,
                                TAG_DONE,TAG_DONE),
                       HANDLE_END
                     )
                   ),
                   WAWA_Centered,h,
                   TAG_DONE,
                   TAG_DONE
                 );
  }
  else
    ShowError();

  if (prginfoh != HANDLE_ERR)
  {
    if (ComputeGadgets(prginfoh))
    {
      if (OpenHandle(prginfoh))
      {
        HandleHandle(prginfoh,NULL,NULL);

        CloseHandle(prginfoh);
        FreeHandleTree(prginfoh);
      }
      else
        ShowError();
    }
    else
      ShowError();
  }

  return 0;
}

/*******************************************************************************
 ---------------------------------- Main Window --------------------------------
 *******************************************************************************/

/**------------------------------------------------------------------------**
 **  Get properties from load-file and insert them into the gadgets.
 **------------------------------------------------------------------------**/

static void SetGadgetsToFileDefaults(void)
{
  if(OpenLoadFile())
  {
    CallFormatMethod(GetFormat(loadmode),Method_GetProperties);
    CloseLoadFile();

    if (h)
    {
      GT_SetGadgetAttrs(GetGadget(h,ID_NCOLORS),GetWindow(h),NULL,
                        GTIN_Number,Output_nColors,
                        TAG_DONE);
      GT_SetGadgetAttrs(GetGadget(h,ID_WIDTH  ),GetWindow(h),NULL,
                        GTIN_Number,Output_Width,
                        TAG_DONE);
      GT_SetGadgetAttrs(GetGadget(h,ID_HEIGHT ),GetWindow(h),NULL,
                        GTIN_Number,Output_Height,
                        TAG_DONE);

      SetPopup(h,popup_gfxmode,Output_Mode);

      // I know, it's ugly, but if the user doesn't ever select one
      // of the resize-number-Gadgets, the number will not be read
      // and evaluate to 0.

      Output_NewWidth  = Output_Width;
      Output_NewHeight = Output_Height;
    }
  }
}

/**------------------------------------------------------------------------**
 **  Search all formats and set all gadgets to match the most likely input
 **  format.
 **------------------------------------------------------------------------**/

void SetMostLikelyLoadFormat(void)
{
  ULONG state;
  int   pri;
  form  fo,bestformat=NULL;

  pri  = -1;
  state=0;
  while (fo=NextFormatObject(&state))
  {
    if (fo->not_valid_format == FALSE && fo->Priority > pri)
    {
      pri=fo->Priority;
      bestformat = fo;
    }
  }

  assert(bestformat != NULL);

  if (h) SetPopup(h,popup_load_format,bestformat->FormatID);
  loadmode = bestformat->FormatID;
  SetGadgetsToFileDefaults();
}


#define dont_if(x) if (!(x))
#define not        !

/**------------------------------------------------------------------------**
 **  Search list of formats to find all formats which may be the input
 **  format.
 **------------------------------------------------------------------------**/

void CheckLoadFormats(void)
{
  ULONG state;
  struct FormatObject *fo;

  Load_is_RGB=FALSE;

  if(OpenLoadFile())
  {
    CallFormatMethodAll( Method_CheckFormat );
    CloseLoadFile();

    state=0;
    while (fo=NextFormatObject(&state))
    {
      if (fo->LoadPict && h)
      {
        SetPopupDisable(popup_load_format,
                        fo->FormatID,
                        fo->not_valid_format);
      }
    }

    fo = GetFormat(loadmode);
    SetMostLikelyLoadFormat();

    Load_is_RGB = fo->RGB_files;
  }
  else DoError(CANT_OPEN_LOAD_FILE);

  if (h) CheckDisableLoadNames();
}


/**------------------------------------------------------------------------**
 **  check which input filename-gadgets to disable
 **------------------------------------------------------------------------**/

static ULONG CheckDisableLoadNames(void)
{
  struct FormatObject *fo;

  fo=GetFormat(loadmode);
  Load_is_RGB = fo->RGB_files; /* only for startup */

  DisableGad(h,ID_LOAD_G   ,!Load_is_RGB);
  DisableGad(h,ID_LOAD_B   ,!Load_is_RGB);
  DisableGad(h,ID_LOADBUT_G,!Load_is_RGB);
  DisableGad(h,ID_LOADBUT_B,!Load_is_RGB);

  return 0;
}


/**------------------------------------------------------------------------**
 **  check which output filename-gadgets to disable
 **------------------------------------------------------------------------**/

static void CheckDisableSaveNames(void)
{
  struct FormatObject *fo;

  fo=GetFormat(savemode);
  Save_is_RGB = fo->RGB_files;

  DisableGad(h,ID_SAVE_G   ,!Save_is_RGB);
  DisableGad(h,ID_SAVE_B   ,!Save_is_RGB);
  DisableGad(h,ID_SAVEBUT_G,!Save_is_RGB);
  DisableGad(h,ID_SAVEBUT_B,!Save_is_RGB);
}

/**------------------------------------------------------------------------**
 **  check which filename-gadgets to disable
 **------------------------------------------------------------------------**/

void CheckDisableFilenames(void)
{
  CheckDisableLoadNames();
  CheckDisableSaveNames();
}

/**------------------------------------------------------------------------**
 **  query flags
 **------------------------------------------------------------------------**/

ULONG GetOutputFlags(void)
{
  return Output_Mode | Output_Dither | Output_FlipFlags | Output_RotateFlags | Output_ColorEffects;
}


/**------------------------------------------------------------------------**
 **  Disable or Enable size-gadgets
 **------------------------------------------------------------------------**/

ULONG DisableSizeGads(void)
{
  //printf("DisableSizeGads\n");

  DisableGad(h,ID_INTERPOL,Output_DoSize == FALSE);
  DisableGad(h,ID_WIDTH   ,Output_DoSize == FALSE);
  DisableGad(h,ID_HEIGHT  ,Output_DoSize == FALSE);
  return NULL;
}


/**------------------------------------------------------------------------**
 **  Get a filename from requester
 **------------------------------------------------------------------------**/

static ULONG GetFilename(ULONG paradest,ULONG paramode)
{
  char *dest=(char *)paradest;
  enum Mode { Load=0,Save=1 } mode=(enum Mode)paramode;
  ULONG id=0;

  switch (mode)
  {
    case Load: GetLoadFilename(dest);
               if (dest==Loadname_r) id=ID_LOAD_R;
               if (dest==Loadname_g) id=ID_LOAD_G;
               if (dest==Loadname_b) id=ID_LOAD_B;
               if (id) GT_SetGadgetAttrs(GetGadget(h,id),GetWindow(h),NULL,
                                         GTST_String,dest,
                                         TAG_DONE);
               if (id==ID_LOAD_R) SetLoadName_r();
               break;

    case Save: GetSaveFilename(dest);
               if (dest==Savename_r) id=ID_SAVE_R;
               if (dest==Savename_g) id=ID_SAVE_G;
               if (dest==Savename_b) id=ID_SAVE_B;
               if (id) GT_SetGadgetAttrs(GetGadget(h,id),GetWindow(h),NULL,
                                         GTST_String,dest,
                                         TAG_DONE);
               break;
  }

  return 0;
}

void EnDisableSetGfxModePopup(void)
{
  /* enable/disable and set GfxMode-popup-gadget */

  switch(savemode)
  {
    case FORM_ILBM :
      DisableGad(h,POPUP_GFXMODE,FALSE);
      break;

    case FORM_LBM  :    case FORM_PCX  :    case FORM_IMG  :    case FORM_GIF  :
    case FORM_TIFF :    case FORM_BMP  :    case FORM_RLE4 :    case FORM_RLE8 :
      SetPopup(h,popup_gfxmode,Output_Mode=GFXMOD_CLUT);
      DisableGad(h,POPUP_GFXMODE,TRUE);
      break;

    case FORM_RGB  :    case FORM_RGB8 :    case FORM_RGBN :    case FORM_PS   :
    case FORM_CVP  :
      SetPopup(h,popup_gfxmode,Output_Mode=GFXMOD_24BIT);
      DisableGad(h,POPUP_GFXMODE,TRUE);
      break;
  }
}

void EnDisableSetNColors(void)
{
  /* enable/disable and set nColors-gadget */

  switch(savemode)
  {
    case FORM_ILBM :
      if (Output_Mode == GFXMOD_CLUT)   DisableGad(h,ID_NCOLORS,FALSE);
      else                              DisableGad(h,ID_NCOLORS,TRUE);
      break;

    case FORM_PCX  :    case FORM_GIF  :
      DisableGad(h,ID_NCOLORS,FALSE);
      break;

    case FORM_RGB  :    case FORM_PS   :
    case FORM_CVP  :
      DisableGad(h,ID_NCOLORS,TRUE);
      break;

    case FORM_LBM  :    case FORM_IMG  :
    case FORM_TIFF :    case FORM_BMP  :
    case FORM_RLE4 :    case FORM_RLE8 :
    case FORM_RGB8 :    case FORM_RGBN :
      assert(0);
      break;
  }
}

/**------------------------------------------------------------------------**
 **  Fills gadgets with output-format-default values.
 **------------------------------------------------------------------------**/

static void SetFormatGfxModes(void)
{
  CheckDisableSaveNames();

  EnDisableSetGfxModePopup();
  EnDisableSetNColors();
}

/**------------------------------------------------------------------------**
 ** -------------------------- CALLBACK FUNCTIONS ------------------------ **
 **------------------------------------------------------------------------**/

static ULONG SetLoadName_r(void)
{
  CheckLoadFormats();
  return 0;
}

static ULONG SetSaveFormat(void)
{
  SetFormatGfxModes();
  return 0;
}

static ULONG SetInputFormat(void)
{
  CheckDisableLoadNames();
  return 0;
}

static ULONG SetGfxMode(void)
{
  EnDisableSetNColors();
  return 0;
}

/*****************************************************************************
 --------------------------- main window construction ------------------------
 *****************************************************************************/

static struct Handle *CrFormatlistPopup(APTR callfunc,ULONG *data,BOOL disable31)
{
  struct TagItem *stringlist;
  ULONG state;
  struct FormatObject *fo;
  int i;
  struct Handle *h;
  static struct TagItem taglist[4] =
  {
    GAGA_CallFunc     ,0,
    GAGA_POP_Ptr      ,0,
    GAGA_POP_Disable31,0,
    TAG_DONE,
  };
  BOOL IsLoadPopup = disable31;

  taglist[0].ti_Data = (ULONG)callfunc;
  taglist[1].ti_Data = (ULONG)data;
  taglist[2].ti_Data = (ULONG)disable31;


  stringlist = AllocateTagItems( nFormats()+1 );
  if (!stringlist) return HANDLE_ERR;

  state=0;
  i=0;
  while (fo=NextFormatObject(&state))
  {
    if ( (!IsLoadPopup && fo->SavePict != NULL) || (IsLoadPopup && fo->LoadPict) )
    {
      stringlist[i].ti_Tag  = (ULONG)fo->FormatName;
      stringlist[i].ti_Data =        fo->FormatID;
      if (disable31 && fo->not_valid_format)
        stringlist[i].ti_Data |= BIT31;
      i++;
    }
  }

  stringlist[i].ti_Tag = TAG_DONE;

  h=CrTextPopupA(taglist,stringlist);

  FreeTagItems(stringlist);

  return h;
}

void Interactive(void)
{
  h=HANDLE_ERR;

  if (BeginNewHandleTree())
  {
    h=CrSmallWindow(
        NewPriXY(
          CrSpaceBox(
            CrSpaceVBox(
              ModifyRasterEqual(TRUE,TRUE,
                CrSpaceHBox(
                  CrBevelBox(
                    TRUE,
                    CrSpaceBox(
                      CrSpaceVBox(
                        CrText(Txt(TXT_LOAD),ALT_TextAttr,&bigfont,TAG_DONE),
                        CrRaster(
                          5,
                          CrFilerequestGadget(GAGA_CallFunc ,&GetFilename,
                                              GAGA_FuncPara , Loadname_r,
                                              GAGA_FuncPara2, 0,
                                              TAG_DONE,TAG_DONE),

                          CrFiller(),
                          NewPri(
                            CrGadget(GAGA_Kind      ,STRING_KIND,
                                     GAGA_CallFunc  ,&SetLoadName_r,
                                     GAGA_ST_Ptr    ,Loadname_r,
                                     GAGA_ID        ,ID_FIRST_STRINGGAD,
                                     TAG_DONE,
                                     TAG_DONE),
                            1000
                          ),
                          CrFiller(),
                          CrText("(R)",TAG_DONE),

                          CrFilerequestGadget(GAGA_CallFunc , &GetFilename,
                                              GAGA_FuncPara , Loadname_g,
                                              GAGA_FuncPara2, 0,
                                              GAGA_ID       , ID_LOADBUT_G,
                                              TAG_DONE,TAG_DONE),
                          CrFiller(),
                          NewPri(
                            CrGadget(GAGA_Kind,STRING_KIND,
                                     GAGA_ID  ,ID_LOAD_G,
                                     GAGA_ST_Ptr  ,Loadname_g,
                                     TAG_DONE,
                                     TAG_DONE),
                            1000
                          ),
                          CrFiller(),
                          CrText("(G)",TAG_DONE),

                          CrFilerequestGadget(GAGA_CallFunc ,&GetFilename,
                                              GAGA_FuncPara ,Loadname_b,
                                              GAGA_FuncPara2,0,
                                              GAGA_ID       ,ID_LOADBUT_B,
                                              TAG_DONE,TAG_DONE),
                          CrFiller(),
                          NewPri(
                            CrGadget(GAGA_Kind,STRING_KIND,
                                     GAGA_ST_Ptr  ,Loadname_b,
                                     GAGA_ID  ,ID_LOAD_B,
                                     TAG_DONE,
                                     TAG_DONE),
                            1000
                          ),
                          CrFiller(),
                          CrText("(B)",TAG_DONE),
                          HANDLE_END
                        ),
                        CrSpaceHBox(
                          NewPri(CrText(Txt(TXT_INPUT_FORMAT),TAG_DONE),0),
                          popup_load_format=CrFormatlistPopup(&SetInputFormat,&loadmode,TRUE),
                          HANDLE_END
                        ),
                        HANDLE_END
                      )
                    )
                  ),
                  CrBevelBox(
                    TRUE,
                    CrSpaceBox(
                      CrSpaceVBox(
                        CrText(Txt(TXT_SAVE),ALT_TextAttr,&bigfont,TAG_DONE),
                        CrRaster(
                          5,
                          CrFilerequestGadget(GAGA_CallFunc ,&GetFilename,
                                              GAGA_FuncPara , Savename_r,
                                              GAGA_FuncPara2, 1,
                                              TAG_DONE,TAG_DONE),
                          CrFiller(),
                          NewPri(
                            CrGadget(GAGA_Kind,STRING_KIND,
                                     GAGA_ST_Ptr  ,Savename_r,
                                     GAGA_ID      ,ID_SAVE_R,
                                     TAG_DONE,
                                     TAG_DONE),
                            1000
                          ),
                          CrFiller(),                          CrText("(R)",TAG_DONE),

                          CrFilerequestGadget(GAGA_CallFunc ,&GetFilename,
                                              GAGA_FuncPara , Savename_g,
                                              GAGA_FuncPara2, 1,
                                              GAGA_ID       , ID_SAVEBUT_G,
                                              TAG_DONE, TAG_DONE),
                          CrFiller(),
                          NewPri(
                            CrGadget(GAGA_Kind,STRING_KIND,
                                     GAGA_ID  ,ID_SAVE_G,
                                     GAGA_ST_Ptr,Savename_g,
                                     TAG_DONE,
                                     TAG_DONE),
                            1000
                          ),
                          CrFiller(),                          CrText("(G)",TAG_DONE),

                          CrFilerequestGadget(GAGA_CallFunc ,&GetFilename,
                                              GAGA_FuncPara , Savename_b,
                                              GAGA_FuncPara2, 1,
                                              GAGA_ID       , ID_SAVEBUT_B,
                                              TAG_DONE, TAG_DONE),
                          CrFiller(),
                          NewPri(
                            CrGadget(GAGA_Kind,STRING_KIND,
                                     GAGA_ID  ,ID_SAVE_B,
                                     GAGA_ST_Ptr  ,Savename_b,
                                     TAG_DONE,
                                     TAG_DONE),
                            1000
                          ),
                          CrFiller(),                          CrText("(B)",TAG_DONE),
                          HANDLE_END
                        ),
                        CrSpaceHBox(
                          NewPri(CrText(Txt(TXT_OUTPUT_FORMAT),TAG_DONE),0),
                          CrFormatlistPopup(&SetSaveFormat,&savemode,FALSE),
                          HANDLE_END
                        ),
                        HANDLE_END
                      )
                    )
                  ),
                  HANDLE_END
                )
              ),
              CrBevelBox(
                TRUE,
                CrSpaceBox(
                  CrSpaceHBox(
                    CrSpaceVBox(
                      CrNormTextBox(
                        Txt(TXT_SIZE),
                        CrSpaceBox(
                          CrVBox(
                            CrCBGadget(Txt(TXT_CUSTOM),
                                       GAGA_Kind,CHECKBOX_KIND,
                                       GAGA_CB_Ptr,&Output_DoSize,
                                       GAGA_CallFunc,&DisableSizeGads,
                                       TAG_DONE,TAG_DONE),
                            CrCBGadget(Txt(TXT_INTERPOLATED),
                                       GAGA_Kind,CHECKBOX_KIND,
                                       GAGA_CB_Ptr,&Output_Interpolated,
                                       GAGA_ID    ,ID_INTERPOL,
                                       GA_Disabled,TRUE,
                                       TAG_DONE,TAG_DONE),
                            CrFiller(),
                            CrSpaceRaster(
                              2,
                              NewPri(CrText(Txt(TXT_WIDTH),TAG_DONE),0),
                              CrGadget(GAGA_Kind,INTEGER_KIND,
                                       GAGA_ID,ID_WIDTH,
                                       GAGA_IN_Ptr,&Output_NewWidth,
                                       GAGA_LowerBound,1,
                                       GAGA_UpperBound,65535,
                                       TAG_DONE,
                                       GA_Disabled,TRUE,
                                       TAG_DONE),
                              NewPri(CrText(Txt(TXT_HEIGHT),TAG_DONE),0),
                              CrGadget(GAGA_Kind,INTEGER_KIND,
                                       GAGA_ID,ID_HEIGHT,
                                       GAGA_IN_Ptr,&Output_NewHeight,
                                       GAGA_LowerBound,1,
                                       GAGA_UpperBound,65535,
                                       TAG_DONE,
                                       GA_Disabled,TRUE,
                                       TAG_DONE),
                              HANDLE_END
                            ),
                            HANDLE_END
                          )
                        )
                      ),
                      CrNormTextBox(                         Txt(TXT_TRANSFORM),
                        CrSpaceBox(  CrSpaceRaster( 2,
                            NewPri(CrText(Txt(TXT_FLIP),TAG_DONE),0),
                            CrTextPopup(GAGA_POP_Ptr,&Output_FlipFlags,
                                        TAG_DONE,
                                        Txt(TXT_FLIP_NOTHING),1<<29,
                                        Txt(TXT_FLIP_H)      ,GFXMOD_FLIPX,
                                        Txt(TXT_FLIP_V)      ,GFXMOD_FLIPY,
                                        Txt(TXT_FLIP_HV)     ,GFXMOD_FLIPX | GFXMOD_FLIPY,
                                        TAG_DONE),
                            NewPri(CrText(Txt(TXT_ROTATE),TAG_DONE),0),
                            CrTextPopup(GAGA_POP_Ptr,&Output_RotateFlags,
                                        TAG_DONE,
                                        Txt(TXT_ROT_NOTHING),1<<29,
                                        Txt(TXT_ROT_PLUS90) ,GFXMOD_ROTATE_L,
                                        Txt(TXT_ROT_MINUS90),GFXMOD_ROTATE_R,
                                        TAG_DONE
                                       ), HANDLE_END
                          ) ) ), HANDLE_END
                    ),
                    CrSpaceVBox(
                      CrNormTextBox(
                        Txt(TXT_COLORS),
                        CrSpaceBox(
                          CrHBox(
                            CrSpaceRaster(
                              2,
                              NewPri(CrText(Txt(TXT_COLORMODE),TAG_DONE),0),
                              popup_gfxmode=CrTextPopup(GAGA_POP_Ptr ,&Output_Mode,
                                          GAGA_ID      ,POPUP_GFXMODE,
                                          GAGA_CallFunc,&SetGfxMode,
                                          TAG_DONE,
                                          "CLUT"      ,GFXMOD_CLUT,
                                          "HAM"       ,GFXMOD_HAM,
                                          "HAM8"      ,GFXMOD_HAM8,
                                          "24 bit"    ,GFXMOD_24BIT,
                                          TAG_DONE),
                              NewPri(CrText(Txt(TXT_COLORS_COL),TAG_DONE),0),
                              CrGadget(GAGA_Kind      ,INTEGER_KIND,
                                       GAGA_IN_Ptr    ,&Output_nColors,
                                       GAGA_UpperBound,32768,
                                       GAGA_LowerBound,2,
                                       GAGA_ID        ,ID_NCOLORS,
                                       TAG_DONE,
                                       TAG_DONE),
                              NewPri(CrText(Txt(TXT_DITHERING),TAG_DONE),0),
                              CrTextPopup(GAGA_POP_Ptr     ,&Output_Dither,
                                          TAG_DONE,
                                          Txt(TXT_DITHER_NONE),1<<29,
                                          "Floyd-Steinberg"  ,GFXMOD_FLOYD,
                                          Txt(TXT_FAST_FLOYD),GFXMOD_FASTFLOYD,
                                          TAG_DONE
                                         ),
                              NewPri(CrText(Txt(TXT_EFFECTS),TAG_DONE),0),
                              CrTextPopup(GAGA_POP_Ptr     ,&Output_ColorEffects,
                                          TAG_DONE,
                                          Txt(TXT_EFX_NONE)          ,1<<29,
                                          Txt(TXT_EFX_BW)            ,GFXMOD_BW,
                                          Txt(TXT_COL_GRAYSCALE)     ,GFXMOD_GRAYSCALE,
                                          Txt(TXT_EFX_INV)           ,GFXMOD_INVERS,
                                          Txt(TXT_EFX_INV_BW)        ,GFXMOD_INVERS | GFXMOD_BW,
                                          Txt(TXT_EFX_RED_FILTER)    ,GFXMOD_NO_G | GFXMOD_NO_B,
                                          Txt(TXT_EFX_GREEN_FILTER)  ,GFXMOD_NO_R | GFXMOD_NO_B,
                                          Txt(TXT_EFX_BLUE_FILTER)   ,GFXMOD_NO_R | GFXMOD_NO_G,
                                          Txt(TXT_EFX_YELLOW_FILTER) ,GFXMOD_NO_B,
                                          Txt(TXT_EFX_MAGENTA_FILTER),GFXMOD_NO_G,
                                          Txt(TXT_EFX_CYAN_FILTER)   ,GFXMOD_NO_R,
                                          TAG_DONE
                                         ),

                              NewPri(CrText(Txt(TXT_BRIGHTNESS),TAG_DONE),0),
                              CrGadget(GAGA_Kind,INTEGER_KIND,
                                       GAGA_UpperBound, 255,
                                       GAGA_LowerBound,-255,
                                       GAGA_IN_Ptr    ,&Output_Brightness,
                                       TAG_DONE,
                                       TAG_DONE),
                              NewPri(CrText(Txt(TXT_CONTRAST),TAG_DONE),0),
                              CrHBox(
                                NewPri(
                                  CrGadget(GAGA_Kind,INTEGER_KIND,
                                           GAGA_UpperBound,1000,
                                           GAGA_LowerBound,0,
                                           GAGA_IN_Ptr    ,&Output_Contrast,
                                           TAG_DONE,
                                           TAG_DONE),
                                  3),
                                CrFiller(),
                                NewPri(CrText("%",TAG_DONE),0),
                                HANDLE_END
                              ),

                              HANDLE_END
                            ),

                            HANDLE_END
                          )
                        )
                      ),
                      HANDLE_END
                    ),
                    HANDLE_END
                  )
                )
              ),
              CrHBox(
                CrGadget(GAGA_Text,Txt(TXT_CONVERT),
                         GAGA_CallFunc,&DoConversion,
                         ALT_TextAttr,&bigfont,
                         TAG_DONE,TAG_DONE),
                CrGadget(GAGA_Text,Txt(TXT_INFOS),
                         GAGA_CallFunc,&DoInformation,
                         ALT_TextAttr,&bigfont,
                         TAG_DONE,TAG_DONE),
                CrGadget(GAGA_Text,Txt(TXT_ABOUT),
                         GAGA_CallFunc,&DoShowPrgInfos,
                         ALT_TextAttr,&bigfont,
                         TAG_DONE,TAG_DONE),
                HANDLE_END
              ),
              HANDLE_END
            )
          ),
          1,0
        ),
        WAWA_ActiveGad,ID_FIRST_STRINGGAD,
        TAG_DONE,
        WA_Title      ,Txt(TXT_WINDOW_TITLE),
        WA_DragBar    ,TRUE,
        WA_DepthGadget,TRUE,
        WA_CloseGadget,TRUE,
        WA_Activate   ,TRUE,
        TAG_DONE
      );
  }
  else
    ShowError();

  if (h != HANDLE_ERR)
  {
    if (ComputeGadgets(h))
    {
      if (OpenHandle(h))
      {
        CheckDisableFilenames();

        DisableGad(h,ID_INTERPOL,TRUE);  /* funkioniert oben nicht richtig :( */
        HandleHandle(h,NULL,NULL);
        CloseHandle(h);
        FreeHandleTree(h);
      }
      else  ShowError();
    }
    else  ShowError();
  }
}
