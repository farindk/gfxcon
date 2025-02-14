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
 **  handle the Popup-action
 **------------------------------------------------------------------------**/

typedef ULONG (*FUNC2)(ULONG,ULONG);

static ULONG PopupFunc(struct Handle *poph,ULONG nr)
{
  poph->data.popup.selection = poph->data.popup.stringlist[nr].ti_Data;

  /* we don't need to pay any attention to BIT31 as this can't be selected
   * as it is disabled !!!
   */

  return EXIT_EXITWINDOW;
}

void ShowPopupSelection(struct Handle *h,struct Window *win)
{
  int i;
  struct TagItem *p;


  /* we have to handle BIT31 as a already selected item can be disabled ! */

  for (p=h->data.popup.stringlist , i=0 ; p->ti_Tag != NULL ; p++ , i++)
    if (h->data.popup.flags & POPUP_BIT31DISABLE)
    {
      if ((p->ti_Data & (~BIT31)) == (h->data.popup.selection & (~BIT31))) break;
    }
    else
      if (p->ti_Data == h->data.popup.selection)            break;

  GT_SetGadgetAttrs(h->data.popup.textgad->data.newgad.gaddata->gad,
		    win,
		    NULL,
		    GTTX_Text,h->data.popup.stringlist[i].ti_Tag,
		    TAG_DONE);
}

static void DoPopup(struct Handle *master,struct Handle *poph,int xpos,int ypos)
{
  int num,nwidth;
  struct TagItem *tag;
  struct Handle **itemarray;
  int allocnum;
  struct Handle *h,*raster;
  int i;

  for (tag=poph->data.popup.stringlist , num=0 ;
       tag->ti_Data != NULL ;
       tag++ , num++)
       {
	 ;
       }

  assert(num>0);

  nwidth=1;
  if (num > 10) nwidth=2;
  if (num > 20) nwidth=3;
  if (num > 30) nwidth=4;
  if (num > 40) nwidth=5;

  allocnum = num;

  if (num % nwidth)
    allocnum += nwidth - (num % nwidth);

  itemarray=cu_calloc(sizeof(APTR),allocnum+1);

  if (BeginNewHandleTree())
  {
    for (i=0;i<allocnum;i++)
    {
      if (i>=num) itemarray[i] = CrFiller();
	     else {
		    BOOL disable;

		    if (poph->data.popup.flags & POPUP_BIT31DISABLE)
		      disable = (poph->data.popup.stringlist[i].ti_Data & (1<<31) ) ? 1 : 0;
		    else
		      disable=FALSE;

		    itemarray[i] = CrGadget(GAGA_Text,
					    poph->data.popup.stringlist[i].ti_Tag,
					  TAG_DONE,
					  GA_Disabled,disable,
					  TAG_DONE);
		    itemarray[i]->data.newgad.gaddata->data.button.popupfunc   = &PopupFunc;
		    itemarray[i]->data.newgad.gaddata->data.button.popuphandle =  poph;
		    itemarray[i]->data.newgad.gaddata->data.button.popupnumber =  i;
		  }
    }
    itemarray[i] = HANDLE_END;

    raster=CrRasterA(nwidth,itemarray);

    h=CrSmallWindow(
	raster,
	HANDLE_END,
	WA_Left,xpos,
	WA_Top ,ypos - (allocnum-2)*FontHeight()/2, /* only an approximation ! */
	WA_AutoAdjust,TRUE,
	TAG_DONE
      );

    if (h != HANDLE_ERR)
    {
      if (ComputeGadgets(h))
      {
	if (OpenHandle(h))
	{
	  HandleHandle(h,NULL,NULL);

	  CloseHandle(h);
	  FreeHandleTree(h);
	}
	else
	  ShowError();
      }
      else
	ShowError();
    }
  }

  cu_free(itemarray);


  ShowPopupSelection(poph,master->data.win.win);

  if (poph->data.popup.destination)
    *(poph->data.popup.destination) = poph->data.popup.selection;
}

/**------------------------------------------------------------------------**
 **  Search the GadgetData belonging to a gadget-pointer.
 **------------------------------------------------------------------------**/

static struct GadgetData *search_gadgetdata(struct GadgetData *gaddat,
					    struct Gadget     *gad)
{
  if (gaddat == NULL) return NULL;

  if (gaddat->gad == gad) return gaddat;

  return search_gadgetdata(gaddat->next,gad);
}


