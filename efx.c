/*******************************************************************************
 *
 * Copyright (C) 1995  Dirk Farin  <dirk.farin@gmail.com>
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

static BOOL FlipX(struct ImageData *image)
{
  int width,height;
  int x,y;
  ULONG left_pixel , right_pixel;

  width =GetImageWidth (image);
  height=GetImageHeight(image);

  ShowMessage(Txt(TXT_DOFLIP_H));

  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width/2;x++)
    {
      if (!GetPixel(image,      x  ,y,& left_pixel)) return FALSE;
      if (!GetPixel(image,width-x-1,y,&right_pixel)) return FALSE;
      if (!SetPixel(image,      x  ,y, right_pixel)) return FALSE;
      if (!SetPixel(image,width-x-1,y,  left_pixel)) return FALSE;
    }
  }

  return TRUE;
}

static BOOL FlipY(struct ImageData *image)
{
  int width,height;
  int x,y;
  ULONG top_pixel , bottom_pixel;

  width =GetImageWidth (image);
  height=GetImageHeight(image);

  ShowMessage(Txt(TXT_DOFLIP_V));

  for (y=0;y<height/2;y++)
  {
    ShowProgress(y,height/2-1);

    for (x=0;x<width;x++)
    {
      if (!GetPixel(image,x,       y  ,&   top_pixel)) return FALSE;
      if (!GetPixel(image,x,height-y-1,&bottom_pixel)) return FALSE;
      if (!SetPixel(image,x,       y  , bottom_pixel)) return FALSE;
      if (!SetPixel(image,x,height-y-1,    top_pixel)) return FALSE;
    }
  }

  return TRUE;
}

static BOOL ToBW(struct ImageData *image)
{
  int width,height;
  int x,y;
  UBYTE brightness;
  UBYTE r,g,b;

  if (!ConvertTo24()) return FALSE;

  width =GetImageWidth (image);
  height=GetImageHeight(image);

  ShowMessage(Txt(TXT_TO_BW));

  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      if (!GetRGBPixel(image,x,y,&r,&g,&b)) return FALSE;
      brightness = (r+g+b)/3;
      if (!SetRGBPixel(image,x,y,brightness,brightness,brightness)) return FALSE;
    }
  }

  return TRUE;
}

static BOOL FilterColors(struct ImageData *image,ULONG flags)
{
  int width,height;
  int x,y;
  UBYTE r,g,b;
  BOOL  no_r,no_g,no_b;

  no_r = flags & GFXMOD_NO_R;
  no_g = flags & GFXMOD_NO_G;
  no_b = flags & GFXMOD_NO_B;

  width =GetImageWidth (image);
  height=GetImageHeight(image);

  if (!ConvertTo24()) return FALSE;

  ShowMessage(Txt(TXT_FILTERING));

  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      if (!GetRGBPixel(image,x,y,&r,&g,&b)) return FALSE;
      if (!SetRGBPixel(image,x,y, no_r ? 0 : r,
                                  no_g ? 0 : g,
                                  no_b ? 0 : b  )) return FALSE;
    }
  }

  return TRUE;
}


static BOOL Inverse(struct ImageData *image)
{
  int width,height;
  int x,y;
  UBYTE r,g,b;

  width =GetImageWidth (image);
  height=GetImageHeight(image);

  if (!ConvertTo24()) return FALSE;

  ShowMessage(Txt(TXT_INVERTING));

  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      if (!GetRGBPixel(image,x,y,&r,&g,&b))          return FALSE;
      if (!SetRGBPixel(image,x,y,255-r,255-g,255-b)) return FALSE;
    }
  }

  return TRUE;
}

static BOOL ChangeBriCon(struct ImageData *image,int brightness,int contrast)
{
  int width,height;
  int x,y;
  UBYTE r,g,b;
  int   big_r,big_g,big_b;

  if (brightness==0 && contrast==100) return TRUE; /* keep image as is */

  width =GetImageWidth (image);
  height=GetImageHeight(image);

  if (!ConvertTo24()) return FALSE;

  ShowMessage(Txt(TXT_BRI_CON));

  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      if (!GetRGBPixel(image,x,y,&r,&g,&b)) return FALSE;

      big_r=r; big_g=g; big_b=b;

      big_r=((big_r+brightness)*contrast)/100;
      big_g=((big_g+brightness)*contrast)/100;
      big_b=((big_b+brightness)*contrast)/100;

      big_r=max(min(big_r,255),0);
      big_g=max(min(big_g,255),0);
      big_b=max(min(big_b,255),0);

      if (!SetRGBPixel(image,x,y,big_r,big_g,big_b)) return FALSE;
    }
  }

  return TRUE;
}


