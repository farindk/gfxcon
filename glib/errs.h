
/*--------------------------- error-codes-system -----------------------------

   Low-word specifies the error type that occured.
   High-word describes the error of the low-word in more detail.

   If the high-word is NULL the error-routines provide a fall-back mode which
   uses only the low-word. I strongly recommend to specify a message for each
   low-word !

   The MSB-bit of the low-word is reserved to indicate an internal error.
   The next bit is reserved to indicate a nonfatal error or warning.

   Program-specific errors may be added; you may use low-words beginning with
   ERR_USER.

  ----------------------------------------------------------------------------*/

#define INTERNAL_ERROR            0x00008000    /* this bits may NOT be used */
#define WARNING_ERROR             0x00004000    /* for anything else !       */

#define ERRORCODE_MASK            0x0000ffff
#define ERRORDETAIL_MASK          0xffff0000

/* general errors */

#define OK                        0x0
#define NO_MEM                    0x0001
#define MISC_ERROR                0x0002   // avoid this !!!

#define KEIN_SPEICHER             0x0001   // obsolete
#define DOS_ERROR                 0x0003   /* look in high-word for _OSERR */

//#define FILE_NICHT_GELADEN      0x0010   // obsolete
//#define FILE_NICHT_GESPEICHERT  0x0011   // obsolete

/* asl */

#define COULDNT_OPEN_FILEREQ      0x0010

/* OpenLibraries */

#define NO_LIBRARY                0x0020

#define LIB_ASL                   0x00100000
#define LIB_CX                    0x00200000
#define LIB_DISKFONT              0x00300000
#define LIB_DOS                   0x00400000
#define LIB_EXEC                  0x00500000
#define LIB_EXPANSION             0x00600000
#define LIB_GADTOOLS              0x00700000
#define LIB_GFX                   0x00800000
#define LIB_ICON                  0x00900000
#define LIB_IFFPARSE              0x00a00000
#define LIB_INTUITION             0x00b00000
#define LIB_KEYMAP                0x00c00000
#define LIB_REXX                  0x00d00000
#define LIB_LAYERS                0x00e00000
#define LIB_TRANSLATOR            0x00f00000
#define LIB_UTILITY               0x01000000
#define LIB_WORKBENCH             0x01100000

#define LIB_204                   0x10000000

#define NO_INTUI204             ( NO_LIBRARY | LIB_204 | LIB_INTUITION )
#define NO_GFX204               ( NO_LIBRARY | LIB_204 | LIB_GFX       )
#define NO_DISKFONT204          ( NO_LIBRARY | LIB_204 | LIB_DISKFONT  )
#define NO_ASL204               ( NO_LIBRARY | LIB_204 | LIB_ASL       )
#define NO_GADTOOLS204          ( NO_LIBRARY | LIB_204 | LIB_GADTOOLS  )
#define NO_WORKBENCH204         ( NO_LIBRARY | LIB_204 | LIB_WORKBENCH )
#define NO_IFFPARSE204          ( NO_LIBRARY | LIB_204 | LIB_IFFPARSE  )
#define NO_LAYERS204            ( NO_LIBRARY | LIB_204 | LIB_LAYERS    )
#define NO_UTILITY204           ( NO_LIBRARY | LIB_204 | LIB_UTILITY   )

/* fonts */

#define FONT_NOT_AVAILABLE        0x0030
#define ERRWARN_NOFONT_FALLBACK ( 0x0031 | WARNING_ERROR )

/* exec */

#define CANT_CREATE_MSGPORT       0x0040

/* windows & co */

#define COULDNT_OPEN_WINDOW       0x0050
#define CANT_LOCK_WBSCREEN        0x0051
#define COULDNT_GET_VISUALINFO    0x0052
#define COULDNT_GET_DRAWINFO      0x0053
#define CANT_CREATE_GADGET        0x0054
#define CANT_CREATE_APPWIN        0x0055
#define WIN_DOESNT_FIT_ON_SCREEN  0x0056
#define RASTERPARAMETERS_INVALID (0x0057 | INTERNAL_ERROR)

/* DOS & co. */

#define FILE_NOT_FOUND            0x0060
#define FILE_CLOSE_ERR            0x0061
#define CANT_GET_PATH_OF_LOCK     0x0062
#define FILENAME_BUFFER_FULL      0x0063

/* Preferences */

#define FILE_PRINTERPREFS           0x00100000

/* iffhandle */

#define NO_IFFHANDLE                0x0070
#define ERR_IFFHANDLE_ERR           0x0071

#define NO_PRINTER_PREFS            ( FILE_NOT_FOUND | FILE_PRINTERPREFS )
#define ERR_CLOSE_PRINTERPREFS_FILE ( FILE_CLOSE_ERR | FILE_PRINTERPREFS )

/* language support */

#define LANGUAGE_DOESNT_EXIST       ( INTERNAL_ERROR | 0x0080 )
#define ERROR_DOESNT_EXIST          ( INTERNAL_ERROR | 0x0081 )

/******************************************************************************

                               user - enhancements

 ******************************************************************************/

 #include "myerrs.h"

