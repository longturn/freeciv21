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

#include <utility>

// Qt
#include <QBuffer>
#include <QByteArray>

// KArchive
#include <KFilterDev>

#include "ioz.h"

struct fz_FILE_s {
  std::unique_ptr<QIODevice> dev;
  QByteArray buffer;
};

/************************************************************************/ /**
   Open memory buffer for reading as fz_FILE.
   If control is TRUE, caller gives up control of the buffer
   so ioz will free it when fz_FILE closed.
 ****************************************************************************/
fz_FILE *fz_from_memory(const QByteArray &buffer)
{
  auto fp = new fz_FILE;
  fp->buffer = buffer;
  fp->dev = std::make_unique<QBuffer>(&fp->buffer);
  fp->dev->open(QIODevice::ReadWrite);
  return fp;
}

/************************************************************************/ /**
   Open file for reading/writing, like fopen.
   The compression method is chosen based on the file extension.
 ****************************************************************************/
fz_FILE *fz_from_file(const char *filename, QIODevice::OpenMode mode)
{
  auto fp = new fz_FILE;
  fp->dev = std::make_unique<KFilterDev>(filename);
  fp->dev->open(mode);
  return fp;
}

/************************************************************************/ /**
   Close file, like fclose. Always returns 0.
 ****************************************************************************/
int fz_fclose(fz_FILE *fp)
{
  delete fp;
  return 0;
}

/************************************************************************/ /**
   Get a line, like fgets.
   Returns NULL in case of error, or when end-of-file reached
   and no characters have been read.
 ****************************************************************************/
char *fz_fgets(char *buffer, int size, fz_FILE *fp)
{
  auto read = fp->dev->readLine(buffer, size);
  return read > 0 ? buffer : nullptr;
}

/************************************************************************/ /**
   Print formated, like fprintf.

   Returns number of bytes actually written, or 0 on error.
 ****************************************************************************/
int fz_fprintf(fz_FILE *fp, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  auto data = QString::vasprintf(format, ap).toUtf8();
  va_end(ap);

  return fp->dev->write(data);
}

/************************************************************************/ /**
   Returns non-zero if there is an error status associated with
   this stream.  Check the device for details.
 ****************************************************************************/
int fz_ferror(fz_FILE *fp)
{
  return fp->dev->errorString().isNull() ? 1 : 0;
}

/************************************************************************/ /**
   Returns the underlying I/O device. Do not delete it.
 ****************************************************************************/
QIODevice *fz_device(fz_FILE *fp) { return fp->dev.get(); }
