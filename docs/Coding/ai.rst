..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

Artificial Intelligence (AI)
****************************

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

This document is about Freeciv21's default AI.

Introduction
============

The Freeciv21 AI is widely recognized as being as good as or better military-wise as the AI of certain other
games it is natural to compare it with. It is, however, still too easy for experienced players, mostly due
to it being very predictable.

Code that implements the AI is divided between the :file:`ai/` and :file:`server/advisors` code directories.
The latter is used also by human players for such automatic helpers such as auto-settlers and auto-explorers.


Long-Term AI Development Goals
==============================

The long-term goals for Freeciv21 AI development are:

* To create a challenging and fun AI for human players to defeat.
* To create an AI that can handle all the ruleset possibilities that Freeciv21 can offer, no matter how
  complicated or unique the implementation of the rules.


Want Calculations
=================

Build calculations are expressed through a structure called :code:`adv_choice`. This has a variable called
"want", which determines how much the AI wants whatever item is pointed to by :code:`choice->type`.
:code:`choice->want` is:

======== ======
Value    Result
======== ======
-199     get_a_boat
< 0      an error
== 0     no want, nothing to do
<= 100   normal want
> 100    critical want, used to requisition emergency needs
> ???    probably an error (1024 is a reasonable upper bound)
> 200    Frequently used as a cap. When want exceeds this value, it is reduced to a lower number.
======== ======

These are ideal numbers, your mileage while travelling through the code may vary considerably. Technology and
Diplomats, in particular, seem to violate these standards.


Amortize
========

Hard fact: :code:`amortize(benefit, delay)` returns :math:`benefit * ((MORT - 1)/MORT)^delay`.

Speculation: What is better... to receive $10 annually starting in 5 years from now or $5 annually starting
from this year? How can you take inflation into account? The function :code:`amortize()` is meant to help you
answer these questions. To achieve this, it re-scales the future benefit in terms of today's money.