static BOOL Resize(struct ImageData *image,int width,int height)
{
  int old_width,old_height;
  int x,y;
  UBYTE r,g,b;
  int   big_r,big_g,big_b;
  struct ImageData *newimage;
  int   last_forget_line = -1;
  int   old_x,old_y;
  int i;

  old_width =GetImageWidth (image);
  old_height=GetImageHeight(image);

  ShowMessage(Txt(TXT_RESIZING));

  newimage=GetBuffer(width,height);
  if (!newimage) return FALSE;

  SetBufferMode(newimage,RGB);
  SetBackgroundColor( newimage,AskBackgroundColor(image) );


  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);
    old_y = (y*old_height)/height;

    for (x=0;x<width;x++)
    {
      old_x = (x*old_width)/width;

      if (!GetRGBPixel(image,old_x,old_y,
                             &r,&g,&b)) return FALSE;

      big_r=r; big_g=g; big_b=b;

      if (!SetRGBPixel(newimage,x,y,big_r,big_g,big_b)) return FALSE;
    }

    for (i=last_forget_line+1 ; i<old_y ; i++)
    { ForgetLine(image,i); }
    last_forget_line=old_y-1;
  }


  FreeBuffer(image);
  SetDefaultBuffer(newimage);
  return TRUE;
}


static BOOL ResizeInterpol(struct ImageData *image,int width,int height)
{
  int old_width,old_height;
  int x,y;
  int xx,yy;
  UBYTE r[2][2],g[2][2],b[2][2];
  int   top_r,top_g,top_b;
  UBYTE bot_r,bot_g,bot_b;

  int   big_r,big_g,big_b;
  struct ImageData *newimage;

  old_width =GetImageWidth (image);
  old_height=GetImageHeight(image);

  if (!ConvertTo24()) return FALSE;

  ShowMessage(Txt(TXT_INTERPOL_RESIZE));

  newimage=GetBuffer(width,height);
  if (!newimage) return FALSE;

  SetBufferMode(newimage,RGB);
  SetBackgroundColor( newimage,AskBackgroundColor(image) );


  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      /* get left top pixel */

      if (!GetRGBPixel(image,xx=(x*old_width )/width,
                             yy=(y*old_height)/height,
                             &r[0][0],&g[0][0],&b[0][0])) return FALSE;

      /* calculate top pixel average */

      if ((x*old_width) % width && (xx+1)<old_width)
      {
        int offset = (x*old_width) % width;

        if (!GetRGBPixel(image,xx+1,
                               yy,
                               &r[1][0],&g[1][0],&b[1][0])) return FALSE;

        top_r = (((width-offset)*r[0][0]) + offset*r[1][0])/width;
        top_g = (((width-offset)*g[0][0]) + offset*g[1][0])/width;
        top_b = (((width-offset)*b[0][0]) + offset*b[1][0])/width;
      }
      else
      {
        top_r = r[0][0];
        top_g = g[0][0];
        top_b = b[0][0];
      }

      /* calculate total pixel average */

      if ((y*old_height) % height && (yy+1)<old_height)
      {
        int offset = (y*old_height) % height;

        /* get bottom left pixel */

        if (!GetRGBPixel(image,xx,
                               yy+1,
                               &r[0][1],&g[0][1],&b[0][1])) return FALSE;

        /* calculate bottom pixel average */

        if ((x*old_width) % width && (xx+1)<old_width)
        {
          int offset = (x*old_width) % width;

          if (!GetRGBPixel(image,xx+1,
                                 yy+1,
                                 &r[1][1],&g[1][1],&b[1][1])) return FALSE;

          bot_r = (((width-offset)*r[0][1]) + offset*r[1][1]) / width;
          bot_g = (((width-offset)*g[0][1]) + offset*g[1][1]) / width;
          bot_b = (((width-offset)*b[0][1]) + offset*b[1][1]) / width;
        }
        else
        {
          bot_r = r[0][1];
          bot_g = g[0][1];
          bot_b = b[0][1];
        }

        big_r = ((height-offset)*top_r + offset*bot_r) / height;
        big_g = ((height-offset)*top_g + offset*bot_g) / height;
        big_b = ((height-offset)*top_b + offset*bot_b) / height;
      }
      else
      {
        big_r=top_r;
        big_g=top_g;
        big_b=top_b;
      }

      if (!SetRGBPixel(newimage,x,y,big_r,big_g,big_b)) return FALSE;
    }
  }


  FreeBuffer(image);
  SetDefaultBuffer(newimage);
  return TRUE;
}


