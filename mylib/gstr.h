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

#include "glib:g.h"
#include "exclude.h"

/**------------------------------------------------------------------------**
 ** This structure contains the values which are important to contruct the **
 ** gadgets and graphics.                                                  **
 **------------------------------------------------------------------------**/

struct ConstructData
{
  int xspacing,yspacing;      /* the standard spacing between gadgets */

  APTR VisualInfo;            /* VI of Workbench (or the screen you specified
                               * at InitConstructData() ).
                               */

  struct TextAttr *NormTextAttr; /* Font which will be used if non is */
  struct TextFont *NormTextFont; /* explicitely specified.            */
  BOOL   font_loaded;            /* Is the font loaded, ie. must it be deallocated */

  UWORD scrheight ,scrwidth;     /* dimensions of the screen          */
  UBYTE scrdepth;                /* number of bitplanes in the screen */
  UWORD bordertop ,borderbottom, /* bordersizes of windows. ##_notxt is for */
                bordertop_notxt; /* windows which won't have a title-bar.   */
  UWORD borderleft,borderright;

  UWORD textpen,          /* the pen-numbers which should be used to paint */
        shinepen,
        shadowpen,
        fillpen,
        highlighttextpen;

  UWORD res_x,res_y;   /* resolution (aspect ratio) from DrawInfo */

  struct Screen *scr;  /* pointer to own screen or NULL for Workbench */
};

extern struct ConstructData ConDa; /* this the only (global) variable of this type */





/**------------------------------------------------------------------------**
 ** Values needed during the construction of the handle-tree.              **
 **
 ** All handles are stored in a linked list which is independent from the
 ** handle-tree (what allows us to add such things as menus and other
 ** things that belong to the entire window).
 **
 ** Every gadget has a GadgetData-structure which contains additional
 ** information about the use of this gadget. These GadgetDatas are stored
 ** in a linked list too.
 ** 'cleanup' is the cleanup-list for the entire handle-tree inclusive
 ** GadgetData and so on.
 **
 ** In 'IDCMP' we collect all the IDCMP-flags that should be set (see
 ** WAWA_AutoIDCMP).
 **
 ** With 'textmeasure_dummyrp' we can get the length of a string before we
 ** open the window.
 **
 ** CurrText#### is the current font which may be modified with the
 ** ALT_TextAttr tag. This value is local to each element. That means it
 ** will be reset at each new handle (if you don't specify the
 ** ALT_KeepSettings tag.
 **
 ** TextPen is the local text pen ( oh, really ? )
 **------------------------------------------------------------------------**/

struct CreateTreeData
{
  /* fields that are only valid during tree construction ! (Cr###) */

  struct Handle      *lasthandle;
  struct GadgetData  *lastgaddata;
  struct CleanupList *cleanup;

  ULONG               IDCMP;

  struct RastPort     textmeasure_dummyrp;

  /* Values that may be altered with tags for each handle are stored here. */
  /* They are local to each handle. Therefor they are reset to the globals */
  /* at each call of NewHandle(). */

  struct TextAttr *CurrTextAttr;
  struct TextFont *CurrTextFont;
  UWORD  fontsloaded;

  UWORD  TextPen;
};

/* flags for 'fontsloaded' */

#define LOADED_CURR_FONT (1<<0)

extern struct CreateTreeData CrTrDat;







/**------------------------------------------------------------------------**
 **  The additional GadgetData structure which allows to automatically
 **  do most of the work with gadgets automatically.
 **------------------------------------------------------------------------**/



struct GadgetData
{
  struct GadgetData *next; /* to chain the GadgetDatas in the list */

  long               kind; /* kind of the gadget (BUTTONKIND ...) */

  struct Gadget     *gad;  /* pointer to the created gadget */

  ULONG        (* func)(ULONG); /* function to call when receiving GADGETUP */
  ULONG         parameter;      /* function-parameter */
  ULONG         parameter2;     /* function-parameter */

  UWORD         flags;          /* see below */

  ULONG         nextactive_id;  /* ID of next gadget to become active */

  union {
    struct { ULONG (*popupfunc)(struct Handle *,ULONG); /* internal !!! */
             struct Handle *popuphandle;                /* -> only for popup-gads !!! */
             ULONG          popupnumber;                /* internal !!! */
                                } button;

    struct { char   *dest;                /* destination for string       */
             double *dbl_dest;            /* destination for double-value */
             double  lowerbound,          /* bounds for double-gad        */
                     upperbound;
             UWORD   flags;               /* see below */
             char   *formatstr;
                                } string;

    struct { long  *dest;
             char   bytelength;           /* length of integer to be written */
             long   lowerbound,
                    upperbound;
             UWORD  flags;
                                } integer;

    struct { long  *dest;
             long   switchflag;
             char   bytelength;
             char   selected;   /* state of the gadget (TRUE = selected) */
                                } check;

