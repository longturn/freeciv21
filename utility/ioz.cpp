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

  FIXME: when zlib support _not_ included, should sanity check whether
  the first few bytes are gzip marker and complain if so.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <zlib.h>

#ifdef HAVE_BZLIB_H
#include <bzlib.h>
#endif

#ifdef HAVE_LZMA_H
#include <lzma.h>
#endif

// Qt
#include <QByteArray>

/* utility */
#include "log.h"
#include "shared.h"
#include "support.h"

#include "ioz.h"

#ifdef FREECIV_HAVE_LIBBZ2
struct bzip2_struct {
  BZFILE *file;
  FILE *plain;
  int error;
  int firstbyte;
  bool eof;
};
#endif /* FREECIV_HAVE_LIBBZ2 */

#ifdef FREECIV_HAVE_LIBLZMA

#define PLAIN_FILE_BUF_SIZE (8096 * 1024) /* 8096kb */
#define XZ_DECODER_TEST_SIZE (4 * 1024) /* 4kb */

/* In my tests 7Mb proved to be not enough and with 10Mb decompression
   succeeded in typical case. */
#define XZ_DECODER_MEMLIMIT (65 * 1024 * 1024) /* 65Mb */
#define XZ_DECODER_MEMLIMIT_STEP                                            \
  (25 * 1024 * 1024) /* Increase 25Mb at a time */
#define XZ_DECODER_MEMLIMIT_FINAL (100 * 1024 * 1024) /* 100Mb */

struct xz_struct {
  lzma_stream stream;
  int out_index;
  uint64_t memlimit;

  /* liblzma bug workaround. This is what stream.avail_out should be,
     calculated correctly. Used only when reading file. */
  int out_avail;
  int total_read;

  FILE *plain;
  uint8_t *in_buf; /* uint8_t is what xz headers use */
  uint8_t *out_buf;
  lzma_ret error;

  /* There seems to be bug in liblzma decompression that if all the
     processing happened when lzma_code() last time was called with
     LZMA_RUN action, it does not set next_out or avail_out variables
     to sane values when lzma_code() is called with LZMA_FINISH.
     We never call lzma_code() with LZMA_RUN action with all the input
     we have left. This hack_byte is kept in storage in case there is no
     more input when we try to get it. This byte can then be provided to
     lzma_code(LZMA_FINISH) */
  char hack_byte;
  bool hack_byte_used;
};

static bool xz_outbuffer_to_file(fz_FILE *fp, lzma_action action);
static void xz_action(fz_FILE *fp, lzma_action action);

#endif /* FREECIV_HAVE_LIBLZMA */

struct mem_fzFILE {
  QByteArray buffer;
  int pos;
};

struct fz_FILE_s {
  enum fz_method method;
  char mode;
  bool memory;
  union {
    struct mem_fzFILE mem;
    FILE *plain; /* FZ_PLAIN */
    gzFile zlib; /* FZ_ZLIB */
#ifdef FREECIV_HAVE_LIBBZ2
    struct bzip2_struct bz2;
#endif
#ifdef FREECIV_HAVE_LIBLZMA
    struct xz_struct xz;
#endif
  } u;
};

/************************************************************************/ /**
   Validate the compression method.
 ****************************************************************************/
static inline bool fz_method_is_valid(enum fz_method method)
{
  switch (method) {
  case FZ_PLAIN:
  case FZ_ZLIB:
#ifdef FREECIV_HAVE_LIBBZ2
  case FZ_BZIP2:
#endif
#ifdef FREECIV_HAVE_LIBLZMA
  case FZ_XZ:
#endif
    return true;
  }
  return false;
}

#define fz_method_validate(method)                                          \
  (fz_method_is_valid(method)                                               \
       ? method                                                             \
       : (fc_assert_msg(                                                    \
              true == fz_method_is_valid(method),                           \
              "Unsupported compress method %d, reverting to plain.",        \
              method),                                                      \
          FZ_PLAIN))

/************************************************************************/ /**
   Open memory buffer for reading as fz_FILE.
   If control is TRUE, caller gives up control of the buffer
   so ioz will free it when fz_FILE closed.
 ****************************************************************************/
fz_FILE *fz_from_memory(const QByteArray &buffer)
{
  // We have to use malloc() because of the union
  fz_FILE *fp = (fz_FILE *) fc_malloc(sizeof(*fp));
  fp->memory = true;
  // Since we have to malloc(), initialize by hand using placement new
  new (&fp->u.mem.buffer) QByteArray(buffer);
  fp->u.mem.pos = 0;

  return fp;
}

