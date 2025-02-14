/***------------------------------------------------------------------------->
 ***  File:	txtsys.c
 ***  Contents: language dependent text output system
 ***
 ***  Files belonging to this one:
 ***	errs.h	     - the #defines of error messages	(public)
 ***	txts.h	     - the #defines of text messages	(public)
 ***	txts.c	     - all language dependent strings	(public)
 ***	txtsys.h     - the public structures		(public)
 ***	txtsys.c     - the routines			(private)
 ***	txtstructs.h - structures used in this module	(private)
 ***
 ***  Version: 1.0
 ***  Date:    27.5.1993
 ***
 ***----------------------------------------------------------------------> */

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

#include "glib:incs.h"

#include "glib:txtsys.h"
#include "glib:txtstructs.h"
#include "glib:errs.h"
#include "glib:txts.h"

#include <errno.h>

extern struct IntuitionBase *IntuitionBase; /* check if intuition is available
					     * as we want to pop up a requester
					     */
extern struct LanguageInfo   alltxts[];     /* a table with the base-pointers
					     * for all languages
					     */

static struct MsgData *errs;	/* the current error-msg-list */
static struct MsgData *txts;	/* the current text-msg-list  */

static int	       nErrors; /* the number of entries in the list */
static int	       nTxts;

static enum Language   CurrentLanguage = LANGUAGE_ILLEGAL;

/* error-requester definition (will be completed by SetLanguage() ) */

static struct EasyStruct err_req =
{
  sizeof(struct EasyStruct), NULL, NULL, ">>> %s <<<", NULL
};


/**------------------------------------------------------------------------**
     void SetLanguage( enum Language );
   ------------------------------------------------------------------------
     Change the language. This function expects that the Language exists
     in the alltxts[] (txts.c) list.
 **------------------------------------------------------------------------**/




/*  Count how many entries there are in the list.
 */
static int CountMsgs(struct MsgData *msglist)
{
  int count=0;

  while (msglist->code)
  {
    count++;
    msglist++; /* go to next entry */
  }

  return count;
}



/* compare-function for qsort()
 */
static int cmpmsgs(struct MsgData *msgtxt1,struct MsgData *msgtxt2)
{
  return msgtxt1->code - msgtxt2->code;
}



void SetLanguage(enum Language language)
{
  int i;

  /* search the alltxts[] list for the entry with the desired language */

  for ( i=0 ; alltxts[i].language != LANGUAGE_ILLEGAL ; i++)
  {
    if (alltxts[i].language == language) break;
  }

  /* copy the fields into modul-global variables */

  CurrentLanguage = alltxts[i].language;
  errs		  = alltxts[i].errors;
  txts		  = alltxts[i].strings;

  assert(CurrentLanguage != LANGUAGE_ILLEGAL);

  /* count the strings */

  nErrors = CountMsgs(errs);
  nTxts   = CountMsgs(txts);

  /* sort the message-codes for a faster access */

  qsort((char *)errs,nErrors,sizeof(struct MsgData),&cmpmsgs);
  qsort((char *)txts,nTxts  ,sizeof(struct MsgData),&cmpmsgs);

  /* localize error-requester */

  err_req.es_Title	 =Txt(TXT_ERRORREQUEST_TITLE);
  err_req.es_GadgetFormat=Txt(TXT_ERRORREQUEST_FORMAT);
}




/**------------------------------------------------------------------------**
     char *Txt(long code);
   ------------------------------------------------------------------------
     Return the string associated with the code. If there is no string
     with this code in the list, NULL will be returned.
 **------------------------------------------------------------------------**/

static char *FindInMsgList(struct MsgData *list,int listlength,int code)
{
  char *string=NULL;
  int	lowerbound,upperbound;
  int	i;

  /* set bounds */

  lowerbound = 0;
  upperbound = listlength-1;

  /* search the string (perhaps it doesn't exist !) */

  while (string == NULL  &&             /* non found yet */
	 lowerbound <= upperbound)	/* search not finished */
  {
    i = (lowerbound+upperbound)/2;  /* shoot in the middle */

    if (list[i].code == code) string=list[i].string;  /* string found */

    if (list[i].code > code)  upperbound=i-1; /* limit the range */
    else		      lowerbound=i+1;
  }

  return string;
}

char *Txt(long code)
{
  char *string;

  string = FindInMsgList(txts,nTxts, code );

  assert(string != NULL);

  return string;
}


static void (*custom_error_func)(char *,ERRval) = NULL;

/**------------------------------------------------------------------------**
     void InstallErrorFunc(void (*func)(char *));
   ------------------------------------------------------------------------
     Install a custom error-output-function. This will be called when you
     call ShowError() or DoError(). Remove it with RemoveErrorFunc().
 **------------------------------------------------------------------------**/

