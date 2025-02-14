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

