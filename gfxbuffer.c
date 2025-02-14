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

int AbsMemMinimum   = 10000;
int AbsMinMemBlock  = 5000;
// int AbsMinChipBlock = 250000;


BOOL CreateSpecialEffects(ULONG flags); /* see efx.c */


struct ImageStripe	/* defines a stripe (or all) of the image */
{
  struct MinNode node;	/* internal use only */

  union Pixel *Plane;

  ULONG        TopPos;	/* number of first line  */
  ULONG        Height;	/* height of this stripe */

  ULONG  Filenr;	 /* number, this file has on disk */
  UBYTE  Unimportant;
  USHORT LockCnt;	 /* VirtualMem-Manager may NOT put this on disk ! */
};


struct ColorLookupTable
{
  UWORD nColors;

  UBYTE *R; /* these arrays will be dynamically allocated */
  UBYTE *G;
  UBYTE *B;
};


struct ImageData
{
  struct MinNode node; /* this node is only for internal use (garbage collecor) */

  UWORD  Width,Height;
  LONG	 CAMG;

  short  backgroundcolor;

  struct List ImageStripes;

  struct ColorLookupTable *Clut;

  enum	 PixelMode PixelMode;

  struct {
	   unsigned HaveCAMG	  : 1;
//	     unsigned HaveCLUT	    : 1;
//	     unsigned PixelsAreRGB  : 1;
//	     unsigned PixelsAreCLUT : 1;
	 } Flags;
};



/*==============================================================================

				Global variables

  ==============================================================================*/




static struct List ImageList;  /* only for internal garbage collection */

static struct ImageData *ImageBuffer;

static struct ImageData   *last_load_image  = 0;
static int		   last_load_y	    = -1;
static struct ImageStripe *last_load_stripe = 0;

static struct ImageData   *last_save_image  = 0;
static int		   last_save_y	    = -1;
static struct ImageStripe *last_save_stripe = 0;

static APTR GetMemory(int size,int n);


/*==============================================================================

				initialization

  ==============================================================================*/




/**------------------------------------------------------------------------**
 **  This routines should be called at the very beginning of the program.
 **------------------------------------------------------------------------**/

void InitGfxBuffer(void)
{
  NewList(&ImageList);
}




/*==============================================================================

				virtual memory

  ==============================================================================*/


/*  delete a temporary file
 */

static void DeleteTemp(ULONG nr)
{
  char buffer[TMP_FILENAME_LENGTH + 15];

  sprintf(buffer,"%s%d%s",GetTempPrefix(),nr,GetTempPostfix());
  DeleteFile(buffer);
}



/*  load a cached stripe from disk.
 */

static BOOL LoadStripe(struct ImageStripe *stripe)
{
  FILE *fp;
  int	length;
  char	buffer[TMP_FILENAME_LENGTH];

  assert(stripe->Plane == NULL);
  assert(stripe->Filenr != 0);

  sprintf(buffer,"%s%d%s",GetTempPrefix(),stripe->Filenr,GetTempPostfix());

  fp=fopen(buffer,"r");
  if (!fp) { SetError(DOS_ERROR); return FALSE; }
  fseek(fp,0,2);
  length=ftell(fp);
  fseek(fp,0,0);

  stripe->Plane = GetMemory(length,1);
  if (!stripe->Plane) { fclose(fp); return FALSE; }

  if (fread((APTR)stripe->Plane,length,1,fp) != 1)
  { SetError(DOS_ERROR); return FALSE; }
  fclose(fp);

  DeleteTemp(stripe->Filenr);
  stripe->Filenr = 0;

  return TRUE;
}




/**------------------------------------------------------------------------**
 **  Search the stripes to find one, which will be splitted in two pieces
 **  and cached to disk.
 **------------------------------------------------------------------------**/

static BOOL GarbageCollector(void)
{
  struct ImageStripe *stripe,*newstr1,*newstr2;
  struct ImageData   *image;
  static unsigned long tempnr = 1;
  FILE	 *f1,*f2;
  char	 buffer[TMP_FILENAME_LENGTH];

  for (image=(struct ImageData *)ImageList.lh_Head ;
       image->node.mln_Succ ;
       image=(struct ImageData *)image->node.mln_Succ)

    for (stripe=(struct ImageStripe *)image->ImageStripes.lh_Head ;
	 stripe->node.mln_Succ ;
	 stripe=(struct ImageStripe *)stripe->node.mln_Succ)
      if (stripe->Plane)
      {
	if (stripe->Unimportant)
	{
	  cu_free(stripe->Plane);
	  stripe->Plane =0;
	  stripe->Height=0;	  /* only for security: stripe cannot be used anymore */

	  /* move node to end of list, as we don't want to spend any time
	   * searching through nodes we don't need any more.
	   */

	  Remove((struct Node *)&stripe->node);
	  AddTail(&image->ImageStripes,(struct Node *)&stripe->node);

	  return TRUE;
	}
	else if (stripe->LockCnt >= 1)
	{
	  /* leider nichts möglich, da es benutzt wird ! */
	}
	else if (stripe->Height > 1)
	{
	  newstr1 = cu_calloc( sizeof(struct ImageStripe),1 );
	  if (!newstr1) { SetError(NO_MEM); return FALSE; }

	  AddHead(&image->ImageStripes,(struct Node *)&newstr1->node);

	  newstr1->TopPos = stripe->TopPos;
	  newstr1->Height = stripe->Height / 2;
	  newstr1->Filenr = tempnr++;
	  sprintf(buffer,"%s%d%s",GetTempPrefix(),newstr1->Filenr,GetTempPostfix());

	  f1=fopen(buffer,"w");
	  if (!f1) { SetError(DOS_ERROR); return FALSE; }
	  if (
	  fwrite((APTR)stripe->Plane,
		 newstr1->Height*image->Width,
		 sizeof(union Pixel),
		 f1) != sizeof(union Pixel) ) { SetError(DOS_ERROR); return FALSE; }
	  fclose(f1);

	  newstr2 = stripe;
	  newstr2->TopPos += newstr1->Height;
	  newstr2->Height -= newstr1->Height;
	  newstr2->Filenr  = tempnr++;
	  sprintf(buffer,"%s%d%s",GetTempPrefix(),newstr2->Filenr,GetTempPostfix());

	  f2=fopen(buffer,"w");
	  if (!f2) { SetError(DOS_ERROR); return FALSE; }
	  if (
	  fwrite((APTR)(stripe->Plane + newstr1->Height*image->Width),
		 newstr2->Height*image->Width,
		 sizeof(union Pixel),
		 f2) != sizeof(union Pixel) ) { SetError(DOS_ERROR); return FALSE; }
	  fclose(f2);

	  cu_free(stripe->Plane);

	  newstr1->Plane = NULL;
	  newstr2->Plane = NULL;

	  return TRUE;
	}
	else  /* Height == 1 */
	{
	  stripe->Filenr = tempnr++;
	  sprintf(buffer,"%s%d%s",GetTempPrefix(),stripe->Filenr,GetTempPostfix());

	  f1=fopen(buffer,"w");
	  if (!f1) { SetError(DOS_ERROR); return FALSE; }

	  if (
	    fwrite((APTR)stripe->Plane,
		   stripe->Height*image->Width,
		   sizeof(union Pixel),
		   f1) != sizeof(union Pixel) ) { SetError(DOS_ERROR); return FALSE; }
	  fclose(f1);

	  cu_free(stripe->Plane);
	  stripe->Plane=NULL;

	  return TRUE;
	}
      }

  return FALSE;
}


/**------------------------------------------------------------------------**
 **  If you are sure (!) that you don't need a line any more, call this
 **  function. This will help the GarbageCollector() to find more memory.
 **------------------------------------------------------------------------**/

void ForgetLine(struct ImageData *image,ULONG y)
{
  struct ImageStripe *stripe;

  for (stripe=(struct ImageStripe *)image->ImageStripes.lh_Head ;
       stripe->node.mln_Succ ;
       stripe=(struct ImageStripe *)stripe->node.mln_Succ)
  {
    if (stripe->TopPos + stripe->Height -1 == y)
    {
      stripe->Unimportant = TRUE;
      return;
    }
  }
}

