.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. include:: /global-include.rst

Debug on MS Windows
*******************

Debugging compiled code on MS Windows |reg| can be challenging if you do not have the time or inclination to
:doc:`/Contributing/dev-env`. This page is to give instructions for Freeciv21 developers to offer a
custom compiled package to a player who wants to help troubleshoot a bug, but needs a better capability than
the released package.

The problem here is two-fold:

#. The released package does not have debugging symbols compiled into the code, since it is not a good
   practice to do so.
#. Qt's console access has been removed from the Window's binaries, which makes getting ``stdout`` or
   ``stderr`` from a command prompt impossible.

There is good news! We have infrastructure in place that makes it easy for us to provide a special debug
enabled Windows package to a player. Here is how to do it:

#. On a development platform of choice, create a new branch following our :doc:`/Contributing/pull-request`
   process called something like ``test/windows-debug-console``.
#. Open :file:`cmake/FreecivDependencies.cmake` and comment the lines for the :code:`if()` block below this
   comment: :code:`# Removes the console window that pops up with the GUI app`.
#. Open :file:`.github/workflows/build.yaml` and change the MSYS2 build step to use ``Debug`` instead of
   ``Release`` for the :code:`DCMAKE_BUILD_TYPE=...` ``cmake`` configure flag.
#. (Optional) Change the value for the package name to update to the action's attachment section from
   :file:`Windows-exe` to :file:`WindowsDebug-exe`
#. Push the change to our GitHub repo and let the CI/CD Actions run. When finished there will be a Debug
   enabled installer package available.

.. tip::
  You can see a sample by reviewing https://github.com/longturn/freeciv21/pull/2564.

Steps for Debugging on Windows
==============================

The player will need a GitHub account. They are free of charge, so there should not be any issue there. Give
the link to the build action's main page after it has completed from the pushed PR above.

These are the steps for the player on their Windows computer:

#. Download :file:`WindowsDebug-exe` and uncompress the ZIP file.
#. Run the installer package ``exe`` and install to a throw away location such as :file:`C:\\Temp`.
#. Right click :guilabel:`Start Menu` and select :guilabel:`Windows PowerShell (Admin)`.
#. At the ``PS C:\WINDOWS\System32>`` prompt, type ``cmd`` and press :guilabel:`Enter`.
#. At the ``C:\WINDOWS\System32>`` prompt, type ``setx /M QT_ASSUME_STDERR_HAS_CONSOLE "1"`` and press
   :guilabel:`Enter`.
#. You sould get the following message: ``SUCCESS: Specified value was saved.``
#. Type ``setx /M QT_FORCE_STDERR_LOGGING "1"`` and press :guilabel:`Enter`.
#. You sould get the following message: ``SUCCESS: Specified value was saved.``
#. Type ``setx /M QT_LOGGING_RULES "freeciv.stacktrace*=true"`` and press :guilabel:`Enter`.
#. You sould get the following message: ``SUCCESS: Specified value was saved.``
#. Type ``exit`` and press :guilabel:`Enter` twice to close the command shell.
#. Right click :guilabel:`Start Menu` and select :guilabel:`Windows PowerShell (Admin)`.
#. At the ``PS C:\WINDOWS\System32>`` prompt, type ``cmd`` and press :guilabel:`Enter`.
#. At the ``C:\WINDOWS\System32>`` prompt, type ``set`` and look for the 3 ``QT_`` values set earlier.
#. Type ``cd C:\Temp\Freeciv21`` and press :guilabel:`Enter`.
#. Type ``freeciv21-client.exe -d debug -l output.log`` and press :guilabel:`Enter`.

At this point the player running the test will will see tons of output to the console window, all if it is
also being sent to the log file. Launcing the game will be slow in debug mode, so it is probably a good idea
to let the player know and to be patient.

Have the player do the tests to get to the crash. Open the log file in Notepad++ or any good text file reader.
The log file will be quite large. Go to the bottom of the contents and have the player paste the last 100 or
so lines to Discord to aid troubleshooting.

Cleaning Up
===========

The player can uninstall the debug package by double-clicking the :file:`Uninstall` icon in the installed
location. They will want to reinstall the regular package to the regular location to fix any shortcuts that
get deleted by the uninstall process.

The envrionment variables can be left as they cause no problems. If the player wants to remove them, they can
be found in
:menuselection:`Start Menu -> Settings -> System -> About -> Advanced System Settings -> Environment Variables`.

The PR can be closed at will as we do not merge the change.
