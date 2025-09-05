.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

.. include:: /global-include.rst

Packaging Freeciv21
*******************

Following our :doc:`release cadence </Contributing/release>` process, we provide a collection of operating
system (OS) specific packages for Freeciv21. More information can also be found at :doc:`/Getting/install`.

We maintain two versions for Freeciv21: *Stable* and *Development*. The *Stable* version is contained in
the ``stable`` branch on GitHub. The *Development* version is maintained in the ``master`` branch on GitHub.

Libraries
=========

As with most software, we rely on some libraries. *Stable* relies on Qt5 and KDE Frameworks 5. However, both
are no longer supported. KDE is only doing security patches to Frameworks 5 and Plasma 5 (KDEs Desktop
Environment), to support Long Term Support (LTS) Linux OS's such as Ubuntu 24.04 LTS.

*Development* relies on Qt6 and KDE Frameworks 6. Both are in mainline support and actively development. However,
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
* Microsoft Windows\ |reg| via MSYS2 emulation and CMake's CPack system with NSIS integration.
* Apple macOS\ |reg| via `vcpkg` and a utility called ``create-dmg`` to create a compressed application bundle.

The Debian and Windows packages are sourced from CMake CPack configuration here: :file:`cmake/CPackConfig.cmake`

The macOS package is built inside GitHub Actions. You can refer to :file:`.github/workflows/build.yaml`. Look
for the ``macOS`` build step.

Outside of the GitHub Actions, we provide native packages for the following platforms:

* Linux containerized packages for `snap` and `flatpak`. The Ubuntu world favors `snap` and the Fedora/Red
  Hat world favors `flatpak`.
* Native :file:`.deb` and :file:`.rpm` packages for many Linux distributions are built using the openSUSE
  Build Service (OBS).

.. note::
  In the future we will remove the CPack generated Debian package and only support OBS. We are in a
  transition period at the moment.

As noted earlier, GitHub Actions uses the current Ubuntu LTS as the base image. You can add newer operating
system editions via a Docker container. We do this on ``master`` branch for the Debian package at this time.
MSYS2 has packages for all the Qt6 and KDE Frameworks 6 components we need. Since macOS relies on the
``vcpkg`` package manager, it is not dependent on the base image.

CPack Debian Package
====================

The CPack Debian package is straightforward. A :file:`.deb` package is effectively a compressed archive with
some configuration settings in plain text files. More information is available at the Debian Wiki:
https://wiki.debian.org/Packaging

CPack does all the work. We simply tell the packaging tool what our specific needs are and the tool does all
the rest. As long as we can compile Freeciv21 with the required libraries we can produce a Debian package.
Given this, we leverage a feature of GitHub Actions to use a more recent edition of Ubuntu inside of a Docker
container for the ``master`` branch. We use native Ubuntu 24.04 LTS for the ``stable`` branch.

CPack Windows Package
=====================

The CPack Windows package is a bit more complicated. It requires Windows, the :doc:`MSYS2 </Contributing/msys2>`
Linux emulation platform, and a packaging tool called NSIS. More information on NSIS is available at:
https://nsis.sourceforge.io/Docs/

CMake's CPack extension natively supports NSIS packages. As with Debian, we use the CPack configuration file
to define the base parameters of the installation. Lastly we have a template file
:file:`cmake/NSIS.template.in` that CPack uses to build the installer.

We also include some files to improve support for Windows:

* :file:`dist/windows/freeciv21-server.cmd`: The server needs some environment variables set to run correctly
  on Windows, so we include with the package.
* :file:`dist/windows/qt.conf`: This file sets some Qt parameters to aid font display and other UI/UX elements
  so Freeciv21 looks its best on Windows.

macOS Package
=============

Apple uses a compressed file called an "Application Bundle" to install applications. CMake CPack does
support building a bundle natively, however with some caveats that makes it hard for us to use. Given this,
we rely on a tool provided by the ``homebrew`` macOS packaging system to do it ourselves.

The main build steps are noted above. A key file in establishing the macOS package is :file:`dist/Info.plist.in`.
This is an XML file that defines key components of Freeciv21 and is incorporated into the package at build
time along with ``create-dmg``.

The main difference between *Stable* and *Development* is *Stable* manually creates the application bundle in
the GitHub action. In *Development* we have incorporated the generation of the application bundle in the
``--target install`` step, so the GitHub action only has to create the :file:`.dmg` installer.

Snap Package
============

* Package website: https://snapcraft.io/freeciv21
* Documentation website: https://snapcraft.io/docs

Source File: :file:`dist/snapcraft.yaml`

Snap packages are generated via a tool called ``snapcraft``. It is actually a snap package itself. Snapcraft
relies on a virtual machine emulation system called ``lxd``. The ``lxd`` system relies on another snap package
called a `base`. A base is a version of Ubuntu LTS.

:strong:`Stable`

For *stable* we are using the base ``core22``, which is Ubuntu 22.04 LTS. It is a bit old, but provides the
package library stability we expect for our stable releases.

:strong:`Development`

For *Development* we are using the base ``core24``, which is Ubuntu 24.04. Since Ubuntu 24.04 LTS does not
have the newer KDE Frameworks 6 libraries we need, we source from a snap that KDE maintains: ``kf6-core24``.

