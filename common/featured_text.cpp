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
#include <cstdarg>
#include <cstring>

// utility
#include "fcintl.h"
#include "log.h"
#include "shared.h"
#include "support.h"

// common
#include "city.h"
#include "combat.h"
#include "game.h"
#include "map.h"
#include "tile.h"
#include "unit.h"

#include "featured_text.h"

#define SEQ_START '['
#define SEQ_STOP ']'
#define SEQ_END '/'

#define MAX_LEN_STR 32
#define log_featured_text qDebug

#define text_tag_list_rev_iterate(tags, ptag)                               \
  TYPED_LIST_ITERATE_REV(struct text_tag, tags, ptag)
#define text_tag_list_rev_iterate_end LIST_ITERATE_REV_END

// The text_tag structure.  See documentation in featured_text.h.
struct text_tag {
  enum text_tag_type type;  // The type of the tag.
  ft_offset_t start_offset; // The start offset (in bytes).
  ft_offset_t stop_offset;  // The stop offset (in bytes).
  union {
    struct {                        // TTT_COLOR only.
      char foreground[MAX_LEN_STR]; // foreground color name.
      char background[MAX_LEN_STR]; // background color name.
    } color;
    struct {                    // TTT_LINK only.
      enum text_link_type type; // The target type of the link.
      int id;                   // The id of linked object.
      char name[MAX_LEN_STR];   // A string to indentify the link.
    } link;
  };
};

enum sequence_type {
  ST_START, // e.g. [sequence].
  ST_STOP,  /* e.g. [/sequence]. */
  ST_SINGLE /* e.g. [sequence/]. */
};

// Predefined colors.
const struct ft_color ftc_any = FT_COLOR(nullptr, nullptr);

const struct ft_color ftc_warning = FT_COLOR("#FF0000", nullptr);
const struct ft_color ftc_log = FT_COLOR("#7F7F7F", nullptr);
const struct ft_color ftc_server = FT_COLOR("#8B0000", nullptr);
const struct ft_color ftc_client = FT_COLOR("#EF7F00", nullptr);
const struct ft_color ftc_editor = FT_COLOR("#0000FF", nullptr);
const struct ft_color ftc_command = FT_COLOR("#006400", nullptr);
struct ft_color ftc_changed = FT_COLOR("#FF0000", nullptr);
const struct ft_color ftc_server_prompt = FT_COLOR("#FF0000", "#BEBEBE");
const struct ft_color ftc_player_lost = FT_COLOR("#FFFFFF", "#000000");
const struct ft_color ftc_game_start = FT_COLOR("#00FF00", "#115511");

const struct ft_color ftc_chat_public = FT_COLOR("#00008B", nullptr);
const struct ft_color ftc_chat_ally = FT_COLOR("#551166", nullptr);
const struct ft_color ftc_chat_private = FT_COLOR("#A020F0", nullptr);
const struct ft_color ftc_chat_luaconsole = FT_COLOR("#006400", nullptr);

const struct ft_color ftc_vote_public = FT_COLOR("#FFFFFF", "#AA0000");
const struct ft_color ftc_vote_team = FT_COLOR("#FFFFFF", "#5555CC");
const struct ft_color ftc_vote_passed = FT_COLOR("#006400", "#AAFFAA");
const struct ft_color ftc_vote_failed = FT_COLOR("#8B0000", "#FFAAAA");
const struct ft_color ftc_vote_yes = FT_COLOR("#000000", "#C8FFD5");
const struct ft_color ftc_vote_no = FT_COLOR("#000000", "#FFD2D2");
const struct ft_color ftc_vote_abstain = FT_COLOR("#000000", "#E8E8E8");

const struct ft_color ftc_luaconsole_input = FT_COLOR("#2B008B", nullptr);
const struct ft_color ftc_luaconsole_error = FT_COLOR("#FF0000", nullptr);
const struct ft_color ftc_luaconsole_warn = FT_COLOR("#CF2020", nullptr);
const struct ft_color ftc_luaconsole_normal = FT_COLOR("#006400", nullptr);
const struct ft_color ftc_luaconsole_verbose = FT_COLOR("#B8B8B8", nullptr);

/**
   Return the long name of the text tag type.
   See also text_tag_type_short_name().
 */
static const char *text_tag_type_name(enum text_tag_type type)
{
  switch (type) {
  case TTT_BOLD:
    return "bold";
  case TTT_ITALIC:
    return "italic";
  case TTT_STRIKE:
    return "strike";
  case TTT_UNDERLINE:
    return "underline";
  case TTT_COLOR:
    return "color";
  case TTT_LINK:
    return "link";
  case TTT_INVALID:
    return nullptr;
  };
  // Don't handle the default case to be warned if a new value was added.
  return nullptr;
}

/**
   Return the name abbreviation of the text tag type.
   See also text_tag_type_name().
 */
