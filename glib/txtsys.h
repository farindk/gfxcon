
enum Language { LANGUAGE_ENGLISH,
                LANGUAGE_GERMAN,
                LANGUAGE_FRENCH,
                LANGUAGE_FINNISH,
                LANGUAGE_ITALIAN,
                LANGUAGE_SPANISH,

                LANGUAGE_ILLEGAL
              };

typedef long ERRval;

void   SetLanguage(enum Language);
enum Language GetCurrentLanguage(void);
char **GetAllLanguageNames(void);
void   SetLanguageFromIndex(int index);

char  *Txt(long code);

void   SetError(ERRval code);
void   ShowError(void);
void   DoError(ERRval code);
ERRval LastError(void);
char  *GetErrorText(ERRval code);

void   InstallErrorFunc(void (*func)(char *,ERRval));
void   RemoveErrorFunc(void);

BOOL   TestTxtIntegrity(void);

