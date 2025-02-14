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

#include "global.h"
#include <exec/execbase.h>
#include <workbench/workbench.h>
#include <proto/wb.h>
#include <proto/icon.h>
#include <proto/exec.h>


#define STACKSIZE 20000

BOOL cli;


// void __regargs __chkabort(void) { }   // REINSERT AFTER DEBUGGING !!!!!

extern struct ExecBase *SysBase;

int startup(int argc,char **argv)
{
  if (argc==0 || argc==1) { cli=FALSE; return wbstart (argc,argv); }
  else			  { cli=TRUE;  return clistart(argc,argv); }
}


struct CleanupList *GlobalCleanup;


static struct TextAttr gadfont = { "topaz.font", 8 }; // { "times.font",18 };
       struct TextAttr bigfont = { "topaz.font",11 }; // { "dutchroyal.font",27 };

#define FONTNAME_LENGTH 24

static char gadfont_name[FONTNAME_LENGTH+1];
static char bigfont_name[FONTNAME_LENGTH+1];

static char  TmpDir[150];
static char  TmpPrefix[150];
static char  TmpPostfix[] = ".tmp";

char *GetTempPrefix(void)  { return TmpPrefix;  }
char *GetTempPostfix(void) { return TmpPostfix; }


static struct {
  enum Language  LanguageID;
  char		*Name;
} Languages[] =
{
  LANGUAGE_ENGLISH , "englisch",
  LANGUAGE_ENGLISH , "english",
  LANGUAGE_GERMAN  , "deutsch",
  LANGUAGE_GERMAN  , "german",
  LANGUAGE_ILLEGAL
};

static void SetLanguageToToolType(char *name)
{
  int i;

  for (i=0;Languages[i].LanguageID != LANGUAGE_ILLEGAL;i++)
  {
    if (stricmp(Languages[i].Name,name)==0)
    {
      SetLanguage(Languages[i].LanguageID);
      return;
    }
  }
}

static void SetFontToToolType(struct TextAttr *ta,char *fontname,char *toolarg)
{
  int i;
  int size;
  char *sizepos=0;

  for (i=0;toolarg[i] != '\0' && i<FONTNAME_LENGTH;i++)
  {
    if (toolarg[i] == ' ') { fontname[i]=0;
			     sizepos=&toolarg[i+1];
			     break;
			   }

    fontname[i]=toolarg[i];
  }

  if (sizepos)
  {
    ta->ta_Name = fontname;
    strcat(fontname,".font");

    size=0;
    while (*sizepos != '\0')
    {
      size *= 10;
      size += *sizepos-'0';
      sizepos++;
    }

    ta->ta_YSize = size;
  }
}

extern int AbsMemMinimum;
extern int AbsMinMemBlock;

void ReadToolTypes(char *name)
{
  struct DiskObject  *diskobj;
  char		     *param;
  char		    **tooltypes;

  diskobj = GetDiskObject(name);
  if (diskobj)
  {
    tooltypes =diskobj->do_ToolTypes;

    if (param = FindToolType(tooltypes,"TEMPDIR"))
    {
      strcpy(TmpDir,param);
      sprintf(TmpPrefix,"%sGFXCONV%x_",param,SysBase->ThisTask);
    }

    if (param = FindToolType(tooltypes,"GADGETFONT"))
      SetFontToToolType(&gadfont,gadfont_name,param);

    if (param = FindToolType(tooltypes,"TITLEFONT"))
      SetFontToToolType(&bigfont,bigfont_name,param);

    if (param = FindToolType(tooltypes,"LANGUAGE"))
      SetLanguageToToolType(param);

    if (param = FindToolType(tooltypes,"STDLOADPATH"))
      SetLoadPath(param);

    if (param = FindToolType(tooltypes,"STDSAVEPATH"))
      SetSavePath(param);

    if (param = FindToolType(tooltypes,"MINMEM"))
      AbsMemMinimum = atoi(param);

    if (param = FindToolType(tooltypes,"MINMEMBLOCK"))
      AbsMinMemBlock = atoi(param);

    FreeDiskObject(diskobj);
  }
}

