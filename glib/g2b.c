
#include "glib:incs.h"
#include "glib:gstr.h"
#include "glib:g.h"
#include "glib:errs.h"
#include "glib:txtsys.h"
#include "glib:clean.h"


/**------------------------------------------------------------------------**
 **  Allocate a new taglist. Copy the source into it (don't copy the
 **  TAG_IGNOREs... ) and leave n_more entries free. These can be filled
 **  with InsertTag().
 **------------------------------------------------------------------------**/

struct TagItem *AllocCopyTaglist(struct TagItem *sourcelist,int n_more)
{
  int count;
  int nr;
  struct TagItem *tstate,*tag;
  struct TagItem *newlist;


  /* count tag items */

  count=0;
  tstate = sourcelist;
  while (tag=NextTagItem(&tstate))
  {
    count++;
  }

  count++;   /* place for the TAG_END */

  count += n_more; /* add free space after that list */


  /* allocate new taglist */

  newlist=AllocateTagItems(count);
  if (!newlist)
  {
    SetError(NO_MEM);
    return NULL;
  }
  cuinsert_CallFunction(&FreeTagListFunc,newlist);


  /* copy sourcelist to taglist */

  nr=0;
  tstate = sourcelist;
  while (tag=NextTagItem(&tstate))
  {
    newlist[nr].ti_Tag	= tag->ti_Tag;
    newlist[nr].ti_Data = tag->ti_Data;
    nr++;
  }

  /* fill with TAG_IGNORE to end of list */

  for ( ; nr<count-1 ; nr++)
    newlist[nr].ti_Tag = TAG_IGNORE;

  /* end tag list with TAG_END */

  newlist[count-1].ti_Tag = TAG_END;

  return newlist;
}


/**------------------------------------------------------------------------**
 ** Insert a tag into a taglist copied by AllocCopyTagList().
 **------------------------------------------------------------------------**/

static void InsertTag(struct TagItem *list,Tag TagCode,Tag TagValue)
{
  struct TagItem *tag;


  /* search for the first TAG_IGNORE */

  tag=list;
  while (tag->ti_Tag != TAG_END)
  {
    if (tag->ti_Tag == TAG_IGNORE)
    {
      tag->ti_Tag  = TagCode;
      tag->ti_Data = TagValue;
      return;
    }

    tag++;
  }

  assert(1==0);  /* there really should be one free
		  * TAG_IGNORE in the list
		  */
}


/**------------------------------------------------------------------------**
 **  Count how many characters are necessary to display an integer number
 **------------------------------------------------------------------------**/

static int CharsForNumber(long n)
{
  char buffer[20];
  sprintf(buffer,"%d",n);
  return (int)strlen(buffer);
}


/**------------------------------------------------------------------------**
 **  Count how many characters are necessary to display an double number
 **------------------------------------------------------------------------**/

static int CharsForDblNumber(double d,char *format)
{
  char buffer[20];
  sprintf(buffer,format,d);
  return (int)strlen(buffer);
}


/**------------------------------------------------------------------------**
 **  Get an integer from memory. Length is dependent on 'len'.
 **------------------------------------------------------------------------**/

LONG GetIntegerValue(void *ptr,int len)
{
  LONG value; /* only for type-casting */

  switch (len)
  {
    case FIELD_BYTE: return value = *((BYTE *)ptr);
    case FIELD_WORD: return value = *((WORD *)ptr);
    case FIELD_LONG: return value = *((LONG *)ptr);
    default: assert(1==0);
  }
}


/**------------------------------------------------------------------------**
 **  Change integer value to fit into the bounds if specified.
 **  You don't need to a 'changed_ptr'.
 **  '*changed_ptr' will be set to true if value has been modified. This
 **  allows the caller-function to call SetGadgetAttrs() (or similar) only
 **  if it's really necessary.
 **------------------------------------------------------------------------**/

LONG SetIntegerToBounds(struct GadgetData *gdat,LONG value,BOOL *changed_ptr)
{
  if (gdat->data.integer.flags & INTEGER_UPPER_BOUND)
    if (value > gdat->data.integer.upperbound)
    { value=gdat->data.integer.upperbound;
      if (changed_ptr) *changed_ptr = TRUE;
    }

  if (gdat->data.integer.flags & INTEGER_LOWER_BOUND)
    if (value < gdat->data.integer.lowerbound)
    { value=gdat->data.integer.lowerbound;
      if (changed_ptr) *changed_ptr = TRUE;
    }

  return value;
}