/************************************************************************/ /**
   Open file for reading/writing, like fopen.
   Parameters compress_method and compress_level only apply
   for writing: for reading try to use the most appropriate
   available method.
   Returns NULL if there was a problem; check errno for details.
   (If errno is 0, and using FZ_ZLIB, probably had zlib error
   Z_MEM_ERROR.  Wishlist: better interface for errors?)
 ****************************************************************************/
fz_FILE *fz_from_file(const char *filename, QIODevice::OpenMode mode,
                      enum fz_method method, int compress_level)
{
  fz_FILE *fp;

  if (!is_reg_file_for_access(filename, mode & QIODevice::WriteOnly)) {
    return NULL;
  }

  fp = (fz_FILE *) fc_malloc(sizeof(*fp));
  fp->memory = false;

  if (mode & QIODevice::WriteOnly) {
    // Writing
    fp->mode = 'w';
  } else {
    // Reading: ignore specified method and try each
    fp->mode = 'r';
    method = FZ_ZLIB;
  }

  fp->method = fz_method_validate(method);

  std::string mode_str;
  if (mode & QIODevice::ReadOnly) {
    mode_str += "r";
  }
  if (mode & QIODevice::WriteOnly) {
    mode_str += "w";
  }
  if (method == FZ_ZLIB) {
    // gz files are binary files, so we should add "b" to mode!
    mode_str += "b";
  }

  switch (fp->method) {
  case FZ_ZLIB:
    if (mode & QIODevice::WriteOnly) {
      mode_str += std::to_string(compress_level);
    }
    fp->u.zlib = fc_gzopen(filename, mode_str.data());
    if (!fp->u.zlib) {
      free(fp);
      fp = NULL;
    }
    return fp;
  case FZ_PLAIN:
    fp->u.plain = fc_fopen(filename, mode_str.data());
    if (!fp->u.plain) {
      free(fp);
      fp = NULL;
    }
    return fp;
  }

  /* Should never happen */
  fc_assert_msg(false, "Internal error in %s() (method = %d)", __FUNCTION__,
                fp->method);
  free(fp);
  return NULL;
}

/************************************************************************/ /**
   Close file, like fclose.
   Returns 0 on success, or non-zero for problems (but don't call
   fz_ferror in that case because the struct has already been
   free'd;  wishlist: better interface for errors?)
   (For FZ_PLAIN returns EOF and could check errno;
   for FZ_ZLIB: returns zlib error number; see zlib.h.)
 ****************************************************************************/
int fz_fclose(fz_FILE *fp)
{
  int error = 0;

  fc_assert_ret_val(NULL != fp, 1);

  if (fp->memory) {
    // Delete the QByteArray by hand because free() won't do it for us
    fp->u.mem.buffer.~QByteArray();
    // Free the data
    free(fp);

    return 0;
  }

  switch (fz_method_validate(fp->method)) {
#ifdef FREECIV_HAVE_LIBLZMA
  case FZ_XZ:
    if (fp->mode == 'w' && !xz_outbuffer_to_file(fp, LZMA_FINISH)) {
      error = 1;
    }
    lzma_end(&fp->u.xz.stream);
    free(fp->u.xz.in_buf);
    free(fp->u.xz.out_buf);
    fclose(fp->u.xz.plain);
    free(fp);
    return error;
#endif /* FREECIV_HAVE_LIBLZMA */
#ifdef FREECIV_HAVE_LIBBZ2
  case FZ_BZIP2:
    if ('w' == fp->mode) {
      BZ2_bzWriteClose(&fp->u.bz2.error, fp->u.bz2.file, 0, NULL, NULL);
    } else {
      BZ2_bzReadClose(&fp->u.bz2.error, fp->u.bz2.file);
    }
    error = fp->u.bz2.error;
    fclose(fp->u.bz2.plain);
    free(fp);
    return BZ_OK == error ? 0 : 1;
#endif /* FREECIV_HAVE_LIBBZ2 */
  case FZ_ZLIB:
    error = gzclose(fp->u.zlib);
    free(fp);
    return 0 > error ? error : 0; /* Only negative Z values are errors. */
  case FZ_PLAIN:
    error = fclose(fp->u.plain);
    free(fp);
    return error;
  }

  /* Should never happen */
  fc_assert_msg(false, "Internal error in %s() (method = %d)", __FUNCTION__,
                fp->method);
  free(fp);
  return 1;
}