void InstallErrorFunc(void (*func)(char *,ERRval) )
{
  assert(custom_error_func == NULL);

  custom_error_func = func;
}


/**------------------------------------------------------------------------**
     void InstallErrorFunc(void (*func)(char *));
   ------------------------------------------------------------------------
     Install a custom error-output-function. This will be called when you
     call ShowError() or DoError(). Remove it with RemoveErrorFunc().
 **------------------------------------------------------------------------**/

void RemoveErrorFunc(void)
{
  assert(custom_error_func != NULL);

  custom_error_func = NULL;
}




/**------------------------------------------------------------------------**
     void SetError(ERRval code);
   ------------------------------------------------------------------------
     Stores the error-code if non is stored yet. This will make error-
     processing more save, as there may be several sequent calls to
     SetError() while only the first (the deepest reason for failing) will
     be stored. It may be displayed using ShowError().
     You may use DoError() as a shortcut for SetError() - ShowError().
     If the error-code is DOS_ERROR, high word will be filled with _OSERR;
 **------------------------------------------------------------------------**/

static ERRval errorcode = OK;

void SetError(ERRval code)
{
  if (errorcode != OK) return; /* there's already an error-code stored */

  if (code == DOS_ERROR) code |= (_OSERR) << 16;

  errorcode=code;
}




/**-------------------------------------------------------------------------**
    char *GetErrorText(ERRval code);
   -------------------------------------------------------------------------
    Return the text belonging to the code.
 **-------------------------------------------------------------------------**/

char *GetErrorTxt(long code)
{
  char *errtxt;

  /* find exact match */

  errtxt=FindInMsgList(errs,nErrors, code );

  /* if there is no exact match, find the error-low word */

  if (!errtxt) errtxt=FindInMsgList(errs,nErrors, code & ERRORCODE_MASK);

  /* if there is still no match, find the ERROR_DOESNT_EXIST - error */

  if (!errtxt) { errtxt=FindInMsgList(errs,nErrors, ERROR_DOESNT_EXIST);
  printf("unknown error-code: %x\n",code);
  }

  /* if there is still no match, find the INTERNAL_ERROR - error */

  if (!errtxt) errtxt=FindInMsgList(errs,nErrors, INTERNAL_ERROR);

  assert(errtxt != NULL);  /* there should really be some error now !!! */

  return errtxt;
}


/**------------------------------------------------------------------------**
     void ShowError(void);
   ------------------------------------------------------------------------
     Display the error stored with SetError(); If intuition is available
     pop up a requester. If we don't have access to intuition print the
     message on 'stdout'.
 **------------------------------------------------------------------------**/

extern BOOL cli;

void ShowError(void)
{
  char *txt;

  txt=GetErrorTxt(errorcode);

  if (custom_error_func)  custom_error_func(txt,errorcode);
  else if (IntuitionBase && !cli) EasyRequest(NULL,&err_req,NULL,txt);
			     else printf("%s: %s\n",err_req.es_Title,txt);

  errorcode = OK; /* remove error */
}




/**-------------------------------------------------------------------------**
    void DoError(ERRval code);
   -------------------------------------------------------------------------
    This is only a shortcut for sequent calls of SetError() and ShowError()
 **-------------------------------------------------------------------------**/

void DoError(ERRval code)
{
  SetError(code);
  ShowError();
}

/**-------------------------------------------------------------------------**
    ERRval LastError(void);
   -------------------------------------------------------------------------
    Return the code of the last error.
 **-------------------------------------------------------------------------**/

ERRval LastError(void)
{
  return errorcode;
}


/**------------------------------------------------------------------------**
     BOOL TestTxtIntegrity(void)           *** DEBUG - only ***
   ------------------------------------------------------------------------
     Test if there are strings missing or defined twice in a language.
     Print a report of missing string to stdout.
     (PS.: I know, the quality of this routine is very low. But I don't
	   need a high-quality routine here!)
 **------------------------------------------------------------------------**/