/**------------------------------------------------------------------------**
 **  Change double value to fit into the bounds if specified.
 **  see SetIntegerToBounds()
 **------------------------------------------------------------------------**/

double SetDoubleToBounds(struct GadgetData *gdat,double value,BOOL *changed_ptr)
{
  if (gdat->data.string.flags & STRING_DBL_UPPER_BOUND)
    if (value > gdat->data.string.upperbound)
    { value=gdat->data.string.upperbound;
      if (changed_ptr) *changed_ptr = TRUE;
    }

  if (gdat->data.string.flags & STRING_DBL_LOWER_BOUND)
    if (value < gdat->data.string.lowerbound)
    { value=gdat->data.string.lowerbound;
      if (changed_ptr) *changed_ptr = TRUE;
    }

  return value;
}


/**------------------------------------------------------------------------**
 ** count how many entries there are in the list
 **------------------------------------------------------------------------**/

int ListLen(const struct List *l)
{
  int len=0;
  struct Node *n;

  n=l->lh_Head;
  while (n->ln_Succ) { len++; n=n->ln_Succ; }

  return len;
}


/**------------------------------------------------------------------------**
 **  Take a list and place for each name a pointer to the array
 **  (no checking of array-bounds !!!). After these pointers, a 0-long
 **  will be written. This function is meant to generate the array for
 **  CYCLE-Gadgets.
 **------------------------------------------------------------------------**/

void List2NameArray(const struct List *l,char **a)
{
  struct Node *n;

  n=l->lh_Head;

  while (n->ln_Succ)
  {
    *a++=n->ln_Name;
    n=n->ln_Succ;
  }
  *a=NULL;
}


/**------------------------------------------------------------------------**
 ** Given a List and a Node, this will find the ordinal position of the
 ** node in the List. Use it to set the GTLV_Selected Attribute or things
 ** like that.
 **------------------------------------------------------------------------**/

int FindNodeNr(const struct Node *ptr,const struct List *list)
{
  struct Node *p;
  int nr;

  for (p=list->lh_Head,nr=0 ; p != ptr ; nr++)
    p=p->ln_Succ;

  return nr;
}


/**------------------------------------------------------------------------**
 **  Get maximum length of the strings in the array. The array must have
 **  the format list the array passed to a CYCLE_KIND gadget; ie. it must
 **  end width a NULL-pointer.
 **------------------------------------------------------------------------**/

int GetMaxLabelLength(char **labels)
{
  int maxwidth=0;

  while (*labels)
  {
    int txtlen=TextWidth(*labels);
    maxwidth=max( txtlen , maxwidth );
    labels++;
  }

  return maxwidth;
}


/**------------------------------------------------------------------------**
 **  CrGadget
 **------------------------------------------------------------------------**/

struct Handle *CrGadgetA(struct TagItem *mytaglist,struct TagItem *systaglist)
{
  struct Handle     *h;
  struct GadgetData *gdat;
  int		     kind;
  struct TagItem    *tag;
  struct TagItem    *taglistcopy;

  {{{{{
  int cnt=0;
  struct TagItem *tstate = mytaglist,*ti;
  while (ti=NextTagItem(&tstate))
  {
    //printf("%p,%p\n",ti->ti_Tag,ti->ti_Data);
    cnt++;
  }
  //printf("TagCnt: %d\n",cnt);
  }}}}}


  /* alloc new handle and GadgetData */

  if (!(h=NewHandle()))
    return HANDLE_ERR;

  if (!(gdat=cu_calloc( 1,sizeof(struct GadgetData) ) ))
    return HANDLE_ERR;

  h->data.newgad.gaddata = gdat;


  /* check global tags */

  TreatGlobalTags(h,mytaglist);


  /* get kind of gadget */

  kind = GetTagData ( GAGA_Kind , BUTTON_KIND , mytaglist );
  /* fill in non-gadget-specific data */

  h->Type = ha_NewGadget;

