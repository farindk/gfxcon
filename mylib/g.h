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

     /********************************************************************
      *                                                                  *
      * These routines need the Intuition, Graphics, GadTools, Utility,  *
      * Layers (later), DiskFont libraries opened !                      *
      *                                                                  *
      ********************************************************************/

#define T_U TAG_USER

#define ALT_TextColor       (T_U|0x0001) /* Alterate the text color. Changes
                                          * will only affect current element.
                                          */
#define ALT_TextAttr        (T_U|0x0002) /* alterate the text-font. */
#define ALT_KeepSettings    (T_U|0x0003) /* Don't reset the values. */

#define OBJ_AbsolutePos     (T_U|0x0004) /* Specify absolute coordinates for
                                          * an element. This should only be
                                          * used internally.
                                          */
#define OBJ_AbsoluteWidth   (T_U|0x0005) /* Specify absolute width for
                                          * an element. This should only be
                                          * used internally.
                                          */
#define OBJ_AbsoluteHeight  (T_U|0x0006) /* Specify absolute height for
                                          * an element. This should only be
                                          * used internally.
                                          */


/* tags for window construction */

#define WAWA_WinCloseFunc   (T_U|0x0010) /* Function to verify close.
                                          * Return EXIT_EXITWINDOW if you
                                          * really want to * close window.
                                          */
#define WAWA_AutoIDCMP      (T_U|0x0011) /* Set all IDCMP flags needed
                                          * automatically.
                                          */
#define WAWA_ActiveGad      (T_U|0x0012) /* ti_Data is the Gadget-ID
                                          */
#define WAWA_Centered       (T_U|0x0013) /* ti_Data points to Handle in which
                                          * this window will be centered
                                          */


/* tags for gadget construction */

#define GAGA_Kind           (T_U|0x0020) /* kind of gadget
                                          * (default: BUTTON_KIND)
                                          */
#define GAGA_ID             (T_U|0x0021) /* ID of gadget (only for your use)
                                          */
#define GAGA_Text           (T_U|0x0022) /* Text to placed into an BUTTON_KIND
                                          * gadget or to be placed elsewhere.
                                          * I recommend to use this only for
                                          * button-gadgets and use the
                                          * CrText() function instead.
                                          */