/**------------------------------------------------------------------------**
 **  Search the gadget belonging to a id.
 **------------------------------------------------------------------------**/

static struct Gadget *search_gadget(struct GadgetData *gaddat,
				    ULONG	       id)
{
  if (gaddat == NULL) return NULL;

  if (gaddat->gad->GadgetID == id) return gaddat->gad;

  return search_gadget(gaddat->next,id);
}


/**------------------------------------------------------------------------**
 **  Write an integer. Size of pointer ('ptr') destination can be
 **  specified with 'length'.
 **------------------------------------------------------------------------**/

static void WriteInteger(long value,void *ptr,int length)
{
  if (ptr == NULL) return;

  switch (length)
  {
    case FIELD_BYTE: *((BYTE *)ptr) = value; break;
    case FIELD_WORD: *((WORD *)ptr) = value; break;
    case FIELD_LONG: *((LONG *)ptr) = value; break;
    default:
      assert(1==2);
  }
}


/**------------------------------------------------------------------------**
 **  Copy the value of an integer-gadget to its destination.
 **  Perform checking in bounds.
 **  Pass TRUE if new value shall be displayed when it must be modified
 **  when checking it to the bounds.
 **  Handle is only needed if showchange==TRUE.
 **------------------------------------------------------------------------**/

static void GetIntegerVal(struct GadgetData *gdat,
			  BOOL showchange,
			  struct Handle *h)
{
  long value;
  BOOL value_changed=FALSE;


  /* get value of gadget */

  value = ((struct StringInfo *)gdat->gad->SpecialInfo)->LongInt;


  /* check on bounds */

  value = SetIntegerToBounds(gdat,value,&value_changed);

  WriteInteger(value,
	       gdat->data.integer.dest,
	       gdat->data.integer.bytelength);

  if (showchange && value_changed)
  {
    GT_SetGadgetAttrs(gdat->gad,h->data.win.win,NULL,
		      GTIN_Number,value,
		      TAG_DONE);
  }
}


/**------------------------------------------------------------------------**
 **  Copy the contents of a string-gadget to the buffer specified in
 **  its GadgetData.
 **------------------------------------------------------------------------**/

static void GetStringVal(struct GadgetData *gd)
{
  if (gd->data.string.dest == NULL) return; /* no destination available ! */

  strcpy (
	   gd->data.string.dest ,
	   ((struct StringInfo *)gd->gad->SpecialInfo)->Buffer
	 );
}


#ifndef DONT_NEED_GAD_DOUBLE

/**------------------------------------------------------------------------**
 **  Copy the value from a double (string) -gadget in its destination if
 **  available.
 **  (Just the same as GetIntegerVal(), but this time with double-values.)
 **------------------------------------------------------------------------**/

static void GetDoubleVal(struct GadgetData *gdat,
			 BOOL showchange,
			 struct Handle *h)
{
  double value;
  BOOL	 value_changed=FALSE;

  /* get value of gadget */

  sscanf( ((struct StringInfo *)gdat->gad->SpecialInfo)->Buffer,
	  "%lf",
	  &value
	);

  /* check on bounds */

  value = SetDoubleToBounds(gdat,value,&value_changed);

  if (gdat->data.string.dbl_dest)
    *(gdat->data.string.dbl_dest) = value;

  if (showchange) /* Show always when possible to provide a standart
		   * output format.
		   */
  {
    char buffer[25];

    sprintf(buffer,gdat->data.string.formatstr,value);
    GT_SetGadgetAttrs(gdat->gad,h->data.win.win,NULL,
		      GTST_String,buffer,
		      TAG_DONE);
  }
}

#endif


/**------------------------------------------------------------------------**
 **  Switch a flag. Used only by CHECKBOX_KIND gadgets.
 **------------------------------------------------------------------------**/

static void SwitchFlag(struct GadgetData *gdat)
{
  void *ptr;
  long flag;
  int  length;
  long flagfield;

  //printf("Switch !\n");

  assert(gdat->kind == CHECKBOX_KIND);

  ptr	 = gdat->data.check.dest;
  flag	 = gdat->data.check.switchflag;
  length = gdat->data.check.bytelength;

  if (ptr == NULL) return; /* no destination ! */

  flagfield = GetIntegerValue(ptr,length);  /* get field */

  if (gdat->data.check.selected) flagfield &= ~flag;  /* change flag */
  else				 flagfield |=  flag;

  gdat->data.check.selected = !gdat->data.check.selected;

  WriteInteger(flagfield,ptr,length); /* write field */
}


