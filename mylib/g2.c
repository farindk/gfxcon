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
#include "glib:gstr.h"
#include "glib:g.h"
#include "glib:errs.h"
#include "glib:txtsys.h"
#include "glib:clean.h"

/**------------------------------------------------------------------------**
 ** Remove a tag (all tags with this tag-value) from a taglist by setting
 ** it to TAG_IGNORE
 **------------------------------------------------------------------------**/
void RemoveTag(Tag tag,struct TagItem *taglist)
{
  struct TagItem *tstate;
  struct TagItem *ti;


  tstate=taglist;

  while (ti=NextTagItem(&tstate))
  {
    if (ti->ti_Tag == tag) ti->ti_Tag = TAG_IGNORE;
  }
}


/**------------------------------------------------------------------------**
 ** Given a taglist like this: TAG1,TAG2,TAG3,TAG_END,TAG4,TAG5,TAG_END
 ** the function will return a pointer to the second taglist (&TAG4).
 **
 ** The first taglist may (of course) not contain any TAG_MORE - tags
 **------------------------------------------------------------------------**/

struct TagItem *SecondTaglist(struct TagItem *sourcelist)
{
  struct TagItem *ti;


  /* scan taglist until TAG_END is reached */

  for (ti=sourcelist ; ti->ti_Tag != TAG_END ; ti++)
  {
    ;
  }

  /* move behind TAG_END */

  ti=(struct TagItem *)((ULONG)ti + sizeof(Tag));

  return ti;
}

/**------------------------------------------------------------------------**
 **  Simple function to free the taglist passed. Use this to automatically
 **  free taglists using cuinsert_CallFunction().
 **------------------------------------------------------------------------**/
void FreeTagListFunc(struct TagItem *ti)
{
  assert(ti != NULL);

  FreeTagItems(ti);
}


#ifndef DONT_NEED_TEXT

/**------------------------------------------------------------------------**
 **  CrText - Text won't be copied. Be sure to let it stay alive for
 **	      future compatibility.
 **------------------------------------------------------------------------**/

struct Handle *CrAllTextsA(char *text,struct TagItem *mytaglist)
{
  struct Handle *h;


  /* check for unexpected input errors */

  assert(text != NULL);
  assert(mytaglist != NULL);


  /* allocate a new handle */

  if (!(h=NewHandle())) return HANDLE_ERR;


  /* Check global tags (font ...) */

  TreatGlobalTags(h,mytaglist);


  /* insert values */

  h->Type = ha_Text;

  h->minh = FontHeight();
  h->minw = TextWidth(text);

  h->flags |= ABSOLUTE_WIDTH | ABSOLUTE_HEIGHT;

  /* object-specific values */

  h->data.text.text  = text;
  h->data.text.font  = CrTrDat.CurrTextFont;
  h->data.text.color = CrTrDat.TextPen;

  h->xpri = 1;
  h->ypri = 0;

  /* link the handle to the list */

  LinkHandle(h);

  return h;
}


struct Handle *CrText(char *text,Tag tag, ...)
{
  return CrAllTextsA(text,(struct TagItem *)&tag);
}

#endif

/**------------------------------------------------------------------------**
 **  CrWindow
 **
 **  CrSmallWindow - This will set the priorities of the INNER (!) element
 **		     to 0, what will result in a window as small as
 **		     possible.
 **------------------------------------------------------------------------**/

struct Handle *CrWindowA(struct Handle *in,
			 struct TagItem *mytaglist,
			 struct TagItem *systaglist)
{
  struct Handle *h;
  struct TagItem *taglistcopy;
  struct TagItem *tag;
  static struct TagItem const windowflag_tags[] =
  {
    WAWA_AutoIDCMP , AUTO_SET_IDCMP  ,
    TAG_END
  };


  /* check for input errors */

  if (in==HANDLE_ERR) return HANDLE_ERR;

  assert(in != NULL);
  assert(mytaglist  != NULL);
  assert(systaglist != NULL);


  /* allocate a new handle */

  if (!(h=NewHandle()))
    return HANDLE_ERR;


  /* check global tags */

  TreatGlobalTags(h,mytaglist);


  /* copy the system-tags - taglist */

