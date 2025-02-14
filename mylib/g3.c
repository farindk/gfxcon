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


static void CalcMinSize(struct Handle *);
static void CalcPositions(struct Handle *,int,int);
static void CalcSizes(struct Handle *);


/**------------------------------------------------------------------------**
 **  Return the maximum minimum (??? - !!!) height of a row in a raster
 **------------------------------------------------------------------------**/

static int MaxRowMinHeight(struct Handle *h)
{
  struct Handle *p;
  int height;

  height=0;
  for (p=h;p != HANDLE_END;p=p->right)
    height = max(height,p->minh);

  return height;
}

/**------------------------------------------------------------------------**
 **  Return the maximum minimum (??? - !!!) width of a column in a raster
 **------------------------------------------------------------------------**/

static int MaxColumnMinWidth(struct Handle *h)
{
  struct Handle *p;
  int width;

  width=0;
  for (p=h;p != HANDLE_END;p=p->down)
    width = max(width,p->minw);

  return width;
}


/**------------------------------------------------------------------------**
 **  Calculate minimum size of a Raster.
 **------------------------------------------------------------------------**/

static void CalcMinRasterSize(struct Handle *h)
{
  struct Handle *p1,*p2;


  /* Calculate minimum size of all subhandles */

  for (p1=h->in ; p1 != NULL ; p1=p1->right)
  for (p2=p1    ; p2 != NULL ; p2=p2->down)
    CalcMinSize(p2);


  /* calc minimum width */

  if (h->data.raster.flags & EQUAL_WIDTH)
  {
    int maxwidth=0;

    for (p1=h->in ; p1 != NULL ; p1=p1->right) /* add width of all columns */
    {
      int w = MaxColumnMinWidth(p1);
      maxwidth = max(maxwidth,w);
    }

    h->minw = h->data.raster.num_x * maxwidth;
    h->data.raster.max_sub_width = maxwidth;
  }
  else
  {
    h->minw=0;

    for (p1=h->in ; p1 != NULL ; p1=p1->right) /* add width of all columns */
      h->minw += MaxColumnMinWidth(p1);
  }

  h->minw += (h->data.raster.num_x-1) * h->data.raster.spacex;


  /* calc minimum height */

  if (h->data.raster.flags & EQUAL_HEIGHT)
  {
    int maxheight=0;
    for (p1=h->in ; p1 != NULL ; p1=p1->down) /* add height of all rows */
    {
      int h = MaxRowMinHeight(p1);
      maxheight = max(maxheight,h);
    }

    h->minh = h->data.raster.num_y * maxheight;
    h->data.raster.max_sub_height = maxheight;
  }
  else
  {
    h->minh=0;

    for (p1=h->in ; p1 != NULL ; p1=p1->down) /* add height of all rows */
      h->minh  += MaxRowMinHeight(p1);
  }

  h->minh += (h->data.raster.num_y-1) * h->data.raster.spacey;
}


/**------------------------------------------------------------------------**
 **  Calc minimum size of this handle (and all handles below this one)
 **------------------------------------------------------------------------**/

static void CalcMinSize(struct Handle *h)
{
  switch (h->Type)
  {
    case ha_Window:
    case ha_BevelBox:
    case ha_SpaceBox:
    case ha_TextBox:
      CalcMinSize(h->in); /* recursively call me again */

      h->minw = h->in->minw + h->shrinkw;
      h->minh = h->in->minh + h->shrinkh;


      /*--- special element - calculations ---*/

      /* make a TextBox that width that its title can be drawn completely */

#ifndef DONT_NEED_TEXTBOX

      if (h->Type == ha_TextBox)
      {
	CalcMinSize(h->data.textbox.texthandle);
	h->minw = max(h->minw , h->data.textbox.texthandle->minw + h->shrinkw);
      }
#endif

      break;

    case ha_Text:
    case ha_Filler:
    case ha_NewGadget:
    case ha_ProgressBox:
    case ha_Image:
    case ha_Popup:
    case ha_TextDisplay:

      /* these are already set */

      break;

    case ha_Raster:
      CalcMinRasterSize(h);
      break;

    default:
      assert(1==2);
      break;
  }
}


/**------------------------------------------------------------------------**
 **  Calculate sizes of the subhandles of a raster.
 **------------------------------------------------------------------------**/