  h->data.newgad.ng.ng_GadgetText = (char *)GetTagData ( GAGA_Text    , NULL , mytaglist );
  h->data.newgad.ng.ng_TextAttr   = CrTrDat.CurrTextAttr;
  h->data.newgad.ng.ng_GadgetID   = GetTagData ( GAGA_ID      , 0    , mytaglist );
  h->data.newgad.ng.ng_Flags	  = 0;
  h->data.newgad.ng.ng_Flags	 |= GetTagData ( GAGA_TextPos , NULL , mytaglist );


  /* function to call on IDCMP_GADGETDOWN */

  gdat->func	   = (ULONG (*)(ULONG))GetTagData ( GAGA_CallFunc  , NULL , mytaglist );
  //printf("Insert Callbackfunc: %p\n",gdat->func);
  gdat->parameter  =	     GetTagData ( GAGA_FuncPara  , NULL , mytaglist );
  gdat->parameter2 =	     GetTagData ( GAGA_FuncPara2 , NULL , mytaglist );

  if (FindTagItem(GAGA_FuncPara2,mytaglist))
  {
    gdat->flags |= TWO_PARAMETERS;
  }

  /* gadget to become active after IDCMP_GADGETUP */

  if (tag=FindTagItem(GAGA_NextActive,mytaglist))
  {
    gdat->flags |= NEXT_ACTIVE;
    gdat->nextactive_id = tag->ti_Data;
  }

  /* address where to store gadget pointer */

  h->data.newgad.saveptr = (void *)GetTagData ( GAGA_SavePtr , NULL , mytaglist );


  /* gadget - specific datas */