static BOOL RotateLeft(struct ImageData *image)
{
  int width,height;
  int new_width,new_height;
  int x,y;
  ULONG pixel;
  struct ImageData *newimage;

  width =GetImageWidth (image);
  height=GetImageHeight(image);

  new_width  = height;
  new_height = width;

  ShowMessage(Txt(TXT_ROTATING_90));

  newimage=GetBuffer(new_width,new_height);
  if (!newimage) return FALSE;

  SetBufferMode(newimage,RGB);
  SetBackgroundColor( newimage,AskBackgroundColor(image) );


  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      if (!GetPixel(image   ,x,             y, &pixel)) return FALSE;
      if (!SetPixel(newimage,y,new_height-x-1,  pixel)) return FALSE;
    }

    ForgetLine(image,y);
  }

  FreeBuffer(image);
  SetDefaultBuffer(newimage);
  return TRUE;
}


static BOOL RotateRight(struct ImageData *image)
{
  int width,height;
  int new_width,new_height;
  int x,y;
  ULONG pixel;
  struct ImageData *newimage;

  width =GetImageWidth (image);
  height=GetImageHeight(image);

  new_width  = height;
  new_height = width;


  ShowMessage(Txt(TXT_ROTATING_M90));

  newimage=GetBuffer(new_width,new_height);
  if (!newimage) return FALSE;

  SetBufferMode(newimage,RGB);
  SetBackgroundColor( newimage,AskBackgroundColor(image) );


  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      if (!GetPixel(image   ,          x,  y, &pixel)) return FALSE;
      if (!SetPixel(newimage,new_width-y-1,x,  pixel)) return FALSE;
    }

    ForgetLine(image,y);
  }

  FreeBuffer(image);
  SetDefaultBuffer(newimage);
  return TRUE;
}


BOOL Crop(struct ImageData* image,ULONG x1,ULONG y1,ULONG x2,ULONG y2)
{
  int width,height;

  int old_width,old_height;
  struct ImageData *newimage;
  int x,y;

  UBYTE r,g,b;
  int i;

  width  = x2-x1+1;
  height = y2-y1+1;

  old_width =GetImageWidth (image);
  old_height=GetImageHeight(image);

  ShowMessage("cropping image");

  newimage=GetBuffer(width,height);
  if (!newimage) return FALSE;

  SetBufferMode(newimage,RGB);
  SetBackgroundColor( newimage,AskBackgroundColor(image) );


  for (i=0;i<x1;i++) { ForgetLine(image,i); }

  for (y=0;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      if (!GetRGBPixel(image,x+x1,y+y1,
                             &r,&g,&b)) return FALSE;

      if (!SetRGBPixel(newimage,x,y,r,g,b)) return FALSE;
    }

    ForgetLine(image,y+y1);
  }

  FreeBuffer(image);
  SetDefaultBuffer(newimage);
  return TRUE;
}


BOOL CenterBox(struct ImageData* image,ULONG width,ULONG height,UBYTE border_r,UBYTE border_g,UBYTE border_b)
{
  int old_width,old_height;
  struct ImageData *newimage;
  int x,y;
  UBYTE r,g,b;
  int xoffs,yoffs;

  old_width =GetImageWidth (image);
  old_height=GetImageHeight(image);

  if (width  < old_width ||
      height < old_height)
  {
    ShowMessage("CENTERBOX-rectangle too small");
    return FALSE;
  }

  xoffs = (width -old_width )/2;
  yoffs = (height-old_height)/2;

  ShowMessage("applying CENTERBOX");

  newimage=GetBuffer(width,height);
  if (!newimage) return FALSE;

  SetBufferMode(newimage,RGB);
  SetBackgroundColor( newimage,AskBackgroundColor(image) );


  for (y=0;y<yoffs;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      if (!SetRGBPixel(newimage,x,y,border_r,border_g,border_b)) return FALSE;
    }
  }

  for (;y<yoffs+old_height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<xoffs;x++)
    {
      if (!SetRGBPixel(newimage,x,y,border_r,border_g,border_b)) return FALSE;
    }

    for (;x<xoffs+old_width;x++)
    {
      if (!GetRGBPixel(image,x-xoffs,y-yoffs,&r,&g,&b)) return FALSE;
      if (!SetRGBPixel(newimage,x,y,r,g,b)) return FALSE;
    }

    for (;x<width;x++)
    {
      if (!SetRGBPixel(newimage,x,y,border_r,border_g,border_b)) return FALSE;
    }

