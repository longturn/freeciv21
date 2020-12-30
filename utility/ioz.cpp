/*__            ___                 ***************************************
/   \          /   \          Copyright (c) 1996-2020 Freeciv21 and Freeciv
\_   \        /  __/          contributors. This file is part of Freeciv21.
 _\   \      /  /__     Freeciv21 is free software: you can redistribute it
 \___  \____/   __/    and/or modify it under the terms of the GNU  General
     \_       _/          Public License  as published by the Free Software
       | @ @  \_               Foundation, either version 3 of the  License,
       |                              or (at your option) any later version.
     _/     /\                  You should have received  a copy of the GNU
    /o)  (o/\ \_                General Public License along with Freeciv21.
    \_____/ /                     If not, see https://www.gnu.org/licenses/.
      \____/        ********************************************************/

/***********************************************************************
  An IO layer to support transparent compression/uncompression.
  (Currently only "required" functionality is supported.)

  There are various reasons for making this a full-blown module
  instead of just defining a few macros:

  - Ability to switch between compressed and uncompressed at run-time
    (zlib with compression level 0 saves uncompressed, but still with
    gzip header, so non-zlib server cannot read the savefile).

  - Flexibility to add other methods if desired (eg, bzip2, arbitrary
    external filter program, etc).
***********************************************************************/

#include <stdlib.h>

// Qt
#include <QBuffer>
#include <QByteArray>

// KArchive
#include <KFilterDev>

#include "ioz.h"

/************************************************************************/ /**
   Print formated, like fprintf.

   Returns number of bytes actually written, or 0 on error.
 ****************************************************************************/
int fz_fprintf(QIODevice *fp, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  auto data = QString::vasprintf(format, ap).toUtf8();
  va_end(ap);

  return fp->write(data);
}