  switch (kind)
  {
#ifndef DONT_NEED_GAD_BUTTON

    case BUTTON_KIND:
      taglistcopy = AllocCopyTaglist(systaglist,0);
      if (!taglistcopy) return HANDLE_ERR;

      CrTrDat.IDCMP |= BUTTONIDCMP;

      if (!FindTagItem(GAGA_TextPos,mytaglist))
	h->data.newgad.ng.ng_Flags |= PLACETEXT_IN;

      if (tag=FindTagItem(GAGA_BU_ImageNorm,mytaglist))
      {
	kind	     = GENERIC_KIND;

	gdat->flags |= CUSTOM_GAD;
//	  h->flags    |= KEEP_ASPECT;

	gdat->Flags	   = GFLG_GADGIMAGE;
	gdat->Activation   = GACT_IMMEDIATE | GACT_RELVERIFY;
	gdat->GadgetType   = GTYP_BOOLGADGET;
	gdat->ImageNormHandle = (APTR)tag->ti_Data;
	gdat->ImageNormHandle->flags |= DONT_DRAW;

	h->minw = gdat->ImageNormHandle->minw;
	h->minh = gdat->ImageNormHandle->minh;

	if (tag=FindTagItem(GAGA_BU_ImageSelect,mytaglist))
	{
	  gdat->ImageSelectHandle = (APTR)tag->ti_Data;
	  gdat->ImageSelectHandle->flags |= DONT_DRAW;
	  gdat->Flags	    |= GFLG_GADGHIMAGE;
	}
	else
	  gdat->Flags	    |= GFLG_GADGHCOMP;
      }
      else
      {
	if (h->data.newgad.ng.ng_GadgetText)
	  h->minw = 2+2 + TextWidth(h->data.newgad.ng.ng_GadgetText) + 2+2;
	else
	  h->minw = FontHeight(); /* todo */

	h->minh   = 2 + FontHeight() + 2;
      }

      h->xpri = 1;
      h->ypri = 1;
      break;
#endif

#if !defined(DONT_NEED_GAD_STRING)  || \
    !defined(DONT_NEED_GAD_INTEGER) || \
    !defined(DONT_NEED_GAD_DOUBLE)

    case STRING_KIND:
    case INTEGER_KIND:
      {
	int  nChars=0; /* number of characters in gadget (without cursor) */
	int  CharWidth;


	/* get a copy of the taglist */

	taglistcopy = AllocCopyTaglist(systaglist, 1 + /* GTST_MaxChars */
						   1   /* GTST_String	*/
						   );
	if (!taglistcopy) return HANDLE_ERR;


	/* determine real gadget kind */

	if (FindTagItem( GAGA_ST_Ptr , mytaglist)) kind = STRING_KIND;
	if (FindTagItem( GAGA_IN_Ptr , mytaglist)) kind = INTEGER_KIND;
	if (FindTagItem( GAGA_DB_Ptr , mytaglist)) { kind = STRING_KIND;
						     gdat->data.string.flags |= STRING_DBL;
						   }


	/* set the idcmp */

	if (kind == STRING_KIND)  CrTrDat.IDCMP |= STRINGIDCMP;
	if (kind == INTEGER_KIND) CrTrDat.IDCMP |= INTEGERIDCMP;


	/* set pointers */

	if (tag=FindTagItem(GAGA_ST_Ptr,mytaglist)) gdat->data.string.dest     = (void *)tag->ti_Data;
	if (tag=FindTagItem(GAGA_DB_Ptr,mytaglist)) gdat->data.string.dbl_dest = (void *)tag->ti_Data;
	if (tag=FindTagItem(GAGA_IN_Ptr,mytaglist)) gdat->data.integer.dest    = (void *)tag->ti_Data;


	/* set the bounds */

#ifndef DONT_NEED_GAD_DOUBLE

	if (kind == STRING_KIND && (gdat->data.string.flags & STRING_DBL))
	{
	  if (tag=FindTagItem(GAGA_UpperBound,mytaglist))
	  {
	    int txtlen;
	    gdat->data.string.upperbound  = *((double *)tag->ti_Data);
	    gdat->data.string.flags	 |= STRING_DBL_UPPER_BOUND;

	    txtlen = CharsForDblNumber(gdat->data.string.upperbound,
				       gdat->data.string.formatstr);
	    nChars = max(nChars,txtlen);
	  }
	  if (tag=FindTagItem(GAGA_LowerBound,mytaglist))
	  {
	    int txtlen;
	    gdat->data.string.lowerbound  = *((double *)tag->ti_Data);
	    gdat->data.string.flags	 |= STRING_DBL_LOWER_BOUND;

	    txtlen =CharsForDblNumber(gdat->data.string.upperbound,
				      gdat->data.string.formatstr);
	    nChars = max(nChars,txtlen);
	  }
	}
#endif

#ifndef DONT_NEED_GAD_INTEGER

	if (kind == INTEGER_KIND)
	{
	  if (tag=FindTagItem(GAGA_UpperBound,mytaglist))
	  {
	    int txtlen;
	    gdat->data.integer.upperbound  = tag->ti_Data;
	    gdat->data.integer.flags	  |= INTEGER_UPPER_BOUND;

	    txtlen = CharsForNumber(tag->ti_Data);
	    nChars = max ( nChars , txtlen );
	  }
	  if (tag=FindTagItem(GAGA_LowerBound,mytaglist))
	  {
	    int txtlen;
	    gdat->data.integer.lowerbound  = tag->ti_Data;
	    gdat->data.integer.flags	  |= INTEGER_LOWER_BOUND;

	    txtlen = CharsForNumber(tag->ti_Data);
	    nChars = max ( nChars , txtlen );
	  }
	}

#endif

	/* set length of destination-pointer */

#ifndef DONT_NEED_GAD_INTEGER

	if (kind == INTEGER_KIND)
	  gdat->data.integer.bytelength = GetTagData(GAGA_NumberBytes,4,mytaglist);
#endif


#ifndef DONT_NEED_GAD_DOUBLE

	/* set the double - format string */

	if (kind == STRING_KIND && (gdat->data.string.flags & STRING_DBL))
	{
	  gdat->data.string.formatstr = (void *)GetTagData (
						  GAGA_DoubleFormat,
						  (ULONG)"%lf",
						  mytaglist  );
	}
#endif

	/* set inital value of gadget */

	tag = FindTagItem ( GAGA_DontFillIn , mytaglist );

	if (tag == NULL || tag->ti_Data == FALSE)
	{

#ifndef DONT_NEED_GAD_STRING

	    if (kind == STRING_KIND)
	    {

#ifndef DONT_NEED_GAD_DOUBLE

		if (gdat->data.string.flags & STRING_DBL)
		{
		    if (gdat->data.string.dbl_dest)
		    {
		      char *buffer;
		      double d;

		      buffer=cu_calloc(25,1);
		      if (!buffer) return HANDLE_ERR;

		      d=*(gdat->data.string.dbl_dest);
		      d=SetDoubleToBounds(gdat,d,NULL);
		      sprintf(buffer,gdat->data.string.formatstr,d);
		      InsertTag ( taglistcopy ,
				  GTST_String ,
				  (ULONG)buffer
				);
		    }
		}
		else

#endif

		{
		    if (gdat->data.string.dest)
		    {
		      InsertTag ( taglistcopy ,
				  GTST_String ,
				  (ULONG)gdat->data.string.dest
				);
		    }
		}
	    }

#endif


#ifndef DONT_NEED_GAD_INTEGER

	    if (kind == INTEGER_KIND)
	    {
	      if (gdat->data.integer.dest)
		InsertTag ( taglistcopy ,
			    GTIN_Number ,
			    SetIntegerToBounds(
				gdat,
				GetIntegerValue( gdat->data.integer.dest ,
						 gdat->data.integer.bytelength
					       ),
				NULL
			    )
			  );
	    }

#endif

	}


	/* get number of chars to be displayed */

	nChars = GetTagData(GAGA_CharsWidth,nChars,mytaglist);
	InsertTag(taglistcopy , GTIN_MaxChars , nChars);


	/* set width of gadget */

	if ( kind == STRING_KIND )
	  CharWidth = MaxCharWidth();
	else {
	  assert(kind == INTEGER_KIND);
	  CharWidth = MaxNumWidth();
	}

	h->minw = 4 + 2 + nChars * CharWidth + 2 + 4;
	h->minh = 2 + FontHeight() + 2;

	h->minw += MaxCharWidth(); /* place for the cursor */

	h->flags |= ABSOLUTE_HEIGHT;

	/* ??? There seems to be a bug in the OS: I used a string-gadget with
	 *     a proportional font and the width of the gadget had to be
	 *     one pixel wider then the formula above said. But as there's no
	 *     official documentation about the exact appearance and behaviour
	 *     of GadTools gadgets, I can't fix the bug. Everything works fine
	 *     with a monospace font.
	 */

	h->xpri = 1;
	h->ypri = 0;
      }
      break;
#endif

#ifndef DONT_NEED_GAD_CHECKBOX

    case CHECKBOX_KIND:
      taglistcopy = AllocCopyTaglist(systaglist,1); /* for GTCB_Checked */
      if (!taglistcopy) return HANDLE_ERR;

      CrTrDat.IDCMP |= CHECKBOXIDCMP;

      h->minw = 26;
      h->minh = 11;
      h->xpri = 0;
      h->ypri = 0;

      h->flags |= ABSOLUTE_WIDTH | ABSOLUTE_HEIGHT;

      gdat->data.check.bytelength = GetTagData(GAGA_NumberBytes,4,mytaglist);
      gdat->data.check.switchflag = GetTagData(GAGA_CB_Switch,(1<<0),mytaglist);
      gdat->data.check.dest	  = (void *)GetTagData(GAGA_CB_Ptr,NULL,mytaglist);

      tag=FindTagItem(GAGA_DontFillIn,mytaglist);
      if (tag==NULL || tag->ti_Data==FALSE)
	if (gdat->data.check.dest)
	{
	  gdat->data.check.selected = GetIntegerValue(
					gdat->data.check.dest,
					gdat->data.check.bytelength
				      )
				      & gdat->data.check.switchflag;

	  InsertTag(taglistcopy,
		    GTCB_Checked,
		    gdat->data.check.selected
		   );
	}
      break;
#endif

#ifndef DONT_NEED_GAD_CYCLE

    case CYCLE_KIND:
      {
	char **labels;
	int    listlength;
	struct List *listptr;

	taglistcopy = AllocCopyTaglist(systaglist,1 + /* for GTCY_Labels */
						  1); /* for GTCY_Active */
	if (!taglistcopy) return HANDLE_ERR;


	CrTrDat.IDCMP |= CYCLEIDCMP;

	gdat->data.cycle.dest = (void *)GetTagData(GAGA_CY_Ptr,NULL,mytaglist);
	gdat->data.cycle.bytelength = GetTagData(GAGA_NumberBytes,4,mytaglist);


	/* extract the names from a list */

	if (tag=FindTagItem(GAGA_CY_List,mytaglist))
	{
	  listptr=(struct List *)tag->ti_Data;

	  listlength=ListLen(listptr);
	  labels=(char **)cu_calloc(listlength+1 , sizeof(APTR) );
	  if (!labels) return HANDLE_ERR;

	  gdat->data.cycle.labels = labels;
	  List2NameArray(listptr,labels);

	  InsertTag(taglistcopy, GTCY_Labels , (ULONG) labels);
	}
	else
	{
	  tag=FindTagItem(GTCY_Labels,systaglist);
	  assert(tag != NULL);
	  labels=(void *)tag->ti_Data;
	}


	/* set active selection */

	if (tag=FindTagItem(GAGA_CY_ActiveNode,mytaglist))
	{
	    InsertTag(taglistcopy,
		      GTCY_Active,
		      FindNodeNr((struct Node *)tag->ti_Data,listptr) );
	}
	else
	{
	    tag=FindTagItem(GAGA_DontFillIn,mytaglist);
	    if (!tag || tag->ti_Data == FALSE)
	    {
		if (gdat->data.cycle.dest)
		{
		  InsertTag(taglistcopy,
			    GTCY_Active,
			    GetIntegerValue(gdat->data.cycle.dest,
					    gdat->data.cycle.bytelength) );
		}
	    }
	}


	/* compute size */

	/* I really hope the X-size of the cycle-symbol is fixed ! */
	h->minw = 4 + 20 + GetMaxLabelLength(labels) + 4;
	h->minh = 2 + FontHeight() + 2;
	h->xpri = 1;
	h->ypri = 0;

	h->flags |= ABSOLUTE_HEIGHT;
      }
      break;

#endif

#ifndef DONT_NEED_GAD_LISTVIEW

    case LISTVIEW_KIND:
      {
	int lines_height;
	int chars_width;


	taglistcopy = AllocCopyTaglist(systaglist,1   /* for GTLV_Selected */
						   );
	if (!taglistcopy) return HANDLE_ERR;


	CrTrDat.IDCMP |= LISTVIEWIDCMP;


	/* dest and bytelength */

	gdat->data.listview.dest       = (void *)GetTagData(GAGA_LV_Ptr,NULL,mytaglist);
	gdat->data.listview.bytelength = GetTagData(GAGA_NumberBytes,4   ,mytaglist);


	/* string-gadget */

	if (tag=FindTagItem(GAGA_LV_StringGad,mytaglist))
	{
	  struct Handle *strh;

	  strh=(void *)tag->ti_Data;
	  if (strh == HANDLE_ERR) return HANDLE_ERR;
	  gdat->data.listview.strgadget = strh;
	  strh->flags |= SUBORDINATE;
	}


	/* active node */

	if (tag=FindTagItem(GAGA_LV_ActiveNode,mytaglist))
	{
	  struct List *listptr;

	  listptr=(struct List *)GetTagData(GTLV_Labels,NULL,systaglist);

	  if (listptr)
	    InsertTag(taglistcopy,
		      GTLV_Selected,
		      FindNodeNr((struct Node *)tag->ti_Data,listptr)
		     );
	}
	else
	{
	    tag=FindTagItem(GAGA_DontFillIn,mytaglist);
	    if (!tag || tag->ti_Data == FALSE)
	    {
		if (gdat->data.listview.dest)
		{
		  InsertTag(taglistcopy,
			    GTLV_Selected,
			    GetIntegerValue(gdat->data.listview.dest,
					    gdat->data.listview.bytelength) );
		}
	    }
	}




	/* calc size */

	lines_height = GetTagData(GAGA_LV_nLines    , 5,mytaglist);
	chars_width  = GetTagData(GAGA_LV_CharsWidth,10,mytaglist);

	h->minw = 2+2 + chars_width * MaxCharWidth() + 2+2 + 16;
	h->minh = 2 + lines_height * FontHeight() + 2;

	if (gdat->data.listview.strgadget)
	  h->minh += gdat->data.listview.strgadget->minh;
      }
      break;

#endif

    case TEXT_KIND:
      {
	int width;

	taglistcopy = AllocCopyTaglist(systaglist,1   /* for GTTX_Border */
						   );
	if (!taglistcopy) return HANDLE_ERR;


	CrTrDat.IDCMP |= TEXTIDCMP;

	InsertTag(taglistcopy,GTTX_Border,TRUE);

	/* calc width */

	width=GetTagData(GAGA_TX_CharsWidth,5,mytaglist);

	h->minw = 2+2 + width*MaxCharWidth() + 2+2;
	h->minh = 1 + FontHeight() + 1;

      }
      break;

    default:
      assert(1==2);
      break;
  }