/**------------------------------------------------------------------------**
 **  A stub function that replaces the normal 'calloc()' function, as we
 **  must call our GarbageCollector() somewhere.
 **------------------------------------------------------------------------**/

static APTR GetMemory(int size,int n)
{
  APTR ptr;

  while ( AvailMem(NULL)         < AbsMemMinimum ) { if (!GarbageCollector()) break; }
  while ( AvailMem(MEMF_LARGEST) < AbsMinMemBlock) { if (!GarbageCollector()) break; }

  do {
    ptr=cu_calloc(size,n);
  } while ( ptr==NULL && GarbageCollector() );

  if (ptr==NULL) SetError(NO_MEM);

  return ptr;
}



/**------------------------------------------------------------------------**
 **  Move stripes to MEMF_FAST if they are blocking MEMF_CHIP.
 **------------------------------------------------------------------------**/

void MoveStripesToFastMem(void)
{
  struct ImageStripe *stripe;
  struct ImageData   *image;
  union  Pixel *NewPlane;

  for (image=(struct ImageData *)ImageList.lh_Head ;
       image->node.mln_Succ ;
       image=(struct ImageData *)image->node.mln_Succ)

    for (stripe=(struct ImageStripe *)image->ImageStripes.lh_Head ;
	 stripe->node.mln_Succ ;
	 stripe=(struct ImageStripe *)stripe->node.mln_Succ)
      if (stripe->Plane)
	if (TypeOfMem(stripe->Plane) & MEMF_CHIP)
	{
	  NewPlane=cu_calloc(image->Width*stripe->Height,sizeof(union Pixel));
	  if (NewPlane)
	  {
	    CopyMem(stripe->Plane,NewPlane,image->Width*stripe->Height*sizeof(union Pixel));
	    cu_free(stripe->Plane);
	    stripe->Plane=NewPlane;
	  }
	}
}



/*==============================================================================

			       Color Lookup Table

  ==============================================================================*/



/**------------------------------------------------------------------------**
 **  Allocate a new Color-Lookup-Table.
 **------------------------------------------------------------------------**/

struct ColorLookupTable *GetCLUT(UWORD nColors)
{
  struct ColorLookupTable *clut;
  int n;

  /* get CLUT */

  if (!(clut    = GetMemory(sizeof(*clut),1) )) goto errex1;
  if (!(clut->R = GetMemory( nColors , sizeof(UBYTE) ) )) goto errex2;
  if (!(clut->G = GetMemory( nColors , sizeof(UBYTE) ) )) goto errex3;
  if (!(clut->B = GetMemory( nColors , sizeof(UBYTE) ) )) goto errex4;

  clut->nColors = nColors;

  for (n=0;n<nColors;n++)
  {
    clut->R[n] = EmptyCLUTEntry_R;
    clut->G[n] = EmptyCLUTEntry_G;
    clut->B[n] = EmptyCLUTEntry_B;
  }

  return clut;

errex4: cu_free(clut->G);
errex3: cu_free(clut->R);
errex2: cu_free(clut);
errex1: return NULL;
}




/**------------------------------------------------------------------------**
 **  Free the Color-Lookup-Table
 **------------------------------------------------------------------------**/

void FreeCLUT(struct ColorLookupTable *clut)
{
  assert(clut != NULL);

  cu_free(clut->R);
  cu_free(clut->G);
  cu_free(clut->B);
  cu_free(clut);
}

void RemoveCLUT(struct ImageData *image)
{
  if (image->Clut) FreeCLUT(image->Clut);
  image->Clut=NULL;
}


/**------------------------------------------------------------------------**
 **  Attach the CLUT to an image.
 **------------------------------------------------------------------------**/

void AttachCLUT(struct ColorLookupTable *clut,struct ImageData *image)
{
  assert (image->Clut == NULL);

  image->Clut = clut;
}


/**------------------------------------------------------------------------**
 **  Report the number of colors in this clut.
 **------------------------------------------------------------------------**/

UWORD GetNColors(struct ColorLookupTable *clut)
{
  return clut->nColors;
}

UWORD GetImageNColors(struct ImageData *image)
{
  assert(image->Clut != NULL);
  return image->Clut->nColors;
}



/**------------------------------------------------------------------------**
 **  set a color int the table.
 **------------------------------------------------------------------------**/

void SetColor(struct ColorLookupTable *clut,UWORD nr,UBYTE r,UBYTE g,UBYTE b)
{
  assert( nr<clut->nColors);

  clut->R[nr] = r;
  clut->G[nr] = g;
  clut->B[nr] = b;
}


/**------------------------------------------------------------------------**
 **  get a color from the table.
 **------------------------------------------------------------------------**/

BOOL GetColor(struct ColorLookupTable *clut,UWORD nr,UBYTE *r,UBYTE *g,UBYTE *b)
{
  if (nr > clut->nColors)
  {
    *r=*g=*b=255;
    return FALSE;
  }
  else
  {
    *r=clut->R[nr];
    *g=clut->G[nr];
    *b=clut->B[nr];
    return TRUE;
  }
}

BOOL GetImageColor(struct ImageData *image,UWORD nr,UBYTE *r,UBYTE *g,UBYTE *b)
{
  assert(image->Clut != NULL);
  return GetColor(image->Clut,nr,r,g,b);
}










/*==============================================================================

			    Image - buffer allocation

  ==============================================================================*/


#define MAXIMAGESTRIPES 1024
#define NORMIMAGESTRIPES 128

static BOOL GetImageStripesStructs(struct ImageStripe *stripes[],int nStripes)
{
  int i;

  for (i=0;i<nStripes;i++)
  {
    stripes[i]=GetMemory(sizeof(struct ImageStripe),1);
    if (!(stripes[i])) goto errexit;
  }

  return TRUE;

errexit:
  for ( --i ; i>=0 ; i-- )
    cu_free(stripes[i]);

  return FALSE;
}

/*  get the image-stripes
 */
static BOOL GetImageStripes(struct List *list,UWORD width,UWORD height)
{
  struct ImageStripe *NewImageStripes[MAXIMAGESTRIPES];
  int		      nStripes;
  union   Pixel      *Plane;
  int		      i;
  int		      curr_top;
  int		      height_left;
  int		      this_height;

  /* allocate memory for stripe(s) */

  nStripes=min(NORMIMAGESTRIPES,height); /* this allows a more flexible memory usage */

  do {
    Plane=GetMemory( (height/nStripes)*width, sizeof(union Pixel) );
    if (!Plane) nStripes+=8;
    else {
      if (!GetImageStripesStructs(NewImageStripes,nStripes))
      {
	cu_free(Plane);
	nStripes+=8;
      }
    }

    if (nStripes > MAXIMAGESTRIPES) return FALSE;

    if (height/nStripes < 1)
      return FALSE; /* sorry, can't get memory */

  } while (Plane==NULL || NewImageStripes==NULL);


  /* init Stripes */

  curr_top = 0;
  height_left = height;

  for (i=0 ; i<nStripes ; i++)
  {
    this_height = height_left/(nStripes-i); /* this looks a bit silly, but assures
					     * a better distribution.
					     */
    NewImageStripes[i]->Height = this_height;
    NewImageStripes[i]->TopPos = curr_top;

    curr_top += this_height;
    height_left -= this_height;

    if (i==0) /* only first stripe gets memory for building the image */
    {
      NewImageStripes[i]->Plane = Plane;
    }

    AddTail(list,(struct Node *)&NewImageStripes[i]->node);
  }

  return TRUE;
}



/**------------------------------------------------------------------------**
 **  Get and initialize a buffer. This buffer hasn't got a Color-Table !!!
 **  The table must be allocated with GetCLUT() and attached to the buffer
 **  with AttachCLUT().
 **------------------------------------------------------------------------**/

struct ImageData *GetBuffer( ULONG width, ULONG height)
{
  struct ImageData *NewImageData;

  if (!(NewImageData=GetMemory(sizeof(*NewImageData),1)))
    return NULL;

  NewList(&NewImageData->ImageStripes);

  NewImageData->Width	= width;
  NewImageData->Height	= height;

  NewImageData->backgroundcolor = -1;  // no background


  /* get stripes */

  if (!GetImageStripes(&NewImageData->ImageStripes,width,height))
  {
    cu_free(NewImageData);
    return NULL;
  }