static const char *text_tag_type_short_name(enum text_tag_type type)
{
  switch (type) {
  case TTT_BOLD:
    return "b";
  case TTT_ITALIC:
    return "i";
  case TTT_STRIKE:
    return "s";
  case TTT_UNDERLINE:
    return "u";
  case TTT_COLOR:
    return "c";
  case TTT_LINK:
    return "l";
  case TTT_INVALID:
    return nullptr;
  };
  // Don't handle the default case to be warned if a new value was added.
  return nullptr;
}

/**
   Return the name of the text tag link target type.
 */
static const char *text_link_type_name(enum text_link_type type)
{
  switch (type) {
  case TLT_CITY:
    return "city";
  case TLT_TILE:
    return "tile";
  case TLT_UNIT:
    return "unit";
  case TLT_INVALID:
    fc_assert_ret_val(type != TLT_INVALID, nullptr);
  };
  // Don't handle the default case to be warned if a new value was added.
  return nullptr;
}

/**
   Find inside a sequence the string associated to a particular option name.
   Returns TRUE on success.
 */
static bool find_option(const char *buf_in, const char *option,
                        char *buf_out, size_t write_len)
{
  size_t option_len = qstrlen(option);
  const char *buf_in_ptr = buf_in;
  int len = qstrlen(buf_in);

  while (*buf_in != '\0' && (buf_in - buf_in_ptr) < len) {
    while (QChar::isSpace(*buf_in) && *buf_in != '\0') {
      buf_in++;
    }

    if (0 == fc_strncasecmp(buf_in, option, option_len)) {
      buf_in += option_len; // This is this one.

      while ((QChar::isSpace(*buf_in) || *buf_in == '=')
             && *buf_in != '\0') {
        buf_in++;
      }
      if (*buf_in == '"') { // Quote case.
        const char *end = strchr(++buf_in, '"');

        if (!end) {
          return false;
        }
        if (end - buf_in + 1 > 0) {
          fc_strlcpy(buf_out, buf_in, MIN(end - buf_in + 1, write_len));
        } else {
          *buf_out = '\0';
        }
        return true;
      } else {
        while (QChar::isLetterOrNumber(*buf_in) && write_len > 1) {
          *buf_out++ = *buf_in++;
          write_len--;
        }
        *buf_out = '\0';
        return true;
      }
    }
    buf_in++;
  }
  return false;
}

/**
   Initialize a text_tag structure from a string sequence.
   Returns TRUE on success.
 */
