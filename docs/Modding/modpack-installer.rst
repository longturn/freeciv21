Serving Modpacks for the Modpack Installer
******************************************

This document discusses how to set up a web server so that users can download modpacks you publish.

To host modpacks, you just need a web server that can host plain static files; you do not need to run any
custom code or frameworks on that web server, just to publish files with a specific layout, detailed below.

On the modpack server, there are up to three layers of files required:

1. A list of available modpacks (optional, advanced).
2. One control file per modpack.
3. The individual files comprising the modpacks.

Each of these layers is described in detail below.

Each layer refers to files in the next layer down. References can be with relative URLs (so that a modpack
or set of modpacks can be moved without changing any file contents), or with absolute URLs (so that the
different layers can be hosted on different web servers).

Almost all of these file formats are specific to one major version of Freeciv21; this document only
describes the formats for the major version of Freeciv21 it is shipped with.


1. List of Modpacks
===================

This is only needed if you want to let users browse a list of available modpacks before choosing one to
install. To look at your modpack list instead of the standard one, users will usually have to start the
modpack installer with non-standard arguments (see above).

The modpack list is a standard :literal:`JSON` file with a specific structure.

Here's an example:

.. code-block:: rst

    {
      "info": {
        "options": "+modpack-index-1.0",
        "message": "Example modpack list loaded successfully"
      },
      "modpacks": [
        {
          "name": "Example",
          "version": "0.0",
          "license": "WTFPL",
          "type": "Modpack",
          "url": "https://example.com/example.json",
          "notes": "This is an example"
        }
      ]
    }


This file uses the modpack index format version 1.0, as is indicated in the :literal:`info.options` field.
The optional :literal:`info.message` is displayed on the status line when the modpack installer starts up.
It should be kept to one line.

The modpacks list contains a list of modpacks. This example contains just one modpack. Each modpack may
contain the following fields:

"name", "version", "type"
  These three fields should match those in the :literal:`.json` file which :literal:`URL` links to.

"subtype"
  Optional free text. For tilesets or scenarios, conventionally indicates the map topology with one of
  :literal:`overhead`, :literal:`iso`, :literal:`hex`, or :literal:`hex & iso` (and these will be
  localised). Otherwise use :literal:`-`.

