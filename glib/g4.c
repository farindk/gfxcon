
#include "glib:incs.h"
#include "glib:gstr.h"
#include "glib:g.h"
#include "glib:errs.h"
#include "glib:txtsys.h"
#include "glib:clean.h"


static struct CreateElementsData
{
  /* fields that are only valid during gadget construction ! (OpenHandle() ) */

  struct Gadget      *lastgad;
  struct Window      *win;
  struct RastPort    *rp;
  struct DrawInfo    *drawinfo;

  APTR                VisualInfo;
};

static struct CreateElementsData CrElData;


/**------------------------------------------------------------------------**
 **  Get memory for a bitplane
 **------------------------------------------------------------------------**/

static void FreePlanes(UBYTE *ptr)
{
  FreeVec(ptr);
}

static BOOL GetPlanes(int width,int height,struct BitMap *bm)
{
  UBYTE *plane;
  int    i;

  /* alloc bitplane(s) */

  width = ((width+15)/16)*16;

  plane=AllocVec( bm->Depth * height * width/8 , MEMF_CHIP | MEMF_CLEAR );
  if (plane) cuinsert_CallFunction(&FreePlanes,plane);
  else       return FALSE;


  /* set pointers */

  for (i=0;i<bm->Depth;i++)
    bm->Planes[i] = plane + i * height * width/8;


  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Create an Image
 **------------------------------------------------------------------------**/

static BOOL CreateImage(struct Handle *h)
{
  struct BitMap   bm;
  struct RastPort rp;
  struct DrawArea da;
  struct TmpRas   tr;
  PLANEPTR        tmpras_plane;


  /* init BitMap */

  InitBitMap(&bm, h->data.image.image.Depth , h->w , h->h);


  /* alloc planes */

  if (!GetPlanes(h->w,h->h,&bm)) return FALSE;


  /* init RastPort */

  InitRastPort(&rp);
  rp.BitMap = &bm;


  /* init TmpRas */

  tmpras_plane = AllocRaster(h->w,h->h);
  if (!tmpras_plane) return FALSE;

  InitTmpRas(&tr,tmpras_plane, RASSIZE(h->w,h->h));
  rp.TmpRas = &tr;


  /* init Image-structure */

  h->data.image.image.LeftEdge  = h->x;
  h->data.image.image.TopEdge   = h->y;
  h->data.image.image.Width     = h->w;
  h->data.image.image.Height    = h->h;
  h->data.image.image.ImageData = (UWORD *)bm.Planes[0];


  /* Init DrawArea */

  da.xdiv     = h->data.image.xsize;
  da.ydiv     = h->data.image.ysize;

  if (h->data.image.flags & AUTO_BEVELBOX)
  {
    da.xmul     = h->w - 4;
    da.ymul     = h->h - 2;
    da.xplus    = 2;
    da.yplus    = 1;
  }
  else
  {
    da.xmul     = h->w;
    da.ymul     = h->h;
    da.xplus    = 0;
    da.yplus    = 0;
  }

  da.rp       = &rp;
  da.drawinfo = CrElData.drawinfo;


  /* Draw the Image */

  (*(h->data.image.drawfunc))(&da);


  /* Draw a bevelbox around the image */

  if (h->data.image.flags & AUTO_BEVELBOX)
  {
    DrawBevelBox(&rp,0,0,h->w,h->h,
                  GT_VisualInfo,CrElData.VisualInfo,
                  (h->data.image.flags & BEVELBOX_RECESSED) ? GTBB_Recessed :
                                                              TAG_IGNORE , TRUE ,
                  TAG_DONE);
  }


  FreeRaster(tmpras_plane,h->w,h->h);

  return TRUE;
}

/**------------------------------------------------------------------------**
 **  Draw the element which needs extra drawing (Texts, BevelBoxes ...) .
 **------------------------------------------------------------------------**/

static void DrawHandle(struct Handle *h)
{
  int x,y; /* for all purposes; TAKE CARE !!! */


  if (h->flags & DONT_DRAW) return;

  switch (h->Type)
  {
    case ha_Window:
    case ha_SpaceBox:
    case ha_Filler:
    case ha_NewGadget:
    case ha_ProgressBox:
    case ha_TextDisplay:
      break;

#ifndef DONT_NEED_TEXT

    case ha_Text:  /* text will be drawn centered into the bounding box */
      x = h->x + (h->w - h->minw)/2;
      y = h->y + (h->h - h->minh)/2;

      SetAPen(CrElData.rp , h->data.text.color);
      SetFont(CrElData.rp , h->data.text.font);

      Move(CrElData.rp , x , y + h->data.text.font->tf_Baseline);
      Text(CrElData.rp , h->data.text.text , strlen(h->data.text.text) );
      break;

#endif

#ifndef DONT_NEED_BEVELBOX

    case ha_BevelBox:
      DrawBevelBox(CrElData.rp, h->x, h->y , h->w, h->h,
                    GT_VisualInfo,CrElData.VisualInfo,
                    h->data.bevelbox.recessed ? GTBB_Recessed : TAG_IGNORE,TRUE,
                    TAG_DONE);
      break;

#endif

#ifndef DONT_NEED_TEXTBOX

    case ha_TextBox:

      /* draw BevelBox, title-text will be drawn with an independent handle */

      y=h->data.textbox.texthandle->data.text.font->tf_YSize/2; /* top box offset */

      DrawBevelBox(CrElData.rp, h->x, h->y + y , h->w, h->h-y,
                   GT_VisualInfo,CrElData.VisualInfo,
                   GTBB_Recessed,TRUE,
                   TAG_DONE);
      DrawBevelBox(CrElData.rp, h->x+2, h->y + y+1 , h->w-4, h->h-y-2,
                   GT_VisualInfo,CrElData.VisualInfo,
                   TAG_DONE);
      break;

#endif

    case ha_Image:
      DrawImage(CrElData.rp, &h->data.image.image , 0 , 0 );
      break;

    case ha_Popup:
      ShowPopupSelection(h,CrElData.win);
      break;

    default:
      assert(1==2);
      break;
  }
}


/**------------------------------------------------------------------------**
 **  This is the main opening function, which opens the window after it
 **  has opened all subordinated handles. Then it will draw all window gfx.
 **------------------------------------------------------------------------**/

static void FreeDrawInfo(struct Handle *h)
{
  FreeScreenDrawInfo(h->data.win.scr,h->data.win.drawinfo);
}

static void CallUnlockScreen(struct Screen *scr)
{
  UnlockPubScreen(NULL,scr);
}

static BOOL OpenWindowHandle(struct Handle *h)
{
  struct Handle *list;
  struct Window *win;
  BOOL           success;


  /* init CrElData fields local to each window construction */

  CrElData.rp      = NULL; /* only for security */
  CrElData.win     = NULL; /* only for security */


  /* Lock screen */

  if (!h->data.win.scr)
  {
    h->data.win.scr=LockPubScreen(NULL); /* lock the screen */

    if (!h->data.win.scr)
    {
      SetError(CANT_LOCK_WBSCREEN);
      return FALSE;
    }

    cuinsert_CallFunction(&CallUnlockScreen,h->data.win.scr);
  }


  /* get drawinfo */

  h->data.win.drawinfo=GetScreenDrawInfo(h->data.win.scr);

  if (!h->data.win.drawinfo) { SetError(COULDNT_GET_DRAWINFO);
                               return FALSE;
                             }

  cuinsert_CallFunction(&FreeDrawInfo,h);
  CrElData.drawinfo = h->data.win.drawinfo;


  /* init gadgetlist */

  h->data.win.glist = NULL;
  CrElData.lastgad = CreateContext(&h->data.win.glist);

  /* Init all handles belonging to this window.
   * Scan the list twice, opening the SUBORDINATE handles in the first pass
   * and the other in the second.
   */

  for (list=h->data.win.handlelist , success=TRUE ;
       list != NULL && success;
       list=list->next)
  {
    if (list->flags & SUBORDINATE)
      success=OpenHandle(list);
  }

  for (list=h->data.win.handlelist ;
       list != NULL && success;
       list=list->next)
  {
    if (!(list->flags & SUBORDINATE))
      success=OpenHandle(list);
  }

  cuinsert_GadgetList(h->data.win.glist);

  if (!success) return FALSE;

  {
    struct TagItem WinPrologTags[] =
    {
      { WA_Width   , 0 },
      { WA_Height  , 0 },
      { TAG_IGNORE , 0 },
      { TAG_IGNORE , 0 },
      { WA_Gadgets , 0 },
      { WA_IDCMP   , 0 },
      { TAG_MORE   , 0 }
    };

#   define WINPRO_W     0
#   define WINPRO_H     1
#   define WINPRO_L     2
#   define WINPRO_T     3
#   define WINPRO_GADS  4
#   define WINPRO_IDCMP 5
#   define WINPRO_MORE  6


    /* try to open the window */

    /* init window-prolog */

    WinPrologTags[ WINPRO_W    ].ti_Data = h->w;
    WinPrologTags[ WINPRO_H    ].ti_Data = h->h;

    if (h->data.win.glist != NULL)
    {
      WinPrologTags[ WINPRO_GADS ].ti_Tag  = WA_Gadgets; /* as this can be cleared */
      WinPrologTags[ WINPRO_GADS ].ti_Data = (ULONG)h->data.win.glist;
    }
    else
      WinPrologTags[ WINPRO_GADS ].ti_Tag  = TAG_IGNORE;

    WinPrologTags[ WINPRO_IDCMP ].ti_Data = h->data.win.IDCMP;

    WinPrologTags[ WINPRO_MORE  ].ti_Data = (ULONG)h->data.win.taglist;

    if (h->data.win.center_handle)
    {
      WinPrologTags[WINPRO_L].ti_Tag  = WA_Left;
      WinPrologTags[WINPRO_T].ti_Tag  = WA_Top;

      WinPrologTags[WINPRO_L].ti_Data = (h->data.win.center_handle->w - h->w)/2 +
                                         h->data.win.center_handle->data.win.win->LeftEdge;
      WinPrologTags[WINPRO_T].ti_Data = (h->data.win.center_handle->h - h->h)/2 +
                                         h->data.win.center_handle->data.win.win->TopEdge;
    }

    /* open window */

    win = OpenWindowTagList( NULL , WinPrologTags );

    if (cuinsert_OpenWindow(win))
    {
      /* get window-datas */

      h->data.win.win   =   CrElData.win   =   win;
      h->data.win.rp    =   CrElData.rp    =   win->RPort;
    }
    else
      return FALSE;
  }


  /* draw graphics for all handles, SUBORDINATE first */

  GT_RefreshWindow(win,NULL);

  for (list=h->data.win.handlelist ; list != NULL ; list=list->next)
  {
    if (list->flags & SUBORDINATE)
      DrawHandle(list);
  }

  for (list=h->data.win.handlelist ; list != NULL ; list=list->next)
  {
    if (!(list->flags & SUBORDINATE))
      DrawHandle(list);
  }


  return TRUE;
}

BOOL OpenGadget(struct Handle *h)
{
  struct Gadget  *gad;
  struct TagItem *taglist;
  struct GadgetData *gdat;


  /* check input */

  assert (h->Type == ha_NewGadget);
  assert (h != NULL);
  assert (h != HANDLE_ERR);


  /* fill NewGadget with position and size */

  gdat=h->data.newgad.gaddata;

  if (h->flags & ABSOLUTE_WIDTH)
  {
    h->data.newgad.ng.ng_LeftEdge   = h->x + (h->w - h->minw)/2;
    h->data.newgad.ng.ng_Width      = h->minw;
  }
  else
  {
    h->data.newgad.ng.ng_LeftEdge   = h->x;
    h->data.newgad.ng.ng_Width      = h->w;
  }

  if (h->flags & ABSOLUTE_HEIGHT)
  {
    h->data.newgad.ng.ng_TopEdge    = h->y + (h->h - h->minh)/2;
    h->data.newgad.ng.ng_Height     = h->minh;
  }
  else
  {
    h->data.newgad.ng.ng_TopEdge    = h->y;
    h->data.newgad.ng.ng_Height     = h->h;
  }


  if (h->flags & ABSOLUTE_POS)
  {
    h->data.newgad.ng.ng_TopEdge  = h->y;
    h->data.newgad.ng.ng_LeftEdge = h->x;
  }


  /* set misc. values */

  h->data.newgad.ng.ng_VisualInfo = CrElData.VisualInfo;

  taglist=h->data.newgad.taglist;

#ifndef DONT_NEED_GAD_LISTVIEW

  /* handle ListView-Gadget (String-Gadget) */

  if (h->data.newgad.kind == LISTVIEW_KIND)
  {
    static struct TagItem listview_prolog[]=
    {
      GTLV_ShowSelected,NULL,
      TAG_MORE         ,NULL
    };

    if (gdat->data.listview.strgadget)
    {
      listview_prolog[0].ti_Data = (ULONG)gdat->data.listview.strgadget->
                                            data.newgad.gaddata->
                                              gad;
      listview_prolog[1].ti_Data = (ULONG) taglist;

      taglist=listview_prolog;
    }
  }

#endif

  /* create gadget */

  gad=CreateGadgetA(  h->data.newgad.kind ,
                      CrElData.lastgad    ,
                     &h->data.newgad.ng   ,
                      taglist );

  gdat->gad = gad;

  if (h->data.newgad.saveptr)
    *(h->data.newgad.saveptr) = gad;

  CrElData.lastgad = gad;

  if (!gad)
  {
    SetError(CANT_CREATE_GADGET);
    return FALSE;
  }


  /* handle custom gadgets */

  if (h->data.newgad.kind == GENERIC_KIND)
  {
    gad->Flags         = gdat->Flags;
    gad->Activation    = gdat->Activation;
    gad->SpecialInfo   = gdat->SpecialInfo;
    gad->GadgetType   |= gdat->GadgetType;

    if (gdat->ImageNormHandle)
      gad->GadgetRender  = &(gdat->ImageNormHandle->data.image.image);

    if (gdat->ImageSelectHandle)
      gad->SelectRender  = &(gdat->ImageSelectHandle->data.image.image);
  }

  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Universal open handle - function which can handle all types of handles.
 **------------------------------------------------------------------------**/

BOOL OpenHandle(struct Handle *h)
{
  BOOL success;

  assert(h != NULL);
  assert(h != HANDLE_ERR);

  switch (h->Type)
  {
    case ha_Window:

      /* get new CleanupList to store all allocations made during opening. */

      if (!(h->data.win.open_cleanup = NewCleanup(h->data.win.cleanup)))
      {
        success=FALSE;
        break;
      }


      /* reverse list (to get the String-gadgets in the right order, as we
       * don't want to confuse the users with the TAB-key).
       */

      {
        struct Handle *newlist=NULL;
        struct Handle *current;
        struct Handle *next;

        for (current = h->data.win.handlelist ;
             current != NULL ;
             current = next)
        {
          next = current->next;
          current->next = newlist;
          newlist = current;
        }

        h->data.win.handlelist = newlist;
      }


      /* set misc. values */

      CrElData.VisualInfo = h->data.win.VisualInfo;

      success=OpenWindowHandle(h);

      CloseCleanup();
      break;

    case ha_NewGadget:
      success=OpenGadget(h);
      break;

    case ha_Text:
    case ha_BevelBox:
    case ha_SpaceBox:
    case ha_TextBox:
    case ha_Filler:
    case ha_ProgressBox:
    case ha_Popup:
    case ha_TextDisplay:
      break;

    case ha_Image:
      CreateImage(h);
      break;

    default:
      assert(1==2);
      break;
  }

  return success;
}

/**------------------------------------------------------------------------**
 **  Close the window(s) ...
 **  The handle may be reused !!!
 **------------------------------------------------------------------------**/

void CloseHandle(struct Handle *h)
{
  assert(h->Type == ha_Window);

  DoCleanupFree(h->data.win.open_cleanup);
}


/****************************************************************************

                                Draw - Functions

 ****************************************************************************/

static UWORD cursor_x,cursor_y;

static int ConvertX(struct DrawArea *da,int x)
{
  assert(da->xdiv != 0);
  return (x*da->xmul)/da->xdiv + da->xplus;
}

static int ConvertY(struct DrawArea *da,int y)
{
  assert(da->ydiv != 0);
  return (y*da->ymul)/da->ydiv + da->yplus;
}

void DrMove(struct DrawArea *da,int x,int y)
{
  cursor_x = x;
  cursor_y = y;

  Move( da->rp , ConvertX(da,x) , ConvertY(da,y) );
}

void DrDraw(struct DrawArea *da,int x,int y)
{
  cursor_x = x;
  cursor_y = y;

  Draw( da->rp , ConvertX(da,x) , ConvertY(da,y) );
}

void DrLine(struct DrawArea *da,int x1,int y1,int x2,int y2)
{
  DrMove(da,x1,y1);
  DrDraw(da,x2,y2);
}

void DrSetPen(struct DrawArea *da,UWORD penname)
{
  SetAPen( da->rp , da->drawinfo->dri_Pens[penname] );
}

void DrFill(struct DrawArea *da,int x,int y)
{
  Flood( da->rp , 1 , ConvertX(da,x) , ConvertY(da,y) );
}

void DrMirrorY(struct DrawArea *da,int y)
{
  int middle_y;
  int final_y;
  int src_y;
  int dest_y;
  int i;
  int d;
  struct BitMap *bm = da->rp->BitMap;

  middle_y = ConvertY(da,y);
  src_y    = middle_y-1;
  dest_y   = middle_y+1;
  final_y  = bm->Rows - 1;

  for ( ; src_y >= 0 && dest_y <= final_y ; src_y-- , dest_y++ )
  {
    for (i=0;i<bm->BytesPerRow;i++)
      for (d=0;d<bm->Depth;d++)
      {
        * ( bm->Planes[d] + dest_y * bm->BytesPerRow + i ) =
        * ( bm->Planes[d] + src_y  * bm->BytesPerRow + i );
      }
  }
}