Since the whole ``snapcraft`` setup is itself an emulation layer, you cannot build snaps inside of a Docker
container. This means we cannot use the same process that we use for the Debian package, which is fine for us,
as we get everything we need from the team over at KDE.

:strong:`Snap Notes, Tips and Tricks`

The straightforward build steps are like this. Let us start with installing the base packages we need:

.. code-block:: sh

  $ sudo apt install snapd
  $ sudo ln -s /var/lib/snapd/snap /snap
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
  quickest work around is to turn off your firewall: ``sudo systemctl stop firewalld``. Obviously you will want
  to do this on a well known network and not in a coffee shop. Enable the firewall when finished with
  ``systemctl start``.

* If you get an error in one of the phases, you can open a shell to the ``lxd`` container either before or
  after the stage: ``sudo snapcraft stage --shell`` or ``sudo snapcraft stage --shell-after``. This allows you
  to look at where files are being stored (in this example the *stage* step), evaluate environment variables
  and generally try to troubleshoot any error messages you are getting.

* If you are doing a lot of runs of ``snapcraft`` during development, it is good to start with a clean state.
  You can clean the environment with: ``sudo snapcraft clean``.

* You can run the snap in a shell as well to troubleshoot with ``snap run --shell freeciv21.freeciv21-client``.

As it relates to Natural Language Support (NLS), snap has some challenges. In the *Stable* environment, NLS
is not enabled and is not well supported by either the ``core22`` or ``core24`` bases alone. The KDE provided
``kf5-core22`` or ``kf5-core24`` build snaps attempt to get NLS working, but there are upstream bugs in the
setup of a snap launch system called a ``command-chain``.  The good news is this is all fixed in the
*Development* side in the ``master`` branch. With the ``core24`` base and the KDE provided ``kf6-core24``
build snap, NLS works as expected. The Freeciv21 developers will keep and eye on the *Stable* side to see if
any upstream improvements are pushed periodically to see if we can get NLS to work there.

Lastly, ``snapcraft`` has a utility to upload a generated snap package to the snap store automatically. We
use this capability in our :file:`.github/workflows/release.yaml` using a GitHub action called
``snapcore/action-publish``. Snapcraft uses an authentication mechanism called a *store credential* to
authenticate and upload a package. The credential expires periodically. Run this command to generate a new
one: ``snapcraft export-login -``. After entering our email address and password, ``snapcraft`` will output
the credential to ``stdout`` and it can be copied into our GitHub ``STORE_SECRETS`` secret.

Flatpak Package
===============

* Package website: https://flathub.org/apps/net.longturn.freeciv21
* Documentation website: https://docs.flathub.org/

The Flatpak source system resides in a separate GitHub repository: ``net.longturn.freeciv21``, which is a
fork of the main package at https://github.com/flathub/net.longturn.freeciv21.

We currently only support the ``stable`` branch with Flatpak. There are plans to support ``master`` with
Flathub beta. See: https://github.com/longturn/freeciv21/issues/2513.

Setting up development for Flatpak is relatively straightforward:

.. code-block:: sh

  $ git clone --depth 1 --recursive git@github.com:longturn/net.longturn.freeciv21.git flatpak
  $ cd flatpak
  $ git remote add upstream https://github.com/flathub/net.longturn.freeciv21
  $ git fetch upstream
  $ git pull upstream master
  $ git submodule update --recursive --remote


.. note::
  We use the ``--depth 1 --recursive`` command with ``git`` because we rely on a shared module for Lua
  support. This is also why we have the ``git submodule`` command in the steps.

Now you can edit the :file:`net.longturn.freeciv21.yaml` configuration file. Let us start with installing the
base packages we need:

.. code-block:: sh

  $ sudo apt install flatpak-builder
  $ sudo flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo


Here are the build-time steps to create the flatpak package:

.. code-block:: sh

  $ flatpak-builder --install-deps-from=flathub --force-clean build net.longturn.freeciv21.yaml
  $ flatpak-builder --user --install --force-clean build net.longturn.freeciv21.yaml
  $ flatpak run net.longturn.freeciv21
  $ flatpak remove net.longturn.freeciv21


As with the snap package, flatpak packages rely on some platform dependencies, which are also flatpak packages.
For flatpak we use a ``runtime`` that matches the run time environment for the application. The *Stable*
version of Freeciv21 needs Qt5, so we leverage the ``5.15-24.08`` runtime provided by KDE, which is Qt v5.15
and built in August of 2024. We also need an SDK (Software Development Kit). As you would expect, we use the
``org.kde.Sdk`` SDK.

Flathub will build the package from source by grabbing it from our GitHub repository. Near the bottom of the
configuration file you will see where we tag a ``git`` commit ID, which should correspond to the tagged
release of a ``stable`` branch Git Tag.

:strong:`Flatpak Notes, Tips and Tricks`

