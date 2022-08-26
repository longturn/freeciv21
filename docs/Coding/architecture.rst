..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>

Architecture
************

Freeciv21 uses a client/server model where two programs interact over a network connection. The *client*
(``freeciv21-client``) is the program that you see on the screen. It handles user commands and forwards them
to the second program, that we call the *server* (``freeciv21-server``). The server checks the commands,
computes their result and sends updates back to the client. The client is capable of managing a server for
its own needs, which is used for single player games. It can also connect to a server managed externally,
which is how multiplayer games work.

The Freeciv21 code is old and evolved from a program written in C. It still bears a lot of this history, but
this is being worked on. New code is written in C++, using modern constructs when it makes sense and relying
heavily on the `Qt Framework <https://doc.qt.io>`_ for platform abstraction. This is currently most visible
in the client, but there are plans are to convert the server as well.

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
