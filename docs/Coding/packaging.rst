.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. include:: /global-include.rst

Packaging Freeciv21
*******************

Following our :doc:`release cadence </Contributing/release>` process, we provide a collection of operating
system (OS) specific packages for Freeciv21. More information can also be found at :doc:`/Getting/install`.

We maintain two versions for Freeciv21: ``stable`` and ``development``. The ``stable`` version is contained in
the ``stable`` branch on GitHub. The ``development`` version is maintained in the ``master`` branch on GitHub.

Libraries
=========

As with most software, we rely on some libraries. Stable relies on Qt5 and KDE Frameworks 5. However, both are
no longer supported. KDE is only doing security patches to Frameworks 5 and Plasma 5 (KDEs Desktop
Environment), to support Long Term Support (LTS) Linux OSs such as Ubuntu 24.04 LTS.

Development relies Qt6 and KDE Frameworks 6. Both are in mainline support and actively development. However,
only very new Linux distributions have the KDE Frameworks 6 libraries included. Stability was too late to be
included in Ubuntu 24.04 LTS. This makes packaging for master branch a bit more complicated.

There are other libraries we rely on as as seen in :doc:`/Getting/compile`, but the ones from Qt and KDE
impact packaging more than the others.

Here are some useful Qt and KDE Links:

* https://endoflife.date/qt
* https://invent.kde.org
* https://community.kde.org/Schedules/Frameworks
* https://community.kde.org/Schedules/Plasma_5
* https://community.kde.org/Schedules/Plasma_6

We provide native packages via GitHub Actions for the following platforms:

* Debian and its variants via CMake's CPack system.
* Windows via MSYS2 emulation and CMake's CPack system with NSIS integration.
* macOS via `vcpkg` and a macOS utility called ``create-dmg`` to create an application bundle.

We also provide Linux containerized packages for `snap` and `flatpak`. The Ubuntu world favors `snap` and the
Fedora/Red Hat world favors `flatpak`.

The Debian and Windows packages are sourced from CMake CPack configuration here: :file:`cmake/CPackConfig.cmake`

The macOS package is built inside GitHub Actions. You can refer to :file:`.github/workflows/build.yaml`. Look
for ``macOS`` build step.

As noted earlier, GitHub Actions uses Ubuntu LTS as the base image. You can add newer operating systems via
a Docker container. We do this on ``master`` branch for the Debian package at this time. MSYS2 has packages
for all the Qt6 and KDE Frameworks 6 components we need. Since macOS relies on the ``vcpkg`` package manager,
it is not dependent on the base image.

Debian Package
==============

The Debian package is straight forward. A :file:`.deb` package is effectively a compressed archive with some
configuration settings in plain text files. More information is available at the Debian WiKi:
https://wiki.debian.org/Packaging

CPack does all the work. We simply tell the packaging tool what our specific needs are and the tool does all
the rest. As long as we can compile Freeciv21 with the required libraries we can produce a Debian package.
Given this, we leverage a feature of GitHub Actions to use a more recent edition of Ubuntu inside of a Docker
container for the ``master`` branch. We use native Ubuntu 24.04 LTS for the ``stable`` branch.

Windows Package
===============

The Windows package is a bit more complicated. It requires Windows, the :doc:`MSYS2 </Contributing/msys2>`
emulation platform, and a packaging tool called NSIS. More information on NSIS is available at:
https://nsis.sourceforge.io/Docs/

CMake's CPack extension natively supports NSIS packages. As with Debian, we use the CPack configuration file
to define the base parameters of the installation. Lastly we have a template file
:file:`cmake/NSIS.template.in` that CPack uses to build the installer.

We also include some files to improve support for windows:

* :file:`dist/windows/freeciv21-server.cmd`: The server needs some environment variables set to run correctly
  on Windows, so we include with the package.
* :file:`dist/windows/qt.conf`: This file sets some Qt parameters to aid font display and other UI/UX elements
  so Freeciv21 looks its best on Windows.

macOS Package
=============

Apple |reg| uses a compressed file called an "Application Bundle" to install applications. CMake CPack does
not support building a bundle natively so we rely on a tool provided by Apple to do it ourselves.

