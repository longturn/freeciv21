NSIS Windows Installation
=========================

The Freeciv21 community provides a Microsoft Windows\ |reg| installer when new releases are tagged.
Compiling for Windows requires the setup of a special development environment based on `MSYS2
<https://www.msys2.org/>`_, which can be daunting for new users who just want to play the game. The
following is instructions for installing on Windows.

To obtain the installer, go to the Freeciv21 `Releases <https://github.com/longturn/freeciv21/releases>`_
page. Each release has an assets section and the Windows installer and its hash are included. Click to
download and double-click the file to run.

.. _admin:

Local Admin Installation
************************

The following steps assume you :emphasis:`are` logged on as a local Administrator. If you :emphasis:`are
not` then jump to the non-admin_ section below.

If you :emphasis:`are` logged on as a local Administrator, a Windows User Access Control (UAC) window will
appear and ask: "Do you want to allow this app from an unknown publisher to make changes to your device".
Click :strong:`Yes` and the following panel will load after an uncompress process has completed.

Click :strong:`Next` to continue...

.. image:: ../_static/nsis-installer/01-Welcome.png
    :align: center
    :height: 400
    :alt: Welcome Panel

Check the box to accept the GNU 3.0 License and then click :strong:`Next` to continue...

.. image:: ../_static/nsis-installer/02-License.png
    :align: center
    :height: 400
    :alt: GNU License Acceptenace Panel

By default, the installer only installs the client, server and modpack installer. If you want other
utilities, you will need to pick them from the list. Click :strong:`Next` to continue...

.. image:: ../_static/nsis-installer/03-Components.png
    :align: center
    :height: 400
    :alt: Select Components Panel

Select the installation type and then click :strong:`Next` to continue...

.. image:: ../_static/nsis-installer/04-Admin-Multi-User.png
    :align: center
    :height: 400
    :alt: Admin Installation Panel

By default, the Admin installer will place the files in the :literal:`C:\\Program Files` folder. Change if
you want and click :strong:`Next` to continue...

.. image:: ../_static/nsis-installer/05a-Admin-Folder.png
    :align: center
    :height: 400
    :alt: Admin Instalation Folder Panel

This panel shows the files being installed. No action is needed.

.. image:: ../_static/nsis-installer/06-Copy-Files.png
    :align: center
    :height: 400
    :alt: Copying Files Panel

If you want to run Freeciv21 right away, leave the box checked and click :strong:`Finish`. Otherwise,
uncheck the box before clicking :strong:`Finish` to complete the installation.

.. image:: ../_static/nsis-installer/07-Finish.png
    :align: center
    :height: 400
    :alt: Finish Panel

You will find an icon for the client on the Desktop or in the Start Menu.

.. _non-admin:

Non-Local Admin (Unpriviledged User) Instalation
************************************************

If you :emphasis:`are not` logged on as a local Administrator there is no UAC prompt and the following panel
will load after an uncompress process has completed.

Click :strong:`Next` to continue...

.. image:: ../_static/nsis-installer/01-Welcome.png
    :align: center
    :height: 400
    :alt: Welcome Panel

Check the box to accept the GNU 3.0 License and then click :strong:`Next` to continue...

.. image:: ../_static/nsis-installer/02-License.png
    :align: center
    :height: 400
    :alt: GNU License Acceptenace Panel

By default, the installer only installs the client, server and modpack installer. If you want other
utilities, you will need to pick them from the list. Click :strong:`Next` to continue...

.. image:: ../_static/nsis-installer/03-Components.png
    :align: center
    :height: 400
    :alt: Select Components Panel

The Non-Admin Installer places the files in your User Directory by default. Change if you want and click
:strong:`Next` to continue...

.. image:: ../_static/nsis-installer/05b-User-Folder.png
    :align: center
    :height: 400
    :alt: User Instalation Folder Panel

This panel shows the files being installed. No action is needed.

.. image:: ../_static/nsis-installer/06-Copy-Files.png
    :align: center
    :height: 400
    :alt: Copying Files Panel

If you want to run Freeciv21 right away, leave the box checked and click :strong:`Finish`. Otherwise,
uncheck the box before clicking :strong:`Finish` to complete the installation.

.. image:: ../_static/nsis-installer/07-Finish.png
    :align: center
    :height: 400
    :alt: Finish Panel

You will find an icon for the client on the Desktop or in the Start Menu.

.. |reg|    unicode:: U+000AE .. REGISTERED SIGN
