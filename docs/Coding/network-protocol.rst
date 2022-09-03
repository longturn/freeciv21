..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>

Network Protocol
****************

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
