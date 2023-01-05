..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 louis94 <m_louis30@yahoo.com>
    SPDX-FileCopyrightText: 2022 Pranav Sampathkumar <pranav.sampathkumar@gmail.com>

Evaluating a Pull Request
*************************

This page mostly targets the Longturn Admins who will be asked to review (evaluate) a submitted Pull Request.
It is the policy of the Longturn community that all (100%) Pull Requests have at least one reviewer complete
an evaluation of a change and either approve or make suggestions for improvements before a merge into the
master branch.

Code PRs
========

For each Pull Request, a set of tests are run automatically. When the tests do not pass, the author is
expected to fix the problems before someone tries to test the code.

This page assumes the user knows how to use :file:`git`, compile Freeciv21 and use GitHub.

:strong:`Create A Testing Branch`

.. code-block:: rst

  $ git fetch upstream master
  $ git checkout -b testing/pr_[pr-number] upstream/master


:strong:`Get A Copy Of The Pull Request`

.. code-block:: rst

  $ wget https://github.com/longturn/freeciv21/pull/[pr-number].diff -O pr[pr-number].diff


:strong:`Apply The Downloaded Update To Local`

.. code-block:: rst

  $ patch -p1 < pr[pr-number].diff


:strong:`Ensure The Build Directory Is Empty`

.. code-block:: rst

  $ rm -Rf build


:strong:`Configure And Compile The Code`

.. code-block:: rst

  $ cmake . -B build -G Ninja -DCMAKE_INSTALL_PREFIX=$PWD/build/install
  $ cmake --build build
  $ cmake --build build --target install
  $ cmake --build build --target package      # MSys2 and Debian Linux Only


:strong:`Read The Issue's Notes`

Ensure you understand what is being reported. Test the scenario as written. Make notes/comments in the PR.
Approve the review request if warranted. if not, state why. If further commits are added to the PR, you will
probably have to re-download the diff and run another test.

:strong:`Run an autogame`

If it is a big change, it might be worthwhile to run an entire game with just AI to make sure it does not
break anything. You can compile the code, with additional checks such as address sanitizer with
:code:`$ cmake . --preset ASan`. Once the code is compiled, you can run the autogame with
:code:`./build/freeciv21-server -r ./data/test-autogame.serv`. You can also observe the game with
:code:`./build/freeciv21-client -a -p 5556 -s localhost`. ASan by default halts on every error, this is
sometimes useful to developers to fix the errors sequentially. If you'd rather prefer listing all the errors
at once, set the environment variable using :code:`$ export ASAN_OPTIONS="halt_on_error=0"`

:strong:`Cleanup`

* Remove the downloaded diff files: :code:`$ rm *.diff`.

* Remove any untracked files: :code:`$ git status`. Look for any untracked files and delete them

* Stash changes: :code:`$ git stash`.

* Checkout the ``master`` branch and delete the testing branch:

.. code-block:: rst

  $ git checkout master
  $ git branch -d testing/pr_[pr-number]


Art PRs
=======

If a Pull Request includes art (graphics, music, etc), you should check not only the inner quality of the
art, but also how it fits within what is already there. It is sometimes preferable to use lower quality
sprites if they fit better with the general style of a tileset.

A recurring issue with graphics and sound assets is their licensing and attribution. Much more than code,
images and music files get copied over, merged, or renamed, and authorship information is quickly lost. Make
sure that the author of the PR understands where the files come from and who authored them. If possible, ask
the original author directly if we can include their art.

We request that all assets file be accompanied with license and copyright information in the form of a
`license file <https://reuse.software/spec/#comment-headers>`_. You will find many examples in the
repository. The license should be `compatible with version 3 of the GPL
<https://www.gnu.org/licenses/license-list.html>`_.

.. warning::
  Please be extra careful when submitted graphics are present `on Freeciv-Web
  <https://github.com/Lexxie9952/fcw.org-server>`_, as doubts have been repeatedly raised about the validity
  of some of the copyright claims made by the main developer of that project. We were also asked not to use
  their graphics, and we will respect this even if it would be allowed by law. As a rule, we only accept
  assets present on FCW if we can prove that they were taken from somewhere else --- and in that case, we
  refer to the original source for licensing information.