/**------------------------------------------------------------------------**
 **  Handle a IntuiMessage automatically.
 **------------------------------------------------------------------------**/

ULONG HandleIMsg(struct Handle *h,struct IntuiMessage *imsg)
{
  ULONG returnflags = 0;
  struct Gadget *g;	      /* points to gadget which caused the message */
  struct GadgetData *gdat;


  /* get the gadget and GadgetData, if this message has been sent by a gadget */

  if (imsg->Class == IDCMP_GADGETUP || imsg->Class == IDCMP_GADGETDOWN ||
      imsg->Class == IDCMP_MOUSEMOVE )
  {
    g = ((struct Gadget *)imsg->IAddress);

    assert(g != NULL);

    gdat = search_gadgetdata(h->data.win.gaddata,g);

    assert(gdat != NULL);
  }



  /*--------- message dispatcher ---------*/


  switch (imsg->Class)
  {
    case IDCMP_CLOSEWINDOW:
      if (h->data.win.winclosefunc) returnflags |= (h->data.win.winclosefunc());
      else			    returnflags |= EXIT_EXITWINDOW;
      break;

    case IDCMP_ACTIVEWINDOW:
      {
	struct Gadget *activegad;

	activegad = search_gadget(h->data.win.gaddata , h->data.win.activegad);
	assert (activegad != NULL);
	ActivateGadget(activegad,h->data.win.win,NULL);
      }
      break;

    case IDCMP_GADGETUP:

      assert(gdat != NULL);

      if (gdat->RealHandle)
      {
	struct Handle *poph=gdat->RealHandle;

	switch (poph->Type)
	{
	  case ha_Popup:
	    DoPopup(h,poph,imsg->MouseX + h->data.win.win->LeftEdge,
			   imsg->MouseY + h->data.win.win->TopEdge   );
	    break;

	  default:
	    assert(1==0);
	    break;
	}
      }
      else
      switch (gdat->kind)
      {

#ifndef DONT_NEED_GAD_INTEGER
	case INTEGER_KIND:
	  GetIntegerVal(gdat,TRUE,h);
	  break;
#endif

#if !defined(DONT_NEED_GAD_STRING) || !defined(DONT_NEED_GAD_DOUBLE)
	case STRING_KIND:
	  if (gdat->data.string.flags & STRING_DBL)
#ifndef DONT_NEED_GAD_DOUBLE
	    GetDoubleVal(gdat,TRUE,h);
#else
	    ;
#endif
	  else
#ifndef DONT_NEED_GAD_STRING
	    GetStringVal(gdat);
#else
	    ;
#endif
	  break;
#endif

#ifndef DONT_NEED_GAD_CHECKBOX
	case CHECKBOX_KIND:
	  //printf("Check!\n");
	  SwitchFlag(gdat);
	  break;
#endif

#ifndef DONT_NEED_GAD_CYCLE
	case CYCLE_KIND:
	  WriteInteger(imsg->Code , gdat->data.cycle.dest , gdat->data.cycle.bytelength);
	  break;
#endif

#ifndef DONT_NEED_GAD_LISTVIEW
	case LISTVIEW_KIND:
	  WriteInteger(imsg->Code , gdat->data.listview.dest , gdat->data.listview.bytelength);
	  break;
#endif

	case BUTTON_KIND:
	  if (gdat->data.button.popupfunc)
	    returnflags |=
	      (*gdat->data.button.popupfunc)(gdat->data.button.popuphandle,
					     gdat->data.button.popupnumber);
	  break;
      }

      if (gdat->flags & NEXT_ACTIVE)
      {
	struct Gadget *nextgad;

	nextgad=search_gadget(h->data.win.gaddata , gdat->nextactive_id);
	if (nextgad) ActivateGadget(nextgad,h->data.win.win,NULL);
      }

      //printf("Callback %p\n",gdat->func);

      if (gdat->func)
      {
	if (gdat->flags & TWO_PARAMETERS)
	  returnflags |= (*(FUNC2)gdat->func)(gdat->parameter,gdat->parameter2);
	else
	  returnflags |= (*gdat->func)(gdat->parameter);
      }

      break;
  }

  return returnflags;
}


/**------------------------------------------------------------------------**
 **  Rescue the contents of strings, integer, "double" ... - gadgets, as
 **  they don't always send a IDCMP_GADGETUP message, which makes it
 **  impossible to hear about all changes.
 **------------------------------------------------------------------------**/