* Due to the way that Flathub "hosts" packages, with each in their own repository, it makes updating the
  package a bit interesting. Our Longturn repository fork is ``origin`` in your local and the Flathub
  repository is ``upstream``. When you update the package you follow a similar pattern like updating code in
  the main Freeciv21 repository. You push changes to ``origin`` and then open a Pull Request (PR) in
  ``upstream``. When you open a PR in ``upstream`` you will be asked some questions. Answer them and then
  submit. The ``flathubbot`` will automatically run. If the build is successful, then you can merge it.

* Every downloaded file has to have a hash (``sha256``) associated with it. If you get stuck and do not know
  how to get the hash, then run the ``flatpak-builder`` command in your local. The process will error out and
  give you the hash that was expected. You can then edit the configuration with the proper hash. Be sure you are
  grabbing files from ``raw.githubusercontent.com`` or tagged package files from a Releases page.

* The KDE ``runtime`` we use is hosted here: https://invent.kde.org/packaging/flatpak-kde-runtime. You can
  look at the list of protected branches to determine the correct version to use.


openSUSE Build Service (OBS)
============================

* Package Repository (stable): https://build.opensuse.org/project/show/home:longturn
* Package Repository (development): https://build.opensuse.org/project/show/home:louis94_m:freeciv21
* Package Downloads (stable): https://download.opensuse.org/repositories/home:/longturn/
* Package Downloads (development): https://download.opensuse.org/repositories/home:/louis94_m:/freeciv21/
* OBS User Guide: https://openbuildservice.org/help/manuals/obs-user-guide/
* OBS RPM Guidelines: https://en.opensuse.org/openSUSE:Specfile_guidelines
* Fedora RPM Guidelines: https://docs.fedoraproject.org/en-US/packaging-guidelines/
* OBS DEB Guidelines: https://en.opensuse.org/openSUSE:Build_Service_Debian_builds

Source Files: Refer to :file:`.obs/` in our GitHub repository.

OBS is a package building service hosted by the folks over at openSUSE. It gives us the ability to support
packages for multiple distributions for both the :file:`.deb` and :file:`.rpm` worlds.

As of this writing, the stable OBS repository only supports the *Stable* release and all of the source files
are hosted at OBS. Support for the *Development* edition is on our sandbox at this time. The ``master``
branch hosts the files that OBS needs in the :file:`.obs/` directory as noted above.

Both versions of our package rely on an OBS hosted file named :file:`_service`. This is an XML file telling
OBS how to build the package. The file on the *Stable* repository is very simple, just download the tarball
and build. The *Development* version of the file is more complex as we rely on some GitHub integration tools:
``obs_scm``, ``tar``, ``recompress``, and ``set_version``. All are hosted in OBS at the ``openSUSE:Tools``
project.

:strong:`Stable Support`

The *Stable* package is supported on the following distributions:

* Debian 12 Bookworm LTS
* Debian 13 Trixie LTS
* Debian Testing (future 14 Forky)
* Debian Unstable (Sid)
* Fedora 41
* Fedora 42
* Fedora Rawhide
* openSUSE Tumbleweed
* Ubuntu 22.04 LTS
* Ubuntu 24.04 LTS
* Ubuntu 24.10
* Ubuntu 25.04

:strong:`Development Support`

The *Development* package is supported on the following distributions:

* Debian 13 Trixie LTS
* Debian Testing (future 14 Forky)
* Debian Unstable (Sid)
* Fedora 41
* Fedora 42
* Fedora Rawhide
* openSUSE Tumbleweed
* Ubuntu 24.10
* Ubuntu 25.04

:strong:`DEB Packages`

The :file:`.deb` packaging system relies on the following files:

* :file:`debian.changelog` --- A list of changes between versions.
* :file:`debian.control` --- The package control file with list of build and runtime dependencies.
* :file:`debian.rules` --- You can override the ``debhelper`` system with this file.
* :file:`freeciv21.dsc` --- The Debian Source Control file.

The :file:`freeciv21.dsc` controls the whole process. For *Stable* we have code in the file telling the system
where to get the source :file:`tar.gz`. For *Development* we use an OBS service integration called ``obs_scm``
that pulls the code from our GitHub repository. The ``tar`` and ``recompress`` services builds a :file:`tar.gz`
automatically. Lastly the ``set_version`` service helps us with auto versioning.

To update the package on *Stable*:

#. Edit :file:`debian.changelog` and add a new version to the top.
#. Edit :file:`freeciv21.dsc` with a new ``Version`` and update the ``Files`` and ``DebTransform-Tar``
   sections with a new tarball from our releases page on GitHub.

:strong:`RPM Packages`

The :file:`.rpm` packaging system relies on the following files:

* :file:`freeciv21.changes` --- A list of changes between versions.
* :file:`freeciv21.spec` --- The control file for the ``rpmbuild`` system. One file can support multiple
  :file:`.rpm` based distributions.

The :file:`freeciv21.spec` controls the whole process. On *Stable* it relies on what we have in the
:file:`freeciv21.dsc` to know where to get the tarball for compilation. For *Development* we use the same
system as for the :file:`.deb` packages.

To update the package on *Stable*:

#. Ensure you have updated the :file:`freeciv21.dsc` as noted in the DEB section above.
#. Edit :file:`freeciv21.spec` with a new ``version``.
