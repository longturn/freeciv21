.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: XHawk87 <hawk87@hotmail.co.uk>

.. include:: /global-include.rst

.. _manuals-advanced-docker:

Docker
******

This is a step-by-step guide on how to set up a freeciv21 server running in a
docker container on a linux server. This is intended purely as an example,
there are many different ways you can configure and arrange server files.

Preparing the server
====================

First, we need to make sure that git and docker are installed as they will both
be necessary to obtain sources, rulesets, build the server images, and run the
games.

.. code-block:: sh

  git --version
  docker --version

For this guide, we're going to be using Docker Rootless mode for better
container isolation. See: https://docs.docker.com/engine/security/rootless/
However, running docker as root should also work fine with some tweaks.

Whatever version of git your distro provides should be fine:

.. code-block:: sh

  sudo apt install git

Next, we decide where we want to create our server directory. Here, we're
just putting it in the home directory:

.. code-block:: sh

  mkdir freeciv21
  cd freeciv21
  mkdir servers
  mkdir games

The :file:`servers` directory will be used to maintain server sources for each
different version being used to run a game. The :file:`games` directory will be
used to maintain the active games.

Now, we create a group and user for running fc21 servers for better isolation. 

.. code-block:: sh

  sudo groupadd --system --gid 100555 freeciv21
  sudo useradd --system --uid 100555 --gid freeciv21 --shell /usr/sbin/nologin freeciv21
  sudo usermod -aG freeciv21 "$USER"
  newgrp freeciv21
  newgrp "$USER"

Here, we're going with the default UID and GID of ``556`` which gets mapped to
``100555`` by Docker's rootless mode user namespace mapping, and granting the
current user access to this group, so we can manage server files directly.

.. note::

  If you change the UID and GID, you also need to set the UID and GID args to
  match when building the server image in docker.

.. _getting-install-docker:

Build a freeciv21-server docker image
=====================================

In this example, we're going to build the latest stable build of the server:

.. code-block:: sh

  cd servers
  git clone https://github.com/longturn/freeciv21.git --branch stable stable-latest
  cd stable-latest
  docker build -t freeciv21-server:stable-latest .

See :ref:`Packaging with Docker <coding-packaging-docker>` for detailed build 
instructions.

Setting up a new game
=====================

In this example, we're going to set up an LTT2 game from the LTT-LTX repo.

.. code-block:: sh

  cd ~/freeciv21/games
  mkdir test01
  cd test01
  git clone https://github.com/longturn/LTT-LTX.git rulesets
  mv rulesets/LTT2 data
  mkdir saves logs config
  sudo chown freeciv21:freeciv21 saves logs
  wget -O config/database.lua https://raw.githubusercontent.com/longturn/freeciv21/refs/heads/master/lua/database.lua

Authentication will use an sqlite database stored in the saves directory. We
need to create a :file:`config/fc_auth.conf` file with these contents:

.. code-block:: ini

  [fcdb]
  backend="sqlite"
  database="/home/freeciv21/saves/fcdb.sqlite"

.. warning::

  The database path is internal to the container and should not be changed
  unless you mount it in a different location when running it in docker.

You should fill out :file:`data/LTT2/players.serv` with the players for this game,
and modify :file:`data/LTT2/LTT2.serv` with any game-specific settings. See
:doc:`Server Settings File </Manuals/Server/settings-file>`

For a proper game, it is recommended to have these filled out already in a git
repo branch and clone that instead of cloning the LTT-LTX repo directly so that
they can be easily recovered or built upon for future games.

Running freeciv21-server from a docker image
============================================

To set up a new game:

.. code-block:: sh

  game_dir="$HOME/freeciv21/games/test01"
  docker run --rm --interactive --tty \
    --name freeciv21-test01 \
    --mount "type=bind,source=$game_dir/data,destination=/usr/local/share/freeciv21,ro" \
    --mount "type=bind,source=$game_dir/saves,destination=/home/freeciv21/saves" \
    --mount "type=bind,source=$game_dir/logs,destination=/home/freeciv21/logs" \
    --mount "type=bind,source=$game_dir/config,destination=/home/freeciv21/.config/freeciv21,ro" \
    --publish "5556:5556/tcp" \
    freeciv21-server:stable-latest \
    --log '/home/freeciv21/logs/server.log' \
    --saves '/home/freeciv21/saves' \
    --auth --Database '/home/freeciv21/.config/freeciv21/fc_auth.conf' \
    --Newusers

