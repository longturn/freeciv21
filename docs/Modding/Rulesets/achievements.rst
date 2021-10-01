Achievements
************

Achievements are something a player can gain in game by reaching set goals. Achievements active in the ruleset
are defined in :file:`game.ruleset`.

Depending on whether achievement is defined unique or not, they are granted only to first player, or all
players, to reach the goal for the achievement. Goal is defined by achievement type and another value specific
to that achievement type.


Achievement Types
=================

Map_Known
    Achievement is granted when player has mapped at least :code:`<value>%` of the world.

Spaceship
    This achievement is granted when player launches spaceship. code:`<Value>` is ignored.

Multicultural
    Achievement is granted when player has citizens of at least code:`<value>` different nationalities (across
    all their cities).

Cultured_City
    Achievement is granted when player has a city with at least code:`<value>` culture points.

Cultured_Nation
    Achievement is granted when player has at least code:`<value>` culture points.

Lucky
    Achievement is granted on turn when random number generator gives value less than code:`<value>` out of
    10,000.

Huts
    Achievement is granted once player has entered code:`<value>` huts.

Metropolis
    Achievement is granted once there's city of at least size code:`<value>` in the player's empire.

Literate
    Achievement is granted when player's literacy percent is at least code:`<value>`.

Land_Ahoy
    Achievement is granted when player has seen code:`<value>` different islands/continents. Home continent
    counts, so to give achievements for finding first other continent, use value 2.
