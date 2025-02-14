
#ifndef OPTIONS_H
#define OPTIONS_H

extern BOOL  CLImode;   // TRUE if program was started with CLI interface.


#define MAX_FILENAME_LENGTH 400
#define MAX_PATH_LENGTH     250
#define MAX_FILE_LENGTH     150

extern char Loadname  [];
extern char Loadname_g[];
extern char Loadname_b[];
extern char Savename  [];
extern char Savename_g[];
extern char Savename_b[];

#define Loadname_r Loadname
#define Savename_r Savename


// input and output format

extern ULONG loadmode;
extern ULONG savemode;
extern BOOL  Load_is_RGB;
extern BOOL  Save_is_RGB;


// conversion parameters

extern ULONG Output_nColors;
extern  LONG Output_Brightness;
extern  LONG Output_Contrast;

extern ULONG Output_Interpolated;
extern ULONG Output_Width;         /* V1.8: now (almost) OBSOLET */
extern ULONG Output_Height;

/* command parameters */

extern ULONG  Output_DoResize;     /* rev. V1.8 */
extern BOOL   Output_ResizeH;
extern BOOL   Output_ResizeV;
extern double Output_ResizeFactor;

extern ULONG Output_DoSize;   /* semantics changed V1.8: Do a SIZE-operation */
extern ULONG Output_NewWidth;
extern ULONG Output_NewHeight;

extern ULONG Output_DoBoxfit; /* new V1.8 */
extern BOOL  Output_Boxfit_MayEnlarge;
extern ULONG Output_BoxfitWidth;
extern ULONG Output_BoxfitHeight;


extern ULONG Output_Mode;
extern ULONG Output_Dither;
extern ULONG Output_FlipFlags;
extern ULONG Output_RotateFlags;
extern ULONG Output_ColorEffects;

extern ULONG Output_Crop_x1;
extern ULONG Output_Crop_y1;
extern ULONG Output_Crop_x2;
extern ULONG Output_Crop_y2;
extern BOOL  Output_DoCrop;

extern BOOL  Output_DoCenterBox;
extern ULONG Output_CenterBox_Width;
extern ULONG Output_CenterBox_Height;
extern UBYTE Output_CenterBox_R;
extern UBYTE Output_CenterBox_G;
extern UBYTE Output_CenterBox_B;

extern UBYTE EmptyCLUTEntry_R;
extern UBYTE EmptyCLUTEntry_G;
extern UBYTE EmptyCLUTEntry_B;

extern ULONG Output_ColorOffset;
extern  LONG Output_SortCLUT;

extern UBYTE Output_JPEG_Quality;
extern BOOL  Output_JPEG_Progressive;

#endif