static bool text_tag_init_from_sequence(struct text_tag *ptag,
                                        enum text_tag_type type,
                                        ft_offset_t start_offset,
                                        QString &qsequence)
{
  ptag->type = type;
  ptag->start_offset = start_offset;
  ptag->stop_offset = FT_OFFSET_UNSET;

  QByteArray ba = qsequence.toLocal8Bit();
  const char *sequence = ba.constData();

  switch (type) {
  case TTT_BOLD:
  case TTT_ITALIC:
  case TTT_STRIKE:
  case TTT_UNDERLINE:
    return true;
  case TTT_COLOR:
    if (!find_option(sequence, "foreground", ptag->color.foreground,
                     sizeof(ptag->color.foreground))
        && !find_option(sequence, "fg", ptag->color.foreground,
                        sizeof(ptag->color.foreground))) {
      ptag->color.foreground[0] = '\0';
    }
    if (!find_option(sequence, "background", ptag->color.background,
                     sizeof(ptag->color.background))
        && !find_option(sequence, "bg", ptag->color.background,
                        sizeof(ptag->color.background))) {
      ptag->color.background[0] = '\0';
    }
    return true;
    break;
  case TTT_LINK: {
    char buf[64];
    const char *name;
    int i;

    if (!find_option(sequence, "target", buf, sizeof(buf))
        && !find_option(sequence, "tgt", buf, sizeof(buf))) {
      log_featured_text("text_tag_init_from_sequence(): "
                        "target link type not set.");
      return false;
    }

    ptag->link.type = TLT_INVALID;
    for (i = 0; (name = text_link_type_name(text_link_type(i))); i++) {
      if (0 == fc_strncasecmp(buf, name, qstrlen(name))) {
        ptag->link.type = text_link_type(i);
        break;
      }
    }
    if (ptag->link.type == TLT_INVALID) {
      log_featured_text("text_tag_init_from_sequence(): "
                        "target link type not supported (\"%s\").",
                        buf);
      return false;
    }

    switch (ptag->link.type) {
    case TLT_CITY:
      if (!find_option(sequence, "id", buf, sizeof(buf))) {
        log_featured_text("text_tag_init_from_sequence(): "
                          "city link without id.");
        return false;
      }
      if (!str_to_int(buf, &ptag->link.id)) {
        log_featured_text("text_tag_init_from_sequence(): "
                          "city link without valid id (\"%s\").",
                          buf);
        return false;
      }

      if (!find_option(sequence, "name", ptag->link.name,
                       sizeof(ptag->link.name))) {
        // Set something as name.
        fc_snprintf(ptag->link.name, sizeof(ptag->link.name), "CITY_ID%d",
                    ptag->link.id);
      }
      return true;
    case TLT_TILE:
      struct tile *ptile;
      int x, y;

      if (!find_option(sequence, "x", buf, sizeof(buf))) {
        log_featured_text("text_tag_init_from_sequence(): "
                          "tile link without x coordinate.");
        return false;
      }
      if (!str_to_int(buf, &x)) {
        log_featured_text("text_tag_init_from_sequence(): "
                          "tile link without valid x coordinate "
                          "(\"%s\").",
                          buf);
        return false;
      }

      if (!find_option(sequence, "y", buf, sizeof(buf))) {
        log_featured_text("text_tag_init_from_sequence(): "
                          "tile link without y coordinate.");
        return false;
      }
      if (!str_to_int(buf, &y)) {
        log_featured_text("text_tag_init_from_sequence(): "
                          "tile link without valid y coordinate "
                          "(\"%s\").",
                          buf);
        return false;
      }

      ptile = map_pos_to_tile(&(wld.map), x, y);
      if (!ptile) {
        log_featured_text("text_tag_init_from_sequence(): "
                          "(%d, %d) are not valid coordinates "
                          "in this game.",
                          x, y);
        return false;
      }
      ptag->link.id = tile_index(ptile);
      fc_snprintf(ptag->link.name, sizeof(ptag->link.name), "(%d, %d)",
                  TILE_XY(ptile));
      return true;
    case TLT_UNIT:
      if (!find_option(sequence, "id", buf, sizeof(buf))) {
        log_featured_text("text_tag_init_from_sequence(): "
                          "unit link without id.");
        return false;
      }
      if (!str_to_int(buf, &ptag->link.id)) {
        log_featured_text("text_tag_init_from_sequence(): "
                          "unit link without valid id (\"%s\").",
                          buf);
        return false;
      }

      if (!find_option(sequence, "name", ptag->link.name,
                       sizeof(ptag->link.name))) {
        // Set something as name.
        fc_snprintf(ptag->link.name, sizeof(ptag->link.name), "UNIT_ID%d",
                    ptag->link.id);
      }
      return true;
    case TLT_INVALID:
      fc_assert_ret_val(ptag->link.type != TLT_INVALID, false);
      break;
    };
  } break;
  case TTT_INVALID:
    fc_assert(type != TTT_INVALID); // return below
  };
  return false;
}

/**
   Initialize a text_tag structure from a va_list.

   What's should be in the va_list:
   - If the text tag type is TTT_BOLD, TTT_ITALIC, TTT_STRIKE or
   TTT_UNDERLINE, there shouldn't be any extra argument.
   - If the text tag type is TTT_COLOR, then there should be 1 argument of
   type 'struct ft_color'.
   - If the text tag type is TTT_LINK, then there should be 2 extra
 arguments. The first is type 'enum text_link_type' and will determine the
 type of the following argument:
     - If the link type is TLT_CITY, last argument is typed 'struct city *'.
     - If the link type is TLT_TILE, last argument is typed 'struct tile *'.
     - If the link type is TLT_UNIT, last argument is typed 'struct unit *'.

   Returns TRUE on success.
 */
static bool text_tag_initv(struct text_tag *ptag, enum text_tag_type type,
                           ft_offset_t start_offset, ft_offset_t stop_offset,
                           va_list args)
{
  ptag->type = type;
  ptag->start_offset = start_offset;
  ptag->stop_offset = stop_offset;

  switch (type) {
  case TTT_BOLD:
  case TTT_ITALIC:
  case TTT_STRIKE:
  case TTT_UNDERLINE:
    return true;
  case TTT_COLOR: {
    const struct ft_color color = va_arg(args, struct ft_color);

    if ((nullptr == color.foreground || '\0' == color.foreground[0])
        && (nullptr == color.background || '\0' == color.background[0])) {
      return false; // No color at all.
    }

    if (nullptr != color.foreground && '\0' != color.foreground[0]) {
      sz_strlcpy(ptag->color.foreground, color.foreground);
    } else {
      ptag->color.foreground[0] = '\0';
    }

    if (nullptr != color.background && '\0' != color.background[0]) {
      sz_strlcpy(ptag->color.background, color.background);
    } else {
      ptag->color.background[0] = '\0';
    }
  }
    return true;
    break;
  case TTT_LINK: {
    ptag->link.type = text_link_type(va_arg(args, int));
    switch (ptag->link.type) {
    case TLT_CITY: {
      struct city *pcity = va_arg(args, struct city *);

      if (!pcity) {
        return false;
      }
      ptag->link.id = pcity->id;
      sz_strlcpy(ptag->link.name, city_name_get(pcity));
    }
      return true;
    case TLT_TILE: {
      struct tile *ptile = va_arg(args, struct tile *);

      if (!ptile) {
        return false;
      }
      ptag->link.id = tile_index(ptile);
      fc_snprintf(ptag->link.name, sizeof(ptag->link.name), "(%d, %d)",
                  TILE_XY(ptile));
    }
      return true;
    case TLT_UNIT: {
      struct unit *punit = va_arg(args, struct unit *);

      if (!punit) {
        return false;
      }
      ptag->link.id = punit->id;
      sz_strlcpy(ptag->link.name, unit_name_translation(punit));
    }
      return true;
    case TLT_INVALID:
      fc_assert_ret_val(ptag->link.type != TLT_INVALID, false);
    };
  } break;
  case TTT_INVALID:
    fc_assert(type != TTT_INVALID); // return below
  };
  return false;
}

