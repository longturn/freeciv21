..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

Agents
******

An agent is a piece of code which is responsible for a certain area. An agent will be given a specification by
the user of the agent and a set of objects which the agent can control (the production queue of a city, a
city, a unit, a set of units or the whole empire). The user can be a human player or another part of the code
including another agent. There is no extra interaction between the user and the agent needed after the agent
got its task description.

Examples of agents:

* An agent responsible for moving a certain Unit from Tile A to Tile B
* An agent responsible for maximize the food production of a city
* An agent responsible for the production queue of a city
* An agent responsible for defending a city
* An agent responsible for a city
* An agent responsible for all cities

An agent may use other agents to accomplish its goal. Such decencies form a hierarchy of agents. The position
in this hierarchy is denoted by a level. A higher level means more complexity. So an agent of level ``n`` can
only make use of agents of level (``n-1``) or lower. Level ``0`` defines actions which are carried out at the
server and are atomic actions (actions which cannot be simulated at the client).

By such a definition, an agent does not have to be implemented in C++ and also does not have to make use of
:file:`client/governor.[cpp,h]`.

The core of an agent consist of two parts: a part which makes decisions and a part which carries out the
decision. The results of the first part should be made available. An agent lacking the execution part is
called advisor.

An agent should provide a GUI besides the core.

Implementation
==============

The received task description and any decisions made can be saved in attributes. An agent should not
assume anything. This especially means :strong:`no magic numbers`. Everything should be configurable by
the user.

Use :file:`client/governor.[cpp,h]` to get informed about certain events. Do not hesitate to add more
callbacks. Use the :file:`client/governor`:code:`wait_for_requests()` function instead of the
:file:`client/civclient`:code:`wait_till_request_got_processed()` function.