/************************************************************************/ /**
   Get a line, like fgets.
   Returns NULL in case of error, or when end-of-file reached
   and no characters have been read.
 ****************************************************************************/
char *fz_fgets(char *buffer, int size, fz_FILE *fp)
{
  fc_assert_ret_val(NULL != fp, NULL);

  if (fp->memory) {
    int i, j;

    for (i = fp->u.mem.pos, j = 0;
         i < fp->u.mem.buffer.size() && j < size - 1 /* Space for '\0' */
         && fp->u.mem.buffer[i] != '\n'
         && (fp->u.mem.buffer[i] != '\r' || fp->u.mem.buffer.size() == i + 1
             || fp->u.mem.buffer[i + 1] != '\n');
         i++) {
      buffer[j++] = fp->u.mem.buffer[i];
    }

    if (j < size - 2) {
      /* Space for both newline and terminating '\0' */
      if (i + 1 < fp->u.mem.buffer.size() && fp->u.mem.buffer[i] == '\r'
          && fp->u.mem.buffer[i + 1] == '\n') {
        i += 2;
        buffer[j++] = '\n';
      } else if (i < fp->u.mem.buffer.size()
                 && fp->u.mem.buffer[i] == '\n') {
        i++;
        buffer[j++] = '\n';
      }
    }

    if (j == 0) {
      return NULL;
    }

    fp->u.mem.pos = i;
    buffer[j] = '\0';

    return buffer;
  }

  switch (fz_method_validate(fp->method)) {
#ifdef FREECIV_HAVE_LIBLZMA
  case FZ_XZ: {
    int i, j;

    for (i = 0; i < size - 1; i += j) {
      size_t len = 0;
      bool line_end;

      for (j = 0, line_end = false;
           fp->u.xz.out_avail > 0 && !line_end && j < size - i - 1;
           j++, fp->u.xz.out_avail--) {
        buffer[i + j] = fp->u.xz.out_buf[fp->u.xz.out_index++];
        fp->u.xz.total_read++;
        if (buffer[i + j] == '\n') {
          line_end = true;
        }
      }

      if (line_end || size <= j + i + 1) {
        buffer[i + j] = '\0';
        return buffer;
      }

      if (fp->u.xz.hack_byte_used) {
        size_t hblen = 0;

        fp->u.xz.in_buf[0] = fp->u.xz.hack_byte;
        len = fread(fp->u.xz.in_buf + 1, 1, PLAIN_FILE_BUF_SIZE - 1,
                    fp->u.xz.plain);
        len++;

        if (len <= 1) {
          hblen = fread(&fp->u.xz.hack_byte, 1, 1, fp->u.xz.plain);
        }
        if (hblen == 0) {
          fp->u.xz.hack_byte_used = false;
        }
      }
      if (len == 0) {
        if (fp->u.xz.error == LZMA_STREAM_END) {
          if (i + j == 0) {
            /* Plain file read complete, and there was nothing in xz buffers
               -> end-of-file. */
            return NULL;
          }
          buffer[i + j] = '\0';
          return buffer;
        } else {
          fp->u.xz.stream.next_out = fp->u.xz.out_buf;
          fp->u.xz.stream.avail_out = PLAIN_FILE_BUF_SIZE;
          xz_action(fp, LZMA_FINISH);
          fp->u.xz.out_index = 0;
          fp->u.xz.out_avail =
              fp->u.xz.stream.total_out - fp->u.xz.total_read;
          if (fp->u.xz.error != LZMA_OK
              && fp->u.xz.error != LZMA_STREAM_END) {
            return NULL;
          }
        }
      } else {
        lzma_action action;

        fp->u.xz.stream.next_in = fp->u.xz.in_buf;
        fp->u.xz.stream.avail_in = len;
        fp->u.xz.stream.next_out = fp->u.xz.out_buf;
        fp->u.xz.stream.avail_out = PLAIN_FILE_BUF_SIZE;
        if (fp->u.xz.hack_byte_used) {
          action = LZMA_RUN;
        } else {
          action = LZMA_FINISH;
        }
        xz_action(fp, action);
        fp->u.xz.out_avail = fp->u.xz.stream.total_out - fp->u.xz.total_read;
        fp->u.xz.out_index = 0;
        if (fp->u.xz.error != LZMA_OK && fp->u.xz.error != LZMA_STREAM_END) {
          return NULL;
        }
      }
    }

    buffer[i] = '\0';
    return buffer;
  } break;
#endif /* FREECIV_HAVE_LIBLZMA */
#ifdef FREECIV_HAVE_LIBBZ2
  case FZ_BZIP2: {
    char *retval = NULL;
    int i = 0;
    int last_read;

    /* See if first byte is already read and stored */
    if (fp->u.bz2.firstbyte >= 0) {
      buffer[0] = fp->u.bz2.firstbyte;
      fp->u.bz2.firstbyte = -1;
      i++;
    } else {
      if (!fp->u.bz2.eof) {
        last_read =
            BZ2_bzRead(&fp->u.bz2.error, fp->u.bz2.file, buffer + i, 1);
        i += last_read; /* 0 or 1 */
      }
    }
    if (!fp->u.bz2.eof) {
      /* Leave space for trailing zero */
      for (;
           i < size - 1 && fp->u.bz2.error == BZ_OK && buffer[i - 1] != '\n';
           i += last_read) {
        last_read =
            BZ2_bzRead(&fp->u.bz2.error, fp->u.bz2.file, buffer + i, 1);
      }
      if (fp->u.bz2.error != BZ_OK
          && (fp->u.bz2.error != BZ_STREAM_END || i == 0)) {
        retval = NULL;
      } else {
        retval = buffer;
      }
      if (fp->u.bz2.error == BZ_STREAM_END) {
        /* EOF reached. Do not BZ2_bzRead() any more. */
        fp->u.bz2.eof = true;
      }
    }
    buffer[i] = '\0';
    return retval;
  }
#endif /* FREECIV_HAVE_LIBBZ2 */
  case FZ_ZLIB:
    return gzgets(fp->u.zlib, buffer, size);
  case FZ_PLAIN:
    return fgets(buffer, size, fp->u.plain);
  }

  /* Should never happen */
  fc_assert_msg(false, "Internal error in %s() (method = %d)", __FUNCTION__,
                fp->method);
  return NULL;
}