//    ForgetLine(image,y-yoffs);
  }

  for (;y<height;y++)
  {
    ShowProgress(y,height-1);

    for (x=0;x<width;x++)
    {
      if (!SetRGBPixel(newimage,x,y,border_r,border_g,border_b)) return FALSE;
    }
  }


  FreeBuffer(image);
  SetDefaultBuffer(newimage);

  return TRUE;
}


BOOL CreateSpecialEffects(ULONG flags)
{
  /* New V1.8 */

  if (Output_DoCrop)
  {
    if (!Crop(GetDefaultBuffer(),Output_Crop_x1,Output_Crop_y1,Output_Crop_x2,Output_Crop_y2)) return FALSE;
  }


  {
  ULONG ImageWidth  = GetImageWidth (GetDefaultBuffer());
  ULONG ImageHeight = GetImageHeight(GetDefaultBuffer());


  if (Output_DoResize && Output_ResizeFactor!=1.0)
  {
    if (Output_ResizeH)  ImageWidth  = (ULONG)(((double)ImageWidth )*Output_ResizeFactor);
    if (Output_ResizeV)  ImageHeight = (ULONG)(((double)ImageHeight)*Output_ResizeFactor);
  }


  if (Output_DoSize && (Output_NewWidth !=ImageWidth ||
                        Output_NewHeight!=ImageHeight ))
  {
    ImageWidth  = Output_NewWidth;
    ImageHeight = Output_NewHeight;
  }



  if (Output_DoBoxfit)
  {
    if (Output_Boxfit_MayEnlarge ||
        ImageWidth  > Output_BoxfitWidth ||
        ImageHeight > Output_BoxfitHeight)
    {
      // Breite genau probieren:

      ULONG newwidth  = Output_BoxfitWidth;
      ULONG newheight = ImageHeight * newwidth / ImageWidth;


      // Wenns nicht paßt, dann andersherum

      if (newheight > Output_BoxfitHeight)
      {
        newheight = Output_BoxfitHeight;
        newwidth  = ImageWidth * newheight / ImageHeight;
      }

      ImageWidth  = newwidth;
      ImageHeight = newheight;
    }
  }


  if (ImageWidth  != GetImageWidth (GetDefaultBuffer()) ||
      ImageHeight != GetImageHeight(GetDefaultBuffer()) )
  {
    if (ImageWidth  < 1) ImageWidth  = 1;
    if (ImageHeight < 1) ImageHeight = 1;

    if (Output_Interpolated)
    { if (!ResizeInterpol(GetDefaultBuffer(),ImageWidth,ImageHeight)) return FALSE; }
    else
    { if (!Resize        (GetDefaultBuffer(),ImageWidth,ImageHeight)) return FALSE; }
  }
  }



  if (Output_DoCenterBox)
  {
    if (!CenterBox(GetDefaultBuffer(),Output_CenterBox_Width,Output_CenterBox_Height,
                                      Output_CenterBox_R,
                                      Output_CenterBox_G,
                                      Output_CenterBox_B)) return FALSE;
  }




  if (flags & GFXMOD_FLIPX )    if (!FlipX      (GetDefaultBuffer())) return FALSE;
  if (flags & GFXMOD_FLIPY )    if (!FlipY      (GetDefaultBuffer())) return FALSE;
  if (flags & GFXMOD_ROTATE_L ) if (!RotateLeft (GetDefaultBuffer())) return FALSE;
  if (flags & GFXMOD_ROTATE_R ) if (!RotateRight(GetDefaultBuffer())) return FALSE;
  if (flags & GFXMOD_BW    )    if (!ToBW       (GetDefaultBuffer())) return FALSE;
  if (flags & GFXMOD_INVERS)    if (!Inverse    (GetDefaultBuffer())) return FALSE;

  if (flags & ( GFXMOD_NO_R | GFXMOD_NO_G | GFXMOD_NO_B ) )
                            if (!FilterColors(GetDefaultBuffer(),flags)) return FALSE;

  if (!ChangeBriCon(GetDefaultBuffer(),Output_Brightness,Output_Contrast)) return FALSE;

  return TRUE;
}