  /* set kind of gadget (may change during switch()...) */

  h->data.newgad.kind = kind;
  gdat->kind	      = kind;


  /* attach taglist */

  h->data.newgad.taglist = taglistcopy;


  /* insert handle in list */

  LinkHandle(h);
  LinkGadData(gdat);

  return h;
}



struct Handle *CrGadget(Tag tag, ...)
{
  return CrGadgetA((struct TagItem *)&tag,SecondTaglist((struct TagItem *)&tag));
}


#ifndef DONT_NEED_GAD_CHECKBOX

struct Handle *CrCBGadget(char *text,Tag tag, ...)
{
  struct TagItem *mylist,*syslist;
/*
  static struct TagItem checkbox_prolog[] =
  {
    GAGA_Kind,CHECKBOX_KIND,
    TAG_MORE , NULL
  };
*/

  mylist =(struct TagItem *)&tag;
  syslist=SecondTaglist((struct TagItem *)&tag);

//  checkbox_prolog[1].ti_Data = (ULONG)&mylist;

  return CrHBox(
		 CrGadgetA(mylist,syslist),
		 CrFiller(),
		 NewPri(CrText(text,TAG_DONE),0),
		 HANDLE_END
	       );
}

#endif


/**------------------------------------------------------------------------**
 **  CrArrowGadget
 **------------------------------------------------------------------------**/

