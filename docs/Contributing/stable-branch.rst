.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

Maintaining the Stable Branch
*****************************

There are two branches that the development team maintains in our GitHub repository: ``master`` and
``stable``. The ``master`` branch is the :emphasis:`development` branch and as the name implies, the
``stable`` branch is for :emphasis:`stable` releases.

Primary development occurs on the ``master`` branch. Due to the nature of development, it is never guaranteed
that the ``master`` branch will work at all times. While we strive to keep ``master`` working, there may be
occasions where we are introducing major breaking changes that take time to resolve. This is why we have a
``stable`` branch. As development occurs on the ``master`` branch, there are going to be times when we want to
back-port a commit (a single patch) or a Pull Request (a collection of commits) over to the ``stable`` branch.

The ``stable`` branch is created when preparing for the first Release Candidate towards a stable release, see
:doc:`release`.
This page documents the rules and procedures for maintaining the ``stable`` branch.

Requirements for a Back-Port
============================

Only specific categories of commits will be approved for a back-port from ``master`` to ``stable``. They are:

* :strong:`Bug Fixes` -- Not 100% of bug fixes should be back-ported, but certainly most should be considered.
  `Breaking` bugs that are found and patched should always be back-ported.
* :strong:`Documentation` -- We can support a ``latest`` and ``stable`` version in our documentation system.
  If there are issues found in our documentation that explicitly targets ``stable``, then we should back-port
  or author a Pull Request with a target of ``stable``.

:strong:`Things we will not back-port`

* :strong:`New Features or general improvements` -- This is what ``master`` branch is for.

As we often do, we will allow common sense to dictate any deviations from these rules. However, it should be
generally understood that there should be a careful consideration of what to back-port from ``master`` to
``stable``.

Tagging Commits for Back-Port
=============================

When a community member authors and publishes a :doc:`Pull Request (PR) <pull-request>`, they can add text to
the primary comment that the PR is a back-port candidate. If the whole PR is not a candidate, but a specific
commit within the PR is, then this distinction should be highlighted as part of the comment. The full Commit
ID is the definitive reference point to reduce ambiguity.

As is customary, all PRs should target the ``master`` branch.

Who Approves a Back-Port Request?
=================================

As is our standard, every Pull Request (PR) must have a peer review approval before a merge. The person
assigned this task has the authority to approve or deny the back-port request. This authority comes with a
catch; the peer reviewer must add the commit(s) to a back-port project that the admins will use to keep track
of patches that need to be back-ported.

The back-port project is here: https://github.com/orgs/longturn/projects/3. You can find it by going to the
main Longturn GitHub repository page and click on the projects tab.

As part of the review process, simply tag the PR to the project and add a ``back-port`` label to aid tracking.
Once the PR has been merged into ``master``, go get the commit ID(s) and add those to the appropriate column
in the project. Multiple Commit IDs should be separated by a space.

Maintaining Tracked Back-Port Commit Candidates
===============================================

On a periodic basis, for example every two or three weeks, an administrator will open the project and use the
information to build a commit cherry-pick file for ``git`` to use.

Follow these steps:

#. Checkout ``stable`` on your local: ``git checkout stable``
#. Copy and paste the values of the Commit ID column to a plain text file, such as :file:`commits.txt`
#. Run ``git cherry-pick -x $(cat commits.txt)``
#. Push ``stable`` to upstream: ``git push upstream``
#. Update the project sheet to denote which commits were back-ported.
