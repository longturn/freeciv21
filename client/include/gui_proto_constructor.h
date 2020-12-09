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

#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif

#ifdef GUI_CB_MODE
#define GUI_FUNC_PROTO(_type, _func, ...)                                   \
  EXTERN_C _type gui_##_func(__VA_ARGS__);                                  \
  EXTERN_C _type _func(__VA_ARGS__);
#else
#define GUI_FUNC_PROTO(_type, _func, ...) EXTERN_C _type _func(__VA_ARGS__);
#endif


