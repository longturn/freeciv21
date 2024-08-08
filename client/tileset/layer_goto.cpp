// SPDX-License-Identifier: GPLv3-or-later
// SPDX-FileCopyrightText: Freeciv authors
// SPDX-FileCopyrightText: Freeciv21 authors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#include "layer_goto.h"

#include "goto.h"
#include "tilespec.h"

namespace freeciv {

layer_goto::layer_goto(struct tileset *ts) : freeciv::layer(ts, LAYER_GOTO)
{
}

void layer_goto::load_sprites()
{
  // Digit sprites. We have a cascade of fallbacks.
  QStringList patterns = {QStringLiteral("path.turns_%1"),
                          QStringLiteral("city.size_%1")};
  assign_digit_sprites(tileset(), m_states[GTS_MP_LEFT].turns.data(),
                       m_states[GTS_MP_LEFT].turns_tens.data(),
                       m_states[GTS_MP_LEFT].turns_hundreds.data(),
                       patterns);
  patterns.prepend(QStringLiteral("path.steps_%1"));
  assign_digit_sprites(tileset(), m_states[GTS_TURN_STEP].turns.data(),
                       m_states[GTS_TURN_STEP].turns_tens.data(),
                       m_states[GTS_TURN_STEP].turns_hundreds.data(),
                       patterns);
  patterns.prepend(QStringLiteral("path.exhausted_mp_%1"));
  assign_digit_sprites(tileset(), m_states[GTS_EXHAUSTED_MP].turns.data(),
                       m_states[GTS_EXHAUSTED_MP].turns_tens.data(),
                       m_states[GTS_EXHAUSTED_MP].turns_hundreds.data(),
                       patterns);

  // Turn steps
  m_states[GTS_MP_LEFT].specific =
      load_sprite(tileset(), {"path.normal"}, false);
  m_states[GTS_EXHAUSTED_MP].specific =
      load_sprite(tileset(), {"path.exhausted_mp"}, false);
  m_states[GTS_TURN_STEP].specific =
      load_sprite(tileset(), {"path.step"}, false);

  // Waypoint
  m_waypoint = load_sprite(tileset(), {"path.waypoint"}, true);
}

/**
 * Fill in the given sprite array with any needed goto sprites.
 */
std::vector<drawn_sprite>
layer_goto::fill_sprite_array(const tile *ptile, const tile_edge *pedge,
                              const tile_corner *pcorner,
                              const unit *punit) const
{
  if (!ptile || !goto_is_active()) {
    return {};
  }

  std::vector<drawn_sprite> sprs;

  enum goto_tile_state state;
  int length;
  bool waypoint;
  if (!goto_tile_state(ptile, &state, &length, &waypoint)) {
    return {};
  }

  if (length >= 0) {
    fc_assert_ret_val(state >= 0, {});
    fc_assert_ret_val(state < m_states.size(), {});

    if (auto sprite = m_states[state].specific) {
      sprs.emplace_back(tileset(), sprite, false, 0, 0);
    }

    // Units
    auto sprite = m_states[state].turns[length % NUM_TILES_DIGITS];
    sprs.emplace_back(tileset(), sprite);

    // Tens
    length /= NUM_TILES_DIGITS;
    if (length > 0
        && (sprite =
                m_states[state].turns_tens[length % NUM_TILES_DIGITS])) {
      sprs.emplace_back(tileset(), sprite);
    }

    // Hundreds (optional)
    length /= NUM_TILES_DIGITS;
    if (length > 0
        && (sprite =
                m_states[state].turns_hundreds[length % NUM_TILES_DIGITS])) {
      sprs.emplace_back(tileset(), sprite);
      // Divide for the warning: warn for thousands if we had a hundreds
      // sprite
      length /= NUM_TILES_DIGITS;
    }

    // Warn if the path is too long (only once by tileset).
    if (length > 0 && !m_warned) {
      qInfo(_("Tileset \"%s\" doesn't support long goto paths, such as %d. "
              "Path not displayed as expected."),
            tileset_name_get(tileset()), length);
      m_warned = true;
    }
  }

  if (waypoint) {
    sprs.emplace_back(tileset(), m_waypoint, false, 0, 0);
  }

  return sprs;
}

} // namespace freeciv
