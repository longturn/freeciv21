Using the Modpack Installer Utility
***********************************

Installing Modpacks
===================

The modpack installer is a simple tool to download custom Freeciv21 content called :strong:`modpacks` from 
the Internet, and automatically install them into the correct location to be used by the Freeciv21 client and 
server.

A :strong:`modpack` can consist of one or more of:

* :strong:`rulesets` for game rules
* :strong:`scenarios` for game maps (with or without players/cities/etc)
* :strong:`tilesets` for game graphics
* :strong:`soundsets` or :strong:`musicsets` for sound effects and in-game music

They can be installed using the modpack installer tool :file:`freeciv21-modpack-qt` that comes with the 
standard installation. There is also a command-line-only tool called :file:`freeciv21-modpack`, which is 
mainly useful for headless game servers.

In standard Freeciv21 builds, when you start the graphical modpack installer, it will show a list of 
modpacks curated by the Freeciv21 developers and longturn.net community. You can select one and click 
:guilabel:`Install modpack`; the tool will download the files for the selected modpack, and any other 
modpacks it depends on, and install them ready for the main Freeciv21 programs to use.

.. image:: ../_static/images/gui-elements/modpack-installer.png
    :align: center
    :alt: Freeciv21 Modpack Installer Qt GUI application

You can also point the installer at modpacks or lists on other servers:

* If someone has given you an individual modpack URL (ending in :literal:`.json`), you can paste it into the 
  :guilabel:`Modpack URL` box, and when you click :guilabel:`Install modpack`, the modpack will immediately  
  downloaded and install. 

.. note:: If you have been given a URL ending up :literal:`.modpack` or :literal:`.mpdl`, 
    that is probably for legacy Freeciv and not Freeciv21.

* If someone has given you a URL for a list of modpacks (also ending in :literal:`.json`) and you want to 
  browse them with the standard Freeciv21 modpack installer build, you need to start the tool with that URL  
  on its command line, for instance:

.. code-block:: rst

    freeciv21-modpack --List https://example.com/3.1/my-modpacks.json


.. note:: The capital letter in :literal:`--List`

    The tool has some other command-line options, but most users will not need to use them. Use 
    :literal:`--help` for a list of them.

Once you have installed a modpack, how you use it depends on the modpack type:

* Scenarios (maps) should be listed under :guilabel:`Start scenario game` from the client start page, or 
  from the game server prompt via :literal:`/list scenarios`.

.. tip:: For network play, scenarios need only be installed on the game server.

* Rulesets should appear on the :guilabel:`Ruleset` drop-down from the client's :guilabel:`Start new game` 
  page. On the game server, you can load a ruleset with :literal:`/read <name>` or failing that perhaps 
  :literal:`/rulesetdir <name>`.

* Tilesets should appear for selection in the local client options, in the appropriate topology-specific 
  :guilabel:`Tileset` drop-down under :guilabel:`Graphics`.

.. note:: Tilesets should be installed on the client machine.

* Soundsets and musicsets should appear in the dropdowns on the :guilabel:`Sound` page of the local 
  client options.

With standard Freeciv21 builds, modpacks get installed into a per-user area, not into the main Freeciv21 
installation. So you shouldn't need any special permissions to download them, and if you uninstall the 
Freeciv21 game, any modpacks you downloaded are likely to remain on your system. Conversely, if you delete 
downloaded modpacks by hand, the standard rulesets, tilesets, etc. supplied with Freeciv21 won't be touched.

The precise location where files are downloaded to depends on your build and platform. For Unix systems, it 
is likely to be under the hidden :file:`~/.local/share/freeciv21` directory in your home directory. For 
Windows based sytems it will be in your user profile directory in a hidden :literal:`AppData` folder, 
typically, :file:`C:\\Users\\[MyUserName]\\AppData\\Roaming\\Freeciv21` It is likely to be near where the 
Freeciv21 client stores its saved games.

Most modpacks are specific to a particular major version of Freeciv21; for instance, while a 3.0 ruleset or 
tileset can be used with all Freeciv21 3.0.x releases, it cannot be used as-is with any 3.1.x release. So, 
most modpacks are installed in a specific directory for the major version, such as 
:file:`~/.local/share/freeciv21/3.1/` on Unix. 

.. note:: The modpack installer displays which version it will install for at the top of its window.

An exception to this is scenario maps; scenarios created for one version of Freeciv21 can usually be loaded 
in later versions, so they are installed in a version-independent location (typically 
:file:`~/.local/share/freeciv21/scenarios/` on Unix).

Once a modpack is installed, there is no uninstall action, and if you remove the files by hand, the 
installer will still consider the modpack to be installed; the installer maintains its own database 
(:file:`.control/modpacks.db`) listing which modpack versions are installed, but does not keep track of 
which files were installed by which modpack. If the database gets out of sync with reality (or is deleted), 
it's harmless for already installed modpacks and the main Freeciv21 programs (which do not consult the 
database), but can confuse the modpack installer's dependency tracking later.

Modpacks consist mostly of data files read by the Freeciv21 engine; they do not contain compiled binary code 
(and are thus platform-independent). Rulesets can contain code in the form of Lua scripts, but this is 
executed in a sandbox to prevent obvious security exploits. Modpacks are installed to a specific area and 
cannot overwrite arbitrary files on your system. Nevertheless, you should only install modpacks from sources 
you trust.
