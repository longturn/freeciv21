Metaserver API
**************

The Metaserver is the service that allows server operators to have their games
listed in the client network page. It uses a simple API based on HTTP and JSON.
By default, the client communicates with the Metaserver at
``https://longturn.net/meta/``; this can be changed at configure time using the
CMake variable ``FREECIV_META_URL``. The rest of this page documents the
`endpoints`_ and `data types`_ used in the Metaserver API.

The two main uses of the Metaserver are to retrieve a list of available servers
and to register a server in the list. To list the servers, simply use
:http:get:`https://longturn.net/meta/`. Registration of a server is done with
:http:post:`https://longturn.net/meta/announce/`. The same endpoint can be used
to renew or modify a registration; inactive servers are removed from the list
after 10 minutes. Finally, when a server is no longer accessible (for instance
when it's shutting down), it should notify the Metaserver by posting to
:http:post:`https://longturn.net/meta/leave/`. This will remove the entry from
the list immediately so that users don't try to connect to a dead server.

Endpoints
=========

.. attention::
    All requests to the Metaserver API must be sent with only
    ``application/json`` in the ``Accept`` HTTP header. In particular, requests
    accepting HTML are reserved for future use.

.. http:get:: https://longturn.net/meta/

    The base URL is used to retrieve the list of all servers currently
    registered with the Metaserver. No filtering is performed: this is the
    responsibility of the client.

    **Example response**:

    .. sourcecode:: http

        HTTP/1.1 200 OK
        Content-Type: application/json

        {
          "servers": [
            {
              "url": "fc21://example.org:5556",
              "message": "Server 1"
            },
            {
              "url": "fc21://example.org:5557",
              "message": "Server 2"
            }
          ],
          "status": "ok"
        }

    The Metaserver responds with a simple list of available servers. Entries in
    the list are encoded as `server objects`_.

.. http:post:: https://longturn.net/meta/announce/

    The ``announce`` endpoint is used by servers that wish to be listed by the
    Metaserver. The request must contain the
    :ref:`server object <Server objects>` that the server wishes to insert into
    the list. The only required field is the URL, which acts as a unique
    identifier for a server.

    Servers added with ``announce`` are listed for approximately 10 minutes
    before they are removed automatically. In order to continue being listed, a
    server needs to ``announce`` its availability every few minutes, keeping the
    same URL. Other fields can be updated by submitting their new value.
    Updating an entry also resets the deletion timeout.

    The host specified in the URL must match the IP address sending the request.
    An error is returned when this is not the case, because it is unlikely that
    someone could reach the server if the Metaserver can't.

    If the request was successful, the Metaserver replies with a short JSON
    object:

    .. sourcecode:: http

        HTTP/1.1 200 OK
        Content-Type: application/json

        {
          "status": "ok"
        }

    Otherwise, a JSON object describing the reason is sent in addition to an
    HTTP error code:

    .. sourcecode:: http

        HTTP/1.1 400 Bad Request
        Content-Type: application/json

        {
          "status": "error",
          "message": "Human-readable message"
        }

    Note that currently, the server may also reply with HTML for some types of
    errors.

.. http:post:: https://longturn.net/meta/leave/

    This endpoint can be used by a server that no longer wishes to be listed.
    The request must contain a :ref:`server object <Server Objects>` with the
    URL to remove. This endpoint is subject to the same IP requirements as
    :http:post:`https://longturn.net/meta/announce/` and the possible responses
    are identical.

Data types
==========

Server objects
--------------

Servers are represented as JSON objects (``{ ... }``) with the attributes listed
below. In most contexts, all attributes except the URL are optional. The server
always sends complete objects with optional fields replaced by empty strings.

.. data:: url

    An URL containing the host and port at which the server can be reached.
    The scheme is always ``fc21://`` and the rest should be ignored.

.. data:: id

    Intended as a unique identifier for the game, but this is not enforced.
    May only contain letters, numbers, underscores, and hyphens.

.. data:: message

    A free-text message set by the server operator.

.. data:: patches

    A short description of patches applied to the server, if applicable.

.. data:: capability

    The server network capability string, used to ensure compatibility.

.. data:: version

    The version of the server.

.. data:: available

    The number of available players in the game.

.. data:: humans

    The number of human players in the game.

.. data:: nations

    All nations in the game (including humans, A.I., barbarians and dead
    players), as a list of `nation objects`_.

Nation objects
--------------

These objects represent nations in the games. They are encoded as JSON objects
(``{ ... }``) with the following attributes:

.. data:: user

    The username of the player currently controlling the nation, or the empty
    string. This is subject to the Freeciv21 restrictions on usernames.

.. data:: nation

    The name of the nation, as defined in the ruleset. This is not translated.

.. data:: leader

    The name of the nation's leader, as chosen at the beginning of the game.

.. data:: type

    The type of player, one of ``Human``, ``A.I.``, ``Barbarian``, or ``Dead``.
    A dash (``-``) may be used to indicate an unknown player type.
