
#include "global.h"
#include "options.h"

char Loadname  [MAX_FILENAME_LENGTH];
char Loadname_g[MAX_FILENAME_LENGTH];
char Loadname_b[MAX_FILENAME_LENGTH];
char Savename  [MAX_FILENAME_LENGTH];
char Savename_g[MAX_FILENAME_LENGTH];
char Savename_b[MAX_FILENAME_LENGTH];


ULONG loadmode   =FORM_JPEG,savemode   =FORM_ILBM;
BOOL  Load_is_RGB=FALSE    ,Save_is_RGB=FALSE;

 LONG Output_Brightness   = 0;
 LONG Output_Contrast     = 100;

ULONG Output_Width        = 0,Output_Height       = 0;
ULONG Output_Interpolated = FALSE;

ULONG Output_Mode         = GFXMOD_CLUT;
ULONG Output_nColors      = 16;
ULONG Output_Dither       = 1<<29;
ULONG Output_FlipFlags    = 1<<29;
ULONG Output_RotateFlags  = 1<<29;
ULONG Output_ColorEffects = 1<<29;

ULONG Output_Crop_x1;
ULONG Output_Crop_y1;
ULONG Output_Crop_x2;
ULONG Output_Crop_y2;
BOOL  Output_DoCrop;

BOOL  Output_DoCenterBox;
ULONG Output_CenterBox_Width;
ULONG Output_CenterBox_Height;
UBYTE Output_CenterBox_R;
UBYTE Output_CenterBox_G;
UBYTE Output_CenterBox_B;

UBYTE EmptyCLUTEntry_R;
UBYTE EmptyCLUTEntry_G;
UBYTE EmptyCLUTEntry_B;

ULONG Output_ColorOffset;
 LONG Output_SortCLUT;

ULONG  Output_DoResize;     /* rev. V1.8 */
BOOL   Output_ResizeH;
BOOL   Output_ResizeV;
double Output_ResizeFactor;

ULONG Output_DoSize;   /* semantics changed V1.8: Do a SIZE-operation */
ULONG Output_NewWidth;
ULONG Output_NewHeight;

ULONG Output_DoBoxfit; /* new V1.8 */
BOOL  Output_Boxfit_MayEnlarge;
ULONG Output_BoxfitWidth;
ULONG Output_BoxfitHeight;

UBYTE Output_JPEG_Quality = 75;
BOOL  Output_JPEG_Progressive = FALSE;



BOOL  CLImode = FALSE;