  taglistcopy=AllocCopyTaglist(systaglist,1 /* WA_IDCMP */
					   );

  if (!(taglistcopy))
  {
    SetError(NO_MEM);
    return HANDLE_ERR;
  }
//  cuinsert_CallFunction(&FreeTagListFunc,taglistcopy);

  h->data.win.taglist = taglistcopy;



  /*--- insert values ---*/

  h->leftborder = ConDa.borderleft;

  /* topborder (topborder size can vary depending on wether a title is
   * present or not and several different things ...)
   */

  if (TagInArray( WA_Title       , (Tag *)systaglist ) ||
      TagInArray( WA_DragBar     , (Tag *)systaglist ) ||
      TagInArray( WA_DepthGadget , (Tag *)systaglist ) ||
      TagInArray( WA_CloseGadget , (Tag *)systaglist )
     )
       h->topborder = ConDa.bordertop;
     else
       h->topborder = ConDa.bordertop_notxt;

  h->shrinkw = h->leftborder + ConDa.borderright;
  h->shrinkh = h->topborder  + ConDa.borderbottom;

  h->w	  = ConDa.scrwidth;
  h->h	  = ConDa.scrheight;

  h->in = in;

  h->Type = ha_Window;


  /*--- object specific values ---*/

  /* set default flags */

  h->data.win.flags = AUTO_SET_IDCMP;


  /* set tag-flags */

  h->data.win.flags = PackBoolTags( h->data.win.flags ,
				    mytaglist ,
				    windowflag_tags );

  /* winclosefunc */

  if (GetTagData(WA_CloseGadget,FALSE,systaglist))
    CrTrDat.IDCMP |= IDCMP_CLOSEWINDOW;

  h->data.win.winclosefunc = (ULONG (*)(void))GetTagData(WAWA_WinCloseFunc,NULL,mytaglist);


  /* center around handle */

  h->data.win.center_handle = (struct Handle *)GetTagData(WAWA_Centered,NULL,mytaglist);


  /* activegad */

  if (tag=FindTagItem(WAWA_ActiveGad,mytaglist))
  {
    h->data.win.flags |= ACTIVATE_GADGET;
    h->data.win.activegad = tag->ti_Data;
  }


  /* IDCMP */

  h->data.win.IDCMP |= GetTagData(WA_IDCMP,NULL,systaglist);
  RemoveTag(WA_IDCMP,taglistcopy);

  if (h->data.win.flags & AUTO_SET_IDCMP)
    h->data.win.IDCMP |= CrTrDat.IDCMP;

  if (h->data.win.flags & ACTIVATE_GADGET)
    h->data.win.IDCMP |= IDCMP_ACTIVEWINDOW;


  /* Tree construction finished, close cleanup and set global fields. */

  h->data.win.gaddata	 = CrTrDat.lastgaddata;
  h->data.win.handlelist = CrTrDat.lasthandle;
  h->data.win.cleanup	 = CrTrDat.cleanup;
  h->data.win.VisualInfo = ConDa.VisualInfo;
  h->data.win.scr	 = ConDa.scr;


  /* Close the Handle-Tree-CleanupList. This was created in BeginNewHandleTree() */

  CloseCleanup();

  return h;
}

struct Handle *CrWindow(struct Handle *in,Tag tag, ...)
{
  return CrWindowA(in,
		   (struct TagItem *)&tag,
		   SecondTaglist((struct TagItem *)&tag)
		  );
}

struct Handle *CrSmallWindow(struct Handle *in,Tag tag, ...)
{
  return CrWindowA(NewPri(in,0),
		   (struct TagItem *)&tag,
		   SecondTaglist((struct TagItem *)&tag)
		  );
}


#ifndef DONT_NEED_BEVELBOX

/**------------------------------------------------------------------------**
 **  CrBevelBox
 **------------------------------------------------------------------------**/

struct Handle *CrBevelBox(BOOL recessed,struct Handle *in)
{
  struct Handle *h;

  if (in == HANDLE_ERR) return HANDLE_ERR;

  if (!(h=NewHandle()))
    return HANDLE_ERR;

  h->shrinkw = 2* 2 ;
  h->shrinkh = 2* 1 ;