void RescueGadInput(struct Handle *h)
{
  struct GadgetData *gd;


  /* scan throuh list of GadgetData and get value from each gadget */

  for (gd=h->data.win.gaddata ;
       gd != NULL;
       gd=gd->next)
  {
    switch (gd->kind)
    {
#if !defined(DONT_NEED_GAD_STRING) || !defined(DONT_NEED_GAD_DOUBLE)
      case STRING_KIND:
	if (gd->data.string.flags & STRING_DBL)
#ifndef DONT_NEED_GAD_DOUBLE
	  GetDoubleVal(gd,FALSE,0);
#else
	  ;
#endif
	else
#ifndef DONT_NEED_GAD_STRING
	  GetStringVal(gd);
#else
	  ;
#endif
	break;
#endif

#ifndef DONT_NEED_GAD_INTEGER
      case INTEGER_KIND:
	GetIntegerVal(gd,FALSE,0);
	break;
#endif

      case BUTTON_KIND:
      case CYCLE_KIND:
      case CHECKBOX_KIND:
      case LISTVIEW_KIND:
      case TEXT_KIND:
	break;

      case GENERIC_KIND:
	break;

      default:
	assert(1==2);
	break;
    }
  }
}


/**------------------------------------------------------------------------**
 **  Perform the standart event-handling.
 **  You may plug in your own signal-handling code via the 'mysigmask'- and
 **  'func'-parameters. Pass a mask of all signals that you want to handle
 **  by yourself. If a signal, set in this mask is recieved, 'func' will be
 **  called with the received signals (width the handle-private signals
 **  masked out). Return TRUE, if you want to leave the HandleHandle()-
 **  function (all gadget values will be saved !).
 **------------------------------------------------------------------------**/

void HandleHandle(struct Handle *h,ULONG mysigmask,ULONG (*func)(ULONG sigs))
{
  struct IntuiMessage *imsg,mymsg;
  BOOL	 exitloop = FALSE;
  ULONG  handlesignalmask,received;

  assert (h != HANDLE_ERR);
  assert (h != NULL);
  assert (h->Type == ha_Window);

  handlesignalmask = 1 << h->data.win.win->UserPort->mp_SigBit;

  while (!exitloop)
  {
    received=Wait(handlesignalmask | mysigmask);

    if (received & handlesignalmask)
      while(imsg=GT_GetIMsg(h->data.win.win->UserPort))
      {
	memcpy(&mymsg,imsg,sizeof(struct IntuiMessage));
	GT_ReplyIMsg(imsg);

	exitloop = HandleIMsg(h,&mymsg) & EXIT_EXITWINDOW;
      }

    if (received & mysigmask)
    {
      exitloop = func(received & (~handlesignalmask)) & EXIT_EXITWINDOW;
    }
  }

  RescueGadInput(h);
}


/**------------------------------------------------------------------------**
 **  Find a gadget-struct belonging to a ID.
 **------------------------------------------------------------------------**/

struct Gadget *GetGadget(struct Handle *h,ULONG id)
{
  return search_gadget(h->data.win.gaddata , id);
}


/**------------------------------------------------------------------------**
 **  Find the window belonging to the handle.
 **------------------------------------------------------------------------**/

struct Window *GetWindow(struct Handle *h)
{
  return h->data.win.win;
}

/**------------------------------------------------------------------------**
 **  disable or enable a gadget
 **------------------------------------------------------------------------**/

static struct GadgetData *find_gaddat(struct GadgetData *gd,ULONG id)
{
  if (gd==NULL) return NULL;
  if (gd->gad->GadgetID == id) return gd;
  return find_gaddat(gd->next,id);
}

void DisableGad(struct Handle *h,ULONG id,BOOL disable)
{
  struct GadgetData *gd;

  gd=find_gaddat(h->data.win.gaddata,id);
  assert(gd != NULL);

  if (  ((gd->flags & IS_DISABLED) && !disable) ||
       !((gd->flags & IS_DISABLED) &&  disable) )
  {
    gd->flags &= ~IS_DISABLED;
    if (disable) gd->flags |= IS_DISABLED;

    if (gd->kind == GENERIC_KIND)
    {
      if (disable)
	OffGadget(gd->gad,h->data.win.win,NULL);
      else
	OnGadget(gd->gad,h->data.win.win,NULL);
    }
    else
      GT_SetGadgetAttrs(gd->gad,h->data.win.win,NULL,GA_Disabled,disable,TAG_DONE);

  }
}


