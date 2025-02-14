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

#include "glib:incs.h"
#include "glib:g.h"
#include "glib:gstr.h"
#include "glib:txtsys.h"
#include "glib:errs.h"
#include "glib:clean.h"
#include "glib:libsys.h"

struct ConstructData ConDa; /* screen dependant graphics datas */

/* fallback-font that will be used if space is critical */

static struct TextAttr	fallback_textattr = { "topaz.font",8 };
static struct TextFont *fallback_textfont = NULL;

static struct CleanupList *conda_cleanup; /* parent cleanuplist for everything!
					   * allocated in this module. Handle-
					   * cleanuplists are sublists to
					   * this one.
					   */


/**------------------------------------------------------------------------**
 **  Set important screen dependant system values.
 **
 **  parent_screen  - screen on which windows will open. Pass NULL to open
 **		      windows on the Workbench (the default public screen).
 **
 **  parent_cleanup - connection to your global CleanupList. You may pass
 **		      NULL, but then you have to call CleanupConstructData().
 **------------------------------------------------------------------------**/
BOOL InitConstructData(struct Screen      *parent_screen,
		       struct CleanupList *parent_cleanup)
{
  struct Screen *scr;	 /* screen on which to open */
  BOOL		lockedWB;
  struct DrawInfo *di;


  /* create new cleanup list */

  if (!(conda_cleanup = NewCleanup(parent_cleanup)))
    return FALSE;


  /* Clear ConstructData */

  memset(&ConDa,0,sizeof(struct ConstructData) );


  /* Get the screen to which this ConstructData will relate. Use parent_screen
   * if available. If not, use the Worbench. */

  if (parent_screen) scr=parent_screen;
		else {
		       scr=LockPubScreen(NULL); /* lock the screen */

		       if (scr)    lockedWB=TRUE;
				 else
				 {
				   lockedWB=FALSE;
				   SetError(CANT_LOCK_WBSCREEN);
				   goto errexit;
				 }
		     }


  /* Get VisualInfo */

  ConDa.VisualInfo = GetVisualInfo( scr , TAG_DONE );

  if (! cuinsert_VisualInfo(ConDa.VisualInfo) )
    goto errexit;


  /* Set Screen */

  ConDa.scr = parent_screen;


  /* Get window border sizes */

  ConDa.bordertop_notxt=scr->WBorTop;
  ConDa.bordertop      =scr->WBorTop + scr->Font->ta_YSize + 1;
  ConDa.borderbottom   =scr->WBorBottom;
  ConDa.borderleft     =scr->WBorLeft;
  ConDa.borderright    =scr->WBorRight;


  /* Get system-default pens */

  di=GetScreenDrawInfo(scr);

  if (!di) { SetError(COULDNT_GET_DRAWINFO);
	     goto errexit;
	   }

  ConDa.textpen 	 = di->dri_Pens[TEXTPEN];
  ConDa.shinepen	 = di->dri_Pens[SHINEPEN];
  ConDa.shadowpen	 = di->dri_Pens[SHADOWPEN];
  ConDa.highlighttextpen = di->dri_Pens[HIGHLIGHTTEXTPEN];
  ConDa.fillpen 	 = di->dri_Pens[FILLPEN];

  /* get aspect ratio */

  ConDa.res_x = di->dri_Resolution.X;
  ConDa.res_y = di->dri_Resolution.Y;

  FreeScreenDrawInfo(scr,di);


  /* Get screen dimensions */

  ConDa.scrheight = scr->Height;
  ConDa.scrwidth  = scr->Width;
  ConDa.scrdepth  = scr->BitMap.Depth;


  /* set spacing , pay attention on aspect ratio */

  ConDa.xspacing = INTERWIDTH;
  ConDa.yspacing = (ConDa.xspacing * ConDa.res_x) / ConDa.res_y;


  /* open fallback font */

  assert(fallback_textfont == NULL);

  if (!(fallback_textfont = cu_OpenDiskFont(&fallback_textattr) ))
    goto errexit;

