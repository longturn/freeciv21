// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
// SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

#pragma once

struct player;

class demographic {
public:
  demographic(const char *name, int (*get_value)(const player *),
              const char *(*get_text)(int value), bool larger_is_better);

  /// Returns the untranslated name of this demographic.
  const char *name() const { return m_name; }

  int evaluate(const player *pplayer) const;
  const char *text(int value) const;

  /// Returns true \c lhs is worse (this usually means smaller) than \c rhs.
  bool compare(int lhs, int rhs) const
  {
    return m_larger_is_better ? lhs < rhs : rhs < lhs;
  }

private:
  const char *m_name;
  int (*m_get_value)(const player *);
  const char *(*m_get_text)(int value);
  bool m_larger_is_better;
};
