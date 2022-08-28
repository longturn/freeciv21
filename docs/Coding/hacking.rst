..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 Pranav Sampathkumar <pranav.sampathkumar@gmail.com>
    SPDX-FileCopyrightText: 2022 NIKEA-SOFT

Freeciv21 Hacker's Guide
************************

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

This guide is intended to be a help for developers, wanting to mess with Freeciv21 programming.


The Server
==========

General:

The server main loop basically looks like:

.. code-block:: rst

    while (server_state == RUN_GAME_STATE) { /* looped once per turn */
    do_ai_stuff();   /* do the ai controlled players */
    sniff_packets(); /* get player requests and handle them */
    end_turn();      /* main turn update */
    game_next_year();


Most time is spend in the :code:`sniff_packets()` function, where a :code:`select()` call waits for packets or
input on stdin(server-op commands).

Server Autogame Testing
-----------------------

Code changes should always be tested before submission for inclusion into the GitHub source tree. It is
useful to run the client and server as autogames to verify either a particular savegame no longer shows a
fixed bug, or as a random sequence of games in a while loop overnight.

To start a server game with all AI players, create a file (below named civ.serv) with lines such as the
following:

.. code-block:: rst

    # set gameseed 42       # repeat a particular game (random) sequence
    # set mapseed 42        # repeat a particular Map generation sequence
    # set timeout 3         # run a client/server autogame
    set timeout -1          # run a server only autogame
    set minplayers 0        # no human player needed
    set ec_turns 0          # avoid timestamps in savegames
    set aifill 7            # fill to 7 players
    hard                    # make the AI do complex things
    create Caesar           # first player (with known name) created and
                            # toggled to AI mode
    start                   # start game


.. note::
    The server prompt is unusable when game with :code:`timeout` set to -1 is running. You can stop such game
    with single :code:`ctrl+c`, and continue by setting :code:`timeout` to -1 again.


The commandline to run server-only games can be typed as variations of:

.. code-block:: rst

    $ while( time server/freeciv21-server -r civ.serv ); do date; done
    ---  or  ---
    $ server/freeciv21-server -r civ.serv -f buggy1534.sav.gz


To attach one or more clients to an autogame, remove the :code:`start` command, start the server program and
attach clients to created AI players. Or type :code:`aitoggle <player>` at the server command prompt for each
player that connects. Finally, type :code:`start` when you are ready to watch the show.

.. note::
    The server will eventually flood a client with updates faster than they can be drawn to the screen,
    thus it should always be throttled by setting a timeout value high enough to allow processing of the large
    update loads near the end of the game.


The autogame mode with :code:`timeout -1` is only available in ``DEBUG`` versions and should not be used with
clients as it removes virtually all the server gating controls.

If you plan to compare results of autogames the following changes can be helpful:

* :code:`define __FC_LINE__` to a constant value in :file:`./utility/log.h`.
* :code:`undef LOG_TIMERS` in :file:`./utility/timing.h`.
* deactivation of the event cache (:code:`set ec_turns 0`).


Data Structures
===============

For variable length list of fx Units and cities Freeciv21 uses a :code:`genlist`, which is implemented in
:file:`utility/genlist.cpp`. By some macro magic type specific macros have been defined, avoiding much trouble.

For example a Tile struct (the pointer to it we call :code:`ptile`) has a Unit list, :code:`ptile->units`; to
iterate though all the Units on the Tile you would do the following:

.. code-block:: rst

    unit_list_iterate(ptile->units, punit) {
    /* In here we could do something with punit, which is a pointer to a
        unit struct */
    } unit_list_iterate_end;


Note that the macro itself declares the variable :code:`punit`. Similarly there is a

.. code-block:: rst

    city_list_iterate(pplayer->cities, pcity) {
    /* Do something with pcity, the pointer to a city struct */
    } city_list_iterate_end;


There are other operations than iterating that can be performed on a list; inserting, deleting, or sorting
etc. See :file:`utility/speclist.h`. Note that the way the :code:`*_list_iterate macro` is implemented means
you can use "continue" and "break" in the usual manner.

One thing you should keep in the back of your mind. Say you are iterating through a Unit list, and then
somewhere inside the iteration decide to disband a Unit. In the server you would do this by calling
:code:`wipe_unit(punit)`, which would then remove the Unit node from all the relevant Unit lists. However, by
the way :code:`unit_list_iterate` works, if the removed Unit was the following node :code:`unit_list_iterate`
will already have saved the pointer, and use it in a moment, with a segfault as the result. To avoid this, use
:code:`unit_list_iterate_safe` instead.

You can also define your own lists with operations like iterating. Read how in :file:`utility/speclist.h`.

Network and Packets
===================

The basic network code is located in :file:`server/sernet.cpp` and :file:`client/clinet.cpp`.

All information passed between the server and clients, must be sent through the network as serialized packet
structures. These are defined in :file:`common/packets.h`.

For each ``foo`` packet structure, there is one send and one receive function:

.. code-block:: rst

    int send_packet_foo(struct connection *pc, struct packet_foo *packet);
    struct packet_foo * receive_packet_foo(struct connection *pc);


The :code:`send_packet_foo()` function serializes a structure into a bytestream and adds this to the send
buffer in the connection struct. The :code:`receive_packet_foo()` function de-serializes a bytestream into a
structure and removes the bytestream from the input buffer in the connection struct. The connection struct is
defined in :file:`common/connection.h`.

Each structure field in a structure is serialized using architecture independent functions such as
:code:`dio_put_uint32()` and de-serialized with functions like :code:`dio_get_uint32()`.

A packet is constituted by a header followed by the serialized structure data. The header contains the
following fields (the sizes are defined in :file:`common/packets.cpp`:code:`packet_header_set()`):

.. code-block:: rst

    uint16 : length (the length of the entire packet)
    uint16 : type   (e.g. PACKET_TILE_INFO)


For backward compatibility reasons, packets used for the initial protocol (notably before checking the
capabilities) have different header fields sizes as defined in
:file:`common/packets.c`:code:`packet_header_init()`:

.. code-block:: rst

    uint16 : length (the length of the entire packet)
    uint8  : type   (e.g. PACKET_SERVER_JOIN_REQ)


To demonstrate the route for a packet through the system, here is how a Unit disband is performed:

#. A player disbands a Unit.
#. The client initializes a packet_unit_request structure and calls the packet layer function
   :code:`send_packet_unit_request()` with this structure and packet type: :code:`PACKET_UNIT_DISBAND`.
#. The packet layer serializes the structure, wraps it up in a packet containing the ``packetlength`` type
   and the serialized data. Finally, the data is sent to the server.
#. On the server the packet is read. Based on the type, the corresponding de-serialize function is called
   by the :code:`get_packet_from_connection()` function.
#. A :code:`packet_unit_request` is initialized with the bytestream.
#. Since the incoming packet is a request, the server sends a :code:`PACKET_PROCESSING_STARTED` packet to the
   client. A request in this context is every packet sent from the client to the server.
#. Finally the corresponding packet-handler, the :code:`handle_unit_disband()` function, is called with the
   newly constructed structure.
#. The handler function checks if the disband request is legal (i.e. the sender really the owner of the Unit),
   etc.
#. The Unit is disbanded via the :code:`wipe_unit()` and :code:`send_remove_unit()` functions.
#. Now an integer, containing the ``id`` of the disbanded Unit is wrapped into a packet along with the type
   :code:`PACKET_REMOVE_UNIT`: :code:`send_packet_generic_integer()`.
#. The packet is serialized and sent across the network.
#. The packet-handler returns and the end of the processing is announced to the client with a
   :code:`PACKET_PROCESSING_FINISHED` packet.
#. On the client the :code:`PACKET_REMOVE_UNIT` packet is deserialized into a :code:`packet_generic_integer`
   structure.
#. The corresponding client handler function is now called :code:`handle_remove_unit()`, and finally the Unit
   is disbanded.

Notice that the two packets (:code:`PACKET_UNIT_DISBAND` and :code:`PACKET_REMOVE_UNIT`) were generic packets.
That means the packet structures involved, are used by various requests. The :code:`packet_unit_request()`
function is for example also used for the packets :code:`PACKET_UNIT_BUILD_CITY` and
:code:`PACKET_UNIT_CHANGE_HOMECITY`.

When adding a new packet type, check to see if you can reuse some of the existing packet types. This saves you
the trouble of writing new serialize or deserialize functions.

The :code:`PACKET_PROCESSING_STARTED` and :code:`PACKET_PROCESSING_FINISHED` packets from above serve two main
purposes:

#. They allow the client to identify what causes a certain packet the client receives. If the packet is framed
   by :code:`PACKET_PROCESSING_STARTED` and :code:`PACKET_PROCESSING_FINISHED` packets it is the causes of the
   request. If not the received packet was not caused by this client (server operator, other clients, server
   at a new turn)

#. After a :code:`PACKET_PROCESSING_FINISHED` packet the client can test if the requested action was performed
   by the server. If the server has sent some updates the client data structure will now hold other values.

The :code:`PACKET_FREEZE_HINT` and :code:`PACKET_THAW_HINT` packets serve two purposes:

#. Packets sent between these two packets may contain multiple information packets which may cause multiple
   updates of some GUI items. :code:`PACKET_FREEZE_HINT` and :code:`PACKET_THAW_HINT` can now be used to
   freeze the GUI at the time :code:`PACKET_FREEZE_HINT` is received and only update the GUI after the
   :code:`PACKET_THAW_HINT` packet is received.

#. Packets sent between these two packets may contain contradicting information which may confuse a
   client-side AI (agents for example). So any updates sent between these two packets are only processed after
   the :code:`PACKET_THAW_HINT` packet is received.

The following areas are wrapped by :code:`PACKET_FREEZE_HINT` and :code:`PACKET_THAW_HINT`:

* The data sent if a new game starts.
* The data sent to a reconnecting player.
* The end turn activities.

Network Improvements
====================

In the past, when a connection send buffer in the server got full we emptied the buffer contents and continued
processing. Unfortunately, this caused incomplete packets to be sent to the client, which caused crashes in
either the client or the server, since the client cannot detect this situation. This has been fixed by closing
the client connection when the buffer is emptied.

We also had, and still have, several problems related to flow control. Basically the problem is the server can
send packets much faster than the client can process them. This is especially true when in the end of the turn
the AIs move all their Units. Unit moves in particular take a long time for the client to process since by
default smooth Unit moves is on.

There are 3 ways to solve this problem:

#. We wait for the send buffers to drain before continuing processing.
#. We cut the player's connection and empty the send buffer.
#. We lose packets (this is similar to 2), but can cause an incoherent state in the client.

We mitigated the problem by increasing the send buffer size on the server and making it dynamic. We also added
in strategic places in the code calls to a new :code:`flush_packets()` function that makes the server stall
for some time draining the send buffers. Strategic places include whenever we send the whole Map. The maximum
amount of time spent per :code:`flush_packets()` call is specified by the ``netwait`` variable.

To disconnect unreachable clients we added two other features: the server terminates a client connection if it
does not accept writes for a period of time (set using the :literal:`tcptimeout` variable). It also pings the
client after a certain time elapses (set using the :literal:`pingtimeout` variable). If the client does not
reply its connection is closed.

Graphics
========

Currently the graphics is stored in the PNG file format.

If you alter the graphics, then make sure that the background remains transparent. Failing to do this means
the mask-pixmaps will not be generated properly, which will certainly not give any good results.

Each terrain Tile is drawn in 16 versions, all the combinations with a green border in one of the main
directions. Hills, Mountains, Forests, and Rivers are treated in special cases.

Isometric tilesets are drawn in a similar way to how civ2 draws (that is why civ2 graphics are compatible). For
each base terrain type there exists one Tile sprite for that terrain. The Tile is blended with nearby Tiles to
get a nice-looking boundary. This is erroneously called "dither" in the code.

Non-isometric tilesets draw the Tiles in the "original" Freeciv21 way, which is both harder and less pretty.
There are multiple copies of each Tile, so that a different copy can be drawn depending on the terrain type of
the adjacent Tiles. It may eventually be worthwhile to convert this to the civ2 system or another one
altogether.

Diplomacy
=========

A few words about the Diplomacy system. When a Diplomacy meeting is established, a treaty structure is created
on both of the clients and on the server. All these structures are updated concurrently as clauses are added
and removed.

Map Structure
=============

The Map is maintained in a pretty straightforward C array, containing X*Y Tiles. You can use the function
:code:`struct tile *map_pos_to_tile(x, y)` to find a pointer to a specific Tile. A Tile has various fields;
see the struct in :file:`common/map.h`.

You may iterate Tiles, you may use the following methods:

.. code-block:: rst

    whole_map_iterate(tile_itr) {
      /* do something */
    } whole_map_iterate_end;


for iterating all Tiles of the Map;

.. code-block:: rst

    adjc_iterate(center_tile, tile_itr) {
      /* do something */
    } adjc_iterate_end;


for iterating all Tiles close to ``center_tile``, in all *valid* directions for the current topology (see
below);

.. code-block:: rst

    cardinal_adjc_iterate(center_tile, tile_itr) {
      /* do something */
    } cardinal_adjc_iterate_end;


for iterating all Tiles close to ``center_tile``, in all *cardinal* directions for the current topology (see
below);

.. code-block:: rst

    square_iterate(center_tile, radius, tile_itr) {
      /* do something */
    } square_iterate_end;


for iterating all Tiles in the radius defined ``radius`` (in real distance, see below), beginning by
``center_tile``;

.. code-block:: rst

    circle_iterate(center_tile, radius, tile_itr) {
      /* do something */
    } square_iterate_end;


for iterating all Tiles in the radius defined ``radius`` (in square distance, see below), beginning by
``center_tile``;

.. code-block:: rst

    iterate_outward(center_tile, real_dist, tile_itr) {
      /* do something */
    } iterate_outward_end;


for iterating all Tiles in the radius defined ``radius`` (in real distance, see below), beginning by
``center_tile``. Actually, this is the duplicate of square_iterate, or various tricky ones defined in
:file:`common/map.h`, which automatically adjust the Tile values. The defined macros should be used whenever
possible, the examples above were only included to give people the knowledge of how things work.

Note that the following:

.. code-block:: rst

    for (x1 = x-1; x1 <= x+1; x1++) {
      for (y1 = y-1; y1 <= y+1; y1++) {
        /* do something */
      }
    }


is not a reliable way to iterate all adjacent Tiles for all topologies, so such operations should be avoided.


Also available are the functions calculating distance between Tiles. In Freeciv21, we are using 3 types of
distance between Tiles:

* The :code:`map_distance(ptile0, ptile1)` function returns the *Manhattan* distance between Tiles, i.e. the
  distance from :code:`ptile0` to :code:`ptile1`, only using cardinal directions. For example,
  :code:`(abs(dx) + ads(dy))` for non-hexagonal topologies.

* The :code:`real_map_distance(ptile0, ptile1)` function returns the *normal* distance between Tiles, i.e. the
  minimal distance from :code:`ptile0` to :code:`ptile1` using all valid directions for the current topology.

* The :code:`sq_map_distance(ptile0, ptile1)` function returns the *square* distance between Tiles. This is a
  simple way to make Pythagorean effects for making circles on the Map for example. For non-hexagonal
  topologies, it would be :code:`(dx * dx + dy * dy)`. Only useless square root is missing.


Different Types of Map Topology
-------------------------------

Originally Freeciv21 supports only a simple rectangular Map. For instance a 5x3 Map would be conceptualized as

.. code-block:: rst

    <- XXXXX ->
    <- XXXXX ->
    <- XXXXX ->


and it looks just like that under "overhead" (non-isometric) view. The arrows represent an east-west
wrapping. But under an isometric-view client, the same Map will look like:

.. code-block:: rst

    <-   X     ->
    <-  X X    ->
    <- X X X   ->
    <-  X X X  ->
    <-   X X X ->
    <-    X X  ->
    <-     X   ->


where "north" is to the upper-right and "south" to the lower-left. This makes for a mediocre interface.

An isometric-view client will behave better with an isometric Map. This is what Civ2, SMAC, Civ3, etc. all
use. A rectangular isometric Map can be conceptualized as

.. code-block:: rst

   <- X X X X X  ->
   <-  X X X X X ->
   <- X X X X X  ->
   <-  X X X X X ->


North is up and it will look just like that under an isometric-view client. Of course under an overhead-view
client it will again turn out badly.

Both types of Maps can easily wrap in either direction: north-south or east-west. Thus there are four types
of wrapping: flat-earth, vertical cylinder, horizontal cylinder, and torus. Traditionally Freeciv21 only wraps
in the east-west direction.


Topology, Cardinal Directions and Valid Directions
--------------------------------------------------

A *cardinal* direction connects Tiles per a *side*. Another *valid* direction connects Tiles per a *corner*.

In non-hexagonal topologies, there are 4 cardinal directions, and 4 other valid directions. In hexagonal
topologies, there are 6 cardinal directions, which matches exactly the 6 valid directions.

Note that with isometric view, the direction named "North" (``DIR8_NORTH``) is actually not from the top to
the bottom of the screen view. All directions are turned a step on the left (e.g. :math:`pi/4` rotation with
square Tiles and :math:`pi/3` rotation for hexagonal Tiles).


Different Coordinate Systems
----------------------------

In Freeciv21, we have the general concept of a "position" or "Tile". A Tile can be referred to in any of
several coordinate systems. The distinction becomes important when we start to use non-standard maps (see
above).

Here is a diagram of coordinate conversions for a classical Map.

.. code-block:: rst

      map        natural      native       index

      ABCD        ABCD         ABCD
      EFGH  <=>   EFGH     <=> EFGH   <=> ABCDEFGHIJKL
      IJKL        IJKL         IJKL


Here is a diagram of coordinate conversions for an iso-map.

.. code-block:: rst

      map          natural     native       index

        CF        A B C         ABC
       BEIL  <=>   D E F   <=>  DEF   <=> ABCDEFGHIJKL
      ADHK        G H I         GJI
       GJ          J K L        JKL


Below each of the coordinate systems are explained in more detail. Note that hexagonal topologies are always
considered as isometric.

Map (or "Standard") Coordinates
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

All of the code examples above are in Map coordinates. These preserve the local geometry of square Tiles,
but do not represent the global Map geometry well. In Map coordinates, you are guaranteed, so long as we use
square Tiles, that the Tile adjacency rules

.. code-block:: rst

    |  (map_x-1, map_y-1)    (map_x, map_y-1)   (map_x+1, map_y-1)
    |  (map_x-1, map_y)      (map_x, map_y)     (map_x+1, map_y)
    |  (map_x-1, map_y+1)    (map_x, map_y+1)   (map_x+1, map_y+1)


are preserved, regardless of what the underlying Map or drawing code looks like. This is the definition of
the system.

With an isometric view, this looks like:

.. code-block:: rst

    |                           (map_x-1, map_y-1)
    |              (map_x-1, map_y)            (map_x, map_y-1)
    | (map_x-1, map_y+1)          (map_x, map_y)              (map_x+1, map_y-1)
    |             (map_x, map_y+1)            (map_x+1, map_y)
    |                           (map_x+1, map_y+1)


Map coordinates are easiest for local operations (e.g. 'square_iterate' and friends, translations, rotations,
and any other scalar operation) but unwieldy for global operations.

When performing operations in Map coordinates (like a translation of Tile :code:`(x, y)` by :code:`(dx, dy)`
-> :code:`(x + dx, y + dy)`), the new Map coordinates may be unsuitable for the current Map. In case, you
should use one of the following functions or macros:

* :code:`map_pos_to_tile()`: return ``NULL`` if normalization is not possible;

* :code:`normalize_map_pos()`: return ``TRUE`` if normalization have been done (wrapping X and Y coordinates
  if the current topology allows it);

* :code:`is_normal_map_pos()`: return ``TRUE`` if the Map coordinates exist for the Map;

* :code:`CHECK_MAP_POS()`: assert whether the Map coordinates exist for the Map.

Map coordinates are quite central in the coordinate system, and they may be easily converted to any other
coordinates: :code:`MAP_TO_NATURAL_POS()`, :code:`MAP_TO_NATIVE_POS()`, or :code:`map_pos_to_index()`
functions.

Natural Coordinates
^^^^^^^^^^^^^^^^^^^

Natural coordinates preserve the geometry of Map coordinates, but also have the rectangular property of
native coordinates. They are unwieldy for most operations because of their sparseness. They may not have
the same scale as Map coordinates and, in the iso case, there are gaps in the natural representation of a
Map.

With classical view, this looks like:

.. code-block:: rst

      (nat_x-1, nat_y-1)    (nat_x, nat_y-1)   (nat_x+1, nat_y-1)
      (nat_x-1, nat_y)      (nat_x, nat_y)     (nat_x+1, nat_y)
      (nat_x-1, nat_y+1)    (nat_x, nat_y+1)   (nat_x+1, nat_y+1)


With an isometric view, this looks like:

.. code-block:: rst

    |                            (nat_x, nat_y-2)
    |             (nat_x-1, nat_y-1)          (nat_x+1, nat_y-1)
    | (nat_x-2, nat_y)            (nat_x, nat_y)              (nat_x+2, nat_y)
    |             (nat_x-1, nat_y+1)          (nat_x+1, nat_y+1)
    |                            (nat_x, nat_y+2)


Natural coordinates are mostly used for operations which concern the user view. It is the best way to
determine the horizontal and the vertical axis of the view.

The only coordinates conversion is done using the :code:`NATURAL_TO_MAP_POS()` function.

Native Coordinates
^^^^^^^^^^^^^^^^^^

With classical view, this looks like:

.. code-block:: rst

      (nat_x-1, nat_y-1)    (nat_x, nat_y-1)   (nat_x+1, nat_y-1)
      (nat_x-1, nat_y)      (nat_x, nat_y)     (nat_x+1, nat_y)
      (nat_x-1, nat_y+1)    (nat_x, nat_y+1)   (nat_x+1, nat_y+1)


With an isometric view, this looks like:

.. code-block:: rst

    |                            (nat_x, nat_y-2)
    |            (nat_x-1, nat_y-1)          (nat_x, nat_y-1)
    | (nat_x-1, nat_y)            (nat_x, nat_y)            (nat_x+1, nat_y)
    |           (nat_x-1, nat_y+1)          (nat_x, nat_y+1)
    |                            (nat_x, nat_y+2)


Neither is particularly good for a global Map operation such as Map wrapping or conversions to or from Map
indexes. Something better is needed.

Native coordinates compress the Map into a continuous rectangle. The dimensions are defined as
:code:`map.xsize x map.ysize`. For instance, the above iso-rectangular Map is represented in native
coordinates by compressing the natural representation in the X axis to get the 3x3 iso-rectangle of

.. code-block:: rst

    ABC       (0,0) (1,0) (2,0)
    DEF  <=>  (0,1) (1,1) (2,1)
    GHI       (0,2) (1,2) (3,2)


The resulting coordinate system is much easier to use than Map coordinates for some operations. These
include most internal topology operations (e.g., :code:`normalize_map_pos`, or :code:`whole_map_iterate`) as
well as storage (in ``map.tiles`` and savegames, for instance).

In general, native coordinates can be defined based on this property; the basic Map becomes a continuous
(gap-free) cardinally-oriented rectangle when expressed in native coordinates.

Native coordinates can be easily converted to Map coordinates using the :code:`NATIVE_TO_MAP_POS()` function,
to index using the code:`native_pos_to_index()` function and to Tile (shortcut) using the
:code:`native_pos_to_tile()` function.

After operations, such as the :code:`FC_WRAP(x, map.xsize)` function, the result may be checked with the
:code:`CHECK_NATIVE_POS()` function.

Index Coordinates
^^^^^^^^^^^^^^^^^

Index coordinates simply reorder the Map into a continuous (filled-in) one-dimensional system. This
coordinate system is closely tied to the ordering of the Tiles in native coordinates, and is slightly
easier to use for some operations (like storage) because it is one-dimensional. In general you cannot assume
anything about the ordering of the positions within the system.

Indexes can be easily converted to native coordinates using the :code:`index_to_native_pos()` function or to
Map positions (shortcut) using the :code:`index_to_map_pos()` function.

A Map index can tested using the :code:`CHECK_INDEX` macro.

With a classical rectangular Map, the first three coordinate systems are equivalent. When we introduce
isometric Maps, the distinction becomes important, as demonstrated above. Many places in the code have
introduced :code:`map_x/map_y` or :code:`nat_x/nat_y` to help distinguish whether Map or native coordinates
are being used. Other places are not yet rigorous in keeping them apart, and will often just name their
variables :code:`x` and :code:`y`. The latter can usually be assumed to be Map coordinates.

Note that if you don't need to do some abstract geometry exploit, you will mostly use Tile pointers, and give
to Map tools the ability to perform what you want.

Note that :code:`map.xsize` and :code:`map.ysize` define the dimension of the Map in :code:`_native_`
coordinates.

Of course, if a future topology does not fit these rules for coordinate systems, they will have to be refined.

Native Coordinates on an Isometric Map
--------------------------------------

An isometric Map is defined by the operation that converts between Map (user) coordinates and native
(internal) ones. In native coordinates, an isometric Map behaves exactly the same way as a standard one. See
`Native Coordinates`_, above.

Converting from Map to native coordinates involves a :math:`pi/2` rotation (which scales in each dimension by
:math:`sqrt(2)`) followed by a compression in the :code:`X` direction by a factor of 2. Then a translation is
required since the "normal set" of native coordinates is defined as
:code:`{(x, y) | x: [0..map.xsize) and y: [0..map.ysize)}` while the normal set of Map coordinates must
satisfy :code:`x >= 0` and :code:`y >= 0`.

Converting from native to Map coordinates (a less cumbersome operation) is the opposite.

.. code-block:: rst

    |                                       EJ
    |          ABCDE     A B C D E         DIO
    | (native) FGHIJ <=>  F G H I J <=>   CHN  (map)
    |          KLMNO     K L M N O       BGM
    |                                   AFL
    |                                    K

Note that:

.. code-block:: rst

  native_to_map_pos(0, 0) == (0, map.xsize-1)
  native_to_map_pos(map.xsize-1, 0) == (map.xsize-1, 0)
  native_to_map_pos(x, y+2) = native_to_map_pos(x,y) + (1,1)
  native_to_map_pos(x+1, y) = native_to_map_pos(x,y) + (1,-1)


The math then works out to:

.. code-block:: rst

  map_x = ceiling(nat_y / 2) + nat_x
  map_y = floor(nat_y / 2) - nat_x + map.xsize - 1

  nat_y = map_x + map_y - map.xsize
  nat_x = floor(map_x - map_y + map.xsize / 2)


which leads to the macros :code:`NATIVE_TO_MAP_POS()`, and :code:`MAP_TO_NATIVE_POS()` that are defined in
:file:`map.h`.

Unknown Tiles and Fog of War
----------------------------

In :file:`common/player.h`, there are several fields:

.. code-block:: rst

    struct player {
      ...
      struct dbv tile_known;

      union {
        struct {
          ...
        } server;

    struct {
        struct dbv tile_vision[V_COUNT];
        } client;
      };
    };


While :code:`tile_get_known()` returns:

.. code-block:: rst

    /* network, order dependent */
    enum known_type {
    TILE_UNKNOWN = 0,
    TILE_KNOWN_UNSEEN = 1,
    TILE_KNOWN_SEEN = 2,
    };


The values :code:`TILE_UNKNOWN` and :code:`TILE_KNOWN_SEEN` are straightforward. :code:`TILE_KNOWN_UNSEEN` is
a Tile of which the user knows the terrain, but not recent cities, Roads, etc.

:code:`TILE_UNKNOWN` Tiles never are (nor should be) sent to the client. In the past, :code:`UNKNOWN` Tiles that
were adjacent to :code:`UNSEEN` or :code:`SEEN` were sent to make the drawing process easier, but this has now
been removed. This means exploring new land may sometimes change the appearance of existing land (but this is
not fundamentally different from what might happen when you transform land). Sending the extra info, however,
not only confused the goto code but allowed cheating.

Fog of War is the fact that even when you have seen a Tile once you are not sent updates unless it is inside
the sight range of one of your Units or cities.

We keep track of Fog of War by counting the number of Units and cities of each client that can see the Tile.
This requires a number per player, per Tile, so each :code:`player_tile` has a :code:`short[]`. Every time a
Unit, city, or somthing else can observe a Tile 1 is added to its player's number at the Tile, and when it
cannot observe any more (killed/moved/pillaged) 1 is subtracted. In addition to the initialization/loading of
a game this array is manipulated with the :code:`void unfog_area(struct player *pplayer, int x, int y, int
len)` and :code:`void fog_area(struct player *pplayer, int x, int y, int len)` functions. The :code:`int len`
variable is the radius of the area that should be fogged/unfogged, i.e. a ``len`` of 1 is a normal Unit. In
addition to keeping track of Fog of War, these functions also make sure to reveal :code:`TILE_UNKNOWN` Tiles
you get near, and send information about :code:`TILE_UNKNOWN` Tiles near that the client needs for drawing.
They then send the Tiles to the :code:`void send_tile_info(struct player *dest, int x, int y)` function, which
then sets the correct ``known_type`` and sends the Tile to the client.

If you want to just show the terrain and Cities of the square the function :code:`show_area()` does this. The
Tiles remain fogged. If you play without Fog of War all the values of the seen arrays are initialized to 1. So
you are using the exact same code, you just never get down to 0. As changes in the "fogginess" of the Tiles
are only sent to the client when the value shifts between zero and non-zero, no redundant packages are sent.
You can even switch Fog of War on or off in game just by adding or subtracting 1 to all the Tiles.

We only send city and terrain updates to the players who can see the Tile. So a city, or improvement, can
exist in a square that is known and fogged and not be shown on the Map. Likewise, you can see a city in a
fogged square even if the city does not exist. It will be removed when you see the Tile again. This is done by
1) only sending info to players who can see a Tile and 2) keeping track of what info has been sent so the game
can be saved. For the purpose of 2), each player has a Map on the server (consisting of ``player_tile`` and
``dumb_city`` fields) where the relevant information is kept.