  name_cu_node("ConDa fallbackfont");


  /* Set default font to default screen font */

  /* todo: get the System-Default-Font from the Prefs */

  ConDa.NormTextAttr = scr->Font;
  ConDa.NormTextFont = di->dri_Font;


  /* unlock the screen if it had been locked */

  if (lockedWB) UnlockPubScreen(NULL,scr);

  CloseCleanup();

  return TRUE;

errexit:
  if (lockedWB) UnlockPubScreen(NULL,scr);
  DoCleanupFree(conda_cleanup);
  return FALSE;
}



/**------------------------------------------------------------------------**
 **  Do the cleanups.
 **------------------------------------------------------------------------**/
void CleanupConstructData(void)
{
  DoCleanupFree(conda_cleanup);
}


/**------------------------------------------------------------------------**
 **  SetNormFont()
 **------------------------------------------------------------------------**/
void SetNormFont(struct TextAttr *ta)
{
  struct TextFont *tf;

  SetCleanup(conda_cleanup);

  if (ConDa.font_loaded)
  {
    cu_CloseFont(ConDa.NormTextFont);
    ConDa.font_loaded=FALSE;
  }

  tf=cu_OpenDiskFont(ta);
  if (tf)
  {
    ConDa.NormTextAttr = ta;
    ConDa.NormTextFont = tf;
    ConDa.font_loaded=TRUE;
  }
  else
  {
    ConDa.NormTextAttr = &fallback_textattr;
    ConDa.NormTextFont =  fallback_textfont;
  }

  CloseCleanup();
}


/**------------------------------------------------------------------------**
 **  Mark all libraries used by this module
 **------------------------------------------------------------------------**/
void MarkLibs4GraphSys(BOOL cli)
{
  if (cli)
  {
    MarkLibrary(OPENLIB_DISKFONT ,0);
    MarkLibrary(OPENLIB_GRAPHICS ,0);
    MarkLibrary(OPENLIB_INTUITION,0);
    MarkLibrary(OPENLIB_LAYERS   ,0);
  }
  else
  {
    MarkLibrary(OPENLIB_DISKFONT ,36);
    MarkLibrary(OPENLIB_GADTOOLS ,37);
    MarkLibrary(OPENLIB_GRAPHICS ,36);
    MarkLibrary(OPENLIB_INTUITION,36);
    MarkLibrary(OPENLIB_LAYERS   ,36);
    MarkLibrary(OPENLIB_UTILITY  ,37);
  }
}






struct CreateTreeData CrTrDat;

/**------------------------------------------------------------------------**
 **  Initialize the fields in CrTrDat that must be reset befor each new
 **  Handle-tree construction.
 **------------------------------------------------------------------------**/

