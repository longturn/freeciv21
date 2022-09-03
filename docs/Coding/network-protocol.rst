..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>

Network Protocol
****************

All information passed between the client and server must be sent through the network. Freeciv21 uses a
custom binary protocol. The data flowing between the client and server is segmented in *packets*, each with
a specific meaning and data contents. In most cases, it is enough to know about the packets themselves.
Unless you intend to modify the low-level details, you can safely ignore how packets are serialized to binary
data and transported over the network.

Network and Packets
===================

The basic network code is located in :file:`server/sernet.cpp` and :file:`client/clinet.cpp`.

All information passed between the server and clients, must be sent through the network as serialized packet
structures. These are defined in :file:`common/packets.h`.

For each ``foo`` packet structure, there is one send and one receive function:

.. code-block:: cpp

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

.. code-block:: cpp

    uint16 : length (the length of the entire packet)
    uint16 : type   (e.g. PACKET_TILE_INFO)


For backward compatibility reasons, packets used for the initial protocol (notably before checking the
capabilities) have different header fields sizes as defined in
:file:`common/packets.c`:code:`packet_header_init()`:

.. code-block:: cpp

    uint16 : length (the length of the entire packet)
    uint8  : type   (e.g. PACKET_SERVER_JOIN_REQ)


To demonstrate the route for a packet through the system, here is how a unit disband is performed:

#. A player disbands a unit.
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
#. The handler function checks if the disband request is legal (i.e. the sender really the owner of the unit),
   etc.
#. The unit is disbanded via the :code:`wipe_unit()` and :code:`send_remove_unit()` functions.
#. Now an integer, containing the ``id`` of the disbanded unit is wrapped into a packet along with the type
   :code:`PACKET_REMOVE_UNIT`: :code:`send_packet_generic_integer()`.
#. The packet is serialized and sent across the network.
#. The packet-handler returns and the end of the processing is announced to the client with a
   :code:`PACKET_PROCESSING_FINISHED` packet.
#. On the client the :code:`PACKET_REMOVE_UNIT` packet is deserialized into a :code:`packet_generic_integer`
   structure.
#. The corresponding client handler function is now called :code:`handle_remove_unit()`, and finally the unit
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
the AIs move all their units. Unit moves in particular take a long time for the client to process since by
default smooth unit moves is on.

There are 3 ways to solve this problem:

#. We wait for the send buffers to drain before continuing processing.
#. We cut the player's connection and empty the send buffer.
#. We lose packets (this is similar to 2), but can cause an incoherent state in the client.

We mitigated the problem by increasing the send buffer size on the server and making it dynamic. We also added
in strategic places in the code calls to a new :code:`flush_packets()` function that makes the server stall
for some time draining the send buffers. Strategic places include whenever we send the whole map. The maximum
amount of time spent per :code:`flush_packets()` call is specified by the ``netwait`` variable.

To disconnect unreachable clients, the server pings the
client after a certain time elapses (set using the :literal:`pingtimeout` variable). If the client does not
reply its connection is closed.

Utilizing Delta for Network Packets
===================================

If delta is enabled for this packet, the packet-payload (after the bytes used by the packet-header) is followed
by the ``delta-header``. See :doc:`hacking`, in the "Network and Packets" chapter, to learn how to understand
the packet-header. The ``delta-header`` is a bitvector which represents all non-key fields of the packet. If
the field has changed the corresponding bit is set and the field value is also included in ``delta-body``. The
values of the unchanged fields will be filled in from an old version at the receiving side. The old version
filled in from is the previous packet of the same kind that has the same value in each key field. If the
packet's kind do not have any key fields the previous packet of the same kind is used. If no old version
exists the unchanged fields will be assumed to be zero.

For a ``bool`` field, another optimization called ``bool-header-folding`` is applied. Instead of sending an
indicator in the bitvector if the given ``bool`` value has changed, and so using 1 byte for the real value,
the actual value of the ``bool`` is transfered in the bitvector bit of this ``bool`` field.

Another optimization called ``array-diff`` is used to reduce the amount of elements transfered if an array is
changed. This is independent of the ``delta-header`` bit, i.e. it will only be used if the array has changed
its value and the bit indicates this. Instead of transferring the whole array only a list of ``index`` and
``new value of this index`` pairs are transferred. The ``index`` is 8 bit and the end of this pair list is
denoted by an ``index`` of 255.

For fields of struct type (or arrays of struct) the following function is used to compare entries, where foo
stands for the name of the struct:

.. code-block:: rst

    bool are_foo_equal(const struct foo *a, const struct foo *b);


The declaration of this function must be made available to the generated code by having it :code:`#include`
the correct header. The includes are hard-coded in :file:`generate_packets.py`.

Compression
===========