static void DrawArrowRight(struct DrawArea *da)
{
  DrSetPen(da,TEXTPEN);

  DrMove(da,15,50);
  DrDraw(da,15,42);
  DrDraw(da,50,42);
  DrDraw(da,50,25);
  DrDraw(da,85,50);

  DrMirrorY(da,50);

  DrFill(da,60,50);
}

static void DrawArrowRightSelected(struct DrawArea *da)
{
  DrSetPen(da,TEXTPEN);

  DrMove(da,20,50);
  DrDraw(da,20,42);
  DrDraw(da,55,42);
  DrDraw(da,55,25);
  DrDraw(da,90,50);

  DrMirrorY(da,50);

  DrFill(da,60,50);
}

struct Handle *CrArrowGadgetA(struct TagItem *mytags, struct TagItem *systags)
{
  static struct TagItem mytagprolog[] =
  {
    GAGA_BU_ImageNorm  ,0,
    GAGA_BU_ImageSelect,0,
    GAGA_Kind	       ,BUTTON_KIND,
    TAG_MORE	       ,0
  };

  mytagprolog[0].ti_Data = (ULONG)CrImage(100,100,100,&DrawArrowRight,
					  GAGA_IMG_BevelBox,FALSE,
					  TAG_DONE),
  mytagprolog[1].ti_Data = (ULONG)CrImage(100,100,100,&DrawArrowRightSelected,
					  GAGA_IMG_BevelBox,TRUE,
					  TAG_DONE),
  mytagprolog[3].ti_Data = (ULONG)mytags;