.. note::

  The :file:`$game_dir/` paths are external to the host, and the destinations
  and later paths are internal to the container. Make sure not to mix these up
  as it will lead to it not being able to locate those files.

If you want to host multiple games on the same machine, you will need to change
the ``--publish $port:5556/tcp`` line to a different ``$port`` to connect to the
game. The second port number is only used within the container and should not
be changed.

The example uses a sqlite database for authentication. Please see
:ref:`Authentication and Database Support <manuals-server-fcdb>` for more
details.

To set up the database, run:

.. code-block:: rst

    /fcdb lua sqlite_createdb()

The ``--Newusers`` argument allows players to join and create their accounts.
When you're ready to start the game, ``/quit`` to shut down the server and omit
this option to require authentication.

To start the game:

.. code-block:: sh

  game_dir="$HOME/freeciv21/games/test01"
  docker run --rm --interactive --tty --detach \
    --name freeciv21-test01 \
    --mount "type=bind,source=$game_dir/data,destination=/usr/local/share/freeciv21,ro" \
    --mount "type=bind,source=$game_dir/saves,destination=/home/freeciv21/saves" \
    --mount "type=bind,source=$game_dir/logs,destination=/home/freeciv21/logs" \
    --mount "type=bind,source=$game_dir/config,destination=/home/freeciv21/.config/freeciv21,ro" \
    --publish "5556:5556/tcp" \
    freeciv21-server:stable-latest \
    --log '/home/freeciv21/logs/server.log' \
    --saves '/home/freeciv21/saves' \
    --auth --Database '/home/freeciv21/.config/freeciv21/fc_auth.conf' \
    --read LTT2.serv

Note the changes, ``--detach`` will run the container in the background so that
it's not tied to your current user session.

``--read $ruleset`` will load and start the game with the settings you set up 
earlier. See :ref:`--read <manuals-server-command_line-read>`.

To load a scenario or save game:

.. code-block:: sh

  game_dir="$HOME/freeciv21/games/test01"
  docker run --rm --interactive --tty --detach \
    --name freeciv21-test01 \
    --mount "type=bind,source=$game_dir/data,destination=/usr/local/share/freeciv21,ro" \
    --mount "type=bind,source=$game_dir/saves,destination=/home/freeciv21/saves" \
    --mount "type=bind,source=$game_dir/logs,destination=/home/freeciv21/logs" \
    --mount "type=bind,source=$game_dir/config,destination=/home/freeciv21/.config/freeciv21,ro" \
    --publish "5556:5556/tcp" \
    freeciv21-server:test01 \
    --log '/home/freeciv21/logs/server.log' \
    --saves '/home/freeciv21/saves' \
    --auth --Database '/home/freeciv21/.config/freeciv21/fc_auth.conf' \
    --file '/home/freeciv21/saves/freeciv-T0003-Y-3800-interrupted.sav.gz' 

Listing running games
=====================

.. code-block:: sh

  docker ps -f 'name=freeciv21-'

Attach and Detach from a running game
=====================================

If you want to attach to the running game's console:

.. code-block:: sh

  docker attach freeciv21-test01

.. note::

  The standard way to detach from a docker container without killing it is to
  press Ctrl-P followed by Ctrl-Q. If you press Ctrl-C, it will interrupt the
  server.

Sending commands to a running game
==================================

If you need to run bash commands inside the running game container:

.. code-block:: sh

  docker exec -it --user root freeciv21-test01 /bin/bash -l

Omit ``--user root`` to run commands without elevation.

Omit ``-it`` and replace ``/bin/bash -l`` with a single command to run if you don't
need it to be an interactive session.

Safe Shutdown
=============

To stop a running game, it's a good idea to attach to the server to save the
game and stop it manually using server commands. However, you can use:

.. code-block:: sh

  docker stop --time=600 freeciv21-test01

.. note::

  The ``--time`` argument can be omitted in most cases as the default interval of
  10 seconds is sufficient to allow for graceful shutdown in most cases,
  however for large maps with lots of cities, the server can be slow to respond
  at TC and might need longer.

