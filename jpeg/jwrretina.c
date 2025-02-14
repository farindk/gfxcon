/*
 * jwrretina.c
 *
 * by Lutz Vieweg 1993
 *
 * based on code by
 *
 * Thomas G. Lane.
 */

#include "jinclude.h"

#undef GLOBAL

#include <inline/exec.h>
#include <exec/libraries.h>
#include <dos/dos.h>

#define C_ONLY
#include <inline/retina.h>


/*
 * Haven't yet got around to making this work with text-format output,
 * hence cannot handle pixels wider than 8 bits.
 */

#ifndef EIGHT_BIT_SAMPLES
  Sorry, this code only copes with 8-bit JSAMPLEs. /* deliberate syntax err */
#endif


/*
 * Open the retina screen
 */

struct Library * RetinaBase;
static struct RetinaScreen * rscr = 0;
static long akty;

METHODDEF void
output_init (decompress_info_ptr cinfo)
{
  
  if (0 == (RetinaBase = OpenLibrary( "retina.library", 0 ))) ERREXIT(cinfo->emethods, "unable to open retina.library");
  
  
  if (cinfo->out_color_space == CS_GRAYSCALE) {

    /* GrayScale screen oeffnen... */
    
    rscr = Retina_OpenScreen(cinfo->image_width, cinfo->image_height, MID_DEFAULT_08, RSFF_AUTOADJUST, 0);
    if (rscr == 0) ERREXIT(cinfo->emethods, "unable to open 8-bit screen on Retina board");
    Retina_LoadPalette(rscr, 0, 256, 0);
    
  } else if (cinfo->out_color_space == CS_RGB) {
  
    if (cinfo->quantize_colors == TRUE) {
      rscr = Retina_OpenScreen(cinfo->image_width, cinfo->image_height, MID_DEFAULT_08, RSFF_AUTOADJUST, 0);
      if (rscr == 0) ERREXIT(cinfo->emethods, "unable to open 8-bit screen on Retina board");
    }
    else {
      /* 24-bit screen oeffnen */
      rscr = Retina_OpenScreen(cinfo->image_width, cinfo->image_height, MID_DEFAULT_24, RSFF_AUTOADJUST, 0);
      if (rscr == 0) ERREXIT(cinfo->emethods, "unable to open 24-bit screen on Retina board");
    }
    
  } else {
    ERREXIT(cinfo->emethods, "output format has to be 8-bit or 24-bit color");
  }
  
  akty = 0;
}


/*
 * Write some pixel data.
 */

METHODDEF void
put_pixel_rows (decompress_info_ptr cinfo, int num_rows,
		JSAMPIMAGE pixel_data)
{
  JSAMPROW ptr0, ptr1, ptr2;
  long width;
  int row;
  long col;
  unsigned char * retmem;
  long rmembase = rscr->rs_BitMap + akty*rscr->rs_Modulo;
  
  width = cinfo->image_width;
  if (width < rscr->rs_Width) width = rscr->rs_Width;
  
  
  for (row = 0; row < num_rows; row++) {
	
	if (akty < rscr->rs_Height) {
		ptr0 = pixel_data[0][row];
		ptr1 = pixel_data[1][row];
		ptr2 = pixel_data[2][row];
		
		{
			Retina_OwnRetina();
			
			retmem = Retina_SetSegmentPtr( rmembase );
			
			for (col = width; col > 0; col--) {
				
				*retmem++ = *ptr2++;
				*retmem++ = *ptr1++;
				*retmem++ = *ptr0++;
				
			}
			
			Retina_DisownRetina();
			rmembase += rscr->rs_Modulo;
		}
		
	}
	
	akty++;
	
  }
}

METHODDEF void
put_gray_rows (decompress_info_ptr cinfo, int num_rows,
	       JSAMPIMAGE pixel_data)
{
  JSAMPROW ptr0;
  long width = cinfo->image_width;
  int row;
  long col;
  unsigned char * retmem;
	long rmembase = rscr->rs_BitMap + akty*rscr->rs_Modulo;
  width = (width <= rscr->rs_Width)? width : rscr->rs_Width;
  
  
  for (row = 0; row < num_rows; row++) {
	
	if (akty < rscr->rs_Height) {
		ptr0 = pixel_data[0][row];
		
		{
			Retina_OwnRetina();
			
			retmem = Retina_SetSegmentPtr( rmembase );
			
			for (col = width; col > 0; col--) {
				
				*retmem++ = *ptr0++;
				
			}
			
			Retina_DisownRetina();
			rmembase += rscr->rs_Modulo;
		}
		
	}
	
	akty++;
	
  }
}

/*
 * Write some pixel data when color quantization is in effect.
 */

METHODDEF void
put_demapped_rgb (decompress_info_ptr cinfo, int num_rows,
		  JSAMPIMAGE pixel_data)
{
  JSAMPROW ptr0;
  int row;
  long col;
  unsigned char * retmem;
  long width = cinfo->image_width;
	long rmembase = rscr->rs_BitMap + akty*rscr->rs_Modulo;
  width = (width <= rscr->rs_Width)? width : rscr->rs_Width;
  
  
  for (row = 0; row < num_rows; row++) {
	
	if (akty < rscr->rs_Height) {
		ptr0 = pixel_data[0][row];
		
		{
			Retina_OwnRetina();
			
			retmem = Retina_SetSegmentPtr( rmembase );
			
			for (col = width; col > 0; col--) {
				
				*retmem++ = *ptr0++;
				
			}
			
			Retina_DisownRetina();
			rmembase += rscr->rs_Modulo;
		}
		
	}
	
	akty++;
	
  }
}

/*
 * Write the color map.
 */

static unsigned char palbuf[256*3];

METHODDEF void
put_color_map (decompress_info_ptr cinfo, int num_colors, JSAMPARRAY colormap)
{
 unsigned char * plt = palbuf;
 unsigned char * p0 = cinfo->colormap[0];
 unsigned char * p1 = cinfo->colormap[1];
 unsigned char * p2 = cinfo->colormap[2];
 short x;
    
  if (cinfo->out_color_space == CS_RGB) {
    cinfo->methods->put_pixel_rows = put_demapped_rgb;
    
    for (x = 255; (x--);) {
    	
    	*plt++ = *p0++;
    	*plt++ = *p1++;
    	*plt++ = *p2++;
    	
    }
    
    Retina_LoadPalette(rscr, 0, 256, palbuf);
  }
  else
    ERREXIT(cinfo->emethods, "quantize gray?!? Don't be a fool!");
}

static void clrret(void) {
	
	if (rscr) Retina_CloseScreen(rscr);
	if (RetinaBase) CloseLibrary(RetinaBase);
	
}

/*
 * Finish up at the end of the file.
 */

METHODDEF void
output_term (decompress_info_ptr cinfo)
{
   Wait(SIGBREAKF_CTRL_C);
   /* clrret(); */
   
}


/*
 * The method selection routine for Retina format output.
 * This should be called from d_ui_method_selection if PPM output is wanted.
 */

void
jselwretina (decompress_info_ptr cinfo)
{
  atexit(&clrret);
  cinfo->methods->output_init = output_init;
  cinfo->methods->put_color_map = put_color_map;
  if (cinfo->out_color_space == CS_RGB)
    cinfo->methods->put_pixel_rows = put_pixel_rows;
  else
    cinfo->methods->put_pixel_rows = put_gray_rows;
  cinfo->methods->output_term = output_term;
}