  return CrGadgetA(
		   mytagprolog,
		   systags
		  );
}

struct Handle *CrArrowGadget(Tag tag, ...)
{
  struct TagItem *mylist,*syslist;

  mylist =(struct TagItem *)&tag;
  syslist=SecondTaglist((struct TagItem *)&tag);

  return CrArrowGadgetA(mylist,syslist);
}


/**------------------------------------------------------------------------**
 **  CrFilereqGadget
 **------------------------------------------------------------------------**/

static void DrawFilerequestImage(struct DrawArea *da)
{
  DrSetPen(da,TEXTPEN);

  DrMove(da,130,30);
  DrDraw(da, 10,30);
  DrDraw(da, 10,90);
  DrDraw(da,130,90);
  DrDraw(da,130,10);
  DrDraw(da, 90,10);
  DrDraw(da, 70,30);

  DrFill(da,110,20);
}

static void DrawFilerequestSelectedImage(struct DrawArea *da)
{
  DrSetPen(da,TEXTPEN);

  DrMove(da,130,30);
  DrDraw(da, 10,30);
  DrDraw(da, 10,90);
  DrDraw(da,130,90);
  DrDraw(da,145,50);
  DrDraw(da, 25,50);
  DrDraw(da, 10,90);

  DrMove(da,130,50);
  DrDraw(da,130,10);
  DrDraw(da, 90,10);
  DrDraw(da, 70,30);

  DrFill(da,110,20);
}

