/*-*----------------------------------------------------------------------*-*
 *-*----------------------------------------------------------------------*-*
 * *                                                                      * *
 * *  Routines for easy allocation and deallocation of system         __  * *
 * *  resources.                                                  /| |_   * *
 * *                                               Dirk Farin    /_|.| .  * *
 * *                                                                      * *
 *-*----------------------------------------------------------------------*-*
 *-*----------------------------------------------------------------------*-*/

#include "glib:incs.h"
#include "glib:txtsys.h"
#include "glib:errs.h"
#include "glib:libsys.h"
#include "glib:clean.h"

//#define malloc(x)   AllocVec(x,NULL)
//#define calloc(a,b) AllocVec(a*b,MEMF_CLEAR)
//#define free(x)     FreeVec(x)

// struct Library *GadToolsBase,*WorkbenchBase,*DiskfontBase;

/* Each allocation, reservation (and so on) is stored in a linked list
 * of CleanupNode-structures. Each knot saves the information of one
 * allocation. The type is stored in the Type-enum and the type-specific
 * data in the para-union.
 * A special knot type is the cln_ClnSubList knot. With this kind of knot
 * it is possible to create a dependence tree of CleanupLists. Every
 * child-tree will be freed if its parent list is freed.
 */

struct CleanupList
{
  struct MinList      list;   /* list of CleanupNodes
                               */
  struct CleanupList *parent; /* pointer to the parent CleanupList-struct
                               */
  struct CleanupList *last;   /* the CleanupList that was active before
                               * this one.
                               */
};

struct CleanupNode
{
  struct MinNode node;

#ifndef NDEBUG
  char   *name;
#endif

  enum {
         cln_clnSubList,
         cln_malloc,
         cln_OpenWindow,
         cln_OpenScreen,
         cln_fopen,
         cln_GadgetList,
         cln_CreateMsgPort,
         cln_AddAppWindow,
         cln_OpenLibrary,
         cln_OpenDiskFont,
         cln_CallFunction,
         cln_VisualInfo,
         cln_InsertList
       }
       Type;

  union {
    struct { struct CleanupList *l;  } cln_clnSubList;
    struct { void *ptr;              } cln_malloc;
    struct { struct Window    *win;  } cln_OpenWindow;
    struct { struct AppWindow *win;
             struct MsgPort   *port; } cln_AddAppWindow;
    struct { struct Node      *node; } cln_InsertList;
    struct { struct Gadget    *gad;  } cln_GadgetList;
    struct { struct Library   *lib;  } cln_OpenLibrary;
    struct { struct TextFont  *font; } cln_OpenDiskFont;
    struct { struct MsgPort   *port; } cln_CreateMsgPort;
    struct { APTR             *vi;   } cln_VisualInfo;
    struct { FILE *handle;           } cln_fopen;
    struct { void __regargs (*func)(void *);
             void *parameter;        } cln_CallFunction;
  } para;
};

/**------------------------------------------------------------------------**
 **  Mark the libraries used in this module
 **------------------------------------------------------------------------**/

void MarkLibs4CleanupSys(BOOL cli)
{
  if (cli)
  {
    MarkLibrary(OPENLIB_INTUITION,0);
    MarkLibrary(OPENLIB_DISKFONT ,0);
  }
  else
  {
    MarkLibrary(OPENLIB_INTUITION,36);
    MarkLibrary(OPENLIB_DISKFONT ,36);
    MarkLibrary(OPENLIB_GADTOOLS ,37);
    MarkLibrary(OPENLIB_WORKBENCH,37);
  }
}


/*--- the CleanupList currently being used ---*/

static struct CleanupList *currentlist=0;

/**------------------------------------------------------------------------**
 **  Create a new CleanupList. If the new list shall be made dependant on  **
 **  a parent list, this list must be passed. If not, pass NULL.           **
 **------------------------------------------------------------------------**/