static ULONG AskStackSize(void)
{
  struct Process *pr;
  char *upper,*lower;
  ULONG total;

  pr=(struct Process *)FindTask(NULL);

  if (pr->pr_Task.tc_Node.ln_Type == NT_PROCESS && pr->pr_CLI != NULL)
  {
    upper = (char *)pr->pr_ReturnAddr + sizeof(ULONG);
    total = *(ULONG *)pr->pr_ReturnAddr;
//  lower = upper - total;
  }
  else
  {
    upper = (char *)pr->pr_Task.tc_SPUpper;
    lower = (char *)pr->pr_Task.tc_SPLower;
    total = upper - lower;
  }

  return total;
}

struct MsgPort* msgport;

void ClearTmpIfFirst(void)
{
  int first;

  Forbid();

  if (FindPort("GfxConPort")==NULL)
  {
    /* wir sind das einzige GfxCon-Programm */

    first=1;

    msgport=CreatePort("GfxConPort",0);
  }
  else
  {
    first=0;
    msgport=NULL;
  }

  Permit();

  if (first)
  {
    char buffer[250];

    sprintf(buffer,"delete >nil: <nil: %sGFXCONV#?_#? quiet",TmpDir);
    Execute(buffer,0,0);
  }
}

void __autoopenfail(char* lib)
{
}

int main(int argc,char **argv)
{
  int returncode=10;
  char		   *ProgName;  /* the name of our program */
  struct WBStartup *wbstup;
  struct WBArg	   *wbarg;

  if (AskStackSize() < STACKSIZE)
  { printf("Sorry, need %d bytes (or a bit more) stack!\n",STACKSIZE); return 100; }

  strcpy(TmpDir,"T:");   // (rev. 1.8  / v1.7 value: "sys:t/")
  sprintf(TmpPrefix,"%sGFXCONV%x_",TmpDir,SysBase->ThisTask);

  if (argc!=0)
  {
    ProgName = argv[0];
  } else
  {
    wbstup=(struct WBStartup *)argv;
    wbarg =wbstup->sm_ArgList;
    ProgName = wbarg->wa_Name;
  }

  if (SysBase->LibNode.lib_Version < 36 && argc<=1)
  {
    printf("Sorry, need Kick2.0 for GUI !\n");
    return 100;
  }

  if (!TestTxtIntegrity()) return 100;

  SetLanguage(LANGUAGE_GERMAN);

  InitGfxBuffer();

  if (GlobalCleanup = NewCleanup(NULL))
  {
    if (argc<=1)
    {
      MarkLibs4GraphSys(FALSE);
      MarkLibs4CleanupSys(FALSE);
      MarkLibrary(OPENLIB_ICON,36);
      MarkLibrary(OPENLIB_ASL ,36);
      MarkLibrary(OPENLIB_DOS ,37);
    }
    else
    {
      MarkLibs4GraphSys(TRUE);
      MarkLibs4CleanupSys(TRUE);
      MarkLibrary(OPENLIB_ICON,0);
      MarkLibrary(OPENLIB_DOS ,0);
    }

    if (!OpenLibraries())
      ShowError();
    else
    {
      ReadToolTypes(ProgName);

      ClearTmpIfFirst();

      if (argc<=1)
      {
	if (!(InitConstructData(NULL,GlobalCleanup)))
	{
	  ShowError();
	}
	else
	{
	  SetNormFont(&gadfont);
	  returncode=startup(argc,argv);
	}
      }
      else
	returncode=startup(argc,argv);
    }

    if (msgport) DeletePort(msgport);

    DoCleanupFree(GlobalCleanup);
  }
  else
    ShowError();

  return returncode;
}


