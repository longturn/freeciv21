#include "map_updates_handler.h"
#include "city.h"
#include "options.h"
#include "tilespec.h"

FC_CPP_DECLARE_LISTENER(freeciv::map_updates_handler)

namespace freeciv {

/**
 * @class map_updates_handler
 * @brief Records regions of the map that should be updated
 *
 * This class records a list of all regions on the map that need an update,
 * and the extent of the necessary update (as a combination of @ref
 * update_type). It has nothing to do with tilesets and doesn't know about
 * the map geometry, but some tilesets assumptions are hard-coded.
 */

/**
 * Constructor
 */
map_updates_handler::map_updates_handler(QObject *parent) : QObject(parent)
{
  map_updates_handler::listen();
}

/**
 * Clears all pending updates.
 */
void map_updates_handler::clear()
{
  m_full_update = false;
  m_updates.clear();
}

/**
 * Registers a city for update. If `full`, also update the city label.
 * @see update_city_description
 */
void map_updates_handler::update(const city *city, bool full)
{
  if (!m_full_update) {
    const auto tile = city_tile(city);
    if (full && (gui_options->draw_map_grid || gui_options->draw_borders)) {
      m_updates[tile] |= update_type::city_map;
    } else {
      // Assumption: city sprites are as big as unit sprites
      m_updates[tile] |= update_type::unit;
    }
    emit repaint_needed();
  }
}

/**
 * Registers a tile for update. If `full`, also update the neighboring area.
 */
void map_updates_handler::update(const tile *tile, bool full)
{
  if (!m_full_update) {
    if (full) {
      m_updates[tile] |= update_type::tile_full;
    } else {
      m_updates[tile] |= update_type::tile_single;
    }
    emit repaint_needed();
  }
}

/**
 * Registers a unit for update.
 */
void map_updates_handler::update(const unit *unit, bool full)
{
  if (!m_full_update) {
    const auto tile = unit_tile(unit);
    if (full && gui_options->draw_native) {
      update_all();
    } else if (full && unit_drawn_with_city_outline(unit, true)) {
      m_updates[tile] |= update_type::city_map;
      emit repaint_needed();
    } else {
      m_updates[tile] |= update_type::unit;
      emit repaint_needed();
    }
  }
}

/**
 * Requests an update of the whole (visible) map.
 */
void map_updates_handler::update_all()
{
  m_updates.clear();
  m_full_update = true;
  emit repaint_needed();
}

/**
 * Registers a city label for update.
 * @see update(const city *, bool)
 */
void map_updates_handler::update_city_description(const city *city)
{
  if (!m_full_update) {
    m_updates[city_tile(city)] |= update_type::city_description;
    emit repaint_needed();
  }
}

/**
 * Registers a tile label for update.
 */
void map_updates_handler::update_tile_label(const tile *tile)
{
  if (!m_full_update) {
    m_updates[tile] |= update_type::tile_label;
    emit repaint_needed();
  }
}

} // namespace freeciv
