..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

Architecture
************

Freeciv21 is a client/server empire-building, civilization style of game. The client is pretty dumb. Almost
all calculations are performed on the server.

The source code has the following important directories:

* :file:`dependencies`: code from upstream projects.
* :file:`utility`: utility functionality that is not freeciv21-specific.
* :file:`common`: data structures and code used by both the client and server.
* :file:`server`: common server code.
* :file:`client`: common client code.
* :file:`data`: graphics, rulesets and stuff.
* :file:`translations`: localization files.
* :file:`ai`: the ai, later linked into the server.
* :file:`tools`: Freeciv21 support executables.

Freeciv21 is written in C and C++. Header files should be compatible with C++ so that C++ add-ons are
possible.