static void CalcRasterSizes(struct Handle *h)
{
  struct Handle *p1,*p2,*p;
  int	 portion[NSUBHANDLES];
  int	 pri_sum;
  int	 diff;
  int	 i;
  int	 sum;

  /*-------------------- calculate widths ----------------------*/

  /* calculate priority-sum */

  pri_sum=0;
  for (p=h->in ; p != HANDLE_END ; p=p->right)
    pri_sum += p->xpri;


  /* difference between available size and current minimum size */

  diff = h->w - h->minw;

  /* calculate additional space-portion which each handle-element will receive */

  sum=0; /* the total space distributed */

  for (p=h->in ,i=0 ; p != HANDLE_END ; p=p->right , i++)
  {
    if (pri_sum) portion[i] = (p->xpri * diff) / pri_sum;
    else	 portion[i] = 0;

    sum += portion[i];
  }


  /* distribute the "rounding error" */

  if (pri_sum)
  {
    for (i=0;sum < diff;i++) { portion[i]++; sum++; }
    for (i=0;sum > diff;i++) { portion[i]--; sum--; }
  }


  /* add portion to width of each column */

  for (p1=h->in , i=0 ; p1 != HANDLE_END ; p1=p1->right , i++)
  {
    for (p2=p1 ; p2 != HANDLE_END ; p2=p2->down)
      if (h->data.raster.flags & EQUAL_WIDTH)
	p2->w = h->data.raster.max_sub_width + portion[i];
      else
	p2->w = MaxColumnMinWidth(p1)+portion[i];
  }




  /*-------------------- calculate heights ----------------------*/

  /* calculate priority-sum */

  for (p=h->in , pri_sum=0 ; p != HANDLE_END ; p=p->down)
    pri_sum += p->ypri;


  /* difference between available size and current minimum size */

  diff = h->h - h->minh;


  /* calculate portion which each handle-element will receive */

  sum=0; /* the total space distributed */

  for (p=h->in ,i=0 ; p != HANDLE_END ; p=p->down , i++)
  {
    if (pri_sum) portion[i] = (p->ypri * diff) / pri_sum;
    else	 portion[i] = 0;

    sum += portion[i];
  }


  /* distribute the "rounding error" */

  if (pri_sum)
  {
    for (i=0;sum < diff;i++) { portion[i]++; sum++; }
    for (i=0;sum > diff;i++) { portion[i]--; sum--; }
  }


  /* add portion to height of each row, or reset it to its minimum size */

  for (p1=h->in , i=0 ; p1 != HANDLE_END ; p1=p1->down , i++)
  {
    for (p2=p1 ; p2 != HANDLE_END ; p2=p2->right)
      if (h->data.raster.flags & EQUAL_WIDTH)
	p2->h = h->data.raster.max_sub_height + portion[i];
      else
	p2->h = MaxRowMinHeight(p1)+portion[i];
  }




  /* calc sizes if sub-handles */

  for (p1=h->in ; p1 != HANDLE_END ; p1=p1->down)
    for (p2=p1 ; p2 != HANDLE_END ; p2=p2->right)
      CalcSizes(p2);
}


/**------------------------------------------------------------------------**
 **  Set the sizes of the SUB-Handles !
 **------------------------------------------------------------------------**/

static void CalcSizes(struct Handle *h)
{
  /*--------- restore aspect of this element -------------------*/

  if (h->flags & KEEP_ASPECT)
  {
    assert(h->w > 0);
    assert(h->h > 0);
    assert(h->minw > 0);
    assert(h->minh > 0);

    if (h->w / h->h < h->minw / h->minh)  /* too height */
    {
      h->h = ( h->w / h->minw ) * h->minh;
    }
    else
    if (h->w / h->h > h->minw / h->minh)  /* too width  */
    {
      h->w = ( h->h / h->minh ) * h->minw;
    }
  }


  switch (h->Type)
  {
    case ha_Window:
    case ha_BevelBox:
    case ha_SpaceBox:
    case ha_TextBox:

      /* If interior wants to expand in a direction, let it do so.
       * If not, it keeps its minimum size.
       */

      if (h->in->xpri) h->in->w = h->w - h->shrinkw;
		  else h->in->w = h->in->minw;
      if (h->in->ypri) h->in->h = h->h - h->shrinkh;
		  else h->in->h = h->in->minh;

      CalcSizes(h->in);


      /*--- special calculations ---*/

#ifndef DONT_NEED_TEXTBOX

      /* calc size of TextBox-title text */

      if (h->Type == ha_TextBox)
      {
	h->data.textbox.texthandle->w = h->w;
	h->data.textbox.texthandle->h = h->data.textbox.texthandle->minh;
      }

#endif

      /* set window-dimensions (window will collapse to interior bound) */

      if (h->Type == ha_Window)
      {
	h->w = h->in->w + h->shrinkw;
	h->h = h->in->h + h->shrinkh;
      }

      break;

    case ha_Raster:
      CalcRasterSizes(h);
      break;

    case ha_Text:
    case ha_Filler:
    case ha_ProgressBox:
    case ha_Image:
    case ha_Popup:
    case ha_TextDisplay:
      break;

    case ha_NewGadget:
      if (h->data.newgad.kind == LISTVIEW_KIND)
      {
	struct Handle *strh;

	strh = h->data.newgad.gaddata->data.listview.strgadget;
	strh->w = h->w;
	strh->h = strh->minh;
	strh->x = h->x;
	strh->y = h->y+h->h;
      }

      if (h->data.newgad.kind == GENERIC_KIND)
      {
	if (h->data.newgad.gaddata->ImageNormHandle)
	{
	  h->data.newgad.gaddata->ImageNormHandle->w = h->w;
	  h->data.newgad.gaddata->ImageNormHandle->h = h->h;
	}

	if (h->data.newgad.gaddata->ImageSelectHandle)
	{
	  h->data.newgad.gaddata->ImageSelectHandle->w = h->w;
	  h->data.newgad.gaddata->ImageSelectHandle->h = h->h;
	}
      }
      break;

    default:
      assert(1==2);
      break;
  }
}


