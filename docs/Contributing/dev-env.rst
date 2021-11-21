Set up a Development Environment
********************************

Contributing code to the Freeciv21 project or contributing to any of the games that the Longturn.net community
manages requires a bit of setup. This document should get you up and running with the basics. It should be
noted, that you typically only have to go through this process once unless you setup a new workstation.


Workstation
===========

Freeciv21 can be developed on Linux, Windows and MacOS. Any current version of these OS's is acceptible. For
Windows, you will need to setup the Msys2 environment to do development. Refer to :doc:`msys2` for more
information. :strong:`Linux` is the preferred platform.

Technically all you need is a text editor of some kind to edit the files, but most people prefer to use an
IDE.

All platforms can use `KDevelop <https://www.kdevelop.org/download>`_. However, there are some caveats:

* On Windows, due to the nature of the integration with Msys2, native compilation and debugging is not
  supported.
* On MacOS, KDevelop is still considered experimental.

For the best results, especially if you are editing game code and not just Longturn game rulesets or
documentation, you will want :strong:`Linux` to be your worksation OS. Many of the current developers use a
Debain variant such as Ubuntu. Instructions for getting all of the tools needed for Debian Linux can be found
in :doc:`../General/install`. Refer to the section titled `Debian Linux Notes`. Don't follow the last few
steps to clone the repository (e.g. the :code:`git clone` command), that will happen in a bit.


GitHub
======

The Longturn.net Community uses the online source code control and revision system known as
`GitHub <https://github.com/>`_. To contribute, you will need an account on this platform. There is no cost.

With an account setup, you can go to the `Longturn <https://github.com/longturn>`_ community repository page
and :strong:`fork` a repository (such as the Freeciv21 repository) to your personal GitHub account. Go to the
main page of the repository you want to fork and you will find a :strong:`fork` button in the upper-right
corner.

In order to get code to and from the forked repository to your local workstation, you need to setup an
SSH key pair to share with GitHub. Follow these
`instructions <https://docs.github.com/en/authentication/connecting-to-github-with-ssh>`_.

With that set up, now it's time to clone the forked repository from your personal GitHub account to a local
copy (e.g. sandbox) on your workstation. The typical way to do this is with the :code:`https` protocol.
However, this only works if you want to download a copy of a repository and not push any changes back up. To
do that, you have to use the :code:`ssh` protocol instead.

First make a working directory to place the files in:

.. code-block:: rst

  $ mkdir -p $HOME/GitHub

  $ cd $HOME/GitHub


Now you want to clone the respository. You can get the appropriate command by going to your forked copy in a
browser, click the code button and then select the SSH option as shown in this sample screenshot:

.. image:: ../_static/images/github_clone_ssh.png
    :align: center
    :height: 250
    :alt: GitHub Clone SSH


.. code-block:: rst

  ~/GitHub$ git clone git@github.com:[username]/freeciv21.git


This will clone the forked repository to the :file:`~/GitHub/freeciv21` directory on your computer.

The final repository setup item is to link the original Longturn project repository to your local sandbox on
your computer:

.. code-block:: rst

  ~/GitHub/freeciv21$ git remote add upstream https://github.com/longturn/freeciv21.git


You will also need to set a couple global configuration settings so :code:`git` knows a bit more about you.

.. code-block:: rst

  ~/GitHub/freeciv21$ git config --global user.email [email address associated with GitHub]
  ~/GitHub/freeciv21$ git config --global user.name [your first and last name]


Now you are ready to edit some code! When ready, follow the steps to submit a pull request here:
:doc:`pull-request`.