#define GAGA_TextPos        (T_U|0x0023) /* the PLACETEXT_xxx attributes.
                                          * (will be or'ed to ng.flags)
                                          */
#define GAGA_SavePtr        (T_U|0x0024) /* Save pointer to Gadget-structure
                                          * to this location.
                                          */
#define GAGA_CallFunc       (T_U|0x0025) /* Call this function if a
                                          * IDCMP_GADGETUP message arrives.
                                          */
#define GAGA_FuncPara       (T_U|0x0026) /* Call the function above with this
                                          * parameter.  (default: NULL)
                                          */
#define GAGA_FuncPara2      (T_U|0x003F) /* Call the function above with this
                                          * as second parameter. (default: NULL)
                                          */


#define GAGA_ST_Ptr         (T_U|0x0027) /* Pointer where to save the string.  */
#define GAGA_IN_Ptr         (T_U|0x0028) /*    "      "    "   "   "  integer. */
#define GAGA_DB_Ptr         (T_U|0x0029) /*    "      "    "   "   "  double.  */
                                         /* These tags may override the
                                          * GAGA_Kind tag, but you must set a
                                          * GAGA_Kind value (I suggest to set it
                                          * to STRING_KIND). ti_Data may be NULL
                                          * although this makes no sense.
                                          */
#define GAGA_UpperBound     (T_U|0x002A) /* upper bound of integer or double value */
#define GAGA_LowerBound     (T_U|0x002B) /* lower bound  "    "     "   "      "   */
                                         /* IMPORTANT: if you use this with an
                                          * "double"-gadget, must must not pass
                                          * the value as parameter but a pointer
                                          * to the value !!! (a double is 8 bytes
                                          * long but the tag-value only 4)
                                          */
#define GAGA_CharsWidth     (T_U|0x002C) /* width of gadget in chars
                                          */
#define GAGA_NumberBytes    (T_U|0x002D) /* Length of integer that will
                                          * be written. With the use of
                                          * this, you can build eg. an
                                          * integer-gadget to enter
                                          * values into an byte-integer.
                                          */
#define GAGA_NextActive     (T_U|0x002E) /* ID of gadget to become active
                                          * after this one.
                                          */
#define GAGA_DontFillIn     (T_U|0x002F) /* Don't copy the values pointed
                                          * to by GAGA_##_Ptr in the gadget.
                                          */
#define GAGA_DoubleFormat   (T_U|0x0030) /* Pointer to a C-format-string.
                                          * (Won't be copied !)
                                          */

#define GAGA_CB_Ptr         (T_U|0x0031) /* Pointer to a flag-field
                                          * (length in GAGA_NumberBytes)
                                          */
#define GAGA_CB_Switch      (T_U|0x0032) /* flag(s) to be modified
                                          */

#define GAGA_CY_Ptr         (T_U|0x0033) /* pointer to value
                                          * (see GAGA_NumberBytes)
                                          */
#define GAGA_CY_List        (T_U|0x0034) /* pointer to a List. Node-names
                                          * are used as strings for the gadget.
                                          */
#define GAGA_CY_ActiveNode  (T_U|0x0035) /* node to become active
                                          */

#define GAGA_LV_ActiveNode  (T_U|0x0036) /* node to become active
                                          */
#define GAGA_LV_StringGad   (T_U|0x0037) /* handle of string gadget
                                          */
#define GAGA_LV_Ptr         (T_U|0x0038) /* pointer to data (see: GAGA_NumberBytes)
                                          */
#define GAGA_LV_nLines      (T_U|0x0039) /* lines visible in the listview
                                          */
#define GAGA_LV_CharsWidth  (T_U|0x003A) /* width of listview in chars
                                          */

#define GAGA_PROGR_Percent  (T_U|0x003B) /* Show progress in percent.
                                          */

#define GAGA_IMG_BevelBox   (T_U|0x003C) /* Automatically draw a BevelBox around
                                          * the image (in the image). The ti_Data
                                          * field specifies, if this Box should
                                          * be recessed.
                                          */

#define GAGA_BU_ImageNorm   (T_U|0x003D) /* Image to be displayed if unselected.
                                          */
#define GAGA_BU_ImageSelect (T_U|0x003E) /* Image to be displayed if selected.
                                          */

#define GAGA_TX_CharsWidth  (T_U|0x0040) /* Width of gadget in chars.
                                          */

#define GAGA_POP_Ptr        (T_U|0x0041) /* Pointer to Popup-selection value.
                                          * (only ULONG. This may change in
                                          * the future).
                                          */
#define GAGA_POP_Disable31  (T_U|0x0042) /* Bit 31 in the data-field indicates
                                          * that this item is be disabled.
                                          */
#define GAGA_TXDI_MinLines  (T_U|0x0043) /* Minimum number of lines visible in
                                          * the text-box.
                                          */
#define GAGA_TXDI_MinChars  (T_U|0x0044) /* Minimum number of chars visible in
                                          * a line.
                                          */


#define BIT31 (1<<31)

/* colors that may be set with ALT_TextColor */

#define COLOR_NORMAL    0x10000
#define COLOR_HIGHLIGHT 0x10001
#define COLOR_CUSTOM    0x00000 /* OR' this with the pen number you want to use */


/* byte-width of field modified by e.g. GAGA_IN_Ptr */

#define FIELD_BYTE 1
#define FIELD_WORD 2
#define FIELD_LONG 4


/* function return values (flags) */

#define EXIT_EXITWINDOW (1<<0) /* return this to leave the window */


/* special handles */

#define HANDLE_END   0                     /* use this to end H/V-Boxes ... */
#define HANDLE_ERR   ((struct Handle *)-1) /* an error occured              */




/*---------------- the prototypes ----------------*/


/* initialization */

void MarkLibs4GraphSys(BOOL);
BOOL InitConstructData(struct Screen *parent_scr,struct CleanupList *parent_list);
void CleanupConstructData(void);
void SetNormFont(struct TextAttr *);
BOOL BeginNewHandleTree(void);


/* low level graphic elements */

struct Handle *CrText(char *text,Tag, ...);
struct Handle *CrBevelBox(BOOL recessed,struct Handle *h);
struct Handle *CrLowBox(struct Handle *h);
struct Handle *CrHighBox(struct Handle *h);
struct Handle *CrTextBox(struct Handle *texthandle,struct Handle *h);
struct Handle *CrNormTextBox(char *text,struct Handle *h);
struct Handle *CrSpaceBox(struct Handle *h);
struct Handle *CrImage(UWORD height,UWORD xsize,UWORD ysize,
                       void (*drawfunc)(struct DrawArea *),Tag tag, ...);


/* priority */

struct Handle *NewPri(struct Handle *h,int pri);
struct Handle *NewPriXY(struct Handle *h,int xpri,int ypri);


/* placement */

struct Handle *CrRaster(int xsize,struct Handle *h, ...);
struct Handle *CrRasterA(int xsize,struct Handle **h);
struct Handle *ModifyRasterEqual(BOOL eqwidth,BOOL eqheight,struct Handle *h);
struct Handle *CrHBox(struct Handle *h, ...);
struct Handle *CrVBox(struct Handle *h, ...);
struct Handle *CrFiller(void);
struct Handle *CrSpaceRaster(int xsize,struct Handle *h, ...);
struct Handle *CrSpaceHBox(struct Handle *h, ...);
struct Handle *CrSpaceVBox(struct Handle *h, ...);


/* high level elements */

struct Handle *CrGadget(Tag ti, ...);
struct Handle *CrCBGadget(char *text,Tag ti, ...);
struct Handle *CrArrowGadget(Tag ti, ...);
struct Handle *CrFilerequestGadget(Tag ti, ...);
struct Handle *CrWindow(struct Handle *h,Tag ti, ...);
struct Handle *CrSmallWindow(struct Handle *h,Tag ti, ...);

struct Handle *CrProgressDisplay(Tag ti, ...);
struct Handle *CrTextPopup(Tag ti, ...); /* these tags are handled differently ! */
struct Handle *CrTextPopupA(struct TagItem *taglist,struct TagItem *stringlist);
struct Handle *CrTextDisplayA(struct TagItem *taglist);
struct Handle *CrTextDisplay(Tag ti, ...);

/* modifying gadgets */

void           DisplayProgress(struct Handle *h,
                               struct Handle *prog_h,
                               int            numerator,
                               int            denominator);
void           ChangeText(struct Handle *h,ULONG id,char *text);


/* computing & opening/closing */

BOOL ComputeGadgets(struct Handle *h);
BOOL OpenHandle(struct Handle *h);
void CloseHandle(struct Handle *h);


/* gadget message handling */

void  HandleHandle(struct Handle *h,ULONG mysigmask,ULONG (*func)(ULONG sigs) );
ULONG HandleIMsg(struct Handle *h,struct IntuiMessage *imsg);
void  RescueGadInput(struct Handle *h);
struct Gadget *GetGadget(struct Handle *h,ULONG id);
struct Window *GetWindow(struct Handle *h);


/* draw functions */

void DrMove(struct DrawArea *,int x ,int y);
void DrDraw(struct DrawArea *,int x ,int y);
void DrLine(struct DrawArea *,int x1,int y1,int x2,int y2);
void DrSetPen(struct DrawArea *,UWORD penname);
void DrFill(struct DrawArea *,int x ,int y);
void DrMirrorY(struct DrawArea *,int y);


/* support functions */

int    FindNodeNr(const struct Node *,const struct List *);
void   List2NameArray(const struct List *,char **);
int    MaxCharLength(void);
int    ListLen(const struct List *);
int    MaxNumLength(void);
void   DisableGad(struct Handle *,ULONG id,BOOL disable);
void   SetPopupDisable(struct Handle *,ULONG itemID,BOOL disable);
void   SetPopup(struct Handle *master,struct Handle *,ULONG itemID);
void   WriteTextLine(struct Handle *,struct Handle *td_handle,char *,LONG color);


/***************************** future functions ***********************************


struct RastPort *GetHandleRastPort(struct Handle *h);
struct Window *GetHandleWindow(struct Handle *h);
struct Gadget *GetGadget(struct Handle *h,ULONG id);

BOOL  AttachNewCycleList(struct Handle *h,ULONG ID,struct List *list);

*********************************************************************************/


/*-----------------------------------------------------------------------------
    If you don't need a special feature of this package, I stringly recommend
    to let the compiler know by #defining the corresponding flag. This will
    save memory usage !!!

    Simply #define the things below you don't need in your program.
  -----------------------------------------------------------------------------

 DONT_NEED_GAD_BUTTON
 DONT_NEED_GAD_CHECKBOX
 DONT_NEED_GAD_CYCLE
 DONT_NEED_GAD_STRING
 DONT_NEED_GAD_INTEGER
 DONT_NEED_GAD_DOUBLE
 DONT_NEED_GAD_LISTVIEW
 DONT_NEED_TEXTBOX
 DONT_NEED_TEXT
 DONT_NEED_BEVELBOX
 DONT_NEED_LOWBOX
 DONT_NEED_HIGHBOX

 reserved:

 DONT_NEED_
 DONT_NEED_
 DONT_NEED_
 DONT_NEED_
 DONT_NEED_
 DONT_NEED_
 DONT_NEED_
 DONT_NEED_
 DONT_NEED_
 DONT_NEED_
 DONT_NEED_

  -----------------------------------------------------------------------------*/
