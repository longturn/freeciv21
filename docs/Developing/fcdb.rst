Authentication and Database Support (fcdb)
******************************************

The Freeciv21 server allows the authentication of users, although by default it is not enabled, and anyone can
connect with any name.

In order to support authentication, the Freeciv21 server needs access to a database backend in which to store
the credentials. To support different database backends, the database access code is written in Lua using
luasql. In principle, luasql supports SQLite3, MySQL, and Postgres backends. However, the Freeciv21 server is
only built with SQLite3.

As well as storing and retrieving usernames and passwords, the supplied database access script logs the time
and IP address of each attempted login, although this information is not used by the Freeciv21 server itself.

To use the Freeciv21 database and authentication, the server must be installed properly, as it searches for
:file:`database.lua` in the install location; the server cannot simply be run from a build directory if
authentication is required.

Quick Setup: SQLite
===================

The simplest setup is to use the SQLite backend, and this is probably the best option for new deployments. In
this setup, the authentication data is stored in a simple file accessed directly by the Freeciv21 server;
there is no need for a separate database server process.

To set this up, first create a database configuration file called something like :file:`fc_auth.conf`, with
the 'database' key specifying where the database file is to live (of course it must be readable and writable
by the Freeciv21 server):

.. code-block:: rst

    [fcdb]
    backend="sqlite"
    database="/my/path/to/freeciv21.sqlite"


For more information on the format of this file, see below. There are more settings available, but this file
is entirely sufficient for a SQLite setup.

Now start the server with:

.. code-block:: rst

    freeciv21-server --Database fc_auth.conf --auth --Newusers


The first time you do this, you need to create the database file and its tables with the following server
command:

.. code-block:: rst

    /fcdb lua sqlite_createdb()


Now you can create some users by connecting with the client; due to the :code:`--Newusers` flag, when you
connect with the client with a previously unknown username, the server will prompt for a password and save the
new account to the database.

You may want to prepopulate the users table this way and then restart the server without :code:`--Newusers`
for the actual game, or you can run the game with :code:`--Newusers`.

Advanced SQLite Usage
---------------------

SQLite supports working with a temporary database in memory which is never written to disk. To do this,
specify :code:`database=":memory:"` in the configuration file. The database will last only for the lifetime of
the freeciv-server process; its contents will be lost if the server quits or crashes (it is not saved in the
saved game file). You'll probably need the :code:`--Newusers` option :)

Command-line Options
====================

The following server command-line options are relevant to authentication:

* :code:`-D`, :code:`--Database <conffile>`: Specifies a configuration file describing how to connect to the
  database. Without this, all authentication will fail.
* :code:`-a`, :code:`--auth`: Enable authentication. Without this, anyone will be able to connect without
  authentication, and :code:`--Database` has no effect.
* :code:`-G`, :code:`--Guests`: Allow guests. These are usernames with names starting "guest". If enabled, any
  number of guests may connect without accounts in the database; if a guest name is already in use by a
  connection, a new guest name is generated. Once connected, guests have the same privileges as any other
  account. If this option is not specified, accounts are required to connect, and guest account names are
  forbidden.
* :code:`-N`, :code:`--Newusers`: Allow Freeciv21 clients to create new user accounts through the Freeciv21
  protocol. Without this, only accounts which already exist in the database can connect (which might be
  desirable if you wants users to register via a web front end, for instance).

Lua script database.lua
=======================

This script is responsible for checking usernames, fetching passwords, and saving new users (if
:code:`--Newusers` is enabled). It encapsulates access to the database backend, and hence the details of the
table layout.

The script lives in :file:`data/database.lua` in the source tree, and is installed to 'CMAKE_INSTALL_PREFIX';
depending on the options given to 'cmake' at build time, this may be a location like
:file:`/usr/local/etc/freeciv21/database.lua.`

The supplied version supports basic authentication against a SQLite database; it supports configuration as
shown in the following example:

.. code-block:: rst

    [fcdb]
    backend="sqllite"
    host="localhost"
    user="Freeciv21"
    port="3306"
    password="s3krit"
    database="Freeciv21"
    table_user="auth"
    table_log="loginlog"


If that's sufficient for you, it's not necessary to read on.  Freeciv21 expects the following lua functions
to be defined in :file:`database.lua`:

* try to load data for an existing user
* return TRUE if user exists, FALSE otherwise function :code:`user_load(conn)`
* save a new user to the database function :code:`user_save(conn)`
* log the connection attempt (success is boolean) function :code:`user_log(conn, success)`
* test and initialise the database connection function :code:`database_init()`
* free the database connection function :code:`database_free()`

Where 'conn' is on object representing the connection to the client which requests access.

The return status of all of these functions should be one of:

.. code-block:: rst

    fcdb.status.ERROR
    fcdb.status.TRUE
    fcdb.status.FALSE


indicating an error, a positive or a negative result. The following lua functions are provided by Freeciv21:

* return the client-specified username :code:`auth.get_username(conn)`.
* return the client IP address (string) :code:`auth.get_ipaddr(conn)`.
* tell the server (the MD5 hash of) the correct password to check against.
* for this connection (usually to be called by :code:`user_load()`).
* returns whether this succeeded :code:`auth.set_password(conn, password)`.
* return (the MD5 hash of) the password for this connection (as specified by the client in :code:`user_save()`,
  or as previously set by :code:`set_password()`.
* :code:`auth.get_password(conn)`.
* return a value from the :code:`--Database` configuration file :code:`fcdb.option(type)`.

'type' selects one of the entries in the configuration file by name (for instance :code:`fcdb.option("backend")`).

Freeciv21 also provides some of the same Lua functions that ruleset scripts get -- log.*(), _(), etc -- but
the script is executing in a separate context from ruleset scripts, and does not have access to signals, game
data, etc.