struct CleanupList *NewCleanup(struct CleanupList *parent)
{
  struct CleanupList *l; /* the new list */
  struct CleanupNode *n; /* the node to connect the new
                          * list to the parent list.
                          */

  /* get new list structure */

  if (l=(struct CleanupList *)malloc(sizeof(struct CleanupList)))
  {
    NewList((struct List *)&l->list); /* initialize the list */
    l->last  =currentlist;            /* pointer to last list that was active */
    l->parent=parent;                 /* pointer to parent */

    /* insert a cln_clnSubList-type node into the parent list */

    if (parent) /* only if this is wanted */
    {
      /* allocate node-structure */

      if (n=(struct CleanupNode *)malloc(sizeof(struct CleanupNode)))
      {
        /* initialize node */

        n->Type=cln_clnSubList;
        n->para.cln_clnSubList.l = l;

#ifndef NDEBUG
        n->name="STD SubList";
#endif

        /* add to parent list */

        AddTail( (struct List *)parent , (struct Node *)n );
      }
      else  /* (can't allocate node for parent list) */
      {
        free(l);
        SetError(NO_MEM);
        return NULL;
      }
    }

    /* make the new list the active one */

    currentlist=l;

    return l;
  }
  else
  {
    SetError(NO_MEM);
    return NULL;
  }
}

/**------------------------------------------------------------------------**
 **  Go back to the CleanupList that was active before this one.           **
 **------------------------------------------------------------------------**/
void CloseCleanup(void)
{
  currentlist = currentlist->last;
}

/**------------------------------------------------------------------------**
 **  Use a specific CleanupList next but remember the list that was active **
 **  before (nesting of multiple SetCleanups to the same list are NOT !!!  **
 **  allowed).                                                             **
 **------------------------------------------------------------------------**/
void SetCleanup(struct CleanupList *cl)
{
  cl->last=currentlist;
  currentlist=cl;
}

/*
#ifndef NDEBUG
void name_cu_node(char *name)
{
  ((struct CleanupNode *)currentlist->list.mlh_TailPred)->name = name;
}
#endif
*/

/*
 *   get a new cleanup-node
 */
static struct CleanupNode *getcnnode(void)
{
  return (struct CleanupNode *)calloc(sizeof(struct CleanupNode),1);
}

/* Macro to insert a new node to the current list. (EXTREMELY private!) */

#define INSERT AddTail((struct List *)&currentlist->list,(struct Node *)cn)

/*----------------------------------------------------------------------------
         The new functions which replace the standard functions follow
  ----------------------------------------------------------------------------*/

void *cu_calloc(int size1,int n)
{
  void *ptr;
  struct CleanupNode *cn;

  if (cn=getcnnode())
  {
    ptr=calloc(size1,n);              /* call old function */

    if (ptr)                          /* old function succeded */
    {
      cn->Type=cln_malloc;
      cn->para.cln_malloc.ptr = ptr;

      INSERT;
      name_cu_node("STD calloc");
      return ptr;
    }
    free(cn);                         /* old function failed, free node */
    SetError(NO_MEM);
  } else
    SetError(NO_MEM);
  return 0;                           /* return error-code */
}

static const char *const cu_free_alert =
 "\x00\x88\x0cTrying to free memory, that wasn't allocated !\x00";
static const char *const cu_fclose_alert =
 "\x00\x98\x0cTrying to close file, that wasn't opened !\x00";
static const char *const cu_sublist_alert =
 "\x00\x88\x0cTrying to free a sublist, that doesn't exist !\x00";
static const char *const cu_CloseFont_alert =
 "\x00\x90\x0cTrying to close a font, that doesn't exist !\x00";