struct Handle *CrFilerequestGadgetA(struct TagItem *mytags, struct TagItem *systags)
{
  static struct TagItem mytagprolog[] =
  {
    GAGA_BU_ImageNorm  ,0,
    GAGA_BU_ImageSelect,0,
    GAGA_Kind	       ,BUTTON_KIND,
    TAG_MORE	       ,0
  };

  mytagprolog[0].ti_Data = (ULONG)CrImage(100,150,100,&DrawFilerequestImage,
					  GAGA_IMG_BevelBox,FALSE,
					  TAG_DONE),

  mytagprolog[1].ti_Data = (ULONG)CrImage(100,150,100,&DrawFilerequestSelectedImage,
					  GAGA_IMG_BevelBox,TRUE,
					  TAG_DONE),
  mytagprolog[3].ti_Data = (ULONG)mytags;

  return CrGadgetA(
		   mytagprolog,
		   systags
		  );
}

struct Handle *CrFilerequestGadget(Tag tag, ...)
{
  struct TagItem *mylist,*syslist;

  mylist =(struct TagItem *)&tag;
  syslist=SecondTaglist((struct TagItem *)&tag);

  return CrFilerequestGadgetA(mylist,syslist);
}



/**------------------------------------------------------------------------**
 **  CrTextPopup - Create a popup-gadget.
 **		   Two Tag-Item-lists must be provided. The first is an
 **		   ordinary taglist. The second specifies the strings.
 **		   ti_Tag points to the string to be displayed. ti_Data
 **		   contains the data you want to receive from the gadget.
 **		   The string-list must be terminated with a NULL string-ptr.
 **------------------------------------------------------------------------**/

struct Handle *CrTextPopupA(struct TagItem *taglist,struct TagItem *stringlist)
{
  struct Handle *h,*button,*text;
  struct Handle *all;
  static struct TagItem nillist = { TAG_DONE };
  struct TagItem *tag;

  /* alloc new handle and GadgetData */

  if (!(h=NewHandle()))
    return HANDLE_ERR;


  /* check global tags */

  TreatGlobalTags(h,taglist);


  h->Type = ha_Popup;
  if (!(h->data.popup.stringlist = AllocCopyTaglist(stringlist,0)))
    return HANDLE_ERR;

  h->data.popup.destination = (ULONG *)GetTagData(GAGA_POP_Ptr,NULL,taglist);

  if (h->data.popup.destination)
    h->data.popup.selection = *(h->data.popup.destination);


  if ((tag=FindTagItem(GAGA_POP_Disable31,taglist)) && tag->ti_Data)
    h->data.popup.flags |= POPUP_BIT31DISABLE;

  button=NewPri(CrArrowGadgetA(taglist,&nillist),0);
  if (button == HANDLE_ERR) return HANDLE_ERR;

  assert(button->Type == ha_NewGadget);

  button->data.newgad.gaddata->RealHandle = h;

  text = CrGadget(GAGA_Kind         ,TEXT_KIND,
		  TAG_DONE,
		  TAG_DONE);

  text = NewPriXY(text,1,0);

  h->data.popup.buttongad = button;
  h->data.popup.textgad   = text;

  all = CrHBox(text,button,HANDLE_END);
  if (all == HANDLE_ERR) return HANDLE_ERR;

  all->pri_chain = text;

  LinkHandle(h);

  return all;
}

struct Handle *CrTextPopup(Tag tag, ...)
{
  struct TagItem *taglist;
  struct TagItem *stringlist;

  taglist = (struct TagItem *)&tag;
  stringlist = SecondTaglist((struct TagItem *)&tag);

  return CrTextPopupA(taglist,stringlist);
}

