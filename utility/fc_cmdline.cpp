/***********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

#ifdef HAVE_CONFIG_H
#include <fc_config.h>
#endif

#include <string.h>

/* utility */
#include "fciconv.h"
#include "fcintl.h"
#include "mem.h"
#include "support.h"

#include "fc_cmdline.h"

/**********************************************************************/ /**
   Like strcspn but also handles quotes, i.e. *reject chars are
   ignored if they are inside single or double quotes.
 **************************************************************************/
static size_t fc_strcspn(const char *s, const char *reject)
{
  bool in_single_quotes = FALSE, in_double_quotes = FALSE;
  size_t i, len = strlen(s);

  for (i = 0; i < len; i++) {
    if (s[i] == '"' && !in_single_quotes) {
      in_double_quotes = !in_double_quotes;
    } else if (s[i] == '\'' && !in_double_quotes) {
      in_single_quotes = !in_single_quotes;
    }

    if (in_single_quotes || in_double_quotes) {
      continue;
    }

    if (strchr(reject, s[i])) {
      break;
    }
  }

  return i;
}

/**********************************************************************/ /**
   Splits the string into tokens. The individual tokens are
   returned. The delimiterset can freely be chosen.

   i.e. "34 abc 54 87" with a delimiterset of " " will yield
        tokens={"34", "abc", "54", "87"}

   Part of the input string can be quoted (single or double) to embedded
   delimiter into tokens. For example,
     command 'a name' hard "1,2,3,4,5" 99
     create 'Mack "The Knife"'
   will yield 5 and 2 tokens respectively using the delimiterset " ,".

   Tokens which aren't used aren't modified (and memory is not
   allocated). If the string would yield more tokens only the first
   num_tokens are extracted.

   The user has the responsiblity to free the memory allocated by
   **tokens using free_tokens().
 **************************************************************************/
unsigned int get_tokens(const char *str, char **tokens, size_t num_tokens,
               const char *delimiterset)
{
  unsigned int token;

  fc_assert_ret_val(NULL != str, -1);

  for (token = 0; token < num_tokens && *str != '\0'; token++) {
    size_t len, padlength = 0;

    /* skip leading delimiters */
    str += strspn(str, delimiterset);

    len = fc_strcspn(str, delimiterset);

    /* strip start/end quotes if they exist */
    if (len >= 2) {
      if ((str[0] == '"' && str[len - 1] == '"')
          || (str[0] == '\'' && str[len - 1] == '\'')) {
        len -= 2;
        padlength = 1; /* to set the string past the end quote */
        str++;
      }
    }

    tokens[token] = static_cast<char *>(fc_malloc(len + 1));
    (void) fc_strlcpy(tokens[token], str, len + 1); /* adds the '\0' */

    str += len + padlength;
  }

  return token;
}

/**********************************************************************/ /**
   Frees a set of tokens created by get_tokens().
 **************************************************************************/
void free_tokens(char **tokens, size_t ntokens)
{
  size_t i;

  for (i = 0; i < ntokens; i++) {
    if (tokens[i]) {
      free(tokens[i]);
    }
  }
}