/**------------------------------------------------------------------------**
 **  Change the ProgressBox-Element.
 **  Pass 0 as numerator to clear the Box.
 **------------------------------------------------------------------------**/

void DisplayProgress(struct Handle *h,
		     struct Handle *prog_h,
		     int numerator,
		     int denominator)
{
  int lastpos;
  int endpos;
  struct RastPort *rp;

  /* as this is the pointer to the BevelBox (!) , go into the BevelBox */

  prog_h = prog_h -> in;


  /* cache important values */

  rp=h->data.win.rp;


  /* if numerator is 0, clear the ProgressDisplay */

  if (numerator==0)
  {
    SetAPen (rp , 0);
    RectFill(rp , prog_h->x , prog_h->y ,
		  prog_h->x + prog_h->w -1 ,
		  prog_h->y + prog_h->h -1);

    prog_h->data.progress.lastpos    =0;
    prog_h->data.progress.lastpercent=255; // out of range
    return;
  }

  lastpos=prog_h->data.progress.lastpos;
  if (lastpos==0) lastpos=prog_h->x;

  endpos = prog_h->x + ((prog_h->w-1)*numerator)/denominator;

  if (lastpos == endpos) return;

  SetAPen (rp , prog_h->data.progress.color);
  RectFill(rp , lastpos , prog_h->y ,
		endpos	, prog_h->y + prog_h->h -1);


  /* write percent in the middle */

  if (prog_h->data.progress.flags & SHOW_PERCENT)
  {
    int percent;
    int width;
    char buffer[5];
    int x,y;
    int start,end;

    percent = (100*numerator)/denominator;

    SetFont(rp,prog_h->data.progress.font);

    sprintf(buffer,"%d%%",percent);
    width=TextLength(rp,buffer,strlen(buffer));
    y = prog_h->y+(prog_h->h - prog_h->data.progress.font->tf_YSize)/2;
    y += prog_h->data.progress.font->tf_Baseline;
    x = prog_h->x+(prog_h->w-width)/2;

    start = prog_h->data.progress.over_start;
    end   = prog_h->data.progress.over_end;

    Move(rp,x,y);

    if (start == 0 && end == 0)
    {
      SetAPen(rp,prog_h->data.progress.textcolor);
      SetDrMd(rp,JAM1);
      Text(rp,buffer,strlen(buffer));
      SetDrMd(rp,JAM2);
    }
    else
    if (endpos<start)
    {
      if (prog_h->data.progress.lastpercent != percent)
      {
	SetAPen(rp,0);
	RectFill(rp,start,prog_h->y,end-1,prog_h->y+prog_h->h-1);

	SetAPen(rp,prog_h->data.progress.textcolor);
	Text(rp,buffer,strlen(buffer));
      }
    }
    else
    if (endpos+1 >= end)
    {
      if (prog_h->data.progress.lastpercent != percent)
      {
	SetAPen(rp,prog_h->data.progress.color);
	RectFill(rp,start,prog_h->y,end-1,prog_h->y+prog_h->h-1);

	SetAPen(rp,prog_h->data.progress.textcolor);
	SetDrMd(rp,JAM1);
	Text(rp,buffer,strlen(buffer));
	SetDrMd(rp,JAM2);
      }
    }
    else
    {
      SetAPen(rp,prog_h->data.progress.color);
      RectFill(rp,start,prog_h->y,endpos,prog_h->y+prog_h->h-1);

      SetAPen(rp,0);
      RectFill(rp,endpos+1,prog_h->y,end-1,prog_h->y+prog_h->h-1);

      SetAPen(rp,prog_h->data.progress.textcolor);
      SetDrMd(rp,JAM1);
      Text(rp,buffer,strlen(buffer));
      SetDrMd(rp,JAM2);
    }

    prog_h->data.progress.over_start=x;
    prog_h->data.progress.over_end  =x+width;

    prog_h->data.progress.lastpercent = percent;
  }

  prog_h->data.progress.lastpos = endpos;
}


/**------------------------------------------------------------------------**
 **  ChangeText() - Change the text displayed in a TEXT_KIND gadget.
 **------------------------------------------------------------------------**/

