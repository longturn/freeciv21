..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

Client/Server Model
*******************

Each client player has an attribute block and the server also holds such an attribute block for every player.
All attribute blocks the server holds are included in the save game. The client and server synchronize their
blocks. The server sends its block to the client at game start or reload. The client sends an updated block at
each end of turn to the server. Since the maximum packet size is limited to currently 4k and the attribute
block can have arbitrary size, although limited to 64k in this initial version, the attribute block cannot be
transferred in one packet. So the attribute block is divided into attribute chunks which are reassembled at
the receiver. No part of the server knows any inner structure of the attribute block. For the server an
attribute block is just a block of bytes.

User Interface
==============

Since an attribute block is not a good user interface the user can access the attributes through a mapping,
dictionary, hashmap, or hashtable interface. This hashtable will get serialized to the attribute block and the
other direction around. The key of the hashtable consists of: the (real) ``key``, ``x``, ``y``, and an ``id``.
The (real) ``key`` is an integer, which defines the use and format of this attribute. The values of the
hashtable can have arbitrary length. The internal structure of an value is unknown to the attribute handling.

For easier access there are wrapper functions for the common types Unit, City, player, and Tile. So there are
easy methods for attaching arbitrary data to a Unit, a City, a player (self or other), or a Tile.