void cu_free(void *ptr)
{
  struct CleanupNode *cn;

  /* search the node belonging to the 'ptr'-allocation in the list */

  for (cn=(struct CleanupNode *)currentlist->list.mlh_Head ;
       cn->node.mln_Succ;
       cn=(struct CleanupNode *)cn->node.mln_Succ )
      {
        if (cn->Type == cln_malloc)
        if (cn->para.cln_malloc.ptr == ptr)     /* if node was found */
        {
          Remove((struct Node *)cn);  /* remove that node */

          free(ptr);    /* free the memory */
          free(cn);     /* and the node    */

          return;       /* and return      */
        }
      }

  DisplayAlert(RECOVERY_ALERT,cu_free_alert,20);  /* this node doesn't exist */
}

BOOL cu_fclose(FILE *fh)
{
  struct CleanupNode *cn;

  for (cn=(struct CleanupNode *)currentlist->list.mlh_Head ;
       cn->node.mln_Succ;
       cn=(struct CleanupNode *)cn->node.mln_Succ )
      {
        if (cn->Type == cln_fopen)
        if (cn->para.cln_fopen.handle == fh)
        {
          Remove((struct Node *)cn);
          free(cn);

          if (fclose(fh) == -1)
          {
            SetError(DOS_ERROR);
            return FALSE;
          }
          else
            return TRUE;
        }
      }

  DisplayAlert(RECOVERY_ALERT,cu_fclose_alert,20);

  return -2; /* private error-code (this must not happen!) */
}

struct Library *cu_OpenLibrary(char *name,unsigned long ver)
{
  struct Library *lib;
  struct CleanupNode *cn;

  if (cn=getcnnode())
  {
    lib=OpenLibrary(name,ver);
    if (lib)
    {
      cn->Type=cln_OpenLibrary;
      cn->para.cln_OpenLibrary.lib = lib;

      INSERT;
      name_cu_node("STD OpenLib");
      return lib;
    }
    free (cn);
    SetError(NO_LIBRARY);
  }
  else
    SetError(NO_MEM);

  return 0;
}

struct TextFont *cu_OpenDiskFont(struct TextAttr *ta)
{
  struct TextFont *tf;
  struct CleanupNode *cn;

  if (cn=getcnnode())
  {
    tf=OpenDiskFont(ta);
    if (tf)
    {
      cn->Type=cln_OpenDiskFont;
      cn->para.cln_OpenDiskFont.font = tf;

      INSERT;
      name_cu_node("STD OpenDiskFont");
      return tf;
    }
    free (cn);
    SetError(FONT_NOT_AVAILABLE);
  }
  else
    SetError(NO_MEM);

  return 0;
}

void cu_CloseFont(struct TextFont *tf)
{
  struct CleanupNode *cn;

  for (cn=(struct CleanupNode *)currentlist->list.mlh_Head ;
       cn->node.mln_Succ;
       cn=(struct CleanupNode *)cn->node.mln_Succ )
      {
        if (cn->Type == cln_OpenDiskFont)
        if (cn->para.cln_OpenDiskFont.font == tf)
        {
          Remove((struct Node *)cn);
          free(cn);

          CloseFont(tf);
          return;
        }
      }

  DisplayAlert(RECOVERY_ALERT,cu_CloseFont_alert,20);
}

BOOL cu_AddHead(struct List *l,struct Node *n)
{
  struct CleanupNode *cn;

  if (cn=getcnnode())
  {
    AddHead(l,n);

    cn->Type=cln_InsertList;
    cn->para.cln_InsertList.node = n;

    INSERT;
    name_cu_node("STD AddHead");
    return TRUE;
  }
  else
    SetError(NO_MEM);

  return FALSE;
}

BOOL cu_AddTail(struct List *l,struct Node *n)
{
  struct CleanupNode *cn;

  if (cn=getcnnode())
  {
    AddTail(l,n);

    cn->Type=cln_InsertList;
    cn->para.cln_InsertList.node = n;

    INSERT;
    name_cu_node("STD AddTail");
    return TRUE;
  }
  else
    SetError(NO_MEM);

  return FALSE;
}

