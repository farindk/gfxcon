
#include <utility/tagitem.h>

/* ------------------------------------------------------------*/
#define MULTIPIC_LIB_VERSION     0           /* the current version of multipic.library*/
/* ------------------------------------------------------------*/
/* values for _pi_FileType:*/
#define PFT_UNKNOWN              0           /* UNKONWN image type, unable to load*/
#define PFT_ILBM                 1           /* IFF ILBM*/
#define PFT_DEEP                 2           /* IFF DEEP*/
#define PFT_BMP                  3           /* BMP*/
#define PFT_RGB8                 4           /* IFF RGB8*/
#define PFT_RGBN                 5           /* IFF RGBN*/
#define PFT_YUVN                 6           /* IFF YUVN*/
#define PFT_VLAB                 7           /* IFF VLAB, private VLab format*/
#define PFT_PGM                  8           /* PGM*/
#define PFT_PPM                  9           /* PPM*/
#define PFT_QRT                  10          /* QRT*/
#define PFT_SUNRASTER            11          /* SUNRASTER*/
#define PFT_XIPAINT              12          /* XIPAINT, private VDPaint format*/

/*values for _pi_Flags:*/
#define PIF_IFF (1 << 0)                /* image is stored in an IFF file format*/
#define PIF_ORIGIN_BOTTOM (1 << 1)      /* image is stored upside down*/

struct PicInfo {
 long           pi_Flags;               /* see definitions above*/
 char *         pi_FileName;            /* name of the file (provided to Open() )*/
 char *         pi_TypeName;            /* file type, ascii identifier*/
 short          pi_Type;                /* see PFT_xxx above*/

 short          pi_Width;               /* The width of the image.*/
 short          pi_Height;              /* The height of the image.*/
 short          pi_PageWidth;           /* The page width of the image.*/
 short          pi_PageHeight;          /* The page height of the image.*/

 BYTE           i_AspectX;             /* The X aspect of the image    ny*/
 BYTE           i_AspectY;             /* The Y aspect of the image    ny*/

 unsigned short          pi_CMapEntries;         /* 2..256 with 8 bit images*/
                                        /* 0 or 256 with 24 bit true color images*/
 unsigned short          pi_CMapSize_RGB;
 unsigned short          pi_CMapSize_RRGGBB;
 unsigned short          pi_CMapSize_RGB4;
 unsigned short          pi_CMapSize_RGB32;
 unsigned short          pi_CMapSize_Res1;       /* currently always 0*/
 unsigned short          pi_CMapSize_Res2;       /* currently always 0*/

 unsigned char           pi_Depth;               /* 1..8,12,24,32*/
 unsigned char           pi_BytesPerPixel;       /* 1, 3, 4*/

 unsigned char          pi_RedBits;             /* 1..8  \                     //not yet*/
 unsigned char          pi_GreenBits;           /* 1..8   \ for                //not yet*/
 unsigned char          pi_BlueBits;            /* 1..8   / TRUE-COLOR         //not yet*/
 unsigned char          pi_AlphaBits;           /* 1..8  /                     //not yet*/
 unsigned char          pi_Reserved1Bits;       /* currently always 0*/
 unsigned char          pi_Reserved2Bits;       /* currently always 0*/
};

/* ------------------------------------------------------------*/
/* Flags for BAT_Flags:*/
#define BAF_MERGEPALETTE (1 << 0)        /*recalculate 24 bit true color data if a palette is present*/

/* Tags for MP_SetBufferAttrs*/

#define MP_TagBase                           TAG_USER+0x800       /* Begin counting tags*/
#define BAT_Inc                              MP_TagBase+1        /**/
#define BAT_RedInc                           MP_TagBase+2        /**/
#define BAT_GreenInc                         MP_TagBase+3        /**/
#define BAT_BlueInc                          MP_TagBase+4        /**/
#define BAT_AlphaInc                         MP_TagBase+5        /*not yet*/
#define BAT_DefaultAlphaValue                MP_TagBase+6        /*not yet*/
#define BAT_Flags                            MP_TagBase+7        /*not yet*/
#define BAT_LeftEdge                         MP_TagBase+8        /**/
#define BAT_TopEdge                          MP_TagBase+9        /**/
#define BAT_Width                            MP_TagBase+10       /**/

#define BAT_Mod                              MP_TagBase+11       /*not yet*/
#define BAT_RedMod                           MP_TagBase+12       /*not yet*/
#define BAT_GreenMod                         MP_TagBase+13       /*not yet*/
#define BAT_BlueMod                          MP_TagBase+14       /*not yet*/
#define BAT_AlphaMod                         MP_TagBase+15       /*not yet*/

/* ------------------------------------------------------------*/
/* values for MP_ReadPalette*/

#define PT_RGB                   0
#define PT_RRGGBB                1           /*not yet*/
#define PT_RGB4                  2           /*not yet*/
#define PT_RGB32                 3           /*not yet*/
/* ------------------------------------------------------------*//*define EC_OK                              0*/
#define EC_INTERNAL                        1
#define EC_DOS                             2

#define EC_OUT_OF_MEMORY                   3
#define EC_COULD_NOT_GET_INFORMATION       4
#define EC_END_OF_FILE                     6
#define EC_PLANENUMBER_NOT_SUPPORTED       7
#define EC_COMPRESSION_NOT_SUPPORTED       8
#define EC_COMPRESSIONMODE_NOT_SUPPORTED   9
#define EC_COMPRESSIONMODE_UNKNOWN         10