    struct { long  *dest;
             char  **labels;
             char   bytelength;
                                } cycle;

    struct { long  *dest;
             char   bytelength;
             struct Handle *strgadget; /* connection to the string-gadget that
                                        * MAY be connected to the listview
                                        */
                                } listview;
  } data;


  /* used only for custom gadgets */

  UWORD  Flags;
  UWORD  Activation;
  UWORD  GadgetType;
  APTR   GadgetRender;
  APTR   SelectRender;
  APTR   SpecialInfo;

  struct Handle *ImageNormHandle;
  struct Handle *ImageSelectHandle;

  struct Handle *RealHandle; /* pointer to parent handle (popup ...) */
};

/* GadgetData.flags */

#define NEXT_ACTIVE (1<<0) /* activate next-active-mode (the gadget with
                            * the ID 'nextactive_id' will become active if
                            * this gadget recieves a IDCMP_GADGETUP-message.
                            */
#define IS_DISABLED (1<<1) /* this allows to prevent multiple (unnecessary)
                            * calls to the system to disable (or enable) it
                            */
#define CUSTOM_GAD  (1<<2) /* This gadget is hand-made (ie. own image-datas...).
                            */
#define TWO_PARAMETERS (1<<3) /* function is called with two parameters.
                               */

/* ...string.flags */

#define STRING_DBL             (1<<0) /* treat this string-gad like a double-gad */
#define STRING_DBL_UPPER_BOUND (1<<1) /* perform bound-checking (upper bound) */
#define STRING_DBL_LOWER_BOUND (1<<2) /* perform bound-checking (lower bound) */

/* ...integer.flags */

#define INTEGER_UPPER_BOUND    (1<<1) /* bound-checking (upper bound) */
#define INTEGER_LOWER_BOUND    (1<<2) /* bound-checking (lower bound) */




struct DrawArea
{
  struct RastPort *rp;
  int              xmul ,xdiv;
  int              ymul ,ydiv;
  int              xplus,yplus;
  struct DrawInfo *drawinfo;
};


/* I think these should vanish in the (near) future */

#define NSUBHANDLESX 15
#define NSUBHANDLESY 15
#define NSUBHANDLES  (NSUBHANDLESX * NSUBHANDLESY)



/**------------------------------------------------------------------------**
 ** Each element in a window and the window itself is specified with a
 ** handle. And thus the handle is the most important structure in this
 ** module. Complex handles may consist of several more elemental handles.
 **
 ** Sometimes it is necessary to specify the order in which some handles
 ** have to be opened or drawn (for example: the string gadget for a
 ** listview must be opened before the listview). Therefor it is possible
 ** to declare a handle to be a subordinate of an other handle by setting
 ** the SUBORDINATE flag. Handle with this flag set will then be opened
 ** or drawn before the other.
 **------------------------------------------------------------------------**/

struct Handle
{
  struct Handle *next;        /* list of all handles */

  /* these fields will be set when handle is created */

  UWORD minw,minh;            /* minimum size of the element */
  UWORD shrinkw,shrinkh;      /* total size of borders       */
  UWORD leftborder,topborder; /* size of left and top border */

  /* these fields will be set when positions are calculated */
  /* (exception: w,h will be set in ha_Window to the screen size) */

  UWORD x,y;       /* position     */
  UWORD w,h;       /* size         */

  UWORD xpri,ypri;          /* priorities to expand in these directions */
  struct Handle *pri_chain; /* copy the priorities to this handle too   */

  UWORD flags;          /* see below */

  struct Handle *right, /* pointer to the handle to the right  (in a raster)   */
                *down,  /* pointer to the handle to the bottom (in a raster)   */
                *in;    /* pointer to the handle inside this one (TextBox ...) */

  enum { ha_NewGadget,
         ha_Text,
         ha_TextBox,
         ha_Window,
         ha_Filler,
         ha_BevelBox,
         ha_SpaceBox,
         ha_ProgressBox,
         ha_Image,
         ha_Popup,
         ha_TextDisplay,

         ha_Raster
       } Type;

  union {
          struct { struct TagItem     *taglist;     /* tags to pass to intuition when
                                                     * opening the screen.
                                                     */
                   ULONG             (*winclosefunc)(void); /* CloseWindow-verify function */
                   ULONG               activegad;   /* gadget (ID) to become active when
                                                       window gets active */
                   UWORD              flags;  /* see below */
                   ULONG              IDCMP;  /* IDCMP-flags to set if
                                               * AUTO_SET_IDCMP is set.
                                               */

                   struct Window      *win;
                   struct RastPort    *rp;
                   struct GadgetData  *gaddata;  /* linked list of all GadgetDatas */
                   struct Gadget      *glist;    /* Gadget list to be passed to Window */

                   /* The Global Data For The Whole Tree Follows */