/**
   Print in a string the start sequence of the tag.
 */
static size_t text_tag_start_sequence(const struct text_tag *ptag, char *buf,
                                      size_t len)
{
  switch (ptag->type) {
  case TTT_BOLD:
  case TTT_ITALIC:
  case TTT_STRIKE:
  case TTT_UNDERLINE:
    return fc_snprintf(buf, len, "%c%s%c", SEQ_START,
                       text_tag_type_short_name(ptag->type), SEQ_STOP);
  case TTT_COLOR: {
    size_t ret = fc_snprintf(buf, len, "%c%s", SEQ_START,
                             text_tag_type_short_name(ptag->type));

    if (ptag->color.foreground[0] != '\0') {
      ret += fc_snprintf(buf + ret, len - ret, " fg=\"%s\"",
                         ptag->color.foreground);
    }
    if (ptag->color.background[0] != '\0') {
      ret += fc_snprintf(buf + ret, len - ret, " bg=\"%s\"",
                         ptag->color.background);
    }
    return ret + fc_snprintf(buf + ret, len - ret, "%c", SEQ_STOP);
  } break;
  case TTT_LINK: {
    size_t ret = fc_snprintf(buf, len, "%c%s tgt=\"%s\"", SEQ_START,
                             text_tag_type_short_name(ptag->type),
                             text_link_type_name(ptag->link.type));

    switch (ptag->link.type) {
    case TLT_CITY: {
      struct city *pcity = game_city_by_number(ptag->link.id);

      if (pcity) {
        ret += fc_snprintf(buf + ret, len - ret, " id=%d name=\"%s\"",
                           pcity->id, city_name_get(pcity));
      } else {
        ret += fc_snprintf(buf + ret, len - ret, " id=%d", ptag->link.id);
      }
    } break;
    case TLT_TILE: {
      struct tile *ptile = index_to_tile(&(wld.map), ptag->link.id);

      if (ptile) {
        ret +=
            fc_snprintf(buf + ret, len - ret, " x=%d y=%d", TILE_XY(ptile));
      } else {
        ret += fc_snprintf(buf + ret, len - ret, " id=%d", ptag->link.id);
      }
    } break;
    case TLT_UNIT: {
      struct unit *punit = game_unit_by_number(ptag->link.id);

      if (punit) {
        ret += fc_snprintf(buf + ret, len - ret, " id=%d name=\"%s\"",
                           punit->id, unit_name_translation(punit));
      } else {
        ret += fc_snprintf(buf + ret, len - ret, " id=%d", ptag->link.id);
      }
    } break;
    case TLT_INVALID:
      fc_assert_ret_val(ptag->link.type != TLT_INVALID, 0);
    };

    if (ptag->stop_offset == ptag->start_offset) {
      /* This is a single sequence like [link ... /]. */
      ret += fc_snprintf(buf + ret, len - ret, "%c", SEQ_END);
    }

    return ret + fc_snprintf(buf + ret, len - ret, "%c", SEQ_STOP);
  }
  case TTT_INVALID:
    fc_assert(ptag->type != TTT_INVALID); // return below
  };
  return 0;
}

/**
   Print in a string the stop sequence of the tag.
 */
static size_t text_tag_stop_sequence(const struct text_tag *ptag, char *buf,
                                     size_t len)
{
  if (ptag->type == TTT_LINK && ptag->stop_offset == ptag->start_offset) {
    // Should be already finished.
    return 0;
  }

  return fc_snprintf(buf, len, "%c%c%s%c", SEQ_START, SEQ_END,
                     text_tag_type_short_name(ptag->type), SEQ_STOP);
}

/**
   When the sequence looks like [sequence/] then we insert a string instead.
 */
