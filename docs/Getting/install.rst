.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>


Installing Freeciv21
********************

The developers of Freeciv21 provide pre-compiled binaries and installation packages for tagged releases. They
can be found on the Longturn GitHub Repository for Freeciv21 at
https://github.com/longturn/freeciv21/releases. The Longturn community provides binary packages for
Debian-based Linux distros (Debian, Ubuntu, Mint, etc.), Microsoft Windows\ |reg| (32 and 64-bit), and Apple
macOS\ |reg|. If you are an Arch Linux user, you can find Freeciv21 in the AUR at
https://aur.archlinux.org/packages/freeciv21.

Windows
=======

For more information on using the Windows Installer package, you can read about it at :doc:`windows-install`.

Debian Linux
============

To install the Debian (and Debian variants such as Ubuntu) package, use the ``apt`` command with elevated
privileges like this:

.. code-block:: sh

  $ sudo apt install ./freeciv21_*_amd64.deb


Generic Linux
=============

Freeciv21 is also available as a snap or flatpak containerized application. Different distributions support
one or the other by default.

Snap
----

Debian Linux variants (those that rely on ``apt`` for package management):

.. code-block:: sh

  $ sudo apt install snapd
  $ sudo systemctl enable snapd
  $ sudo snap install freeciv21


Fedora/Red Hat Linux variants (those that rely on ``dnf`` for package management):

.. code-block:: sh

  $ sudo dnf install snapd
  $ sudo systemctl enable snapd
  $ sudo snap install freeciv21


Flatpak
-------

Debian Linux variants (those that rely on ``apt`` for package management):

.. code-block:: sh

  $ sudo apt install flatpak
  $ sudo flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
  $ sudo flatpak install net.longturn.freeciv21


Fedora/Red Hat variants (those that rely on ``dnf`` for package management):

.. code-block:: sh

  $ sudo dnf install flatpak
  $ sudo flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
  $ sudo flatpak install net.longturn.freeciv21


macOS
=====

To install the macOS ``.dmg`` package, you start by double-clicking the file to mount it in Finder. Drag the
game to your Applications folder, or a place of your choosing.  When finished, unmount the package.

.. note::
  In newer versions of macOS, you may get an error message when trying to mount the package: "Freeciv21.app is
  damaged and can't be opened." You will need to adjust the security settings on your computer. Here are some
  website links to help:

  * https://appletoolbox.com/app-is-damaged-cannot-be-opened-mac/
  * https://support.apple.com/guide/mac-help/open-an-app-by-overriding-security-settings-mh40617/15.0/mac/15.0


A Note About Native Language Support
====================================

Freeciv21 is packaged with Native Language Support (NLS), also known as Internationalization (i18n). By
default, Freeciv21 will use the primary language that the client operating system is set to use. However, you
may wish to play the game with a different language.

All the code and strings used in the game are based on US English (en_US) and encoded as UTF8 (en_US.UTF8).
If you wish to play the game in a different language, you can do so by setting an environment variable to the
language code of your choice.

At a minimum, all you need is the two letter code of the language you wish to play with. Here is a list of
them: https://en.wikipedia.org/wiki/List_of_ISO_639_language_codes

.. note::
  We don't support every single language code in the list above, but we do have some translations for many of
  them.

On a Linux based system, open a terminal and set the ``LANG`` variable to the language code. In the example
we pick German (Deutsch).

.. code-block:: sh

  $ export LANG=de_DE.UTF8
  $ path/to/freeciv21-client


That setting will stay in effect as long as the terminal window is open. Freeciv21 will use the environment
context into account.

.. note::
  You can also add the ``export LANG=de_DE.UTF8`` to your user's :file:`.bashrc` or :file:`.bash_profile`. The
  variable will then be set every time you logon to your computer. However, this also sets the language for
  pretty much every application.

The ``LANG`` variable also works on Windows based systems. Open a command prompt, powershell prompt, or
terminal.

.. code-block:: sh

  PS C:\Users\Username> setx LANG de_DE.UTF8


Then open Freeciv21 from the start menu like normal.

.. note::
  As with the note related to Linux based systems. The ``setx`` command sets a user level environment variable
  to the language selected. Any applications that use the ``LANG`` variable will also be impacted. This is
  especially true when working in the MSYS2 environment. You can set the variable to another language following
  the same step above. Simply set the variable to a different locale code.