#ifdef FREECIV_HAVE_LIBLZMA

/************************************************************************/ /**
   Helper function to do given compression action and writing
   results from output buffer to file.
 ****************************************************************************/
static bool xz_outbuffer_to_file(fz_FILE *fp, lzma_action action)
{
  do {
    size_t len;
    size_t total = 0;

    fp->u.xz.error = lzma_code(&fp->u.xz.stream, action);

    if (fp->u.xz.error != LZMA_OK && fp->u.xz.error != LZMA_STREAM_END) {
      return false;
    }

    while (total < PLAIN_FILE_BUF_SIZE - fp->u.xz.stream.avail_out) {
      len = fwrite(fp->u.xz.out_buf, 1,
                   PLAIN_FILE_BUF_SIZE - fp->u.xz.stream.avail_out - total,
                   fp->u.xz.plain);
      total += len;
      if (len == 0) {
        return false;
      }
    }
    fp->u.xz.stream.avail_out = PLAIN_FILE_BUF_SIZE;
    fp->u.xz.stream.next_out = fp->u.xz.out_buf;
  } while (fp->u.xz.stream.avail_in > 0);

  return true;
}

/************************************************************************/ /**
   Helper function to do given decompression action.
 ****************************************************************************/
static void xz_action(fz_FILE *fp, lzma_action action)
{
  fp->u.xz.error = lzma_code(&fp->u.xz.stream, action);
  if (fp->u.xz.error != LZMA_MEMLIMIT_ERROR) {
    return;
  }

  while (fp->u.xz.error == LZMA_MEMLIMIT_ERROR
         && fp->u.xz.memlimit < XZ_DECODER_MEMLIMIT_FINAL) {
    fp->u.xz.memlimit += XZ_DECODER_MEMLIMIT_STEP;
    if (fp->u.xz.memlimit > XZ_DECODER_MEMLIMIT_FINAL) {
      fp->u.xz.memlimit = XZ_DECODER_MEMLIMIT_FINAL;
    }
    fp->u.xz.error = lzma_memlimit_set(&fp->u.xz.stream, fp->u.xz.memlimit);
  }

  fp->u.xz.error = lzma_code(&fp->u.xz.stream, action);
}
#endif /* FREECIV_HAVE_LIBLZMA */