/**------------------------------------------------------------------------**
 **  Calculate positions of raster-subhandles
 **------------------------------------------------------------------------**/

static void CalcRasterPos(struct Handle *h,int xpos,int ypos)
{
  int x,y;
  struct Handle *p1,*p2;

  x=xpos;
  for (p1=h->in ; p1 != HANDLE_END ; p1=p1->right)
  {
    y=ypos;
    for (p2=p1 ; p2 != HANDLE_END ; p2=p2->down)
    {
      CalcPositions(p2,x,y);
      y += p2->h + h->data.raster.spacey;
    }
    x += p1->w + h->data.raster.spacex;
  }
}


/**------------------------------------------------------------------------**
 **  Only set the positions in parameter list, if this handle hasn't the
 **  ABSOLUTE_POS - flag set.
 **------------------------------------------------------------------------**/

void SetPosition(struct Handle *h,int x,int y)
{
  if (h->flags & ABSOLUTE_POS) return;
  else			       {
				 h->x = x;
				 h->y = y;
			       }
}

/**------------------------------------------------------------------------**
 **  Calculate position of the handle (set it to (x,y) ) and the subhandles.
 **------------------------------------------------------------------------**/

static void CalcPositions(struct Handle *h,int x,int y)
{
  switch (h->Type)
  {
    case ha_Window:
      CalcPositions( h->in,
		     h->leftborder,
		     h->topborder
		   );
      break;

    case ha_BevelBox:
    case ha_SpaceBox:
    case ha_TextBox:
      SetPosition(h,x,y);
      CalcPositions( h->in,
		     h->x+h->leftborder + (h->w - h->shrinkw - h->in->w)/2,
		     h->y+h->topborder	+ (h->h - h->shrinkh - h->in->h)/2
		   );

      /* more stuff for more complex elements */

      if (h->Type == ha_TextBox)
      {
	CalcPositions(h->data.textbox.texthandle,
		      h->x,
		      h->y);
      }
      break;

    case ha_Text:
    case ha_Filler:
    case ha_NewGadget:
    case ha_ProgressBox:
    case ha_Image:
    case ha_Popup:
    case ha_TextDisplay:
      SetPosition(h,x,y);
      break;

    case ha_Raster:
      SetPosition(h,x,y);
      CalcRasterPos(h,h->x,h->y);
      break;

    default:
      assert(1==2);
      break;
  }
}


/**------------------------------------------------------------------------**
 **  Do all the size and position calculations. This function will fail if
 **  the window doesn't fit on the screen.
 **------------------------------------------------------------------------**/

BOOL ComputeGadgets(struct Handle *h)
{
  /* check for input errors */

  assert(h->Type == ha_Window);


  /* calculate minimum size */

  CalcMinSize(h);


  /* if element (window) doesn't fit on the screen, return FALSE. */

  if (h->minw > h->w ||
      h->minh > h->h	)
  {
    SetError(WIN_DOESNT_FIT_ON_SCREEN);
    return FALSE;
  }


  /* calculate sizes of elements */

  CalcSizes(h);


  /* calculate element positions */

  CalcPositions(h,-1,-1);  /* ha_Window will ignore the x,y parameters */

  return TRUE;
}