"license"
  Free text summarising the distribution terms for the modpack content, by naming a well-known license, not
  quoting the full license text! Consider using SPDX identifiers (https://spdx.org/licenses/).

"URL"
  The URL to a :literal:`.json` file for the individual modpack. The URL can be either relative in which
  case it's relative to the URL of :literal:`modpack.list`, or absolute - which can be on some other web
  server.

"notes"
   Optional free text; usually shown as a tooltip.


2. Control File: Defining an Individual Modpack
===============================================

This is the core control file for a modpack, specifying what files it contains, where to download them from,
and where they are installed.

Some modpack authors will just publish the URL of an :literal:`.json` file directly, for users to give to
the modpack installer tool. There doesn't have to be a :literal:`modpack.list` file anywhere that refers to
the :literal:`.json` file.

Again, this is a file in standard :literal:`JSON` format. Its filename must end in :literal:`.json`.

Here is an example of a modpack control file:

.. code-block: rst

    {
      "info": {
        "options": "+modpack-1.0",
        "base_url": ".",
        "name": "Some ruleset",
        "type": "Modpack",
        "version": "0.0"
      },
      "files": [
        "some_ruleset.serv",
        "some_ruleset.tilespec",
        "some_ruleset/nation/german.ruleset",
        "some_ruleset/nation/indian.ruleset",
        ...
      ]
    }


The :literal:`info` section has overall control information:

"options"
  Defines the version of the file format. Should be exactly as shown in the example.

"name"
  A short name for the modpack. This is used for version and dependency tracking, so should not
  contain minor version information, and should not change once a modpack has been released for a given
  major version of Freeciv21. Case-insensitive.

"version"
  Textual version information. If another modpack uses this one as a dependency, this string is
  subject to version number comparison (using the rules of Freeciv21's :literal:`cvercmp` library, which
  should give sensible results for most version numbering schemes).

"type"
  This must be one of the following:

* :strong:`Ruleset`: :literal:`foo.serv`, :literal:`foo/*.ruleset`, :literal:`foo/*.lua`, etc.
* :strong:`Tileset`: :literal:`foo.tilespec`, :literal:`foo/*.png`, etc.
* :strong:`Soundset`: :literal:`foo.soundspec`, :literal:`foo/*.ogg`, etc.
* :strong:`Musicset`: :literal:`foo.musicspec`, :literal:`foo/*.ogg`, etc.
* :strong:`Scenario`: :literal:`foo.sav`; installed to a version-independent location.
* :strong:`Modpack`: Conventionally used for modpacks that contain more than one of the above kinds of material
* :strong:`Group`: Contains no files but only depends on other modpacks At the moment, only
  :literal:`Scenario` causes special behavior.

"base_url"
  URL to prepend to the :literal:`src` filenames in the :literal:`files` list. May be relative to the
  :literal:`.json` file -- starting with :literal:`./` -- or absolute in which case the files can be on some
  web server different to where the :literal:`.json` file lives.

The :strong:`files` list defines the individual files comprising your modpack. It must list every file
individually; any files in the same directory on the webserver that are not listed will not be downloaded.
Entries can be strings as shown above, in which case the same file name is used for downloading relative to
:literal:`info.base_url` and installing relative to the data directory. If the installed name is different
from the name on the server, the following syntax can be used instead:

.. code-block: rst

    {
      "url": "some-remote-file",
      "dest": "where-to-install-it"
    }


The URL can be either relative (to :literal:`info.base_url`) or absolute. The two syntaxes can be mixed in
the same modpack.

.. note:: Forward slash :literal:`/` (and not backslash :literal:`\\`) should be used to separate directories.

Some advice on the structure of files in modpacks:

* You should generally install files in a directory named after the modpack, with a few exceptions
  (:literal:`.serv`, :literal:`.tilespec`, :literal:`.soundspec`, and :literal:`.musicspec` files must be
  installed to the top level, and should reference files in your subdirectory). Individual files and
  directories install names should usually not embed version numbers, dates, etc., so that when a new version
  of modpack X is installed, it cleanly overwrites the old   version, rather than leaving both cluttering up
  the user's installation.

* The modpack installer does not stop different modpacks overwriting each other's files, so published
  modpacks should be disciplined about namespace usage. If you've derived from someone else's modpack, you
  should probably give your derivative new filenames, so that both can be installed simultaneously.

* There is no :emphasis:`white-out` facility to delete files from a user's installation -- if a newer
  version of a modpack has fewer files than an old one, the old file will persist in some users'
  installations, so your modpacks should be designed to be tolerant of that.

* At the moment, there is no restriction on what kind of files a given :emphasis:`type` can install, but
  modpacks should stick to installing the advertised kinds of content. It's :strong:`OK` to install extra
  files such as documentation in any case (:file:`LICENSE/COPYING`, :file:`README.txt`, etc.).

* If your modpack contains a ruleset, you should usually install a :literal:`.serv` file at the top level
  (which can be a one-line file consisting of :literal:`rulesetdir <name>`, as this is needed for the server
  to enumerate the available rulesets.

In some cases, a modpack may depend on other modpacks, for instance if it reuses some of their files. This
can be handled by declaring a dependency with respect to the other modpack. Dependencies are listed in the
optional :literal:`dependencies` list of the :literal:`JSON` file. Each entry in that list must contain the
following object:

.. code-block: rst

    {
      "modpack": "...",
      "url": "...",
      "type": "...",
      "version": "..."
    }


The keys are explained below:

* :strong:`modpack`: What the dependency modpack calls itself when installed (that is, :literal:`name`
  from its :literal:`.json` file).
* :strong:`url`: URL to download modpack if needed. Can be relative or absolute.
* :strong:`type`:  Must match :literal:`type` from dependency's :literal:`.json` file.
* :strong:`version`: Minimum version of dependency (as declared in its :literal:`.json` file). Subject to
  version number comparison algorithm.

If the modpack installer thinks the required version, or a newer version, of the dependency is already
installed, it will do nothing, otherwise it will download the dependency modpack, and any of its own
dependencies, recursively.


3. Individual Modpack Files
===========================

These are the files comprising the modpack (:literal:`*.ruleset`, :literal:`*.png`, etc.), that will be
copied verbatim to the user's Freeciv21 profile directory and read by the Freeciv21 client and server. The
modpack installer does not modify the files in any way.

The files must be hosted individually on the web server; the modpack installer tool cannot unpack any
archives such as :file:`.zip` files. Individual scenarios can be compressed (e.g. :file:`.sav.gz`, as the
Freeciv21 engine can uncompress these files.

Because the :literal:`*.json` file can change the file paths / names on download, the layout on the modpack server
doesn't have to correspond with the installed layout. An individual file can be shared between multiple
modpacks, if you want.
