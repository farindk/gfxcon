
#include "glib:incs.h"
#include "glib:errs.h"
#include "glib:g.h"
#include "glib:txtsys.h"
#include "glib:clean.h"
#include "glib:libsys.h"

#define MAXLIBS 17

static ULONG LibsToOpen[1];
static ULONG LibVersion[MAXLIBS];

//#ifdef ____USE_MANUAL_LIBRARY_OPEN____
struct Library *AslBase      , *CxBase        , *DiskfontBase,
               *ExpansionBase,
               *GadToolsBase , *IconBase,
               *IFFParseBase , *KeymapBase,
               *LayersBase   , *RexxSysBase   , *TranslatorBase,
               *UtilityBase  , *WorkbenchBase ;

struct IntuitionBase* IntuitionBase;
struct GfxBase*       GfxBase;

static struct ExecBase *DummyExecBase; /* don't need SysBase,
                                        * EXEC's always opened
                                        */
/**------------------------------------------------------------------------**
 **  Mark a library to be opened
 **------------------------------------------------------------------------**/
void MarkLibrary(ULONG libid,int version)
{
  int arrayindex;
  int bitnr;

  assert(libid+1 <= MAXLIBS);

  /* mark library to be opened */

  arrayindex = libid / 32;
  bitnr      = libid % 32;

  LibsToOpen[arrayindex] |= (1<<bitnr);

  /* change version of library if newer library needed */

  if (version > LibVersion[libid]) LibVersion[libid] = version;
}

static struct { APTR            *base;
                char            *name;
                ERRval           error;
              } libtable[] =
{
  { (APTR *)&AslBase       ,"asl.library"        , LIB_ASL        },
  { (APTR *)&CxBase        ,"commodities.library", LIB_CX         },
  { (APTR *)&DiskfontBase  ,"diskfont.library"   , LIB_DISKFONT   },
  { (APTR *)&DOSBase       ,"dos.library"        , LIB_DOS        },
  { (APTR *)&DummyExecBase ,"exec.library"       , LIB_EXEC       },
  { (APTR *)&ExpansionBase ,"expansion.library"  , LIB_EXPANSION  },
  { (APTR *)&GadToolsBase  ,"gadtools.library"   , LIB_GADTOOLS   },
  { (APTR *)&GfxBase       ,"graphics.library"   , LIB_GFX        },
  { (APTR *)&IconBase      ,"icon.library"       , LIB_ICON       },
  { (APTR *)&IFFParseBase  ,"iffparse.library"   , LIB_IFFPARSE   },
  { (APTR *)&IntuitionBase ,"intuition.library"  , LIB_INTUITION  },
  { (APTR *)&KeymapBase    ,"keymap.library"     , LIB_KEYMAP     },
  { (APTR *)&LayersBase    ,"layers.library"     , LIB_LAYERS     },
  { (APTR *)&RexxSysBase   ,"rexxsyslib.library" , LIB_REXX       },
  { (APTR *)&TranslatorBase,"translator.library" , LIB_TRANSLATOR },
  { (APTR *)&UtilityBase   ,"utility.library"    , LIB_UTILITY    },
  { (APTR *)&WorkbenchBase ,"workbench.library"  , LIB_WORKBENCH  }
};

BOOL OpenLibraries(void)
{
  int bitnr,longnr,id;
  ULONG errorcode_version;

  for (id = 0; id<MAXLIBS ; id++)
  {
    bitnr  = id % 32;
    longnr = id / 32;

    if ( LibsToOpen[longnr] & (1<<bitnr) )
    {
      *(libtable[id].base) = cu_OpenLibrary(libtable[id].name, LibVersion[id]);

      if (!(*(libtable[id].base)))
      {
        switch (LibVersion[id])
        {
          case 37:
            errorcode_version = LIB_204;
            break;
          default:
            errorcode_version = 0;
            break;
        }

        printf("could not open %s\n",libtable[id].name);
        SetError(libtable[id].error | NO_LIBRARY | errorcode_version);
        return FALSE;
      }
    }
  }

  return TRUE;
}
//#endif

/*
void MarkLibrary(ULONG libid,int version)
{
}

BOOL OpenLibraries(void)
{
  return TRUE;
}
*/
