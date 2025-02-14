
struct CleanupList *NewCleanup(struct CleanupList *parent);
void CloseCleanup(void);
void SetCleanup(struct CleanupList *cl);
void DoCleanup (struct CleanupList *cl);
void DoCleanupFree(struct CleanupList *cl);
void MarkLibs4CleanupSys(BOOL);

void *cu_calloc(int size1,int n);
void  cu_free  (void *memptr);
FILE *cu_fopen (char *name,char *mode);
BOOL  cu_fclose(FILE *file);
struct MsgPort *cu_CreateMsgPort(void);
struct Library *cu_OpenLibrary(char *,unsigned long ver);
struct TextFont *cu_OpenDiskFont(struct TextAttr *);
void             cu_CloseFont(struct TextFont *);

BOOL cu_AddHead(struct List *l,struct Node *n);
BOOL cu_AddTail(struct List *l,struct Node *n);

BOOL cuinsert_OpenWindow(struct Window *win);
BOOL cuinsert_GadgetList(struct Gadget *glist);
BOOL cuinsert_AddAppWindow(struct AppWindow *appwin,struct MsgPort *port);
BOOL cuinsert_VisualInfo(APTR vi);
BOOL cuinsert_CallFunction(void (*func)(void *),void *parameter);

//#ifdef NDEBUG
#define name_cu_node(x) /* name_cu_node(x) */
//#else
//void name_cu_node(char *name);
//#endif