/************************************************************************/ /**
   Print formated, like fprintf.

   Note: zlib doesn't have gzvfprintf, but thats ok because its
   fprintf only does similar to what we do here (print to fixed
   buffer), and in addition this way we get to use our safe
   snprintf.

   Returns number of (uncompressed) bytes actually written, or
   0 on error.
 ****************************************************************************/
int fz_fprintf(fz_FILE *fp, const char *format, ...)
{
  int num;
  va_list ap;

  fc_assert_ret_val(NULL != fp, 0);
  fc_assert_ret_val(!fp->memory, 0);

  switch (fz_method_validate(fp->method)) {
#ifdef FREECIV_HAVE_LIBLZMA
  case FZ_XZ: {
    va_start(ap, format);
    num = fc_vsnprintf((char *) fp->u.xz.in_buf, PLAIN_FILE_BUF_SIZE, format,
                       ap);
    va_end(ap);

    if (num == -1) {
      qCritical("Too much data: truncated in fz_fprintf (%u)",
                PLAIN_FILE_BUF_SIZE);
      num = PLAIN_FILE_BUF_SIZE;
    }
    fp->u.xz.stream.next_in = fp->u.xz.in_buf;
    fp->u.xz.stream.avail_in = num;

    if (!xz_outbuffer_to_file(fp, LZMA_RUN)) {
      return 0;
    } else {
      return qstrlen((char *) fp->u.xz.in_buf);
    }
  } break;
#endif /* FREECIV_HAVE_LIBLZMA */
#ifdef FREECIV_HAVE_LIBBZ2
  case FZ_BZIP2: {
    char buffer[65536];

    va_start(ap, format);
    num = fc_vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    if (num == -1) {
      qCritical("Too much data: truncated in fz_fprintf (%lu)",
                (unsigned long) sizeof(buffer));
    }
    BZ2_bzWrite(&fp->u.bz2.error, fp->u.bz2.file, buffer, qstrlen(buffer));
    if (fp->u.bz2.error != BZ_OK) {
      return 0;
    } else {
      return qstrlen(buffer);
    }
  }
#endif /* FREECIV_HAVE_LIBBZ2 */
  case FZ_ZLIB: {
    char buffer[65536];

    va_start(ap, format);
    num = fc_vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    if (num == -1) {
      qCritical("Too much data: truncated in fz_fprintf (%lu)",
                (unsigned long) sizeof(buffer));
    }
    return gzwrite(fp->u.zlib, buffer, (unsigned int) qstrlen(buffer));
  }
  case FZ_PLAIN:
    va_start(ap, format);
    num = vfprintf(fp->u.plain, format, ap);
    va_end(ap);
    return num;
  }

  /* Should never happen */
  fc_assert_msg(false, "Internal error in %s() (method = %d)", __FUNCTION__,
                fp->method);
  return 0;
}

/************************************************************************/ /**
   Return non-zero if there is an error status associated with
   this stream.  Check fz_strerror for details.
 ****************************************************************************/
int fz_ferror(fz_FILE *fp)
{
  fc_assert_ret_val(NULL != fp, 0);

  if (fp->memory) {
    return 0;
  }

  switch (fz_method_validate(fp->method)) {
#ifdef FREECIV_HAVE_LIBLZMA
  case FZ_XZ:
    if (fp->u.xz.error != LZMA_OK && fp->u.xz.error != LZMA_STREAM_END) {
      return 1;
    } else {
      return 0;
    }
    break;
#endif /* FREECIV_HAVE_LZMA */
#ifdef FREECIV_HAVE_LIBBZ2
  case FZ_BZIP2:
    return (BZ_OK != fp->u.bz2.error && BZ_STREAM_END != fp->u.bz2.error);
#endif /* FREECIV_HAVE_LIBBZ2 */
  case FZ_ZLIB: {
    int error;

    (void) gzerror(fp->u.zlib, &error); /* Ignore string result here. */
    return 0 > error ? error : 0; /* Only negative Z values are errors. */
  }
  case FZ_PLAIN:
    return ferror(fp->u.plain);
    break;
  }

  /* Should never happen */
  fc_assert_msg(false, "Internal error in %s() (method = %d)", __FUNCTION__,
                fp->method);
  return 0;
}