                   struct Handle      *handlelist;   /* main list with all handles */
                   struct CleanupList *cleanup;      /* cleanuplist for the entire tree */
                   struct CleanupList *open_cleanup; /* cleanuplist to close the
                                                      * graphics (and not the handle-tre)
                                                      */
                   APTR                VisualInfo;
                   struct Screen      *scr;
                   struct DrawInfo    *drawinfo;
                   struct Handle      *center_handle;
                                                 } win;

          struct { int                 kind;
                   struct NewGadget    ng;
                   struct TagItem     *taglist;
                   struct Gadget     **saveptr; /* save a pointer to the gadget there */
                   struct GadgetData  *gaddata;
                                                 } newgad;

          struct { BOOL                recessed; } bevelbox;

          struct { char               *text;
                   struct TextFont    *font;
                   UWORD               color;
                                                 } text;

          struct { struct Handle      *texthandle;
                                                 } textbox;

          struct { int                 num_x, /* width and height of raster */
                                       num_y;
                   UWORD               max_sub_width;
                   UWORD               max_sub_height;
                   UWORD               spacex,spacey;
                   UWORD               flags;    } raster;

          struct { UWORD               flags;
                   UWORD               color;
                   UWORD               textcolor;
                   struct TextFont    *font;
                   UWORD               lastpos;
                   UWORD               over_start;
                   UWORD               over_end;
                   char                lastpercent;
                                                 } progress;
          struct { struct Image        image;
                   int                 xsize,ysize;
                   void              (*drawfunc)(struct DrawArea *);
                   UWORD               flags;
                                                 } image;
          struct { struct TagItem     *stringlist;
                   UWORD               flags;
                   struct Handle      *buttongad;
                   struct Handle      *textgad;
                   ULONG               selection;
                   ULONG              *destination;
                                                 } popup;
          struct { struct TextFont    *font;
                   UWORD               color;
                                                 } textdisplay;
        } data;
};


/* Handle.flags */

#define ABSOLUTE_POS    (1<<0)
#define ABSOLUTE_WIDTH  (1<<1) /* this means the object will not be larger    */
#define ABSOLUTE_HEIGHT (1<<2) /* than min., but centered into the bound.box  */
#define SUBORDINATE     (1<<3)
#define KEEP_ASPECT     (1<<4) /* don't change the aspect ratio of an element */
#define DONT_DRAW       (1<<5) /* don't draw this (image, etc.) on the screen */

/* .win.flags */

#define AUTO_SET_IDCMP  (1<<0)
#define ACTIVATE_GADGET (1<<1)

/* .progress.flags */

#define SHOW_PERCENT    (1<<0)

/* .raster.flags */

// #define SPACE_RASTER    (1<<0)   obsolete
#define EQUAL_WIDTH     (1<<1)
#define EQUAL_HEIGHT    (1<<2)

/* .image.flags */

#define AUTO_BEVELBOX     (1<<0)
#define BEVELBOX_RECESSED (1<<1)

/* .popup.flags */

#define POPUP_BIT31DISABLE (1<<0)


/* module-functions */


struct Handle *NewHandle(void);
void           LinkHandle (struct Handle *);
void           LinkGadData(struct GadgetData *);
void           TreatGlobalTags(struct Handle *,struct TagItem *taglist);
UWORD          FontHeight(void);
WORD           TextWidth(char *);
WORD           MaxCharWidth(void);
WORD           MaxNumWidth(void);
void           FreeHandleTree(struct Handle *h);
LONG           SetIntegerToBounds(struct GadgetData *,LONG value,BOOL *changed_ptr);
double         SetDoubleToBounds(struct GadgetData *,double value,BOOL *changed_ptr);
void           FreeTagListFunc(struct TagItem *list);
struct TagItem *AllocCopyTaglist(struct TagItem *list,int more);
struct TagItem *SecondTaglist(struct TagItem *source);
LONG           GetIntegerValue(void *ptr,int len);
void           ShowPopupSelection(struct Handle *poph,struct Window *);

/*----------------------------------------------------------------------------*/

#ifndef   DONT_NEED_TEXTBOX    /* ha_Text is used in ha_TextBox */
#  undef  DONT_NEED_TEXT
#endif

#ifndef   DONT_NEED_LOWBOX
#  undef  DONT_NEED_BEVELBOX
#endif

#ifndef   DONT_NEED_HIGHBOX
#  undef  DONT_NEED_BEVELBOX
#endif

#ifndef   DONT_NEED_GAD_DOUBLE
#  undef  DONT_NEED_GAD_STRING
#endif

#ifndef   DONT_NEED_GAD_CHECKBOX
#  undef  DONT_NEED_TEXT
#endif

#ifndef   DONT_NEED_GAD_LISTVIEW
#  undef  DONT_NEED_GAD_STRING
#endif