  h->leftborder = 2;
  h->topborder	= 1;

  h->xpri=1;
  h->ypri=1;

  h->in = in;

  h->Type = ha_BevelBox;

  h->data.bevelbox.recessed = recessed;

  LinkHandle(h);

  return h;
}

#endif



/**------------------------------------------------------------------------**
 **  CrHighBox / CrLowBox
 **------------------------------------------------------------------------**/

#ifndef DONT_NEED_HIGHBOX

struct Handle *CrHighBox(struct Handle *h)
{
  struct Handle *inner,*outer;


  inner=CrBevelBox(TRUE ,h    );
  outer=CrBevelBox(FALSE,inner);

  if (outer==HANDLE_ERR) return HANDLE_ERR;

  outer->pri_chain = inner;

  return outer;
}

#endif

#ifndef DONT_NEED_LOWBOX

struct Handle *CrLowBox(struct Handle *h)
{
  struct Handle *inner,*outer;

  inner=CrBevelBox(FALSE,h    );
  outer=CrBevelBox(TRUE ,inner);

  if (outer==HANDLE_ERR) return HANDLE_ERR;

  outer->pri_chain = inner;

  return outer;
}

#endif

/**------------------------------------------------------------------------**
 **  CrSpaceBox
 **------------------------------------------------------------------------**/

struct Handle *CrSpaceBox(struct Handle *in)
{
  struct Handle *h;

  if (in == HANDLE_ERR) return HANDLE_ERR;

  assert(in != NULL);


  if (!(h=NewHandle()))
    return HANDLE_ERR;

  h->shrinkw = 2* ConDa.xspacing ;
  h->shrinkh = 2* ConDa.yspacing ;

  h->leftborder = ConDa.xspacing;
  h->topborder	= ConDa.yspacing;

  h->xpri = 1;
  h->ypri = 1;

  h->in = in;

  h->Type = ha_SpaceBox;

  LinkHandle(h);

  return h;
}


/**------------------------------------------------------------------------**
 **  CrTextBox
 **------------------------------------------------------------------------**/

#ifndef DONT_NEED_TEXTBOX

struct Handle *CrTextBox(struct Handle *texth,struct Handle *in)
{
  struct Handle *h;

  if (in    == HANDLE_ERR) return HANDLE_ERR;
  if (texth == HANDLE_ERR) return HANDLE_ERR;

  assert(in    != NULL);
  assert(texth != NULL);

  if (!(h=NewHandle()))
    return HANDLE_ERR;

  h->leftborder = 2*2;
  h->topborder	= FontHeight();

  h->shrinkw = 2*2 + 2*2;
  h->shrinkh = h->topborder + 2*1;

  h->data.textbox.texthandle = texth;

  h->xpri = 1;
  h->ypri = 1;

  h->in = in;

  h->Type = ha_TextBox;

  LinkHandle(h);

  h->flags |= SUBORDINATE;

  return h;
}

struct Handle *CrNormTextBox(char *text,struct Handle *in)
{
  return CrTextBox(CrText(text,TAG_DONE),in);
}

#endif

/**------------------------------------------------------------------------**
 **  CrFiller
 **------------------------------------------------------------------------**/

struct Handle *CrFiller(void)
{
  struct Handle *h;

  if (!(h=NewHandle()))
    return HANDLE_ERR;

  h->minw = ConDa.xspacing;
  h->minh = ConDa.yspacing;

  h->Type = ha_Filler;

  LinkHandle(h);

  return h;
}

/**------------------------------------------------------------------------**
 **  CrRaster
 **------------------------------------------------------------------------**/

struct Handle *CrRasterA(int xsize,struct Handle **hptr)
{
  int numhandles;
  int ysize;
  struct Handle *h,*newh;
  int x,y,n;
  int ix,iy;


  /* count and check handles */

  for (n=0 , numhandles=0 ;
       hptr[n] != HANDLE_END ;
       n++ , numhandles++)
  {
    if (hptr[n] == HANDLE_ERR) return HANDLE_ERR;
  }


  /* calculate and check dimensions */

  if ((numhandles % xsize) != 0)
  {
    SetError(RASTERPARAMETERS_INVALID);
    return HANDLE_ERR;
  }

