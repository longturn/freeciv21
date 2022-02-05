Evaluating a Pull Request
*************************

This page mostly targets the Longturn Admins who will be asked to review (evaluate) a submitted Pull Request.
It is the policy of the Longturn community that all (100%) Pull Requests have at least one reviewer complete
an evaluation of a change and either approve or make suggestions for improvements before a merge into the
master branch.

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
  $ cmake --build build --target package      # MSys2 Only

:strong:`Run tests`

..code-block:: sh

  $ cmake --build build --target test

:strong:`Read The Issue's Notes`

Ensure you understand what is being reported. Test the scenario as written. Make notes/comments in the PR.
Approve the review request if warranted. if not, state why. If further commits are added to the PR, you will
probably have to re-download the diff and run another test.

:strong:`Cleanup`

* Remove the downloaded diff files: :code:`$ rm *.diff`.

* Remove any untracked files: :code:`$ git status`. Look for any untracked files and delete them

* Stash changes: :code:`$ git stash`.

* Checkout Master branch and delete the testing branch:

.. code-block:: rst

  $ git checkout master
  $ git branch -d testing/pr_[pr-number]
