
#define OPENLIB_ASL         0
#define OPENLIB_CX          1
#define OPENLIB_DISKFONT    2
#define OPENLIB_DOS         3
#define OPENLIB_EXEC        4
#define OPENLIB_EXPANSION   5
#define OPENLIB_GADTOOLS    6
#define OPENLIB_GRAPHICS    7
#define OPENLIB_ICON        8
#define OPENLIB_IFFPARSE    9
#define OPENLIB_INTUITION  10
#define OPENLIB_KEYMAP     11
#define OPENLIB_LAYERS     12
#define OPENLIB_REXX       13
#define OPENLIB_TRANSLATOR 14
#define OPENLIB_UTILITY    15
#define OPENLIB_WORKBENCH  16

/* !!! a cleanuplist must be active when you call OpenLibraries() !!! */

void MarkLibrary(ULONG libid,int version);
BOOL OpenLibraries(void);