static size_t text_tag_replace_text(const struct text_tag *ptag, char *buf,
                                    size_t len, bool replace_link_text)
{
  if (ptag->type != TTT_LINK) {
    return 0;
  }

  if (replace_link_text) {
    // The client might check if this should be updated or translated.
    switch (ptag->link.type) {
    case TLT_CITY: {
      struct city *pcity = game_city_by_number(ptag->link.id);

      /* Note that if city_tile(pcity) is nullptr, then it is probably an
       * invisible city (see client/packhand.c).  Then, we don't
       * use the current city name which is usually not complete,
       * a dumb string using the city id. */
      if (nullptr != pcity && nullptr != city_tile(pcity)) {
        return fc_snprintf(buf, len, "%s", city_name_get(pcity));
      }
    } break;
    case TLT_TILE:
      break;
    case TLT_UNIT: {
      struct unit *punit = game_unit_by_number(ptag->link.id);

      if (punit) {
        return fc_snprintf(buf, len, "%s", unit_name_translation(punit));
      }
    } break;
    case TLT_INVALID:
      fc_assert_ret_val(ptag->link.type != TLT_INVALID, false);
    };
  }

  if (ptag->link.type == TLT_UNIT) {
    // Attempt to translate the link name (it should be a unit type name).
    return fc_snprintf(buf, len, "%s", _(ptag->link.name));
  } else {
    return fc_snprintf(buf, len, "%s", ptag->link.name);
  }
}

/**
   Returns a new text_tag or nullptr on error.

   Prototype:
   - If tag_type is TTT_BOLD, TTT_ITALIC, TTT_STRIKE or TTT_UNDERLINE, there
   shouldn't be any extra argument.
   - If tag_type is TTT_COLOR:
     struct text_tag *text_tag_new(..., const struct ft_color color);
   - If tag_type is TTT_LINK and you want a city link:
     struct text_tag *text_tag_new(..., TLT_CITY, struct city *pcity);
   - If tag_type is TTT_LINK and you want a tile link:
     struct text_tag *text_tag_new(..., TLT_TILE, struct tile *ptile);
   - If tag_type is TTT_LINK and you want an unit link:
     struct text_tag *text_tag_new(..., TLT_UNIT, struct unit *punit);

   See also comment for text_tag_initv().
 */
struct text_tag *text_tag_new(enum text_tag_type tag_type,
                              ft_offset_t start_offset,
                              ft_offset_t stop_offset, ...)
{
  auto *ptag = new text_tag;
  va_list args;
  bool ok;

  va_start(args, stop_offset);
  ok = text_tag_initv(ptag, tag_type, start_offset, stop_offset, args);
  va_end(args);

  if (ok) {
    return ptag;
  } else {
    delete ptag;
    return nullptr;
  }
}

/**
   This function returns a new pointer to a text_tag which is similar
   to the 'ptag' argument.
 */
struct text_tag *text_tag_copy(const struct text_tag *ptag)
{
  if (!ptag) {
    return nullptr;
  }

  auto *pnew_tag = new text_tag;
  *pnew_tag = *ptag;

  return pnew_tag;
}

/**
   Free a text_tag structure.
 */
void text_tag_destroy(struct text_tag *ptag) { delete ptag; }

/**
   Return the type of this text tag.
 */
enum text_tag_type text_tag_type(const struct text_tag *ptag)
{
  return ptag->type;
}

/**
   Return the start offset (in bytes) of this text tag.
 */
ft_offset_t text_tag_start_offset(const struct text_tag *ptag)
{
  return ptag->start_offset;
}

/**
   Return the stop offset (in bytes) of this text tag.
 */
ft_offset_t text_tag_stop_offset(const struct text_tag *ptag)
{
  return ptag->stop_offset;
}

/**
   Return the foreground color suggested by this text tag.  This requires
   the tag type to be TTT_COLOR.  Returns nullptr on error, "" if unset.
 */
QString text_tag_color_foreground(const struct text_tag *ptag)
{
  if (ptag->type != TTT_COLOR) {
    qCritical("text_tag_color_foreground(): incompatible tag type.");
    return QStringLiteral();
  }

  return ptag->color.foreground;
}

/**
   Return the background color suggested by this text tag.  This requires
   the tag type to be TTT_COLOR.  Returns nullptr on error, "" if unset.
 */
QString text_tag_color_background(const struct text_tag *ptag)
{
  if (ptag->type != TTT_COLOR) {
    qCritical("text_tag_color_background(): incompatible tag type.");
    return QStringLiteral();
  }

  return ptag->color.background;
}

/**
   Return the link target type suggested by this text tag.  This requires
   the tag type to be TTT_LINK.  Returns -1 on error.
 */
enum text_link_type text_tag_link_type(const struct text_tag *ptag)
{
  if (ptag->type != TTT_LINK) {
    qCritical("text_tag_link_type(): incompatible tag type.");
    return TLT_INVALID;
  }

