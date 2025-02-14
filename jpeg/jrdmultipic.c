
/*
 * jrdmultipic.c
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
#include <inline/mpic.h>


static struct PicHandle * phan = 0;
static struct PicInfo * pinf = 0;
struct Library * MultiPicBase = 0;

/* This version is for reading 8-bit, not implemented yet

METHODDEF void
get_text_gray_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
{
  register JSAMPROW ptr0;
  register unsigned int val;
  register long col;
  
  ptr0 = pixel_row[0];
  for (col = cinfo->image_width; col > 0; col--) {
    val = read_pbm_integer(cinfo);
    if (rescale != NULL)
      val = rescale[val];
    *ptr0++ = (JSAMPLE) val;
  }
}

*/


METHODDEF void
get_rgb_row (compress_info_ptr cinfo, JSAMPARRAY pixel_row)
/* This version is for reading 24-bit files */
{
  MP_Read(phan, pixel_row[0], pixel_row[1], pixel_row[2], 0, 1);
  
}


static void clrmpic(void) {
	
	if (phan) MP_Close(phan);
	if (MultiPicBase) CloseLibrary(MultiPicBase);
	
}

/*
 * Read the file header; return image size and component count.
 */

METHODDEF void
input_init (compress_info_ptr cinfo)
{
  long tags[] = {
  	
  	BAT_Flags, BAF_MERGEPALETTE,
  	TAG_DONE
  	
  };
  atexit(clrmpic);
  if (0 == (MultiPicBase = OpenLibrary( "multipic.library", 0 ))) ERREXIT(cinfo->emethods, "unable to open multipic.library");
  
  phan = MP_Open( (unsigned char *)cinfo->input_file, 0);
  if (phan == 0)  ERREXIT(cinfo->emethods, "input picture unavailable or of unknown format");
  pinf = MP_Info (phan);
  if (pinf == 0)  ERREXIT(cinfo->emethods, "input picture unavailable or of unknown format");

  if (!MP_SetBufferAttrs(phan, (struct TagItem *) tags)) ERREXIT(cinfo->emethods, "input picture unavailable or of unknown format");

  cinfo->methods->get_input_row = get_rgb_row;
  cinfo->input_components = 3;
  cinfo->in_color_space = CS_RGB;

  cinfo->image_width = pinf->pi_Width;
  cinfo->image_height = pinf->pi_Height;
  cinfo->data_precision = BITS_IN_JSAMPLE;

}

/*
 * Finish up at the end of the file.
 */

METHODDEF void
input_term (compress_info_ptr cinfo)
{
  /* wird automatisch erledigt */
}


/*
 * The method selection routine for PPM format input.
 * Note that this must be called by the user interface before calling
 * jpeg_compress.  If multiple input formats are supported, the
 * user interface is responsible for discovering the file format and
 * calling the appropriate method selection routine.
 */

GLOBAL void
jselmultipic (compress_info_ptr cinfo)
{
  cinfo->methods->input_init = input_init;
  /* cinfo->methods->get_input_row is set by input_init */
  cinfo->methods->input_term = input_term;
}