BOOL BeginNewHandleTree(void)
{
  static struct TagItem SetToDefaultTags[] = { TAG_DONE };


  /* clear fields */

  CrTrDat.lasthandle  = NULL;
  CrTrDat.lastgaddata = NULL;

  CrTrDat.IDCMP       = NULL;

  InitRastPort(&CrTrDat.textmeasure_dummyrp);


  /* get a cleanup-list for the new handle-tree */

  CrTrDat.cleanup = NewCleanup(conda_cleanup);
  if (!CrTrDat.cleanup) return FALSE;


  /* reset values to defaults */

  TreatGlobalTags(NULL,SetToDefaultTags);


  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Free the entire handle-tree. Called during tree construction, when it
 **  fails (with the parameter set to NULL) or to free the handle after
 **  using it.
 **
 **  If parameter if NULL, the handle being constructed in the moment will
 **  be freed.
 **------------------------------------------------------------------------**/

void FreeHandleTree(struct Handle *h)
{
  if (h) {
	   assert(h->Type == ha_Window);
	   assert(h->data.win.cleanup != NULL);

	   DoCleanupFree(h->data.win.cleanup);
	 }
  else	 {
	   assert(CrTrDat.cleanup != NULL);

	   DoCleanupFree(CrTrDat.cleanup);
	 }
}


/**------------------------------------------------------------------------**
 **  Get a new Handle structure. Just allocate and clear it.
 **------------------------------------------------------------------------**/

struct Handle *NewHandle(void)
{
  struct Handle *h=cu_calloc(sizeof(struct Handle),1);
  name_cu_node("Handle-struct");
  return h;
}


/**------------------------------------------------------------------------**
 **  Insert the hande in the handle-list.
 **------------------------------------------------------------------------**/

void LinkHandle(struct Handle *h)
{
  assert(h != NULL);
  assert(h != HANDLE_ERR);
  assert(h != HANDLE_END);

  /* chain */

  h->next = CrTrDat.lasthandle;
  CrTrDat.lasthandle = h;
}


/**------------------------------------------------------------------------**
 **  Insert the GadgetData in the GadgetData-list.
 **------------------------------------------------------------------------**/

void LinkGadData(struct GadgetData *gd)
{
  assert(gd != NULL);

  /* chain */

  gd->next = CrTrDat.lastgaddata;
  CrTrDat.lastgaddata = gd;
}



/**------------------------------------------------------------------------**
 **  Change the local ConDa values that may be modified with Tags
 **
 **  IMPORTANT: Routine must be save if handle-parameter is NULL
 **		and no tags affecting the handle are specified.
 **------------------------------------------------------------------------**/

void TreatGlobalTags(struct Handle *h,struct TagItem *taglist)
{
  BOOL keepsettings;
  struct TagItem *tag;


  assert(taglist != NULL);


  SetCleanup(conda_cleanup);


  /*--- leave if user wants to keep old settings ---*/

  keepsettings = GetTagData(ALT_KeepSettings , FALSE , taglist);
  if (keepsettings) return;



  /*===--- set local datas to either tag-values or defaults ---===*/



  /*--- set absolute position ---*/

  if (tag=FindTagItem(OBJ_AbsolutePos , taglist))
  {
    h->flags |= ABSOLUTE_POS;

    h->x = (tag->ti_Data >> 16) & 0x0000ffff;
    h->y = (tag->ti_Data      ) & 0x0000ffff;
  }


  /*--- set absolute size ---*/

  if (tag=FindTagItem(OBJ_AbsoluteWidth , taglist))
  {
    h->flags |= ABSOLUTE_WIDTH;
    h->w = tag->ti_Data;
  }

  if (tag=FindTagItem(OBJ_AbsoluteHeight , taglist))
  {
    h->flags |= ABSOLUTE_HEIGHT;
    h->h = tag->ti_Data;
  }


  /*--- set font ---*/

  if (tag=FindTagItem(ALT_TextAttr , taglist))
  {
       struct TextAttr *ta;  /* TextAttr to be set */
       struct TextFont *tf;

       ta = (struct TextAttr *)tag->ti_Data;


       /* open font if this one is different */

       if (ta != CrTrDat.CurrTextAttr)
       {

	    /* free old font */

	    if (CrTrDat.fontsloaded & LOADED_CURR_FONT)
	    {
	      cu_CloseFont(CrTrDat.CurrTextFont);
	      CrTrDat.fontsloaded &= ~LOADED_CURR_FONT;
	    }


	    /* try to open new font */

	    tf = cu_OpenDiskFont(ta);

	    name_cu_node("open local custom DiskFont");


	    /* if not available, warn user and use default */

	    if (tf)  {
		       CrTrDat.fontsloaded |= LOADED_CURR_FONT;
		     }
	       else
		     {
		       DoError(ERRWARN_NOFONT_FALLBACK);

		       ta = ConDa.NormTextAttr;
		       tf = ConDa.NormTextFont;
		     }


	    /* set font */

	    CrTrDat.CurrTextAttr = ta;
	    CrTrDat.CurrTextFont = tf;
       }
  }
  else
  {
    /* free old font */

    if (CrTrDat.fontsloaded & LOADED_CURR_FONT)
    {
      cu_CloseFont(CrTrDat.CurrTextFont);
      CrTrDat.fontsloaded &= ~LOADED_CURR_FONT;
    }

    /* reset to default font */

    CrTrDat.CurrTextAttr = ConDa.NormTextAttr;
    CrTrDat.CurrTextFont = ConDa.NormTextFont;
  }

  SetFont(&CrTrDat.textmeasure_dummyrp , CrTrDat.CurrTextFont);



  /*--- set textcolor ---*/

  if (tag=FindTagItem(ALT_TextColor,taglist))
  {
    if (tag->ti_Data == COLOR_NORMAL)
    {
      CrTrDat.TextPen = ConDa.textpen;
    }
    else if (tag->ti_Data == COLOR_HIGHLIGHT)
    {
      CrTrDat.TextPen = ConDa.highlighttextpen;
    }
    else  /* custom text-color */
    {
      CrTrDat.TextPen = tag->ti_Data & ~COLOR_CUSTOM;
    }
  }
  else
  {
    CrTrDat.TextPen = ConDa.textpen;  /* reset to default */
  }



  CloseCleanup();
}



/**------------------------------------------------------------------------**
 **  Change the priorities of the handle.
 **------------------------------------------------------------------------**/

struct Handle *NewPriXY(struct Handle *h,int xpri,int ypri)
{
  assert(h != HANDLE_END);

  if (h==HANDLE_ERR) return HANDLE_ERR;

  h->xpri=xpri;
  h->ypri=ypri;

  if (h->pri_chain) NewPriXY(h->pri_chain,xpri,ypri);

  return h;
}

/**------------------------------------------------------------------------**
 **  Change both priorities of the handle.
 **------------------------------------------------------------------------**/

struct Handle *NewPri(struct Handle *h,int pri)
{
  return NewPriXY(h,pri,pri);
}



/**------------------------------------------------------------------------**
 **  return height of current font
 **------------------------------------------------------------------------**/

UWORD FontHeight(void)
{
  return CrTrDat.CurrTextFont->tf_YSize;
}


/**------------------------------------------------------------------------**
 **  return width of text in pixels if written with current font
 **------------------------------------------------------------------------**/

WORD TextWidth(char *text)
{
  return TextLength(&CrTrDat.textmeasure_dummyrp , text , strlen(text) );
}

/**------------------------------------------------------------------------**
 **  get size of the widest glyph in the font
 **------------------------------------------------------------------------**/

static char allchars[] = "abcdefghijklmnopqrstuvwxyz"
			 "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			 "������"
			 "0123456789"
			 ",.-;: _#^+*\\!\"�$%&/=?<>"
			 "()[]{}"
			 ;

WORD MaxCharWidth(void)
{
  char *p;
  WORD len = 0;
  static struct TextFont *last_measure_font=NULL;
  static WORD		  last_width;

  /* if the value is already computed, we don't need to recompute it */

  if (CrTrDat.CurrTextFont == last_measure_font)
    return last_width;


  /* new font, have to measure new width */

  for (p=allchars ; *p != 0 ; p++)
  {
    int txtlen = TextLength(&CrTrDat.textmeasure_dummyrp, p , 1);
    len=max(len,txtlen);
  }


  /* save new width */

  last_measure_font = CrTrDat.CurrTextFont;
  last_width	    = len;


  return len;
}


/**------------------------------------------------------------------------**
 **  get size of the widest number-glyph in the font
 **------------------------------------------------------------------------**/

static char allnums[] = "0123456789";

WORD MaxNumWidth(void)
{
  char *p;
  WORD len = 0;
  static struct TextFont *last_measure_font=NULL;
  static WORD		  last_width;

  /* if the value is already computed, we don't need to recompute it */

  if (CrTrDat.CurrTextFont == last_measure_font)
    return last_width;


  /* new font, have to measure new width */

  for (p=allnums ; *p != 0 ; p++)
  {
    int txtlen=TextLength(&CrTrDat.textmeasure_dummyrp, p , 1);
    len=max(len,txtlen);
  }

  /* save new width */

  last_measure_font = CrTrDat.CurrTextFont;
  last_width	    = len;


  return len;
}

