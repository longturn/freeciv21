.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

Formulas
********

This page describes how Freeciv21 computes what happens in the game. It is
intended as a reference for ruleset authors and players who want to heavily
optimize their actions. Developers may occasionally find it useful, too.

Unit Bribe Cost
===============

The amount of gold needed to bribe a unit is primarily governed by who much the
unit costs, the ``base_bribe_cost`` ruleset setting, and how far the unit is
from the nearest capital city (capped to 32 tiles). The amount of gold the
opposing player has also plays a role. The base cost is computed as follows:

.. math::
  \text{base cost} =
    \frac{\text{owner gold} + \texttt{base\_bribe\_cost}}{2 + \text{distance}}
    \times \frac{\text{unit cost in shields}}{10}

Several multiplicative factors are applied on top of the base cost:

* Rulesets can modify this cost through the
  :ref:`Unit_Bribe_Cost_Pct effect <effect-unit-bribe-cost-pct>`:

  .. math::
    f_\text{effects} = 1 + \frac{\texttt{Unit\_Bribe\_Cost\_Pct}}{100}

* Veteran units are more expensive. The cost is increased by two factors, the
  combat power factor :math:`f_\text{power}` and the relative speed of the unit
  compared to a green one, :math:`f_\text{MP}`.
* Finally, a discount is given if the unit is short on HP, reducing the cost by
  up to 50% if the unit has no HP. This is computed as follows:

  .. math::
    f_\text{HP} =
      \frac{1}{2} \left( 1 + \frac{\text{unit HP}}{\text{base HP}} \right)

The total bribe cost is then computed as:

.. math::
  \text{final cost} = \text{base cost}
                        \times f_\text{effects} \times f_\text{power}
                        \times f_\text{MP} \times f_\text{HP}
