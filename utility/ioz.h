/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 __    __          part of Freeciv21. Freeciv21 is free software: you can
/ \\..// \    redistribute it and/or modify it under the terms of the GNU
  ( oo )        General Public License  as published by the Free Software
   \__/         Foundation, either version 3 of the License,  or (at your
                      option) any later version. You should have received
    a copy of the GNU General Public License along with Freeciv21. If not,
                  see https://www.gnu.org/licenses/.
**************************************************************************/

#pragma once

/**********************************************************************
  An IO layer to support transparent compression/uncompression.
  (Currently only "required" functionality is supported.)
***********************************************************************/

/* (Possibly) supported methods (depending on what KArchive supports). */
enum fz_method {
  FZ_PLAIN = 0,
  FZ_ZLIB,
#ifdef FREECIV_HAVE_BZ2
  FZ_BZIP2,
#endif
#ifdef FREECIV_HAVE_LZMA
  FZ_XZ,
#endif
};