/************************************************************************/ /**
   Return string (pointer to static memory) containing an error
   description associated with the file.  Should only call
   this is you know there is an error (eg, from fz_ferror()).
   Note the error string may be based on errno, so should call
   this immediately after problem, or possibly something else
   might overwrite errno.
 ****************************************************************************/
const char *fz_strerror(fz_FILE *fp)
{
  fc_assert_ret_val(NULL != fp, NULL);
  fc_assert_ret_val(!fp->memory, NULL);

  switch (fz_method_validate(fp->method)) {
#ifdef FREECIV_HAVE_LIBLZMA
  case FZ_XZ: {
    static char xzerror[50];
    char *cleartext = NULL;

    switch (fp->u.xz.error) {
    case LZMA_OK:
      cleartext = "OK";
      break;
    case LZMA_STREAM_END:
      cleartext = "Stream end";
      break;
    case LZMA_NO_CHECK:
      cleartext = "No integrity check";
      break;
    case LZMA_UNSUPPORTED_CHECK:
      cleartext = "Cannot calculate the integrity check";
      break;
    case LZMA_MEM_ERROR:
      cleartext = "Mem error";
      break;
    case LZMA_MEMLIMIT_ERROR:
      cleartext = "Memory limit reached";
      break;
    case LZMA_FORMAT_ERROR:
      cleartext = "Unrecognized file format";
      break;
    case LZMA_OPTIONS_ERROR:
      cleartext = "Unsupported options";
      break;
    case LZMA_DATA_ERROR:
      cleartext = "Data error";
      break;
    case LZMA_BUF_ERROR:
      cleartext = "Progress not possible";
      break;
    default:
      break;
    }

    if (NULL != cleartext) {
      fc_snprintf(xzerror, sizeof(xzerror), "XZ: \"%s\" (%d)", cleartext,
                  fp->u.xz.error);
    } else {
      fc_snprintf(xzerror, sizeof(xzerror), "XZ error %d", fp->u.xz.error);
    }
    return xzerror;
  } break;
#endif /* FREECIV_HAVE_LIBLZMA */
#ifdef FREECIV_HAVE_LIBBZ2
  case FZ_BZIP2: {
    static char bzip2error[50];
    const char *cleartext = NULL;

    /* Rationale for translating these:
     * - Some of them provide usable information to user
     * - Messages still contain numerical error code for developers
     */
    switch (fp->u.bz2.error) {
    case BZ_OK:
      cleartext = "OK";
      break;
    case BZ_RUN_OK:
      cleartext = "Run ok";
      break;
    case BZ_FLUSH_OK:
      cleartext = "Flush ok";
      break;
    case BZ_FINISH_OK:
      cleartext = "Finish ok";
      break;
    case BZ_STREAM_END:
      cleartext = "Stream end";
      break;
    case BZ_CONFIG_ERROR:
      cleartext = "Config error";
      break;
    case BZ_SEQUENCE_ERROR:
      cleartext = "Sequence error";
      break;
    case BZ_PARAM_ERROR:
      cleartext = "Parameter error";
      break;
    case BZ_MEM_ERROR:
      cleartext = "Mem error";
      break;
    case BZ_DATA_ERROR:
      cleartext = "Data error";
      break;
    case BZ_DATA_ERROR_MAGIC:
      cleartext = "Not bzip2 file";
      break;
    case BZ_IO_ERROR:
      cleartext = "IO error";
      break;
    case BZ_UNEXPECTED_EOF:
      cleartext = "Unexpected EOF";
      break;
    case BZ_OUTBUFF_FULL:
      cleartext = "Output buffer full";
      break;
    default:
      break;
    }

    if (cleartext != NULL) {
      fc_snprintf(bzip2error, sizeof(bzip2error), "Bz2: \"%s\" (%d)",
                  cleartext, fp->u.bz2.error);
    } else {
      fc_snprintf(bzip2error, sizeof(bzip2error), "Bz2 error %d",
                  fp->u.bz2.error);
    }
    return bzip2error;
  }
#endif /* FREECIV_HAVE_LIBBZ2 */
  case FZ_ZLIB: {
    int errnum;
    const char *estr = gzerror(fp->u.zlib, &errnum);

    return Z_ERRNO == errnum ? fc_strerror(fc_get_errno()) : estr;
  }
  case FZ_PLAIN:
    return fc_strerror(fc_get_errno());
  }

  /* Should never happen */
  fc_assert_msg(false, "Internal error in %s() (method = %d)", __FUNCTION__,
                fp->method);
  return NULL;
}