FILE *cu_fopen(char *name,char *mode)
{
  struct CleanupNode *cn;
  FILE *fh;

  if (!*name) return FALSE;

  if (cn=getcnnode())
  {
    if (fh=fopen(name,mode))
    {
      cn->Type=cln_fopen;
      cn->para.cln_fopen.handle = fh;

      INSERT;
      name_cu_node("STD fopen");
      return fh;
    }
    free (cn);
    SetError(FILE_NOT_FOUND);
  }
  else
    SetError(NO_MEM);

  return 0;
}

struct MsgPort *cu_CreateMsgPort(void)
{
  struct CleanupNode *cn;
  struct MsgPort *port;

  if (cn=getcnnode())
  {
    if (port=CreateMsgPort())
    {
      cn->Type=cln_CreateMsgPort;
      cn->para.cln_CreateMsgPort.port = port;

      INSERT;
      name_cu_node("STD CreateMsgPort");
      return port;
    }
    free (cn);
    SetError(CANT_CREATE_MSGPORT);
  }
  else
    SetError(NO_MEM);

  return 0;
}

/*-----------------------------------------------------------------------------
        The following functions must be called after the old function!
  -----------------------------------------------------------------------------*/

BOOL cuinsert_VisualInfo(APTR vi)
{
  struct CleanupNode *cn;

  if (!vi) { SetError(COULDNT_GET_VISUALINFO); return FALSE; }  /* vi not valid ! */

  if (cn=getcnnode())
  {
    cn->Type=cln_VisualInfo;
    cn->para.cln_VisualInfo.vi = vi;

    INSERT;
    name_cu_node("STD VisualInfo");
    return TRUE;
  }
  else
    SetError(NO_MEM);

  FreeVisualInfo(vi);           /* cleanup now, because DoCleanup()
                                 * can't do it without node         */
  return FALSE;
}

BOOL cuinsert_OpenWindow(struct Window *win)
{
  struct CleanupNode *cn;

  if (!win) { SetError(COULDNT_OPEN_WINDOW); return FALSE; }

  if (cn=getcnnode())
  {
    cn->Type=cln_OpenWindow;
    cn->para.cln_OpenWindow.win = win;

    INSERT;
    name_cu_node("STD OpenWindow");
    return TRUE;
  }
  else
    SetError(NO_MEM);

  CloseWindow(win);
  return FALSE;
}

BOOL cuinsert_GadgetList(struct Gadget *glist)
{
  struct CleanupNode *cn;

  if (!glist) { SetError(CANT_CREATE_GADGET); return FALSE; }

  if (cn=getcnnode())
  {
    cn->Type=cln_GadgetList;
    cn->para.cln_GadgetList.gad = glist;

    INSERT;
    name_cu_node("STD GadgetList");
    return TRUE;
  }
  else
    SetError(NO_MEM);

  FreeGadgets(glist);
  return FALSE;
}

BOOL cuinsert_CallFunction(void (*func)(void *),void *parameter)
{
  struct CleanupNode *cn;

  if (cn=getcnnode())
  {
    cn->Type=cln_CallFunction;
    cn->para.cln_CallFunction.func      = func;
    cn->para.cln_CallFunction.parameter = parameter;

    INSERT;
    name_cu_node("STD CallFunc");
    return TRUE;
  }
  else
    SetError(NO_MEM);

  (*func)(parameter);  /* call the function */
  return FALSE;
}

BOOL cuinsert_AddAppWindow(struct AppWindow *appwin,struct MsgPort *msgport)
{
  struct CleanupNode *cn;
  struct AppMessage *amsg;

  if (!appwin) { SetError(CANT_CREATE_APPWIN); return FALSE; }

  if (cn=getcnnode())
  {
    cn->Type=cln_AddAppWindow;
    cn->para.cln_AddAppWindow.win  = appwin;
    cn->para.cln_AddAppWindow.port = msgport;

    INSERT;
    name_cu_node("STD AddAppWindow");
    return TRUE;
  }
  else
    SetError(NO_MEM);

  /* free the AppWindow */

  RemoveAppWindow(appwin);

  while (amsg=(struct AppMessage *)GetMsg(msgport))
    ReplyMsg((struct Message *)amsg);

  return FALSE;
}