The main build steps are noted above. A key file in establishing the macOS package is :file:`dist/Info.plist`.
This is an XML file that defines key components of Freeciv21 and is incorporated into the package at build
time with ``create-dmg``.

Snap Package
============

* Package website: https://snapcraft.io/freeciv21
* Documentation website: https://snapcraft.io/docs

Source File: :file:`dist/snapcraft.yaml`

Snap packages are generated via a tool called ``snapcraft``. It is actually a snap package itself. Snapcraft
relies on a virtual machine emulation system called ``lxd``. The ``lxd`` system relies on another snap package
called a `base`. A base is a version of Ubuntu LTS.

:strong:`Stable`

For ``stable`` we are using the base ``core22``, which is Ubuntu 22.04 LTS. It is a bit old, but provides the
package library stability we expect for our stable releases.

:strong:`Development`

For ``master`` we are using the base ``core24``, which is Ubuntu 24.04. Since Ubuntu 24.04 LTS does not have
the newer KDE Frameworks 6 libraries we need, we source from a snap that KDE maintains: ``kf6-core24``.

Since the whole ``snarcraft`` setup is itself an emulation layer, you cannot build snaps inside of a Docker
container. This means we cannot use the same process that we use for the Debian package, which is fine for us,
as we get everything we need from the team over at KDE.

:strong:`Snap Notes, Tips and Tricks`

The straight forward build steps are like this. Let us start with installing the base packages we need:

.. code-block:: sh

  $ sudo apt install snapd
  $ sudo snap install lxd
  $ sudo lxd init --auto
  $ sudo snap install snapcraft --classic


Here are the build-time steps to create the snap package:

.. code-block:: sh

  $ mkdir -p build/snap/local
  $ cp data/icons/128x128/freeciv21-client.png build/snap/local
  $ cp dist/snapcraft.yaml build/snap
  $ cd build
  $ sudo snapcraft
  $ sudo snap install --devmode ./freeciv21_*.snap
  $ sudo snap remove --purge freeciv21


You may run into trouble when running ``snapcraft`` to build the package. Here are some notes:

* If you get an error that ``snapcraft`` cannot connect to a network, this is coming from ``lxd``. The
  quickest solution is to turn off your firewall: ``sudo systemctl stop firewalld``. Obviously you will want
  to do this on a well known network and not in a coffee shop. Enable the firewall when finished with
  ``systemctl start``.

* If you get an error in one of the phases, you can open a shell to the ``lxd`` container either before or
  after the stage: ``sudo snapcraft stage --shell`` or ``sudo snapcraft stage --shell-after``. This allows you
  to look at where files are being stored (in this example the `stage` step), evaluate environment variables
  and generally try to troubleshoot any error messages you are getting.

* If you are doing a lot of runs of ``snapcraft`` during development, it is good to start with a clean slate.
  You can clean the environment with: ``sudo snapcraft clean``.

* You can run the snap in a shell as well to troubleshoot with ``snap run --shell freeciv21.freeciv21-client``.

As it relates to Natural Language Support (NLS), snap has some challenges. In the ``stable`` environment, NLS
is not enabled and is not well supported by either the ``core22`` or ``core24`` bases alone. The KDE provided
``kf5-core22`` or ``kf5-core24`` build snaps attempt to get NLS working, but there are upstream bugs in the
setup of a snap launch system call a ``command-chain``.  The good news is this is all fixed in the development
side in the ``master`` branch. With the ``core24`` base and the KDE provided ``kf6-core24`` build snap, NLS
works as expected. The Freeciv21 developers will keep and eye on the ``stable`` side to see if any upstream
improvements are pushed periodically to see if we can get NLS to work there.

Lastly, ``snapcraft`` has a utility to upload a generated snap package to the snap store automatically. We
use this capability in our :file:`.github/workflows/release.yaml` using a GitHub action called
``snapcore/action-publish``. Snapcraft uses an authentication mechanism called a `store credential` to
authenticate and upload a package. The credential expires periodically. Run this command to generate a new
one: ``snapcraft export-login -``. After entering our email address and password, ``snapcraft`` will output
the credential to ``stdout`` and it can be copied into our GitHub ``STORE_SECRETS`` secret.
