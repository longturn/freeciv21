.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

Sound Support
*************

The server sends the game a list of primary and secondary sound tags for certain events. The ``primary`` tags
are those preferred by the current modpack. The game does not need to have these sounds. The ``secondary``
tags should refer to standard sounds that all installations of Freeciv21 should have.

Tags are used to give an easy way to change sounds. A specfile is used to indicate which tags refer to which
sound files. A change of spec file, given as an option at startup, will change sounds. For example:

.. code-block:: sh

  $ freeciv21-client --Sound mysounds.soundspec


will read sound files from :file:`mysounds.soundspec`. You will need to download or copy or link those sounds
into whichever directory is mentioned in this file first, or edit it to refer to the right files. All
references are by default relative to the :file:`data/` directory. Soundpacks can be downloaded from the
Freeciv21 modpack installer.

Tags
====

There are two kinds of sound tags:

* defined in the rulesets
* defined in the program code

While the former can be chosen freely the latter cannot be changed.

The sound tags associated with improvements (wonders and normal buildings), unit movements, and unit combat
have to be set in the rulesets. Freeciv21 just hands these sound tags over to the game where they are
translated into the filenames of the sound files via the soundspec file. Every soundspec should have generic
sound tags for wonders (``w_generic``), normal buildings (``b_generic``), unit movements (``m_generic``), and
unit fights (``f_generic``).

Sound tags associated with certain events are generated in the Freeciv21 code and cannot be configured from
outside. The soundspec file also has to have mapping for these tags. The complete list of such tags can be
found in :file:`data/stdsounds.soundspec`. The name of the tag is C++ enum name (see :file:`common/events.h`)
in lowercase. So ``E_POLLUTION`` becomes the tag ``e_pollution``. There is no generic event tag and no
alternate tags are used.
