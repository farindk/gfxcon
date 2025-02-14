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