  return ptag->link.type;
}

/**
   Return the link target id suggested by this text tag (city id,
   tile index or unit id).  This requires the tag type to be TTT_LINK.
   Returns -1 on error.
 */
int text_tag_link_id(const struct text_tag *ptag)
{
  if (ptag->type != TTT_LINK) {
    qCritical("text_tag_link_id(): incompatible tag type.");
    return TTT_INVALID;
  }

  return ptag->link.id;
}

/**
   Extract a sequence from a string.  Also, determine the type and the text
   tag type of the sequence.  Return 0 on error.
 */
static size_t extract_sequence_text(const char *featured_text, QString &buf,
                                    size_t len, enum sequence_type *seq_type,
                                    enum text_tag_type *type)
{
  const char *buf_in = featured_text;
  const char *stop = strchr(buf_in, SEQ_STOP);
  const char *end = stop;
  const char *name;
  size_t type_len;
  size_t name_len;
  int i;

  if (!stop) {
    return 0; // Not valid.
  }

  // Check sequence type.
  for (buf_in++; QChar::isSpace(*buf_in); buf_in++) {
    ;
  }

  if (*buf_in == SEQ_END) {
    *seq_type = ST_STOP;
    buf_in++;
  } else {
    for (end--; QChar::isSpace(*end); end--) {
      ;
    }

    if (*end == SEQ_END) {
      *seq_type = ST_SINGLE;

      for (end--; QChar::isSpace(*end); end--) {
        ;
      }
    } else {
      *seq_type = ST_START;
    }
  }

  while (QChar::isSpace(*buf_in)) {
    buf_in++;
  }

  // Check the length of the type name.
  for (name = buf_in; name < stop; name++) {
    if (!QChar::isLetter(*name)) {
      break;
    }
  }
  type_len = name - buf_in;

  *type = TTT_INVALID;
  for (i = 0;
       (name = text_tag_type_name(static_cast<enum text_tag_type>(i)));
       i++) {
    name_len = qstrlen(name);
    if (name_len == type_len
        && 0 == fc_strncasecmp(name, buf_in, name_len)) {
      buf_in += name_len;
      *type = static_cast<enum text_tag_type>(i);
      break;
    }
  }
  if (*type == TTT_INVALID) {
    // Try with short names.
    for (i = 0; (name = text_tag_type_short_name(
                     static_cast<enum text_tag_type>(i)));
         i++) {
      name_len = qstrlen(name);
      if (name_len == type_len
          && 0 == fc_strncasecmp(name, buf_in, name_len)) {
        buf_in += name_len;
        *type = static_cast<enum text_tag_type>(i);
        break;
      }
    }
    if (*type == TTT_INVALID) {
      return 0; // Not valid.
    }
  }

  while (QChar::isSpace(*buf_in)) {
    buf_in++;
  }

  if (end - buf_in + 2 > 0) {
    buf = QString(buf_in).left(MIN(end - buf_in + 2, len));
  } else {
    buf = QLatin1String("");
  }
  return stop - featured_text + 1;
}

/**
   Separate the text from the text features.  'tags' can be nullptr.

   When 'replace_link_text' is set, the text used for the signal sequence
   links will be overwritten. It is used on client side to have updated
   links in chatline, to communicate when users don't know share the city
   names, and avoid users making voluntary confusing names when editing
   links in chatline.
 */
size_t featured_text_to_plain_text(const char *featured_text,
                                   char *plain_text, size_t plain_text_len,
                                   struct text_tag_list **tags,
                                   bool replace_link_text)
{
  const char *text_in = featured_text;
  char *text_out = plain_text;
  size_t text_out_len = plain_text_len;

  if (tags) {
    *tags = text_tag_list_new();
  }

  while (*text_in != '\0' && text_out_len > 1) {
    if (SEQ_START == *text_in) {
      // Escape sequence...
      QString buf;
      enum sequence_type seq_type;
      enum text_tag_type type;
      size_t len = extract_sequence_text(text_in, buf, text_out_len,
                                         &seq_type, &type);

      if (len > 0) {
        // Looks a valid sequence.
        text_in += len;
        switch (seq_type) {
        case ST_START:
          if (tags) {
            // Create a new tag.
            text_tag *ptag = new text_tag;

            if (text_tag_init_from_sequence(ptag, type,
                                            text_out - plain_text, buf)) {
              text_tag_list_append(*tags, ptag);
            } else {
              text_tag_destroy(ptag);
              log_featured_text("Couldn't create a text tag with \"%s\".",
                                qUtf8Printable(buf));
            }
          }
          break;
        case ST_STOP:
          if (tags) {
            // Set the stop offset.
            struct text_tag *ptag = nullptr;

            // Look up on reversed order.
            text_tag_list_rev_iterate(*tags, piter)
            {
              if (piter->type == type
                  && piter->stop_offset == FT_OFFSET_UNSET) {
                ptag = piter;
                break;
              }
            }
            text_tag_list_rev_iterate_end;

            if (ptag) {
              ptag->stop_offset = text_out - plain_text;
            } else {
              log_featured_text("Extra text tag end for \"%s\".",
                                text_tag_type_name(type));
            }
          }
          break;
        case ST_SINGLE: {
          // In this case, we replace the sequence by some text.
          struct text_tag tag;

          if (!text_tag_init_from_sequence(&tag, type, text_out - plain_text,
                                           buf)) {
            log_featured_text("Couldn't create a text tag with \"%s\".",
                              qUtf8Printable(buf));
          } else {
            len = text_tag_replace_text(&tag, text_out, text_out_len,
                                        replace_link_text);
            text_out += len;
            text_out_len -= len;
            if (tags) {
              // Set it in the list.
              auto *ptag = new text_tag;

              *ptag = tag;
              ptag->stop_offset = text_out - plain_text;
              text_tag_list_append(*tags, ptag);
            }
          }
        } break;
        };
      } else {
        *text_out++ = *text_in++;
        text_out_len--;
      }
    } else {
      *text_out++ = *text_in++;
      text_out_len--;
    }
  }
  *text_out = '\0';

  return plain_text_len - text_out_len;
}

