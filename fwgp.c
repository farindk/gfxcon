
/********************************************************************************
 *
 * modul name:  fwgp.c
 *
 * contents:    ---
 *
 *
 * to do:
 *   !!! everything !!!
 *
 *
 * Copyright (C) 1995  Dirk Farin  <dirk.farin@gmail.com>
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

void CheckForWGP(form fo)
{
  ULONG signature;

  signature = GetLong(0,0);

  if (signature != ( ((-1)<<24) | ('W'<<16) | ('P'<<8) | ('C') ) )
    fo->not_valid_format=TRUE;
  else
    fo->not_valid_format=FALSE;
}