void ChangeText(struct Handle *h,ULONG id,char *text)
{
  struct GadgetData *gd;

  gd=find_gaddat(h->data.win.gaddata,id);
  assert(gd != NULL);

  GT_SetGadgetAttrs(gd->gad,h->data.win.win,NULL,
		    GTTX_Text,text,
		    TAG_DONE);
}


/**------------------------------------------------------------------------**
 **  change the disabled-state of a popup-gadget item
 **------------------------------------------------------------------------**/

void SetPopupDisable(struct Handle *h,ULONG itemID,BOOL disable)
{
  struct TagItem *ti;

  assert(h->Type == ha_Raster);

  h=h->in->right->data.newgad.gaddata->RealHandle;  /* oh, this is STUPID !!! code */

  assert(h->Type == ha_Popup);


  for (ti=h->data.popup.stringlist ; ti->ti_Tag != NULL ; ti++)
  {
    if ((ti->ti_Data & ~(BIT31)) == (itemID & ~(BIT31)))
	goto found;
  }

  assert(1==2); /* item not found */

found:
  if (disable) ti->ti_Data |=  BIT31;
	  else ti->ti_Data &= ~BIT31;
}


/**------------------------------------------------------------------------**
 **  select a different popup item.
 **------------------------------------------------------------------------**/

void SetPopup(struct Handle *master,struct Handle *poph,ULONG itemnr)
{
  struct TagItem *ti;

  poph=poph->in->right->data.newgad.gaddata->RealHandle;  /* oh, this is STUPID !!! code */

  if (poph->data.popup.flags & POPUP_BIT31DISABLE)
    itemnr &= ~BIT31;

  for (ti=poph->data.popup.stringlist ;
       ti->ti_Tag != NULL ;
       ti++)
  {
    if (poph->data.popup.flags & POPUP_BIT31DISABLE)
    {
      if ((ti->ti_Data & ~BIT31) == itemnr) break;
    }
    else
      if (ti->ti_Data == itemnr) break;
  }

  assert (ti->ti_Tag != NULL);


  poph->data.popup.selection = ti->ti_Data;

  ShowPopupSelection(poph,master->data.win.win);
}


/**------------------------------------------------------------------------**
 **  WriteTextLine - write a line of text in a TextDisplay
 **------------------------------------------------------------------------**/

void WriteTextLine(struct Handle *h,struct Handle *td_handle,char *txt,LONG color)
{
  struct RastPort *rp = h->data.win.rp;
  UWORD 	   textcolor;
  int		   i;
  char		  *ptr = txt;
  int		   begin_y;

  struct Region    *reg,*oldregion;
  struct Rectangle rect;


  if (color == -1) textcolor = td_handle->data.textdisplay.color;
	      else textcolor = color;

  SetAPen(rp,textcolor);
  SetFont(rp,td_handle->data.textdisplay.font);

  if (!(reg=NewRegion())) return;

  rect.MinX=td_handle->x;
  rect.MaxX=td_handle->x+td_handle->w-1;
  rect.MinY=td_handle->y;
  rect.MaxY=td_handle->y+td_handle->h-1;

  if (!OrRectRegion(reg,&rect))
    goto cleanup;

  oldregion=InstallClipRegion(h->data.win.win->WLayer,reg);

  begin_y = td_handle->y + td_handle->h -
	    td_handle->data.textdisplay.font->tf_YSize +
	    td_handle->data.textdisplay.font->tf_Baseline;


  /* while text not entirely written */

  while (ptr < txt+strlen(txt))
  {
    /* draw the text or try to break text if it doesn't fit */

    for (i=strlen(ptr) ; i>0 ; i--)
      if (ptr[i] == ' ' || ptr[i] == '\0')
      {
	if (TextLength(rp,ptr,i) <= td_handle->w)
	{
	  int y;
	  int line;

	  y = begin_y + td_handle->data.textdisplay.font->tf_YSize;

	  for (line=0;line<td_handle->data.textdisplay.font->tf_YSize+1;line++)
	  {
	    ScrollRaster(rp,0,1,td_handle->x,td_handle->y,  /* why not: +1, ??? */
				td_handle->x+td_handle->w-1,
				td_handle->y+td_handle->h-1);
	    y--;

	    Move(rp,td_handle->x,y);
	    Text(rp,ptr,i);

	    Delay(1);
	  }

	  ptr = &ptr[i+1];
	  break;
	}
      }
  }

  InstallClipRegion(h->data.win.win->WLayer,oldregion);

cleanup:
  DisposeRegion(reg);

}