/*-----------------------------------------------------------------------------
                     Functions doing the actual deallocations.
  -----------------------------------------------------------------------------*/

void DoCleanupFree(struct CleanupList *cl);

/**------------------------------------------------------------------------**
 **  Free all allocations in a list, but don't free the list itself.       **
 **------------------------------------------------------------------------**/
void DoCleanup(struct CleanupList *cl)
{
  struct CleanupNode *cn,*next;

  /* free all nodes on the list */

  for (cn=(struct CleanupNode *)cl->list.mlh_TailPred ;
       cn->node.mln_Pred;
       cn=next )
      {

#ifndef NDEBUG
//        if (cn->name) printf("Clean: %s\n",cn->name);
//                 else printf("Cleannode. Name unknown\n");
#endif

        switch (cn->Type)
        {
          case cln_malloc:
            free(cn->para.cln_malloc.ptr);
            break;
          case cln_InsertList:
            Remove(cn->para.cln_InsertList.node);
            break;
          case cln_OpenWindow:
            CloseWindow(cn->para.cln_OpenWindow.win);
            break;
          case cln_fopen:
            fclose(cn->para.cln_fopen.handle);
            break;
          case cln_GadgetList:
            FreeGadgets(cn->para.cln_GadgetList.gad);
            break;
          case cln_CreateMsgPort:
            DeleteMsgPort(cn->para.cln_CreateMsgPort.port);
            break;
          case cln_AddAppWindow:
            {
              struct AppMessage *amsg;

              RemoveAppWindow(cn->para.cln_AddAppWindow.win);

              while (amsg=(struct AppMessage *)
                          GetMsg(cn->para.cln_AddAppWindow.port))
                ReplyMsg((struct Message *)amsg);
            }
            break;
          case cln_OpenLibrary:
            CloseLibrary(cn->para.cln_OpenLibrary.lib);
            break;
          case cln_OpenDiskFont:
            CloseFont(cn->para.cln_OpenDiskFont.font);
            break;
          case cln_VisualInfo:
            FreeVisualInfo(cn->para.cln_VisualInfo.vi);
            break;
          case cln_clnSubList:
            DoCleanup(cn->para.cln_clnSubList.l);
            free     (cn->para.cln_clnSubList.l);
            break;
          case cln_CallFunction:
            (*cn->para.cln_CallFunction.func)(cn->para.cln_CallFunction.parameter);
            break;
        }

        Remove((struct Node *)cn);
        next = (struct CleanupNode *)cn->node.mln_Pred;
        free(cn);
      }
}

/**------------------------------------------------------------------------**
 **  Clean a list and free the list. After that cannot be used anymore.    **
 **------------------------------------------------------------------------**/
void DoCleanupFree(struct CleanupList *cl)
{
  struct CleanupNode *cn;

  if (currentlist==cl)    /* if this was the current list, close it */
    CloseCleanup();

  DoCleanup(cl); /* do the normal cleanup */

  if (cl->parent)  /* free the cln_clnSubList node in the parent list */
  {
    for (cn=(struct CleanupNode *)cl->parent->list.mlh_Head ;
         cn->node.mln_Succ;
         cn=(struct CleanupNode *)cn->node.mln_Succ )
        {
          if (cn->Type == cln_clnSubList)
          if (cn->para.cln_clnSubList.l == cl) /* if the node was found */
          {
            Remove((struct Node *)cn);  /* remove that knot,       */
            free(cn);                   /* free it                 */
            goto dofree;                /* and leave the function. */
          }
        }

    DisplayAlert(RECOVERY_ALERT,cu_sublist_alert,20);
  }

dofree:
  free(cl);  /* free the list itself */
}