The case where a player ``p1`` gives Map info to another player ``p2`` requires some extra information.
Imagine a Tile that neither player sees, but which ``p1`` has the most recent information on. In that case the
age of the players' information should be compared, which is why the player Tile has a ``last_updated`` field.
This field is not kept up to date as long as the player can see the Tile and it is unfogged, but when the Tile
gets fogged the date is updated.

There is a Shared Vision feature, meaning that if ``p1`` gives Shared Vision to ``p2``, every time a function
like :code:`show_area()`, :code:`fog_area()`, :code:`unfog_area()`, or
:code:`give_tile_info_from_player_to_player()` is called on ``p1``, ``p2`` also gets the information. Note
that if ``p2`` gives Shared Vision to ``p3``, ``p3`` also gets the informtion for ``p1``. This is controlled
by ``p1's`` really_gives_vision bitvector, where the dependencies will be kept.

National Borders
----------------

For the display of national Borders (similar to those used in Sid Meier's Alpha Centauri) each Map Tile also
has an ``owner`` field, to identify which nation lays claim to it. If :code:`game.borders` is non-zero, each
city claims a circle of Tiles :code:`game.borders` in Vision Radius. In the case of neighbouring enemy Cities,
Tiles are divided equally, with the older city winning any ties. Cities claim all immediately adjacent Tiles,
plus any other Tiles within the border radius on the same continent. Land Cities also claim Ocean Tiles if
they are surrounded by 5 land Tiles on the same continent. This is a crude detection of inland seas or Lakes,
which should be improved upon.

Tile ownership is decided only by the server, and sent to the clients, which draw border lines between Tiles
of differing ownership. Owner information is sent for all Tiles that are known by a client, whether or not
they are fogged.

Generalized Actions
===================

An action is something a player can do to achieve something in the game. Not all actions are enabler
controlled yet.

Generalized Action Meaning
--------------------------

A design goal for the action sub-system is to keep the meaning of action game rules clear. To achieve this
actions should keep having clear semantics. There should not be a bunch of exceptions to how, for example, an
action enabler is interpreted based on what action it enables. This keeps action related rules easy to
understand for ruleset authors and easy to automatically reason about. Both for parts of Freeciv21 like menus,
help text generation and agents and for third party tools.

Please do not make non-actions into actions because they are similar to actions or because some of the things
Freeciv21 automatically does for actions would be nice to have. Abstract out the stuff you want instead. Make
it apply to both actions and to the thing you wanted.

An action is something a player can order a game entity, the actor, to do. An action does something in the
game itself as defined by the game rules. It should not matter if those game rules run on the Freeciv21 server
or on a human Empire. An action can be controlled by game rules. That control cannot be broken by a patched
client or by a quick player. An action is at the level where the rules apply. A sequence of actions is not an
action. Parts of an action is not an action.

Putting a Unit in a group so they quickly can select it with the rest of the Units in the group and the server
can save what group a Unit belongs to is server side client state, not an action. The rules do not care what
group a Unit belongs to. Adding a Unit to an army where the game rules treat Units in armies different from
Units outside an army, for example by having them attack as one Unit, would be an action.

Putting a Unit under the control of the auto-settlers server side agent is not an action. The player could
modify his client to automatically give the same orders as auto-settlers would have given or even give those
orders by hand.

Leaving a destroyed :unit:`Transport` is not an action. The player cannot order a Unit to perform this action.
Having a Unit destroy its :unit:`Transport` and then leave it is an action. Leaving a :unit:`Transport` "mid
flight", no matter if it was destroyed or not, and having a certain probability of surviving to show up
somewhere else is an action.

Please do not add action (result) specific interpretations of requirements in action enablers. If you need a
custom interpretation define a new actor kind or target kind.

Connections
===========

The code is currently transitioning from 1 or 0 connections per player only, to allow multiple connections
for each player (recall 'player' means a Civilization, see above), where each connection may be either an
"observer" or "controller".

This discussion is mostly about connections on the server. The client only has one real connection
(:code:`client.conn`) – its connection to the server - though it does use some other connection structures
(currently :code:`pplayer->conn`) to store information about other connected clients (e.g., capability
strings).

In the old paradigm, server code would usually send information to a single player, or to all connected
players, usually represented by destination being a ``NULL`` player pointer. With multiple connections per
player things become more complicated. Sometimes information should be sent to a single connection, or to all
connections for a single player, or to all (established) connections, etc. To handle this, "destinations"
should now be specified as a pointer to a :code:`struct conn_list` (list of connections). For convenience the
following commonly applicable lists are maintained:

* :code:`game.all_connections`   -  all connections
* :code:`game.est_connections`   -  established connections
* :code:`game.game_connections`  -  connections observing and/or involved in game
* :code:`pplayer->connections`   -  connections for specific player
* :code:`pconn->self`            -  single connection (as list)

Connections can be classified as follows: (first match applies)

#. :code:`pconn->used == 0`: Not a real connection (closed/unused), should not exist in any list of have any
   information sent to it.

All following cases exist in game.all_connections.

#. :code:`pconn->established == 0`: TCP connection has been made, but initial Freeciv21 packets have not yet
   been negotiated (:code:`join_game` etc.). Exists in :code:`game.all_connections` only. Should not be sent
   any information except directly as result of :code:`join_game` etc. packets, or server shutdown, or
   connection close, etc.

All following cases exist in :code:`game.est_connections`.

#. :code:`pconn->player == NULL`: Connection has been established, but is not yet associated with a player.
   Currently this is not possible, but the plan is to allow this in the future, so clients can connect and
   then see a list of players to choose from, or just control the server, or observe, etc. Two subcases:

   #. :code:`pconn->observer == 0`: Not observing the game. Should receive information about other clients,
      game status etc., but not Map, Units, Cities, etc.

   All following cases exist in game.game_connections.

   #. :code:`pconn->observer == 1`: Observing the game. Exists in :code:`game.game_connections`. Should
      receive game information about Map, Units, Cities, etc.

#. :code:`pconn->player != NULL`: Connected to specific player, either as "observer" or "controller". Exists
   in :code:`game.game_connections`, and in :code:`pconn->player->connections`.


Internationalization (I18N)
===========================

Messages and text in general which are shown in the GUI should be translated by using the :code:`_()` macro.
In addition :code:`qInfo()` and some :code:`qWarning()` messages should be translated. In most cases, the
other log levels (:code:`qFatal()`, :code:`qCritical()`, :code:`qDebug()`, :code:`log_debug()`) should NOT be
localised.

See :file:`utility/fciconv.h` for details of how Freeciv21 handles character sets and encoding. Briefly:

* The data_encoding is used in all data files and network transactions. This is UTF-8.

* The internal_encoding is used internally within Freeciv21. This is always UTF-8 at the server, but can be
  configured by the GUI client. When your charset is the same as your GUI library, GUI writing is easier.

* The local_encoding is the one supported on the command line. This is not under our control, and all output
  to the command line must be converted.