/**
   Apply a tag to a text.  This text can already containing escape
   sequences.  Returns 0 on error.

   Prototype:
   - If tag_type is TTT_BOLD, TTT_ITALIC, TTT_STRIKE or TTT_UNDERLINE, there
   shouldn't be any extra argument.
   - If tag_type is TTT_COLOR:
     size_t featured_text_apply_tag(..., const struct ft_color color);
   - If tag_type is TTT_LINK and you want a city link:
     size_t featured_text_apply_tag(..., TLT_CITY, struct city *pcity);
   - If tag_type is TTT_LINK and you want a tile link:
     size_t featured_text_apply_tag(..., TLT_TILE, struct tile *ptile);
   - If tag_type is TTT_LINK and you want an unit link:
     size_t featured_text_apply_tag(..., TLT_UNIT, struct unit *punit);

   See also comment for text_tag_initv().
 */
size_t featured_text_apply_tag(const char *text_source, char *featured_text,
                               size_t featured_text_len,
                               enum text_tag_type tag_type,
                               ft_offset_t start_offset,
                               ft_offset_t stop_offset, ...)
{
  struct text_tag tag;
  size_t len, total_len = 0;
  va_list args;

  if (start_offset == FT_OFFSET_UNSET || start_offset > qstrlen(text_source)
      || (stop_offset != FT_OFFSET_UNSET && stop_offset < start_offset)) {
    log_featured_text("featured_text_apply_tag(): invalid offsets.");
    return 0;
  }

  va_start(args, stop_offset);
  if (!text_tag_initv(&tag, tag_type, start_offset, stop_offset, args)) {
    va_end(args);
    return 0;
  }
  va_end(args);

  if (start_offset > 0) {
    // First part: before the sequence.
    len = 0;
    while (len < start_offset && *text_source != '\0'
           && featured_text_len > 1) {
      *featured_text++ = *text_source++;
      featured_text_len--;
      len++;
    }
    total_len += len;
  }

  // Start sequence.
  len = text_tag_start_sequence(&tag, featured_text, featured_text_len);
  total_len += len;
  featured_text += len;
  featured_text_len -= len;

  // Second part: between the sequences.
  len = start_offset;
  while (len < stop_offset && *text_source != '\0'
         && featured_text_len > 1) {
    *featured_text++ = *text_source++;
    featured_text_len--;
    len++;
  }
  total_len += len;

  // Stop sequence.
  len = text_tag_stop_sequence(&tag, featured_text, featured_text_len);
  total_len += len;
  featured_text += len;
  featured_text_len -= len;

  // Third part: after the sequence.
  while (*text_source != '\0' && featured_text_len > 1) {
    *featured_text++ = *text_source++;
    featured_text_len--;
    total_len++;
  }
  *featured_text = '\0';

  return total_len;
}

/**
   Get a text link to a city.
   N.B.: The returned string is static, so every call to this function
   overwrites the previous.
 */
const char *city_link(const struct city *pcity)
{
  static char buf[MAX_LEN_LINK];

  fc_snprintf(buf, sizeof(buf), "%c%s tgt=\"%s\" id=%d name=\"%s\" %c%c",
              SEQ_START, text_tag_type_short_name(TTT_LINK),
              text_link_type_name(TLT_CITY), pcity->id, city_name_get(pcity),
              SEQ_END, SEQ_STOP);
  return buf;
}

