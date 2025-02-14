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

union Pixel *LockAndGetLine(struct ImageData *image,int y);
void UnlockLine(struct ImageData *image,int y);

/* Prototypes for functions defined in gfxbuffer.c */
void InitGfxBuffer(void);
void ForgetLine(struct ImageData *image,
		ULONG y);
void MoveStripesToFastMem(void);
struct ColorLookupTable *GetCLUT(UWORD nColors);
void FreeCLUT(struct ColorLookupTable *clut);
void RemoveCLUT(struct ImageData *image);
void AttachCLUT(struct ColorLookupTable *clut,
		struct ImageData *image);
UWORD GetNColors(struct ColorLookupTable *clut);
UWORD GetImageNColors(struct ImageData *image);
void SetColor(struct ColorLookupTable *clut,
	      UWORD nr,
	      UBYTE r,
	      UBYTE g,
	      UBYTE b);
BOOL GetColor(struct ColorLookupTable *clut,
	      UWORD nr,
	      UBYTE *r,
	      UBYTE *g,
	      UBYTE *b);
BOOL GetImageColor(struct ImageData *image,
		   UWORD nr,
		   UBYTE *r,
		   UBYTE *g,
		   UBYTE *b);
struct ImageData *GetBuffer(ULONG width,
			    ULONG height);
void FreeBuffer(struct ImageData *image);
void SetCAMG(struct ImageData *image,
	     LONG CAMG);
BOOL GetCAMG(struct ImageData *image,
	     LONG *CAMG);
void SetBufferMode(struct ImageData *image,
		   enum PixelMode mode);
void SetDefaultBuffer(struct ImageData *image);
struct ImageData *GetDefaultBuffer(void);
UWORD GetImageWidth(struct ImageData *image);
UWORD GetImageHeight(struct ImageData *image);
BOOL SetCLUTPixel(struct ImageData *image,
		  LONG x,
		  LONG y,
		  UWORD color);
BOOL GetCLUTPixel(struct ImageData *image,
		  LONG x,
		  LONG y,
		  UWORD *color);
BOOL SetRGBPixel(struct ImageData *image,
		 LONG x,
		 LONG y,
		 UBYTE r,
		 UBYTE g,
		 UBYTE b);
BOOL GetRGBPixel(struct ImageData *image,
		 LONG x,
		 LONG y,
		 UBYTE *r,
		 UBYTE *g,
		 UBYTE *b);
BOOL GetPixel(struct ImageData *image,
	      LONG x,
	      LONG y,
	      ULONG *data);
BOOL SetPixel(struct ImageData *image,
	      LONG x,
	      LONG y,
	      ULONG data);
BOOL SetHAMByte(struct ImageData *image,
		LONG x,
		LONG y,
		UBYTE hambyte);
BOOL GetHAMByte(struct ImageData *image,
		LONG x,
		LONG y,
		UBYTE *hambyte);
BOOL SetCLUTRow(struct ImageData *image,
		ULONG y,
		UBYTE **buffer,
		UBYTE nPlanes);
BOOL GetCLUTRow(struct ImageData *image,
		ULONG y,
		UBYTE **buffer,
		UBYTE nPlanes);
BOOL SetRGBRow(struct ImageData *image,
	       ULONG y,
	       UBYTE **buffer);
BOOL GetRGBRow(struct ImageData *image,
	       ULONG y,
	       UBYTE **buffer);
BOOL GetHAMRow(struct ImageData *image,
	       ULONG y,
	       UBYTE **buffer);
BOOL GetHAM8Row(struct ImageData *image,
		ULONG y,
		UBYTE **buffer);
BOOL SetHAMRow(struct ImageData *image,
	       ULONG y,
	       UBYTE **buffer);
BOOL SetHAM8Row(struct ImageData *image,
		ULONG y,
		UBYTE **buffer);
BOOL SetEHBRow(struct ImageData *image,
	       ULONG y,
	       UBYTE **buffer);
BOOL ConvertTo24(void);
BOOL ConvertToMode(UWORD nColors,
		   ULONG flags);
void FreeLineBuffer(UWORD nPlanes,
		    UBYTE **buffer);
BOOL GetLineBuffer(ULONG width,
		   UWORD nPlanes,
		   UBYTE **buffer);
void  SetBackgroundColor(struct ImageData*,short);
short AskBackgroundColor(struct ImageData*);

