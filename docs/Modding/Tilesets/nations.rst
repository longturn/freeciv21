Adding a Nation
***************

There are already many nations, but of course some of them are missing. We're quite open to new "nations" even
if they're just part of a larger country, but obscure towns that nobody knows will be rejected. Things like US
states or German Länder are OK. South African provinces are probably not.

The following information is required to add a new nation:

* The name of the citizens (singular and plural), for instance `Indian` and `Indians`.
* A set of "groups" under which the nation will be shown in the client. Usually, you can copy the groups from
  an existing nation.
* A short historical description of the nation, shown in the Help dialog.
* Several (real) leader names, preferably both men and women. Prefer recognized historically important leaders,
  eg `Henri IV <https://en.wikipedia.org/wiki/Henry_IV_of_France>`_ and
  `Napoléon Bonaparte <https://en.wikipedia.org/wiki/Napoleon>`_ but not
  `Jean Casimir-Perier <https://en.wikipedia.org/wiki/Jean_Casimir-Perier>`_.
* Special names for the ruler in certain governments, if applicable. For instance, when in Despotism the
  Egypian leader is called *Pharaoh*.
* A flag, preferably something official (if your nation doesn't have an official flag, think twice before
  including it). Wikipedia has many of those under free licenses.
* A few nations that will be preferred by the server when there's a civil war. For instance, the Conferderate
  are a civil war nation for the Americans.
* City names. List each city only once, even if it changed name in the course of history. Try to use names
  from the same epoch.

Most of the information goes into your nation's ```.ruleset``` file. There are
`many examples <https://github.com/longturn/freeciv21/blob/master/data/nation/egyptian.ruleset>`_ in the
repository. You will also need to include the nation in
`this list <https://github.com/longturn/freeciv21/blob/master/data/nation/CMakeLists.txt>`_ and
`this list <https://github.com/longturn/freeciv21/blob/master/data/default/nationlist.ruleset>`_.

Four variants of the flag are needed:

* A "small" flag, 29x20 with a 1px black border
* A "large" flag, 44x30 with a 1px black border
* A "shield", 14x14 with a 1px black border and a shield shape
* A "large shield", 19x19 with a 1px black border and a shield shape

They go in the `flags <https://github.com/longturn/freeciv21/tree/master/data/flags>`_ directory, and you can
find examples there.
