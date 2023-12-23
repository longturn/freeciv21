# - Locate SDL2_mixer library
# This module defines:
#  SDL2_mixer_LIBRARIES, the name of the library to link against
#  SDL2_mixer_INCLUDE_DIRS, where to find the headers
#  SDL2_mixer_FOUND, if false, do not try to link against
#  SDL2_mixer_VERSION_STRING - human-readable string containing the version of SDL2_mixer
#
# $SDLDIR is an environment variable that would
# correspond to the ./configure --prefix=$SDLDIR
# used in building SDL.
#
# Created by Eric Wing. This was influenced by the FindSDL.cmake
# module, but with modifications to recognize OS X frameworks and
# additional Unix paths (FreeBSD, etc).

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
# Copyright 2012 Benjamin Eikel
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(SDL2_mixer_INCLUDE_DIR SDL_mixer.h
  HINTS
    ENV SDLMIXERDIR
    ENV SDLDIR
  PATH_SUFFIXES include/SDL2 include/SDL2.0 include
)

find_library(SDL2_mixer_LIBRARY
  NAMES SDL2_mixer
  HINTS
    ENV SDLMIXERDIR
    ENV SDLDIR
  PATH_SUFFIXES lib
)

if(SDL2_mixer_INCLUDE_DIR AND EXISTS "${SDL2_mixer_INCLUDE_DIR}/SDL2_mixer.h")
  file(STRINGS "${SDL2_mixer_INCLUDE_DIR}/SDL2_mixer.h" SDL2_mixer_VERSION_MAJOR_LINE REGEX "^#define[ \t]+SDL2_MIXER_MAJOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_mixer_INCLUDE_DIR}/SDL2_mixer.h" SDL2_mixer_VERSION_MINOR_LINE REGEX "^#define[ \t]+SDL2_MIXER_MINOR_VERSION[ \t]+[0-9]+$")
  file(STRINGS "${SDL2_mixer_INCLUDE_DIR}/SDL2_mixer.h" SDL2_mixer_VERSION_PATCH_LINE REGEX "^#define[ \t]+SDL2_MIXER_PATCHLEVEL[ \t]+[0-9]+$")
  string(REGEX REPLACE "^#define[ \t]+SDL2_MIXER_MAJOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_mixer_VERSION_MAJOR "${SDL2_mixer_VERSION_MAJOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL2_MIXER_MINOR_VERSION[ \t]+([0-9]+)$" "\\1" SDL2_mixer_VERSION_MINOR "${SDL2_mixer_VERSION_MINOR_LINE}")
  string(REGEX REPLACE "^#define[ \t]+SDL2_MIXER_PATCHLEVEL[ \t]+([0-9]+)$" "\\1" SDL2_mixer_VERSION_PATCH "${SDL2_mixer_VERSION_PATCH_LINE}")
  set(SDL2_mixer_VERSION_STRING ${SDL2_mixer_VERSION_MAJOR}.${SDL2_mixer_VERSION_MINOR}.${SDL2_mixer_VERSION_PATCH})
  unset(SDL2_mixer_VERSION_MAJOR_LINE)
  unset(SDL2_mixer_VERSION_MINOR_LINE)
  unset(SDL2_mixer_VERSION_PATCH_LINE)
  unset(SDL2_mixer_VERSION_MAJOR)
  unset(SDL2_mixer_VERSION_MINOR)
  unset(SDL2_mixer_VERSION_PATCH)
endif()

set(SDL2_mixer_LIBRARIES ${SDL2_mixer_LIBRARY})
set(SDL2_mixer_INCLUDE_DIRS ${SDL2_mixer_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)

FIND_PACKAGE_HANDLE_STANDARD_ARGS(SDL2_mixer
                                  REQUIRED_VARS SDL2_mixer_LIBRARIES SDL2_mixer_INCLUDE_DIRS
                                  VERSION_VAR SDL2_mixer_VERSION_STRING)

if (SDL2_mixer_FOUND)
  add_library(SDL2_mixer::SDL2_mixer UNKNOWN IMPORTED)
  set_target_properties(SDL2_mixer::SDL2_mixer PROPERTIES
                        IMPORTED_LOCATION "${SDL2_mixer_LIBRARIES}"
                        INTERFACE_INCLUDE_DIRECTORIES "${SDL2_mixer_INCLUDE_DIR}")
endif()

mark_as_advanced(SDL2_mixer_LIBRARY SDL2_mixer_INCLUDE_DIR)