  AddTail(&ImageList,(struct Node *)&NewImageData->node);

  return NewImageData;
}


/* free all stripes in the list
 */

static void FreeStripes(struct List *list)
{
  struct ImageStripe *next,*curr;

  curr=(struct ImageStripe *)list->lh_Head ;

  while (next = (struct ImageStripe *)(curr->node.mln_Succ) )
  {
    if (curr->Plane) cu_free(curr->Plane);
    else
    if (curr->Filenr != 0) DeleteTemp(curr->Filenr);

    Remove((struct Node *)&curr->node);
    cu_free(curr);

    curr=next;
  }
}



/**------------------------------------------------------------------------**
 **  Free the image-buffer (also free the CLUT if one is connected to the
 **  image).
 **------------------------------------------------------------------------**/

void FreeBuffer(struct ImageData *image)
{
  FreeStripes(&image->ImageStripes);

  RemoveCLUT(image);

  Remove((struct Node *)&image->node);

  cu_free(image);

  if (last_load_image == image) last_load_image = 0;
  if (last_save_image == image) last_save_image = 0;
}







/*==============================================================================

			 additional image information

  ==============================================================================*/


/**------------------------------------------------------------------------**
 **  SetCAMG / GetCAMG
 **------------------------------------------------------------------------**/

void SetCAMG(struct ImageData *image,LONG CAMG)
{
  image->CAMG		= CAMG;
  image->Flags.HaveCAMG = TRUE;
}



BOOL GetCAMG(struct ImageData *image,LONG *CAMG)
{
  if (image->Flags.HaveCAMG) { *CAMG = image->CAMG; return TRUE; }
			else return FALSE;
}

/**------------------------------------------------------------------------**
 **  Set the pixel-mode
 **------------------------------------------------------------------------**/

void SetBufferMode(struct ImageData *image,enum PixelMode mode)
{
  image->PixelMode = mode;
}


/**------------------------------------------------------------------------**
 **  set and recall the default buffer
 **------------------------------------------------------------------------**/

void SetDefaultBuffer(struct ImageData *image)
{
  ImageBuffer = image;
}

struct ImageData *GetDefaultBuffer(void)
{
  return ImageBuffer;
}

UWORD GetImageWidth(struct ImageData *image)
{
  return image->Width;
}

UWORD GetImageHeight(struct ImageData *image)
{
  return image->Height;
}









/*==============================================================================

			       pixel manipulation

  ==============================================================================*/


/*  ensure that the line 'y' is in memory. Swap it in, if it's on the disk at
 *  the moment. Or allocate new memory if needed.
 */

enum StripeMode { Get,Set };

static struct ImageStripe *GetImageStripe(struct ImageData *image,
					  ULONG y,
					  enum StripeMode mode)
{
  struct ImageStripe *stripe;
  BOOL	 found;


  if (mode == Get)
  {
    if (image == last_load_image && y == last_load_y)
      if (last_load_stripe->Plane) return last_load_stripe;
  }
  else
  {
    if (image == last_save_image && y == last_save_y)
      if (last_save_stripe->Plane) return last_save_stripe;
  }


  found=FALSE;
  for (stripe = (struct ImageStripe *)image->ImageStripes.lh_Head ;
       stripe->node.mln_Succ ;
       stripe = (struct ImageStripe *)stripe->node.mln_Succ )
  {
    if (stripe->TopPos <= y && y < (stripe->TopPos + stripe->Height) )
    { found=TRUE; break; }
  }

  assert(found == TRUE);  /* the data-struct must be in memory !!! */


  if (mode == Get)
  {
    last_load_image  = image;
    last_load_y      = y;
    last_load_stripe = stripe;
  }
  else
  {
    last_save_image  = image;
    last_save_y      = y;
    last_save_stripe = stripe;
  }


  /* stripe in memory ? */

  if (stripe->Plane) return stripe;    /* image-stripe already in memory !!! */


  /* stripe on disk ? */

  if (stripe->Filenr != 0)
  {
    if (LoadStripe(stripe)) return stripe;
		       else return FALSE;
  }


  /* allocate memory for new stripe */

  stripe->Plane = GetMemory(image->Width * stripe->Height , sizeof(union Pixel) );
  if (stripe->Plane == NULL) return FALSE;
			else return stripe;
}




/**------------------------------------------------------------------------**
 **  Set a CLUT-Pixel
 **------------------------------------------------------------------------**/