BOOL TestTxtIntegrity(void)
{
#ifndef NDEBUG

  BOOL	 isok=TRUE;

  /* sort all lists to allow a quick search */

  {
    struct MsgData *list;
    int 	    entries;
    int 	    i;

    for ( i=0 ; alltxts[i].language != LANGUAGE_ILLEGAL ; i++)
    {
      /* sort the strings */

      list = alltxts[i].strings;
      entries = CountMsgs(list);
      qsort((char *)list,entries,sizeof(struct MsgData),&cmpmsgs);

      /* sort the errors */

      list = alltxts[i].errors;
      entries = CountMsgs(list);
      qsort((char *)list,entries,sizeof(struct MsgData),&cmpmsgs);
    }
  }

  /* test if the same texts (and errors) are available in all languages */

  {
    struct MsgData *ptr;
    int 	    basis_i,current_i;
    int 	    entries;
    int 	    what;

    for (what=0;what<=1;what++)
      for (basis_i=0 ; alltxts[basis_i].language != LANGUAGE_ILLEGAL ; basis_i++)
	for (current_i=0 ;
	     alltxts[current_i].language != LANGUAGE_ILLEGAL ;
	     current_i++)
	{
	  if (current_i == basis_i) continue;

	  ptr=what==0 ? alltxts[basis_i].strings : alltxts[basis_i].errors;
	  entries = CountMsgs(what==0 ? alltxts[current_i].strings :
					alltxts[current_i].errors);

	  while(ptr->code)
	  {
	    if (FindInMsgList(what==0 ? alltxts[current_i].strings :
					alltxts[current_i].errors,
					entries,
					ptr->code) == NULL)
	    {
	      isok=FALSE;
	      printf("%s nr. 0x%08p missing in language nr. %d\n",
						   what==0 ? "text" : "error",
						   ptr->code,
						   alltxts[current_i].language);
	    }

	    ptr++;
	  }
    }
  }

  /* find duplicates */

  {
    struct MsgData *ptr,*curr;
    int 	    what;
    int 	    i;

    for (what=0;what<=1;what++)
      for (i=0; alltxts[i].language != LANGUAGE_ILLEGAL ; i++)
      {
	ptr=what == 0 ? alltxts[i].strings : alltxts[i].errors;

	while (ptr->code)
	{
	  curr = ptr+1;

	  while (curr->code)
	  {
	    if (curr->code == ptr->code)
	    {
	      isok=FALSE;
	      printf("duplicate %s nr. 0x%08p in language nr. %d\n",
			 what==0 ? "text" : "error",
			 ptr->code,
			 alltxts[i].language);
	    }
	    curr++;
	  }

	  ptr++;
	}
      }

  }


  return isok;

#else
  return TRUE;
#endif
}




/**------------------------------------------------------------------------**
     char **GetAllLanguageNames(void)
   ------------------------------------------------------------------------
     Returns a NULL-terminated array of pointers to the names of all
     languages available (ideal for passing to a CYCLE_KIND gadget).
 **------------------------------------------------------------------------**/


static struct { enum Language  language;
		long	       txtindex;
	      }
	      LanguageNames[] = {
				  LANGUAGE_GERMAN,  TXT_GERMAN_NAME,
				  LANGUAGE_ENGLISH, TXT_ENGLISH_NAME,
				  LANGUAGE_FRENCH,  TXT_FRENCH_NAME,
				  LANGUAGE_FINNISH, TXT_FINNISH_NAME,
				  LANGUAGE_ITALIAN, TXT_ITALIAN_NAME,
				  LANGUAGE_SPANISH, TXT_SPANISH_NAME,
				  LANGUAGE_ILLEGAL, ~0
				};

static enum Language   Index2LanguageTable[NLANGUAGES];
static char	      *LanguageName	  [NLANGUAGES+1];

static char *FindLanguageName(enum Language id)
{
  int i;

  for (i=0 ; LanguageNames[i].language != LANGUAGE_ILLEGAL ; i++)
    if (LanguageNames[i].language == id) break;

  assert(LanguageNames[i].language != LANGUAGE_ILLEGAL);

  return Txt( LanguageNames[i].txtindex );
}


char **GetAllLanguageNames(void)
{
  int i,language_index,nindices;

  /* scan the alltxts[] - array */

  for ( i=language_index=0 ; alltxts[i].language != LANGUAGE_ILLEGAL ; i++)
  {
    Index2LanguageTable[language_index] = alltxts[i].language;
    language_index++;
  }
  nindices = language_index;

  /* fill the names of the languages in the array */

  for (i=0;i<nindices;i++)
    LanguageName[i] = FindLanguageName( Index2LanguageTable[i] );

  LanguageName[i] = NULL; /* write final NULL */

  return LanguageName;
}




/**------------------------------------------------------------------------**
     void SetLanguageFromIndex(int index)
   ------------------------------------------------------------------------
     If you pass the pointer returned by GetAllLanguages() to a CYCLE_KIND
     gadget, you will receive a selected language in the code field of the
     IntuiMessage. You may set the current language using this code with
     this routine.
 **------------------------------------------------------------------------**/


void SetLanguageFromIndex(int index)
{
  SetLanguage( Index2LanguageTable[index] );
}



/**------------------------------------------------------------------------**
     enum Language GetCurrentLanguage(void)
   ------------------------------------------------------------------------
     Returns the current language. Use this routine to save the current
     setting in configurations ...
 **------------------------------------------------------------------------**/

enum Language GetCurrentLanguage(void)
{
  return CurrentLanguage;
}

