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

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/diskfont.h>
#include <proto/gadtools.h>
#include <proto/iffparse.h>
#include <proto/utility.h>
#include <proto/layers.h>
#include <proto/wb.h>
#include <proto/dos.h>
#include <proto/asl.h>

#include <intuition/gadgetclass.h>
#include <intuition/intuition.h>
#include <workbench/workbench.h>
#include <libraries/gadtools.h>
#include <libraries/iffparse.h>
#include <libraries/asl.h>
#include <utility/tagitem.h>
#include <exec/memory.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <dos.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <m68881.h>