Suppose we have a constant rate of inflation, :code:`x` percent. Then in five years $10 will buy as much
as :math:`10*(100/(100+x))^5` will buy today. Denoting :math:`100/(100+x)` by :code:`q` we get the general
formula, :code:`N` dollars, :code:`Y` years from now will be worth :math:`N*q^Y` in today's money. If we
receive :code:`N` every year starting :code:`Y` years from now, the total amount receivable (in today's money)
is :math:`N*q^Y / (1-q)`. This is the sum of infinite geometric series. This is exactly the operation that the
:code:`amortize()` function performs, the multiplication by some :math:`q < 1` raised to power :code:`Y`. Note
that the factor :math:`1/(1-q)` does not depend on the parameters :code:`N` and :code:`Y`, and can be ignored.
The connection between the :math:`MORT` constant and the inflation rate :code:`x` is given by
:math:`(MORT - 1) / MORT = q = 100 / (100 + x)`. Thus the current value of :code:`MORT = 24` corresponds to
the inflation rate, or the rate of expansion of your civilization of 4.3%

Most likely this explanation is not what the authors of :code:`amortize()` had in mind, but the basic idea is
correct: the value of the payoff decays exponentially with the delay.

The version of amortize used in the military code (:code:`military_amortize()`) remains a complete mystery.


Estimation of Profit From a Military Operation
==============================================

This estimation is implemented by the :code:`kill_desire()` function, which is not perfect, the
:code:`multi-victim` part is flawed, plus some corrections. In general:

:code:`Want = Operation_Profit * Amortization_Factor`

where:

* :code:`Amortization_Factor` : is a function of the estimated time length of the operation.

:code:`Operation_Profit = Battle_Profit - Maintenance`

where:

* :code:`Maintenance` : = :code:`(Support + Unhappiness_Compensation) * Operation_Time` : Here
  ``Unhappiness_Compensation`` is from a military unit being away from home and ``Support`` is the number of
  Shields spent on supporting this unit per turn.

* :code:`Battle_Profit` : =
  :code:`(Shields_Lost_By_Enemy * Probability_To_Win) - (Shields_Lost_By_Us * Probability_To_Lose)` : That is
  ``Battle_Profit`` is a probabilistic average. It answers the question: "How much better off, on average,
  would we be from attacking this enemy unit?"


Selecting Military Units
========================

The code dealing with choosing military units to be built and targets for them is especially messy.

Military units are requested in the :code:`military_advisor_choose_build()` function. It first considers the
defensive units and then ventures into selection of attackers (if home is safe). There are two possibilities
here: we just build a new attacker or we already have an attacker which was forced, for some reason, to defend.
In the second case it is easy: we calculate how good the existing attacker is and if it is good, we build a
defender to free it up.

Building a brand new attacker is more complicated. First, the :code:`ai_choose_attacker_*` functions are
called to find the first approximation to the best attacker that can be built here. This prototype attacker
is selected using very simple :math:`attack_power * speed` formula. Then, already in the
:code:`kill_something_with()` function, we search for targets for the prototype attacker using the
:code:`find_something_to_kill()` function. Having found a target, we do the last refinement by calling the
:code:`process_attacker_want()` function to look for the best attacker type to take out the target. This type
will be our attacker of choice. Note that the :code:`function process_attacker_want()` function has side-effects
with regards to the Technology selection.

Here is an example:

First the :code:`ai_choose_attacker_land()` function selects a :unit:`Dragoon` because it is strong and fast.
Then the :code:`find_something_to_kill()` function finds a victim for the (virtual) :unit:`Dragoon`, an enemy
:unit:`Riflemen` standing right next to the city. Then the :code:`process_attacker_want()` function figures
out that since the enemy is right beside us, it can be taken out easier using an :unit:`Artillery`. It also
figures that a :unit:`Howitzer` would do this job even better, so bumps up our desire for
:title-reference:`Robotics`.


Ferry System
============

The ferry (i.e. boats transporting land units) system of Freeciv21 is probably better described by statistical
mechanics than by logic. Both ferries and prospective passengers move around in what looks like a random
fashion, trying to get closer to each other. On average, they succeed. This behavior has good reasons behind
it. It is hell to debug, but means that small bugs do not affect the overall picture visibly.

Each turn both boats and prospective passengers forget all about prior arrangements (unless the passenger is
actually *in* the boat). Then each will look for the closest partner, exchange cards, and head towards it.
This is done in a loop which goes through all units in random order.

Because most units recalculate their destination every turn, ignoring prior arrangements is the only good
strategy. It means that a boat will not rely on the prospective passenger to notify it when it is not needed
anymore. This is not very effective, but can only be changed when the prospective passengers behave more
responsibly. See the Diplomat code for more responsible behavior. They try to check if the old target is still
good before trying to find a new one.

When a boat has a passenger, it is a different story. The boat does not do any calculations, instead one of
the passengers is given full control and it is the passenger who drives the boat.

Here are the main data fields used by the system. Value of ``ai.ferry`` in the passenger unit is:

*  ``FERRY_NONE`` : means that the unit has no need of a ferry.
*  ``FERRY_WANTED`` : means that the unit wants a ``ferry >0 : id`` of its ferry.

Value of ``ai.passenger`` in the ferry unit can be either of:

* ``FERRY_AVAILABLE`` : means that the unit is a ferry and is ``available >0 : id`` of its passenger.

When boat-building code stabilizes, it can be seen how many free boats there are, on average, per prospective
passenger. If there are more boats than prospective passengers, it makes sense that only prospective
passengers should look for boats. If boats are few, they should be the ones choosing. This can be done both
dynamically, where both possibilities are coded and the appropriate is chosen every turn, and statically,
after much testing only one system remains. Now they exist in parallel, although each developed to a different
degree.


Diplomacy
=========

The AI's diplomatic behaviour is current only regulated by the ``diplomacy`` server setting.

AI proposes Cease-fire on first contact.

AI is not very trusting for NEUTRAL and PEACE modes, but once it hits ALLIANCE, this changes completely, and
it will happily hand over any technologies and maps it has to you. The only thing that will make the AI
attack you then is if you build a Spaceship.

For people who want to hack at this part of the AI code, please note:

* The ``pplayers_at_war(p1,p2)`` function returns ``FALSE`` if ``p1==p2``
* The ``pplayers_non_attack(p1,p2)`` function returns ``FALSE`` if ``p1==p2``
* The ``pplayers_allied(p1,p2)`` function returns ``TRUE`` if ``p1==p2``
* The ``pplayer_has_embassy(p1,p2)`` function returns ``TRUE`` if ``p1==p2``

For example, we do not ever consider a Nation to be at War with themselves, we never consider a Nation to have
any kind of non-attack treaty with themselves, and we always consider a Nation to have an Alliance with
themself.

The introduction of Diplomacy is fraught with many problems. One is that it usually benefits only human
players and not AI players, since humans are so much smarter, and know how to exploit Diplomacy. For AIs,
they mostly only add constraints on what it can do. This means Diplomacy either has to be optional, or have
fine-grained controls on who can do what Diplomatic deals to whom, which are set from rulesets. The latter is
not yet well implemented.

Difficulty Levels
=================

There are currently seven difficulty levels:

#. Handicapped
#. Novice
#. Easy
#. Normal
#. Hard
#. Cheating
#. Experimental

The ``hard`` level is no-holds-barred. ``Cheating`` is the same except that it has ruleset defined extra
bonuses, while ``normal`` has a number of handicaps. In ``easy``, the AI also does random stupid things
through the :code:`ai_fuzzy()` function. In ``novice`` the AI researches slower than normal players. The
``experimental`` level is only for coding. You can gate new code with the ``H_EXPERIMENTAL`` handicap and test
``experimental`` level AIs against ``hard`` level AIs.

Other handicaps used are:

================= =======
Variable          Result
================= =======
``H_DIPLOMAT``    Cannot build offensive :unit:`Diplomats`.
``H_LIMITEDHUTS`` Can get only 25 gold and :unit:`Barbarians` from Huts.
``H_DEFENSIVE``   Build defensive buildings without calculating need.
``H_RATES``       Cannot set its tax rates beyond government limits.
``H_TARGETS``     Cannot target anything it does not know exists.
``H_HUTS``        Does not know which unseen Tiles have Huts on them.
``H_FOG``         Cannot see through fog of War.
``H_NOPLANES``    Does not build air units.
``H_MAP``         Only knows ``map_is_known`` Tiles.
``H_DIPLOMACY``   Not very good at Diplomacy.
``H_REVOLUTION``  Cannot skip Anarchy.
``H_EXPANSION``   Do not like being much larger than human.
``H_DANGER``      Always thinks its city is in danger.
================= =======

For an up-to-date list of all handicaps and their use for each difficulty level see :file:`ai/handicaps.h`.


Things That Need To Be Fixed
============================

* Cities do not realize units are on their way to defend it.
* AI builds cities without regard to danger at that location.
* AI will not build cross-country roads outside of the city vision radius.
* ``Locally_zero_minimap`` is not implemented when wilderness Tiles change.
* If no path to a chosen victim is found, a new victim should be chosen.
* Emergencies in two Cities at once are not handled properly.
* :unit:`Explorers` will not use ferryboats to get to new lands to explore. The AI will also not build units
  to explore new islands, leaving Huts alone.
* AI sometimes believes that wasting a horde of weak military units to kill one enemy is profitable.
* Stop building shore defense improvements in landlocked Cities with a Lake adjacent.
* Fix the AI valuation of :improvement:`Supermarket`. It currently never builds it. See the
  :code:`farmland_food()` and :code:`ai_eval_buildings()` functions in :file:`advdomestic.cpp`.
* Teach the AI to coordinate the units in an attack.


Idea Space
==========

* Friendly cities can be used as beachheads.
* The :code:`Assess_danger()` function should acknowledge positive feedback between multiple attackers.
* It would be nice for a bodyguard and charge to meet en-route more elegantly.
* The :code:`struct choice` should have a priority indicator in it. This will reduce the number of "special"
  want values and remove the necessity to have want capped, thus reducing confusion.