  ysize = numhandles/xsize;


  /* get new handle */

  if (!(newh=NewHandle()))
    return HANDLE_ERR;


  /* set values */

  newh->Type = ha_Raster;

  newh->data.raster.num_x = xsize;
  newh->data.raster.num_y = ysize;


  /* connect raster */

  n=0;
  for (y=0;y<ysize;y++)
    for (x=0;x<xsize;x++ , n++)
    {
      if (x==0 && y==0) newh->in = hptr[n];

      if (y>0)
      {
	h=newh->in;

	for (ix=0;ix<x  ;ix++) h=h->right;
	for (iy=0;iy<y-1;iy++) h=h->down;

	h->down = hptr[n];
      }

      if (x>0)
      {
	h=newh->in;

	for (ix=0;ix<x-1;ix++) h=h->right;
	for (iy=0;iy<y  ;iy++) h=h->down;

	h->right = hptr[n];
      }
    }

  newh->xpri=1;
  newh->ypri=1;

  return newh;
}

struct Handle *CrRaster(int xsize,struct Handle *h1, ...)
{
  return CrRasterA(xsize,&h1);
}


/**------------------------------------------------------------------------**
 **  CrVBox / CrHBox
 **------------------------------------------------------------------------**/

struct Handle *CrVBox(struct Handle *h, ...)
{
  return CrRasterA(1,&h);
}

struct Handle *CrHBox(struct Handle *h, ...)
{
  int count;
  struct Handle **p;

  for (count=0 , p=&h ; *p != HANDLE_END ; count++ , p++)
  {
    ;
  }

  return CrRasterA(count,&h);
}


/**------------------------------------------------------------------------**
 **  CrSpaceRaster / CrSpaceVBox / CrSpaceHBox
 **------------------------------------------------------------------------**/

struct Handle *CrSpaceRasterA(int xsize,struct Handle **hptr)
{
  struct Handle *h;

  h=CrRasterA(xsize,hptr);
  if (h == HANDLE_ERR) return HANDLE_ERR;

//  h->data.raster.flags |= SPACE_RASTER;
  h->data.raster.spacex = ConDa.xspacing;
  h->data.raster.spacey = ConDa.yspacing;

  return h;
}


struct Handle *CrSpaceRaster(int xsize,struct Handle *h, ...)
{
  return CrSpaceRasterA(xsize,&h);
}





struct Handle *CrSpaceVBox(struct Handle *h, ...)
{
  return CrSpaceRasterA(1,&h);
}


struct Handle *CrSpaceHBox(struct Handle *h, ...)
{
  int count;
  struct Handle **p;

  for (count=0 , p=&h ; *p != HANDLE_END ; count++ , p++)
  {
    ;
  }

  return CrSpaceRasterA(count,&h);
}



struct Handle *ModifyRasterEqual(BOOL eqwidth,BOOL eqheight,struct Handle *h)
{
  if (h == HANDLE_ERR) return HANDLE_ERR;

  if (eqwidth)  h->data.raster.flags |= EQUAL_WIDTH;
  if (eqheight) h->data.raster.flags |= EQUAL_HEIGHT;

  return h;
}


/**------------------------------------------------------------------------**
 **  Create a box which will show the progress of an action.
 **------------------------------------------------------------------------**/

struct Handle *CrProgressDisplayA(struct TagItem *taglist)
{
  struct Handle  *bevel_h;
  struct Handle  *progress_h;
  struct TagItem *tag;

  if (!(progress_h=NewHandle()))
    return HANDLE_ERR;

  TreatGlobalTags(progress_h,taglist);

  progress_h->xpri = 1;
  progress_h->ypri = 1;

  progress_h->Type = ha_ProgressBox;

  LinkHandle(progress_h);

  progress_h->data.progress.color     = ConDa.fillpen;
  progress_h->data.progress.textcolor = CrTrDat.TextPen;
  progress_h->data.progress.font      = CrTrDat.CurrTextFont;

  progress_h->minw = ConDa.xspacing;
  progress_h->minh = ConDa.yspacing;