BOOL SetCLUTPixel(struct ImageData *image,LONG x,LONG y,UWORD color)
{
  struct ImageStripe *Stripe;

  /*
  assert(x>=0);
  assert(x<image->Width);
  assert(y>=0);
  assert(y<image->Height);
  */

  Stripe=GetImageStripe(image,y,Set);
  if (!Stripe) return FALSE;

  Stripe->Plane[image->Width*(y - Stripe->TopPos) + x].color = color;

  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Get a CLUT-Pixel
 **------------------------------------------------------------------------**/

BOOL GetCLUTPixel(struct ImageData *image,LONG x,LONG y,UWORD *color)
{
  struct ImageStripe *Stripe;

  /*
  assert(x>=0);
  assert(x<image->Width);
  assert(y>=0);
  assert(y<image->Height);
  */

  Stripe=GetImageStripe(image,y,Get);
  if (!Stripe) return FALSE;

  *color = Stripe->Plane[image->Width*(y - Stripe->TopPos) + x].color;

  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Set a RGB-Pixel
 **------------------------------------------------------------------------**/

BOOL SetRGBPixel(struct ImageData *image,LONG x,LONG y,UBYTE r,UBYTE g,UBYTE b)
{
  struct ImageStripe *Stripe;
  int index;

  /*
  assert(x>=0);
  assert(x<image->Width);
  assert(y>=0);
  assert(y<image->Height);
  */

  Stripe=GetImageStripe(image,y,Set);
  if (!Stripe) return FALSE;

  index=image->Width*(y - Stripe->TopPos) + x;
  Stripe->Plane[index].rgb.r = r;
  Stripe->Plane[index].rgb.g = g;
  Stripe->Plane[index].rgb.b = b;

  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Get a RGB-Pixel. Use CLUT if PixelMode says so.
 **------------------------------------------------------------------------**/

BOOL GetRGBPixel(struct ImageData *image,LONG x,LONG y,UBYTE *r,UBYTE *g,UBYTE *b)
{
  /*
  assert(x>=0);
  assert(x<image->Width);
  assert(y>=0);
  assert(y<image->Height);
  */

  if (image->PixelMode == RGB)
  {
    struct ImageStripe *Stripe;
    int index;

    Stripe=GetImageStripe(image,y,Get);
    if (!Stripe) return FALSE;

    index=image->Width*(y - Stripe->TopPos) + x;

    *r = Stripe->Plane[index].rgb.r;
    *g = Stripe->Plane[index].rgb.g;
    *b = Stripe->Plane[index].rgb.b;
  }
  else
  {
    UWORD color;

    assert(image->Clut != NULL);

    if (!GetCLUTPixel(image,x,y,&color)) return FALSE;
    if (!GetColor(image->Clut,color,r,g,b)) return FALSE;
  }

  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Get/Set a pixel. You can't do anything useful with this except
 **  changing the orientation of the pixels.
 **------------------------------------------------------------------------**/

BOOL GetPixel(struct ImageData *image,LONG x,LONG y,ULONG *data)
{
  struct ImageStripe *Stripe;

  /*
  assert(x>=0);
  assert(x<image->Width);
  assert(y>=0);
  assert(y<image->Height);
  */

  Stripe=GetImageStripe(image,y,Get);
  if (!Stripe) return FALSE;

  *data = Stripe->Plane[image->Width*(y-Stripe->TopPos)+x].everything;

  return TRUE;
}

BOOL SetPixel(struct ImageData *image,LONG x,LONG y,ULONG data)
{
  struct ImageStripe *Stripe;

  /*
  assert(x>=0);
  assert(x<image->Width);
  assert(y>=0);
  assert(y<image->Height);
  */

  Stripe=GetImageStripe(image,y,Set);
  if (!Stripe) return FALSE;

  Stripe->Plane[image->Width*(y-Stripe->TopPos)+x].everything = data;

  return TRUE;
}



/**------------------------------------------------------------------------**
 **  Set a HAM-Byte ( additional information for HAM-mode )
 **------------------------------------------------------------------------**/

BOOL SetHAMByte(struct ImageData *image,LONG x,LONG y,UBYTE hambyte)
{
  struct ImageStripe *Stripe;

  Stripe=GetImageStripe(image,y,Set);
  if (!Stripe) return FALSE;

  Stripe->Plane[image->Width*(y - Stripe->TopPos) + x].rgb.HAM_Byte = hambyte;

  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Get a HAM-Byte
 **------------------------------------------------------------------------**/

BOOL GetHAMByte(struct ImageData *image,LONG x,LONG y,UBYTE *hambyte)
{
  struct ImageStripe *Stripe;

  Stripe=GetImageStripe(image,y,Get);
  if (!Stripe) return FALSE;

  *hambyte = Stripe->Plane[image->Width*(y - Stripe->TopPos) + x].rgb.HAM_Byte;

  return TRUE;
}





/*==============================================================================

			reading / writing rows of pixels

  ==============================================================================*/

static UBYTE LSB_to_MSB[16] =
{
  0x0001,0x0002,0x0004,0x0008,
  0x0010,0x0020,0x0040,0x0080
};

BOOL SetCLUTRow(struct ImageData *image,ULONG y,UBYTE *buffer[],UBYTE nPlanes)
{
  ULONG x;
  UBYTE color;
  UWORD color16;
  int p,i;
  int bufferindex;
  ULONG currentbit;
  ULONG **buffer32 = (ULONG **)buffer;

  if (nPlanes <= 8)
  {
      bufferindex=0;

      for (x=0;x<image->Width;)
      {
	for (i=0  , currentbit=0x80000000 ;
	     i<32 && x<image->Width;
	     i++  , currentbit >>= 1)
	{
	  color = 0;

	  for (p=0;p<nPlanes;p++)
	    if (buffer32[p][bufferindex] & currentbit) color |= LSB_to_MSB[p];

	  if (!SetCLUTPixel(image,x,y,color)) return FALSE;

	  x++;
	}

	bufferindex++;
      }

      return TRUE;
  }
  else
  {
      bufferindex=0;

      for (x=0;x<image->Width;)
      {
	for (i=0 , currentbit=0x80 ;
	     i<8 && x<image->Width;
	     i++ , currentbit >>= 1)
	{
	  color16 = 0;

	  for (p=0;p<nPlanes;p++)
	    if (buffer[p][bufferindex] & currentbit) color16 |= 1<<p;

	  if (!SetCLUTPixel(image,x,y,color16)) return FALSE;

	  x++;
	}

	bufferindex++;
      }

      return TRUE;
  }
}


BOOL GetCLUTRow(struct ImageData *image,ULONG y,UBYTE *buffer[],UBYTE nPlanes)
{
  ULONG x,xx;
  UWORD color;
  UBYTE p;
  int	bufferindex;
  UBYTE currentbit;
  union Pixel *pixrow;

  for (x=0;x<((image->Width+15) & ~15)/8;x++)
    for (p=0;p<nPlanes;p++)
      buffer[p][x]=0x0;

  pixrow = LockAndGetLine(image,y);
  if (pixrow==NULL) return FALSE;

  bufferindex=0;

  for (x=0;x<image->Width;x+=8)
  {
    for (xx=0;xx<8;xx++)
    {
      currentbit = 0x80 >> xx;

      color = pixrow->color;

      for (p=0;p<nPlanes;p++)
      {
	if (color & (1<<p)) buffer[p][bufferindex] |= currentbit;
      }

      pixrow++;
    }

    bufferindex++;
  }

  UnlockLine(image,y);

  return TRUE;
}


BOOL SetRGBRow(struct ImageData *image,ULONG y,UBYTE *buffer[])
{
  ULONG x;
  UBYTE r,g,b;
  int p;
  int bufferindex;
  int currentbit;

  for (x=0;x<image->Width;x++)
  {
    r=g=b = 0;

    bufferindex = x/8;
    currentbit	= 0x80 >> (x%8);

    for (p=0;p<8;p++)
    {
      if (buffer[p   ][bufferindex] & currentbit) r |= 1<<p;
      if (buffer[p+ 8][bufferindex] & currentbit) g |= 1<<p;
      if (buffer[p+16][bufferindex] & currentbit) b |= 1<<p;
    }

    if (!SetRGBPixel(image,x,y,r,g,b)) return FALSE;
  }

  return TRUE;
}


BOOL GetRGBRow(struct ImageData *image,ULONG y,UBYTE *buffer[])
{
  ULONG x;
  UBYTE p,xx;
  UBYTE currentbit;
  UBYTE srcmaskbit;
  union Pixel *pixrow,*pixel;

  UBYTE *rbufptr,*gbufptr,*bbufptr;

  /* Buffer löschen */

  for (x=0;x<((image->Width+15) & ~15)/8;x++)
    for (p=0;p<24;p++)
      buffer[p][x]=0x0;

  pixrow = LockAndGetLine(image,y);
  if (!pixrow) return FALSE;

  /* --- r g b -> 24 bitplanes (r: 0-7, g: 8-15, b: 16-31) --- */

  /* Zuerst alle bit0 dann bit1 usw. bis bit7 konvertieren */

  for (p=0;p<8;p++)
  {
    srcmaskbit = (1<<p);  /* Bit das ausmaskiert wird. */
    pixel=pixrow;	  /* src-Pixel-Ptr auf Anfang setzen */

    rbufptr=buffer[p   ]; /* Dest-Buffer-Ptrs auf Anfang setzen */
    gbufptr=buffer[p+ 8];
    bbufptr=buffer[p+16];

    for (x=0;x<image->Width;x+=8)
    {
      for (xx=0;xx<8;xx++)
      {
	currentbit =0x80 >> xx;

	if (pixel->rgb.r & srcmaskbit) *rbufptr |= currentbit;
	if (pixel->rgb.g & srcmaskbit) *gbufptr |= currentbit;
	if (pixel->rgb.b & srcmaskbit) *bbufptr |= currentbit;

	pixel++;
      }

      rbufptr++;
      gbufptr++;
      bbufptr++;
    }
  }



  UnlockLine(image,y);

  return TRUE;
}


BOOL GetHAMRow(struct ImageData *image,ULONG y,UBYTE *buffer[])
{
  ULONG x;
  int p;
  int bufferindex;
  int currentbit;
  UBYTE hambyte;

  for (x=0;x<((image->Width+15) & ~15)/8;x++)
    for (p=0;p<6;p++)
      buffer[p][x]=0x0;

  for (x=0;x<image->Width;x++)
  {
    bufferindex = x/8;
    currentbit	= 0x80 >> (x%8);

    if (!GetHAMByte(image,x,y,&hambyte)) return FALSE;

    for (p=0;p<6;p++)
      if (hambyte & (1<<p)) buffer[p][bufferindex] |= currentbit;
  }

  return TRUE;
}

BOOL GetHAM8Row(struct ImageData *image,ULONG y,UBYTE *buffer[])
{
  ULONG x;
  int p;
  int bufferindex;
  int currentbit;
  UBYTE hambyte;

  for (x=0;x<((image->Width+15) & ~15)/8;x++)
    for (p=0;p<8;p++)
      buffer[p][x]=0x0;

  for (x=0;x<image->Width;x++)
  {
    bufferindex = x/8;
    currentbit	= 0x80 >> (x%8);

    if (!GetHAMByte(image,x,y,&hambyte)) return FALSE;

    for (p=0;p<8;p++)
      if (hambyte & (1<<p)) buffer[p][bufferindex] |= currentbit;
  }

  return TRUE;
}


BOOL SetHAMRow(struct ImageData *image,ULONG y,UBYTE *buffer[])
{
  ULONG x;
  int bufferindex;
  int currentbit;
  UBYTE lowbits;
  UBYTE highbits;
  static UBYTE r,g,b;


  GetColor(image->Clut,0,&r,&g,&b);

  for (x=0;x<image->Width;x++)
  {
    bufferindex = x/8;
    currentbit	= 0x80 >> (x%8);

    lowbits=highbits=0;

    if (buffer[0][bufferindex] & currentbit) lowbits  |= 0x01;
    if (buffer[1][bufferindex] & currentbit) lowbits  |= 0x02;
    if (buffer[2][bufferindex] & currentbit) lowbits  |= 0x04;
    if (buffer[3][bufferindex] & currentbit) lowbits  |= 0x08;

    if (buffer[4][bufferindex] & currentbit) highbits |= 0x01;
    if (buffer[5][bufferindex] & currentbit) highbits |= 0x10;

    switch (highbits)
    {
      case 0x00: GetColor(image->Clut,lowbits,&r,&g,&b);
		 break;

      case 0x10: r = lowbits<<4;
		 break;

      case 0x11: g = lowbits<<4;
		 break;

      case 0x01: b = lowbits<<4;
		 break;
    }

    if (!SetRGBPixel(image,x,y,r,g,b)) return FALSE;
  }

  return TRUE;
}

BOOL SetHAM8Row(struct ImageData *image,ULONG y,UBYTE *buffer[])
{
  ULONG x;
  int bufferindex;
  int currentbit;
  UBYTE lowbits;
  UBYTE highbits;
  static UBYTE r,g,b;


  GetColor(image->Clut,0,&r,&g,&b);

  for (x=0;x<image->Width;x++)
  {
    bufferindex = x/8;
    currentbit	= 0x80 >> (x%8);

    lowbits=highbits=0;

    if (buffer[0][bufferindex] & currentbit) lowbits  |= 0x01;
    if (buffer[1][bufferindex] & currentbit) lowbits  |= 0x02;
    if (buffer[2][bufferindex] & currentbit) lowbits  |= 0x04;
    if (buffer[3][bufferindex] & currentbit) lowbits  |= 0x08;
    if (buffer[4][bufferindex] & currentbit) lowbits  |= 0x10;
    if (buffer[5][bufferindex] & currentbit) lowbits  |= 0x20;

    if (buffer[6][bufferindex] & currentbit) highbits |= 0x01;
    if (buffer[7][bufferindex] & currentbit) highbits |= 0x10;

    switch (highbits)
    {
      case 0x00: GetColor(image->Clut,lowbits,&r,&g,&b);
		 break;

      case 0x10: r = lowbits<<2;
		 break;

      case 0x11: g = lowbits<<2;
		 break;

      case 0x01: b = lowbits<<2;
		 break;
    }

    if (!SetRGBPixel(image,x,y,r,g,b)) return FALSE;
  }

  return TRUE;
}

BOOL SetEHBRow(struct ImageData *image,ULONG y,UBYTE *buffer[])
{
  ULONG x;
  UWORD color;
  BOOL	HalfBrite;
  static UBYTE r=0,g=0,b=0;
  int	 p;
  int	 bufferindex;
  int	 currentbit;

  for (x=0;x<image->Width;x++)
  {
    color = 0;

    bufferindex = x/8;
    currentbit	= 0x80 >> (x%8);

    for (p=0;p<5;p++)
      if (buffer[p][bufferindex] & currentbit) color |= 1<<p;

    HalfBrite  = buffer[5][bufferindex] & currentbit;

    if (!GetColor(image->Clut,color,&r,&g,&b)) return FALSE;
    if (HalfBrite) { r /= 2; g /= 2; b /= 2; }

    if (!SetRGBPixel(image,x,y,r,g,b)) return FALSE;
  }

  return TRUE;
}




/*==============================================================================

				color quantization

  ==============================================================================*/



#define CUBE_SIDE 32
#define CUBE_SIZE (CUBE_SIDE*CUBE_SIDE*CUBE_SIDE)
#define N_SHIFT   3   /* shift color value 3 times right to fit in color-cube */
#define IN_CUBE(r,g,b) ((r)*CUBE_SIDE*CUBE_SIDE + (g)*CUBE_SIDE + (b) )
#define CUBEIDX_R(i) (((i)/(CUBE_SIDE*CUBE_SIDE))<<N_SHIFT)
#define CUBEIDX_G(i) ((((i)/CUBE_SIDE)&31)<<N_SHIFT)
#define CUBEIDX_B(i) (((i)&31)<<N_SHIFT)

struct ColorInfo
{
  UBYTE r,g,b;
  ULONG count;
};

/* compare-order: R,G,B */
static int cmp_red(struct ColorInfo *c1,struct ColorInfo *c2)
{
  if (c1->r > c2->r) return  1;     if (c1->r < c2->r) return -1;
  if (c1->g > c2->g) return  1;     if (c1->g < c2->g) return -1;
  if (c1->b > c2->b) return  1;     if (c1->b < c2->b) return -1;
  return 0;
}

/* compare-order: G,B,R */
static int cmp_green(struct ColorInfo *c1,struct ColorInfo *c2)
{
  if (c1->g > c2->g) return  1;     if (c1->g < c2->g) return -1;
  if (c1->b > c2->b) return  1;     if (c1->b < c2->b) return -1;
  if (c1->r > c2->r) return  1;     if (c1->r < c2->r) return -1;
  return 0;
}

/* compare-order: B,R,G */
static int cmp_blue(struct ColorInfo *c1,struct ColorInfo *c2)
{
  if (c1->b > c2->b) return  1;     if (c1->b < c2->b) return -1;
  if (c1->r > c2->r) return  1;     if (c1->r < c2->r) return -1;
  if (c1->g > c2->g) return  1;     if (c1->g < c2->g) return -1;
  return 0;
}

/* Median is defined here as first element to the right of the real! median.
 */
static int FindMedian(struct ColorInfo *colors,int boxlength,int distance)
{
  struct ColorInfo *left,*right;
  int left_cnt,right_cnt;
  int i;

  left=colors; right=&colors[boxlength-1];
  left_cnt  = left->count;
  right_cnt = right->count;


  /* move pointer 'distance' entries away from bounds */

  assert(2*distance <= boxlength);

  for (i=0;i<distance-1;i++)
  {
    left++;  left_cnt  += left ->count;
    right--; right_cnt += right->count;
  }


  /* move pointers until they touch at the median */

  while ((left+1) < right)
  {
    if (left_cnt < right_cnt) { left++;  left_cnt  += left ->count; }
    else		      { right--; right_cnt += right->count; }
  }

  return right-colors;
}

static UBYTE GetAvrg_r(struct ColorInfo *colors,int nColors)
{
  unsigned int value=0,i;

  for (i=0;i<nColors;i++) value += colors[i].r;
  return (UBYTE)(value/nColors);
}

static UBYTE GetAvrg_g(struct ColorInfo *colors,int nColors)
{
  unsigned int value=0,i;

  for (i=0;i<nColors;i++) value += colors[i].g;
  return (UBYTE)(value/nColors);
}

static UBYTE GetAvrg_b(struct ColorInfo *colors,int nColors)
{
  unsigned int value=0,i;

  for (i=0;i<nColors;i++) value += colors[i].b;
  return (UBYTE)(value/nColors);
}

static void SplitBox(struct ColorLookupTable *clut,
		     struct ColorInfo *colors,
		     int boxlength,
		     int availEntries,
		     int firstEntry)
{
  int min_r=32,max_r=-1 , diff_r;
  int min_g=32,max_g=-1 , diff_g;
  int min_b=32,max_b=-1 , diff_b;
  int i;
  int median;

  if (availEntries==1)
  {
    int r,g,b;

    SetColor(clut,firstEntry,
      r=GetAvrg_r(colors,boxlength) +4, /* +4 : middle of range */
      g=GetAvrg_g(colors,boxlength) +4,
      b=GetAvrg_b(colors,boxlength) +4  );

//    printf(Txt(TXT_COLOR_D_DDD),firstEntry,r,g,b);
  }
  else
  {
    for (i=0;i<boxlength;i++)
    {
      min_r = min( min_r , colors[i].r );
      max_r = max( max_r , colors[i].r );
      min_g = min( min_g , colors[i].g );
      max_g = max( max_g , colors[i].g );
      min_b = min( min_b , colors[i].b );
      max_b = max( max_b , colors[i].b );
    }

    diff_r = max_r - min_r;
    diff_g = max_g - min_g;
    diff_b = max_b - min_b;

    if (diff_r >= diff_g && diff_r >= diff_b)
    {
      qsort((APTR)colors,boxlength,sizeof(struct ColorInfo),&cmp_red);
    } else
    if (diff_g >= diff_r && diff_g >= diff_b)
    {
      qsort((APTR)colors,boxlength,sizeof(struct ColorInfo),&cmp_green);
    } else
    if (diff_b >= diff_r && diff_b >= diff_g)
    {
      qsort((APTR)colors,boxlength,sizeof(struct ColorInfo),&cmp_blue);
    }

    {
      int entries_left,entries_right;

      median = FindMedian(colors,boxlength,availEntries/2);

      entries_left  = availEntries/2;
      entries_right = entries_left + (availEntries&1);

      if (availEntries&1)
      {
	if (median > boxlength/2) { entries_left++; entries_right--; }
      }

      SplitBox(clut,colors,median,entries_left,firstEntry);

      SplitBox(clut,&colors[median],boxlength-median,
	       entries_right,
	       firstEntry+entries_left);
    }
  }
}

/* New V1.8 */
static void SortCLUT(struct ColorLookupTable* clut,int dir)
{
  int nColors = GetNColors(clut);
  int i,j;

  for (i=0  ;i<nColors-1;i++)
  for (j=i+1;j<nColors  ;j++)
  {
    UBYTE r1,g1,b1;
    UBYTE r2,g2,b2;
    int bright1,bright2;

    GetColor(clut,i,&r1,&g1,&b1);
    GetColor(clut,j,&r2,&g2,&b2);

    bright1 = r1+g1+b1;
    bright2 = r2+g2+b2;

    if ( (dir ==  1 && bright2<bright1) ||
	 (dir == -1 && bright1<bright2) )
    {
      SetColor(clut,i,r2,g2,b2);
      SetColor(clut,j,r1,g1,b1);
    }
  }
}


static struct ColorLookupTable *GeneratePalette(struct ColorInfo *colors,
						UWORD nColors,
						UWORD nColorsWanted)
{
  struct ColorLookupTable *clut;

  if (!(clut=GetCLUT(nColorsWanted))) return NULL;

  if (nColors <= nColorsWanted)
  {
    int i;


    /* copy all colors to CLUT */

    ShowMessage(Txt(TXT_MORE_REGS_THAN_COLORS));

    for (i=0;i<nColors;i++)
    {
      SetColor(clut,i,colors[i].r,
		      colors[i].g,
		      colors[i].b );
    }


    /* fill with UNUSED-color to end of CLUT */

    for ( ; i<nColorsWanted ; i++)
      SetColor(clut,i,EmptyCLUTEntry_R,EmptyCLUTEntry_G,EmptyCLUTEntry_B);
  }
  else
    SplitBox(clut,colors,nColors,nColorsWanted,0);

  if (Output_SortCLUT)
  {
    SortCLUT(clut,Output_SortCLUT);
  }

  return clut;
}


static struct ColorLookupTable *QuantizeColors(UWORD nColors)
{
  ULONG *colorcube;
  UBYTE r,g,b;
  int x,y,i,k;
  int colors_used=0;
  struct ColorInfo *colors;
  struct ColorLookupTable *clut;

  ShowMessage(Txt(TXT_BUILDING_HISTOGRAMM));

  colorcube = GetMemory(32*32*32,sizeof(ULONG));
  if (!colorcube) { SetError(NO_MEM); return NULL; }


  /* scan image and count colors */

  for (y=0;y<ImageBuffer->Height;y++)
  {
    ShowProgress(y,ImageBuffer->Height-1);

    for (x=0;x<ImageBuffer->Width;x++)
    {
      if (!GetRGBPixel(ImageBuffer,x,y,&r,&g,&b)) { cu_free(colorcube);
						    return NULL;
						  }
      r >>= N_SHIFT;
      g >>= N_SHIFT;
      b >>= N_SHIFT;

      if (++colorcube[ IN_CUBE(r,g,b) ] == 1) colors_used++;
    }
  }

  {
    char buffer[50];
    sprintf(buffer,Txt(TXT_D_DIFFERENT_COLORS),colors_used);
    ShowMessage(buffer);
  }

  colors=GetMemory(colors_used , sizeof(struct ColorInfo));
  if (!colors) { cu_free(colorcube); return NULL; }

  ShowMessage(Txt(TXT_SCANNING_CUBE));

  for (i=k=0;i<CUBE_SIZE;i++)
    if (colorcube[i])
    {
      colors[k].r = CUBEIDX_R(i);
      colors[k].g = CUBEIDX_G(i);
      colors[k].b = CUBEIDX_B(i);
      colors[k].count = colorcube[i];
      k++;
    }

  cu_free(colorcube);

  clut=GeneratePalette(colors,colors_used,nColors);
  if (!clut) { cu_free(colors); return NULL; }

  cu_free(colors);

  return clut;
}


/**------------------------------------------------------------------------**
 **  Create a CLUT with at least (!) nColors for the current image.
 **------------------------------------------------------------------------**/

static BOOL CreateCLUT(UWORD nColors,struct ColorLookupTable **clut)
{
  UWORD nOldColors;
  BOOL	HaveOldColors;

  HaveOldColors = (ImageBuffer->Clut) ? TRUE : FALSE;

  if (HaveOldColors) { nOldColors=GetNColors(ImageBuffer->Clut);
		       if (nOldColors <= nColors && ImageBuffer->PixelMode==CLUT)
		       { *clut=NULL; return TRUE; }
		     }

  *clut = QuantizeColors(nColors);

  if (clut) return TRUE;
  else	    return FALSE;
}



static BOOL CreateGrayscaleCLUT(UWORD nColors,struct ColorLookupTable **clut)
{
  struct ColorLookupTable *newclut;
  int	 i;
  int	 BlackLevel;

  if (!(newclut=GetCLUT(nColors))) return FALSE;
  *clut = newclut;

  for (i=0;i<nColors;i++)
  {
    BlackLevel = (255*i)/nColors;
    SetColor(newclut,i,BlackLevel,BlackLevel,BlackLevel);
  }

  return TRUE;
}




/*==============================================================================

			      bitplane manipulation

  ==============================================================================*/


#define STANDART_ADD_RGBS \
      red   += (int)oldr; \
      green += (int)oldg; \
      blue  += (int)oldb; \
      if (red  <0) red  =0; if (red  >255) red  =255; \
      if (green<0) green=0; if (green>255) green=255; \
      if (blue <0) blue =0; if (blue >255) blue =255;




BOOL FloydDither(LONG x,LONG y,UBYTE r,UBYTE g,UBYTE b)
{
  int red_err,green_err,blue_err;
  int red_dist,green_dist,blue_dist;
  int red,green,blue;
  UBYTE oldr,oldg,oldb;
  UBYTE exact_r,exact_g,exact_b;
  int setx,sety;
  int MaxImageX,MaxImageY;

  MaxImageX=ImageBuffer->Width -1;
  MaxImageY=ImageBuffer->Height-1;


  if (!GetRGBPixel(ImageBuffer,x,y,&exact_r,&exact_g,&exact_b)) return FALSE;

  red_err   = ((int)exact_r) - ((int)r);
  green_err = ((int)exact_g) - ((int)g);
  blue_err  = ((int)exact_b) - ((int)b);


  red_dist=green_dist=blue_dist=0; /* schon verteilten Fehler rücksetzen */


  /* --- rechts --- */

  red_dist   += (red   = (red_err  *7)/16);
  green_dist += (green = (green_err*7)/16);
  blue_dist  += (blue  = (blue_err *7)/16);

  if (x < MaxImageX)
  { setx=x+1; sety=y; }
  else
  {
    setx=0; sety=y+1;	  /* wenn rechts am Rand, dann auf nächste Zeile links */
  }
  if (sety <= MaxImageY)
  {
    if (!GetRGBPixel(ImageBuffer,setx,sety,&oldr,&oldg,&oldb)) return FALSE;
    STANDART_ADD_RGBS
    if (!SetRGBPixel(ImageBuffer,setx,sety,red,green,blue)) return FALSE;
  }


  /* --- links unten --- */

  red_dist   += (red   = (red_err  *3)/16);
  green_dist += (green = (green_err*3)/16);
  blue_dist  += (blue  = (blue_err *3)/16);

  if (x > 0 && y<MaxImageY)
  {
    setx=x-1; sety=y+1;

    if (!GetRGBPixel(ImageBuffer,setx,sety,&oldr,&oldg,&oldb)) return FALSE;
    STANDART_ADD_RGBS
    if (!SetRGBPixel(ImageBuffer,setx,sety,red,green,blue)) return FALSE;
  }


  /* --- unten --- */

  red_dist   += (red   = (red_err  *5)/16);
  green_dist += (green = (green_err*5)/16);
  blue_dist  += (blue  = (blue_err *5)/16);

  if (y < MaxImageY)
  {
    setx=x; sety=y+1;

    if (!GetRGBPixel(ImageBuffer,setx,sety,&oldr,&oldg,&oldb)) return FALSE;
    STANDART_ADD_RGBS
    if (!SetRGBPixel(ImageBuffer,setx,sety,red,green,blue)) return FALSE;
  }


  /* --- rechts unten --- */

  red	= red_err  -red_dist;
  green = green_err-green_dist;
  blue	= blue_err -blue_dist;

  if (x < MaxImageX && y < MaxImageY)
  {
    setx=x+1;
    sety=y+1;

    if (!GetRGBPixel(ImageBuffer,setx,sety,&oldr,&oldg,&oldb)) return FALSE;
    STANDART_ADD_RGBS
    if (!SetRGBPixel(ImageBuffer,setx,sety,red,green,blue)) return FALSE;
  }

  return TRUE;
}




BOOL FastFloydDither(LONG x,LONG y,UBYTE r,UBYTE g,UBYTE b)
{
  int red_err,green_err,blue_err;
  int red,green,blue;
  UBYTE oldr,oldg,oldb;
  UBYTE exact_r,exact_g,exact_b;
  int setx,sety;
  int MaxImageX,MaxImageY;

  MaxImageX=ImageBuffer->Width -1;
  MaxImageY=ImageBuffer->Height-1;

  if (!GetRGBPixel(ImageBuffer,x,y,&exact_r,&exact_g,&exact_b)) return FALSE;

  red_err   = (((int)exact_r) - ((int)r))/2; /* zu verteilender Fehler je Pixel */
  green_err = (((int)exact_g) - ((int)g))/2;
  blue_err  = (((int)exact_b) - ((int)b))/2;


  if (abs(red_err)<=8 && abs(green_err)<=8 && abs(blue_err)<=8)
    return TRUE;


  /* --- rechts --- */

  red	= red_err;
  green = green_err;
  blue	= blue_err;

  if (x < MaxImageX)
  { setx=x+1; sety=y; }
  else
  {
    setx=0; sety=y+1;	  /* wenn rechts am Rand, dann auf nächste Zeile links */
  }
  if (sety <= MaxImageY)
  {
    if (!GetRGBPixel(ImageBuffer,setx,sety,&oldr,&oldg,&oldb)) return FALSE;
    STANDART_ADD_RGBS
    if (!SetRGBPixel(ImageBuffer,setx,sety,red,green,blue)) return FALSE;
  }


  /* --- unten --- */

  red	= red_err;
  green = green_err;
  blue	= blue_err;

  if (y < MaxImageY)
  {
    setx=x; sety=y+1;

    if (!GetRGBPixel(ImageBuffer,setx,sety,&oldr,&oldg,&oldb)) return FALSE;
    STANDART_ADD_RGBS
    if (!SetRGBPixel(ImageBuffer,setx,sety,red,green,blue)) return FALSE;
  }

  return TRUE;
}








#define SQUARE(x) ((x)*(x))

static UBYTE *colorcache;

static BOOL InitColorCache(void)
{
  int  i;

  colorcache=GetMemory(32*32*32,1);
  if (!colorcache) return FALSE;

  for (i=0;i<32*32*32;i++) /* well, this is not 100% clean, but this doesn't hurt */
    colorcache[i]=255;

  return TRUE;
}

static void FreeColorCache(void)
{
  cu_free(colorcache);
}


static BOOL GetBestColorMatch(ULONG x,ULONG y,
			      struct ColorLookupTable *clut,
			      UWORD *best_col)
{
  int	src_r,src_g,src_b;    /* color in the source-image		    */
  int	try_r,try_g,try_b;    /* color trying to replace the original-color */
  int	dist_r,dist_g,dist_b; /* distance in the color-cube		    */
  int	cacheindex;
  int	i,this_dist,dist;
  UBYTE r,g,b;
  int	nColors;

  nColors=GetNColors(clut);

  if (!GetRGBPixel(ImageBuffer,x,y,&r,&g,&b)) return FALSE;

  cacheindex = ((r & (31<<3))<< 7)  |
	       ((g & (31<<3))<< 2)  |
	       ((b & (31<<3))>> 3);

  src_r=r;
  src_g=g;
  src_b=b;

  if (( (*best_col) = (UWORD)colorcache[cacheindex] ) == 255)
  {
    dist = 255*255 * 3 + 1; /* longer than the diagonale**2 in the color-cube */

    for (i=0;i<nColors;i++)
    {
      if (!GetColor(clut,i,&r,&g,&b)) return FALSE;

      try_r=r; dist_r=try_r-src_r;
      this_dist = SQUARE(dist_r);
      if (this_dist >= dist) continue;

      try_g=g; dist_g=try_g-src_g;
      this_dist += SQUARE(dist_g);
      if (this_dist >= dist) continue;

      try_b=b; dist_b=try_b-src_b;
      this_dist += SQUARE(dist_b);
      if (this_dist >= dist) continue;

      dist = this_dist; *best_col = i;
    }
    colorcache[cacheindex] = (UBYTE)(*best_col);
  }

  return TRUE;
}

BOOL ConvertTo24(void);


static BOOL GenerateCLUTBitplanes(struct ColorLookupTable *clut,ULONG ditherflags)
{
  int	x,y;
  UWORD best_col;
  UBYTE r,g,b;	  /* for the dithering */


  /* floyd-dithering needs a 24 bit-representation of the image */

  if (ditherflags & (GFXMOD_FLOYD | GFXMOD_FASTFLOYD))
    if (!ConvertTo24()) return FALSE;


  ShowMessage(Txt(TXT_BITMAP_TO_CLUT));


  if (!InitColorCache()) return FALSE;

  /* search colors which fit best */

  for (y=0;y<ImageBuffer->Height;y++)
  {
    ShowProgress(y,ImageBuffer->Height-1);

      for (x=0;x<ImageBuffer->Width;x++)
      {
	if (!GetBestColorMatch(x,y,clut,&best_col))  { FreeColorCache();
						       return FALSE;
						     }
	if (ditherflags)
	{
	  if (!GetColor(clut,best_col,&r,&g,&b)) return FALSE;

	  if (ditherflags & GFXMOD_FLOYD)
	    FloydDither(x,y,r,g,b);
	  else
	  if (ditherflags & GFXMOD_FASTFLOYD)
	    FastFloydDither(x,y,r,g,b);
	}

	if (!SetCLUTPixel(ImageBuffer,x,y,best_col)) { FreeColorCache();
						       return FALSE; }
      }
  }

  FreeColorCache();

  SetBufferMode( ImageBuffer , CLUT );

  return TRUE;
}


static BOOL GenerateHAMBitplanes(struct ColorLookupTable *clut,ULONG ditherflags,BOOL ham8)
{
  int	x,y;
  UWORD best_col;
  UBYTE r,g,b;	  /* (input buffer) */
  int	last_r=0,last_g=0,last_b=0;
  int	src_r,src_g,src_b;
  int	col_r,col_g,col_b;
  int	dist,this_dist;
  int	dist_r,dist_g,dist_b; /* distances */
  UBYTE HAM_Byte;

  ShowMessage(Txt(TXT_BITMAP_TO_HAM));


  /* floyd-dithering needs a 24 bit-representation of the image */

  if (ditherflags & (GFXMOD_FLOYD | GFXMOD_FASTFLOYD))
    if (!ConvertTo24()) return FALSE;

  if (!InitColorCache()) return FALSE;


  /* search colors which fits best */

  for (y=0;y<ImageBuffer->Height;y++)
  {
    ShowProgress(y,ImageBuffer->Height-1);

    for (x=0;x<ImageBuffer->Width;x++)
    {
      if (!GetBestColorMatch(x,y,clut,&best_col))  { FreeColorCache();
						     return FALSE;
						   }

      /* check if we can get a better match using the HAM feature */

      if (!GetColor(clut,best_col,&r,&g,&b)) return FALSE;
      col_r = r; col_g = g; col_b = b;
      if (!GetRGBPixel(ImageBuffer,x,y,&r,&g,&b)) return FALSE;
      src_r = r; src_g = g; src_b = b;

      HAM_Byte = 0x00 + best_col;

      if (x>0)
      {
	dist_r = col_r - src_r;
	dist_g = col_g - src_g;
	dist_b = col_b - src_b;

	dist = SQUARE(dist_r) + SQUARE(dist_g) + SQUARE(dist_b);

	this_dist = SQUARE(src_g - last_g) + SQUARE(src_b - last_b) ;
	if ( this_dist < dist )
	{
	  col_r = src_r;
	  col_g = last_g;
	  col_b = last_b;
	  dist = this_dist;

	  if (ham8) HAM_Byte = 0x80 + (col_r>>2);
	  else	    HAM_Byte = 0x20 + (col_r>>4);
	}

	this_dist = SQUARE(src_r - last_r) + SQUARE(src_b - last_b) ;
	if ( this_dist < dist )
	{
	  col_r = last_r;
	  col_g = src_g;
	  col_b = last_b;
	  dist = this_dist;

	  if (ham8) HAM_Byte = 0xc0 + (col_g>>2);
	  else	    HAM_Byte = 0x30 + (col_g>>4);
	}

	this_dist = SQUARE(src_r - last_r) + SQUARE(src_g - last_g) ;
	if ( this_dist < dist )
	{
	  col_r = last_r;
	  col_g = last_g;
	  col_b = src_b;
	  dist = this_dist;

	  if (ham8) HAM_Byte = 0x40 + (col_b>>2);
	  else	    HAM_Byte = 0x10 + (col_b>>4);
	}
      }

      if (ditherflags)
      {

	if (ditherflags & GFXMOD_FLOYD)
	  FloydDither(x,y,col_r,col_g,col_b);
	else
	if (ditherflags & GFXMOD_FASTFLOYD)
	  FastFloydDither(x,y,col_r,col_g,col_b);
      }

      if (!SetRGBPixel(ImageBuffer,x,y,col_r,col_g,col_b)) { FreeColorCache();
							     return FALSE; }
      if (!SetHAMByte (ImageBuffer,x,y,HAM_Byte)) { FreeColorCache();
						    return FALSE; }
      last_r = col_r;
      last_g = col_g;
      last_b = col_b;
    }
  }

  FreeColorCache();

  SetBufferMode( ImageBuffer , RGB );

  return TRUE;
}




/**------------------------------------------------------------------------**
 **  Convert an image to 24 bit representation. Do nothing if it is
 **  already a 24 bit-image.
 **------------------------------------------------------------------------**/

BOOL ConvertTo24(void)
{
  int x,y;
  UWORD color;
  UBYTE r,g,b;

  if (ImageBuffer->PixelMode == RGB) return TRUE;

  assert(ImageBuffer->PixelMode == CLUT);

  ShowMessage(Txt(TXT_CLUT_TO_24BIT));

  for (y=0;y<GetImageHeight(ImageBuffer);y++)
  {
    ShowProgress(y,ImageBuffer->Height-1);

    for (x=0;x<ImageBuffer->Width;x++)
    {
      if (!GetCLUTPixel(ImageBuffer,x,y, &color)) return FALSE;
      if (!GetColor(ImageBuffer->Clut,color, &r,&g,&b)) return FALSE;
      if (!SetRGBPixel(ImageBuffer,x,y,r,g,b)) return FALSE;
    }
  }

  SetBufferMode(ImageBuffer, RGB );
  SetBackgroundColor(ImageBuffer,-1);

  RemoveCLUT(ImageBuffer);

  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Convert an image to the representation that the user has specified.
 **------------------------------------------------------------------------**/

BOOL ConvertToMode(UWORD nColors,ULONG flags)
{
  struct ColorLookupTable *clut;
  long CAMG=0;

  if (!CreateSpecialEffects(flags)) return FALSE;

  if (Output_SortCLUT)
    if (!ConvertTo24()) return FALSE;

  if (flags & GFXMOD_CLUT)
  {
    if (flags & GFXMOD_GRAYSCALE)
    { if (!CreateGrayscaleCLUT(nColors,&clut)) return FALSE; }
    else
    { if (!CreateCLUT(nColors,&clut)) return FALSE; }

    if (clut)
    {
      /* replace old clut with new one */

      if (!GenerateCLUTBitplanes(clut , flags & DITHER_FLAGS)) return FALSE;
      RemoveCLUT(ImageBuffer);
      AttachCLUT(clut,ImageBuffer);
    }

    GetCAMG(ImageBuffer,&CAMG);
    SetCAMG(ImageBuffer,CAMG & ~(HAM_KEY | EXTRAHALFBRITE_KEY));
  }
  else
  if (flags & GFXMOD_HAM)
  {
    if (!CreateCLUT(16,&clut))    return FALSE;

    if (clut)
    {
      if (!GenerateHAMBitplanes(clut , flags & DITHER_FLAGS,FALSE)) return FALSE;
      RemoveCLUT(ImageBuffer);
      AttachCLUT(clut,ImageBuffer);
    }

    GetCAMG(ImageBuffer,&CAMG);
    SetCAMG(ImageBuffer, ( (CAMG & ~EXTRAHALFBRITE_KEY) | HAM_KEY ));
  }
  else
  if (flags & GFXMOD_HAM8)
  {
    if (!CreateCLUT(64,&clut))    return FALSE;

    if (clut)
    {
      if (!GenerateHAMBitplanes(clut , flags & DITHER_FLAGS,TRUE)) return FALSE;
      RemoveCLUT(ImageBuffer);
      AttachCLUT(clut,ImageBuffer);
    }

    GetCAMG(ImageBuffer,&CAMG);
    SetCAMG(ImageBuffer, ( (CAMG & ~EXTRAHALFBRITE_KEY) | HAM_KEY ));
  }
  else
  if (flags & GFXMOD_EHB)
  {
//    if (!ConvertTo24())  return FALSE;
//    if (!CreateCLUT(32)) return FALSE;
  }
  else
  if (flags & GFXMOD_24BIT)
  {
    if (!ConvertTo24()) return FALSE;
  }

  return TRUE;
}


/**------------------------------------------------------------------------**
 **  Lock a line in a stripe (from being moved to disk) and return
 **  a pointer to the line to allow direct manipulation of the data.
 **------------------------------------------------------------------------**/
union Pixel *LockAndGetLine(struct ImageData *image,int y)
{
  struct ImageStripe *Stripe;

  Stripe=GetImageStripe(image,y,Get);
  if (!Stripe) return NULL;

  Stripe->LockCnt++;

  return &Stripe->Plane[image->Width*(y-Stripe->TopPos)];
}


/**------------------------------------------------------------------------**
 **  Unlock a line in a stripe.
 **------------------------------------------------------------------------**/
void UnlockLine(struct ImageData *image,int y)
{
  struct ImageStripe *Stripe;

  Stripe=GetImageStripe(image,y,Get);
  assert(Stripe != NULL);

  assert(Stripe->LockCnt > 0);

  Stripe->LockCnt--;
}



/*==============================================================================

				support functions

  ==============================================================================*/


void FreeLineBuffer(UWORD nPlanes,UBYTE *buffer[])
{
  int i;

  for (i=0;i<nPlanes;i++)
  {
    cu_free(buffer[i]);
  }
}

BOOL GetLineBuffer(ULONG width,UWORD nPlanes,UBYTE *buffer[])
{
  int i;

  for (i=0;i<nPlanes;i++)
  {
    buffer[i]=GetMemory(width/8 , 1);
    if (!buffer) FreeLineBuffer(i /* +1 -1 */ ,buffer);
  }

  return TRUE;
}


void SetBackgroundColor(struct ImageData* image,short color)
{
  image->backgroundcolor = color;
}


// -1 for no backgroundcolor

short AskBackgroundColor(struct ImageData* image)
{
  return image->backgroundcolor;
}

