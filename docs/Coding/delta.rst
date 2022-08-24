..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

Utilizing Delta for Network Packets
***********************************

If delta is enabled for this packet the packet-payload, after the bytes used by the packet-header, is followed
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
using the ``DEFLATE`` compression algorithm. To get better compression results, multiple packets are grouped
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

For each of these variant a seperate send and receive function will be generated. The variant for a packet and
a connection is calculated once and then saved in the connection struct.