  tag=FindTagItem(GAGA_PROGR_Percent,taglist);
  if (tag && tag->ti_Data)
  {
    progress_h->data.progress.flags |= SHOW_PERCENT;

    progress_h->minh = 2+FontHeight()+2;
    progress_h->minw = 2+TextWidth("100%")+2;
  }
  bevel_h = CrBevelBox(TRUE,progress_h);

  return bevel_h;
}

struct Handle *CrProgressDisplay(Tag tag, ...)
{
  return CrProgressDisplayA((struct TagItem *)&tag);
}

/**------------------------------------------------------------------------**
 **  Create an Image. A function to draw the picture must be supplied.
 **  'height' is the height of the image in hundredths of the height of a
 **  font-glyph.
 **  'xsize' and 'ysize' define the drawing area coordinate system. The
 **  resolution aspect is 1:1.
 **------------------------------------------------------------------------**/

struct Handle *CrImageA(UWORD height,UWORD xsize,UWORD ysize,
			void (*drawfunc)(struct DrawArea *),
			struct TagItem *taglist)
{
  struct Handle *h;
  static UBYTE planepick[9] = { 0x00,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f,0xff };
  struct TagItem *tag;


  /* get handle */

  if (!(h=NewHandle()))
    return HANDLE_ERR;

  TreatGlobalTags(h,taglist);


  /* get size of bitplanes */

  h->minh = ( height * FontHeight() ) / 100;
  h->minw = ( h->minh * xsize * ConDa.res_y ) / ( ysize * ConDa.res_x );

  h->xpri=1;
  h->ypri=1;

  h->Type=ha_Image;

  h->flags |= ABSOLUTE_WIDTH | ABSOLUTE_HEIGHT | KEEP_ASPECT ;

  h->data.image.image.Depth	 = ConDa.scrdepth;
  h->data.image.image.PlanePick  = planepick[ConDa.scrdepth];
  h->data.image.image.PlaneOnOff = 0;
  h->data.image.image.NextImage  = NULL;

  h->data.image.xsize = xsize;
  h->data.image.ysize = ysize;

  h->data.image.drawfunc = drawfunc;


  /* check tags */

  if (tag=FindTagItem(GAGA_IMG_BevelBox,taglist))
  {
    h->data.image.flags |= AUTO_BEVELBOX;

    if (tag->ti_Data) h->data.image.flags |= BEVELBOX_RECESSED;

    h->minw += 4;
    h->minh += 2;
  }
/*
  if (tag=FindTagItem(GAGA_IWILLFINDTHEBUGSOON,taglist))
  {
    h->minw += 4;
    h->minh += 2;
  }
*/
  LinkHandle(h);


  return h;
}

struct Handle *CrImage(UWORD height,UWORD xsize,UWORD ysize,
		       void (*drawfunc)(struct DrawArea *),Tag tag, ...)
{
  return CrImageA(height,xsize,ysize,drawfunc,(struct TagItem *)&tag);
}



/**------------------------------------------------------------------------**
 **  CrTextDisplay
 **------------------------------------------------------------------------**/

struct Handle *CrTextDisplayA(struct TagItem *taglist)
{
  struct Handle *h;
  int	 nLines;
  int	 nChars;

  if (!(h=NewHandle()))
    return HANDLE_ERR;

  nLines = GetTagData(GAGA_TXDI_MinLines, 1,taglist);
  nChars = GetTagData(GAGA_TXDI_MinChars, 0,taglist);

  h->xpri=1;
  h->ypri=1;

  h->minh = nLines * ( CrTrDat.CurrTextFont->tf_YSize + 1 ) +1;

  if (!nChars)
  {
    if (CrTrDat.CurrTextFont->tf_Flags & FPF_PROPORTIONAL) nChars=20;
    else						   nChars=40;
  }

//  if (!nChars) h->minw = TextWidth("ABCDEFabcdefghijklmnopqrstuvwxyz"); /* nominal window width */
//  else
  h->minw = nChars * MaxCharWidth();

  h->Type = ha_TextDisplay;

  h->data.textdisplay.font  = CrTrDat.CurrTextFont;
  h->data.textdisplay.color = CrTrDat.TextPen;

  LinkHandle(h);

  return h;
}

struct Handle *CrTextDisplay(Tag ti, ...)
{
  return CrTextDisplayA((struct TagItem *)&ti);
}
