
#include "glib:incs.h"
#include "glib:txtsys.h"
#include "glib:errs.h"
#include "glib:clean.h"
#include "glib:g.h"
#include "glib:libsys.h"

#include "glib:gstr.h"

struct CleanupList *globalcleanup;

struct TextAttr gadfont = { "times.font",18 };

void testit(void)
{
  struct Handle *h,*stringgad;
  struct List demolist;
  struct Node n1,n2,n3,n4,n5,n6,n7,n8;

  NewList(&demolist);
  n1.ln_Name="Node 1";
  n2.ln_Name="Node 2";
  n3.ln_Name="Node 3";
  n4.ln_Name="Node 4";
  n5.ln_Name="Node 5";
  n6.ln_Name="Node 6";
  n7.ln_Name="Node 7";
  n8.ln_Name="Node 8";
  AddTail(&demolist,&n1);
  AddTail(&demolist,&n2);
  AddTail(&demolist,&n3);
  AddTail(&demolist,&n4);
  AddTail(&demolist,&n5);
  AddTail(&demolist,&n6);
  AddTail(&demolist,&n7);
  AddTail(&demolist,&n8);


  if (BeginNewHandleTree())
  {
    stringgad=CrGadget(GAGA_Kind,STRING_KIND,TAG_DONE,0,TAG_DONE);

    h=CrSmallWindow(
       CrSpaceBox(
         CrNormTextBox("Text-Box",
                   CrSpaceBox(
                     CrVBox(
                       CrGadget(GAGA_Text,"Drück mich",TAG_DONE,0,TAG_DONE),
                       CrFiller(),
                       CrCBGadget("light switched on",TAG_DONE,0,TAG_DONE),
                       CrFiller(),
                       CrGadget(GAGA_Kind     ,LISTVIEW_KIND,
                                GAGA_LV_StringGad,stringgad,
                                TAG_DONE,0,
                                GTLV_Labels,&demolist,
                                TAG_DONE),
                       HANDLE_END
                     )
                   )
                  )
       ),
       TAG_DONE,0,
       WA_Title      ,"Tralala",
       WA_DragBar    ,TRUE,
       WA_DepthGadget,TRUE,
       WA_CloseGadget,TRUE,
//       WA_Activate   ,TRUE,
       TAG_DONE
      );
  }
  else
    ShowError();

  if (h != HANDLE_ERR)
  {
    if (ComputeGadgets(h))
    {
      if (OpenHandle(h))
      {
        HandleHandle(h,NULL,NULL);

        CloseHandle(h);

        FreeHandleTree(h);
      }
      else
        ShowError();
    }
    else
      ShowError();
  }
}

void main(void)
{
  if (!TestTxtIntegrity()) return;

  SetLanguage(LANGUAGE_GERMAN);

  globalcleanup = NewCleanup(NULL);

  if (!globalcleanup)
    ShowError();
  else
  {
    MarkLibs4GraphSys();
    MarkLibs4CleanupSys();

    if (!OpenLibraries())
      ShowError();
    else
    {
      if (!(InitConstructData(NULL,globalcleanup)))
        ShowError();
      else
      {
        SetNormFont(&gadfont);

        testit();
      }
    }

    DoCleanupFree(globalcleanup);
  }
}