To further reduce the network traffic between the client and the server, the (delta) packets are compressed
using the DEFLATE compression algorithm. To get better compression results, multiple packets are grouped
together and compressed into a chunk. This chunk is then transfered as a normal packet. A chunk packet starts
with the 2 byte ``length`` field, which every packet has. A chunk packet has no type. A chunk packet is
identified by having a too large ``length`` field. If the length of the packet is over ``COMPRESSION_BORDER``,
it is a chunk packet. It will be uncompressed at the receiving side and re-fed into the receiving queue.

If the ``length`` of the chunk packet cannot be expressed in the available space of the 16bit ``length`` field
(>48kb), the chunk is sent as a jumbo packet. The difference between a normal chunk packet and a jumbo chunk
packet is that the jumbo packet has ``JUMBO_SIZE`` in the ``size`` field and has an additional 4 byte
``length`` field after the 2 byte ``length`` field. The second ``length`` field contains the size of the whole
packet (2 byte first ``length` field + 4 byte second ``length`` field + compressed data). The size field of a
normal chunk packet is its ``size`` + ``COMPRESSION_BORDER``.

Packets are grouped for the compression based on the ``PACKET_PROCESSING_STARTED/PACKET_PROCESSING_FINISHED``
and ``PACKET_FREEZE_HINT/PACKET_THAW_HINT`` packet pairs. If the first (freeze) packet is encountered the
packets till the second (thaw) packet are put into a queue. This queue is then compressed and sent as a chunk
packet. If the compression would expand in size the queued packets are sent uncompressed as "normal" packets.

The compression level can be controlled by the ``FREECIV_COMPRESSION_LEVEL`` environment variable.

Files
=====

There are four file/filesets involved in the delta protocol:

#. The definition file: (:file:`common/networking/packets.def`).
#. The packet generator file: (:file:`common/generate_packets.py`).
#. The generated files: :file:`*/*_gen.[cpp,h]` or as a list :file:`client/civclient_gen.cpp`,
   :file:`client/packhand_gen.h`, :file:`common/packets_gen.cpp`, :file:`common/packets_gen.h`,
   :file:`server/hand_gen.h`, and :file:`server/srv_main_gen.cpp`.
#. The overview (this document)

The definition file lists all valid packet types with their fields. The generator takes this as input and
creates the generated files.

For adding and/or removing packets and/or fields you only have to touch the definition file. If you however
plan to change the generated code (adding more statistics for example) you have to change the generator.

Changing The Definition File
============================

Adding a packet:

#. Choose an unused packet number. The generator will make sure that you do not use the same number two times.
#. Choose a packet name. It should follow the naming style of the other packets:
   ``PACKET_<group>_<remaining>``. The ``<group>`` may be ``SERVER``, ``CITY``, ``UNIT``, ``PLAYER``, and ``DIPLOMACY``.
#. Decide if this packet goes from server to client or client to server.
#. Choose the field names and types.
#. Choose packet and field flags.
#. Write the entry into the corresponding section of :file:`common/networking/packets.def`.

If you add a field which is a struct (say :code:`foobar`), you have to write the following functions:
:code:`dio_get_foobar()`, :code:`dio_put_foobar()`, and :code:`are_foobars_equal()`.

Removing a packet:

#. Add a mandatory capability string.
#. Remove the entry from :file:`common/networking/packets.def`.

Adding a field:

Option A:

#. Add a mandatory capability string.
#. Add a normal field line: ``COORD x``.

Option B:

#. Add a non-mandatory capability string (i.e. "new_version").
#. Add a normal field line containing this capability in an add-cap flag: ``COORD x``; add-cap(new_version)

Removing a field:

Option A:

#. Add a mandatory capability string.
#. Remove the corresponding field line.

Option B:

#. Add a non-mandatory capability (i.e. "cleanup")
#. Add to the corresponding field line a remove-cap flag

Capabilities and Variants
=========================

The generator has to generate code which supports different capabilities at runtime according to the
specification given in the definitions with the ``add-cap()`` and ``remove-cap()`` functions. The generator
will find the set of used capabilities for a given packet. Let us say there are two fields with
``add-cap(cap1)`` and one field with a ``remove-cap(cap2)`` flag. So the set of capabilities are ``cap1`` and
``cap2``. At runtime the generated code may run under 4 different capabilities:

* Neither ``cap1`` nor ``cap2`` are set.
* ``cap1`` is set, but ``cap2`` is not.
* ``cap1`` is not set, but ``cap2`` is set.
* ``cap1`` and ``cap2`` are set.

Each of these combinations is called a variant. If ``n`` is the number of capabilities used by the packet the
number of variants is :math:`2^n`.

For each of these variants a seperate send and receive function will be generated. The variant for a packet and
a connection is calculated once and then saved in the connection struct.