/**
   Get a text link to a city tile (make a clickable link to a tile with the
   city name as text).
   N.B.: The returned string is static, so every call to this function
   overwrites the previous.
 */
const char *city_tile_link(const struct city *pcity)
{
  static char buf[MAX_LEN_LINK];
  const char *tag_name = text_tag_type_short_name(TTT_LINK);

  fc_snprintf(buf, sizeof(buf), "%c%s tgt=\"%s\" x=%d y=%d%c%s%c%c%s%c",
              SEQ_START, tag_name, text_link_type_name(TLT_TILE),
              TILE_XY(city_tile(pcity)), SEQ_STOP, city_name_get(pcity),
              SEQ_START, SEQ_END, tag_name, SEQ_STOP);
  return buf;
}

/**
   Get a text link to a tile.
   N.B.: The returned string is static, so every call to this function
   overwrites the previous.
 */
const char *tile_link(const struct tile *ptile)
{
  static char buf[MAX_LEN_LINK];

  fc_snprintf(buf, sizeof(buf), "%c%s tgt=\"%s\" x=%d y=%d %c%c", SEQ_START,
              text_tag_type_short_name(TTT_LINK),
              text_link_type_name(TLT_TILE), TILE_XY(ptile), SEQ_END,
              SEQ_STOP);
  return buf;
}

/**
   Get a text link to an unit.
   N.B.: The returned string is static, so every call to this function
   overwrites the previous.
 */
const char *unit_link(const struct unit *punit)
{
  static char buf[MAX_LEN_LINK];

  fc_snprintf(buf, sizeof(buf), "%c%s tgt=\"%s\" id=%d name=\"%s\" %c%c",
              SEQ_START, text_tag_type_short_name(TTT_LINK),
              text_link_type_name(TLT_UNIT), punit->id,
              unit_name_translation(punit), SEQ_END, SEQ_STOP);
  return buf;
}

/**
   Get a text link to a unit tile (make a clickable link to a tile with the
   unit type name as text).
   N.B.: The returned string is static, so every call to this function
   overwrites the previous.
 */
const char *unit_tile_link(const struct unit *punit)
{
  static char buf[MAX_LEN_LINK];
  const char *tag_name = text_tag_type_short_name(TTT_LINK);

  fc_snprintf(buf, sizeof(buf), "%c%s tgt=\"%s\" x=%d y=%d%c%s%c%c%s%c",
              SEQ_START, tag_name, text_link_type_name(TLT_TILE),
              TILE_XY(unit_tile(punit)), SEQ_STOP,
              unit_name_translation(punit), SEQ_START, SEQ_END, tag_name,
              SEQ_STOP);
  return buf;
}

/**
   Get a text of a unit's vet level.
   N.B.: The returned string is static, so every call to this function
   overwrites the previous.
 */
const char *unit_veteran_level_string(const struct unit *punit)
{
  static char buf[MAX_LEN_LINK];
  const struct veteran_level *vlevel;

  if (!punit) {
    buf[0] = '\0'; /* If no unit, return empty string */
    return buf;
  }

  vlevel = utype_veteran_level(unit_type_get(punit), punit->veteran);
  fc_snprintf(buf, sizeof(buf), "%s", name_translation_get(&vlevel->name));
  return buf;
}

/**
   Get string of when unit gets upgraded to new veteran level.
   N.B.: The returned string is static, so every call to this function
   overwrites the previous.
 */
const char *unit_achieved_rank_string(const struct unit *punit)
{
  static char buf[MAX_LEN_LINK];

  fc_snprintf(buf, sizeof(buf),
              /* TRANS: " and achieved the rank of <veteran level>";
               * preserve leading space */
              _(" and achieved the rank of %s"),
              unit_veteran_level_string(punit));
  return buf;
}

/**
   Get string of unit's attack would be a tired attack or not.
   N.B.: The returned string is static, so every call to this function
   overwrites the previous.
 */
const char *unit_tired_attack_string(const struct unit *punit)
{
  static char buf[MAX_LEN_LINK];

  if (is_tired_attack(punit->moves_left)) {
    fc_snprintf(buf, sizeof(buf),
                /* TRANS: tired; note trailing space */
                _("tired "));
  } else {
    buf[0] = '\0';
  }
  return buf;
}

/**
   Get string of unit's firepower text, i.e. "FP:2 "
   If firepower is equal to one, then an empty string is returned
   so as to shorten the text output.
   N.B.: The returned string is static, so every call to this function
   overwrites the previous.
 */
const char *unit_firepower_if_not_one(int firepower)
{
  static char buf[MAX_LEN_LINK];

  if (firepower == 1) {
    buf[0] = '\0';
  } else {
    fc_snprintf(buf, sizeof(buf),
                /* TRANS: FP = Firepower of a unit; note trailing space */
                _("FP:%d "), firepower);
  }
  return buf;
}
