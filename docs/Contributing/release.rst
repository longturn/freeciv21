.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>s

The Release Process
*******************

The developers of Freeciv21 release new versions of the software following a cadence. The cadence is loosely
defined as: 30 non-documentation pull requests (PR's), 200 commits, or 60 days, whichever comes first. We
use the term "loosely" as there is a large amount of discretion with regard to the cadence. We can speed up
a release at any time to fix a nasty bug or slow a release down to get in a PR a developer is really close
to completing and wants it in the next release. However, we do aim to release around 6 to 8 times per year.

Pre-releases are done from the ``master`` branch, while release candidates and stable releases use the
``stable`` branch, which contains bug fixes backported from master. See
:doc:`the dedicated page <stable-branch>` for how this is done.

These are the general steps to prepare and finalize a release:

#. A release manager will open a draft release notes page from: https://github.com/longturn/freeciv21/releases.
   The easiest way to do this is to copy the contents of the last release, delete the bullet points, and
   add a single bullet of ``* Nothing for this release`` to each section. :strong:`DO NOT` add a tag at this
   time. This prevents an accidental release.
#. A release manager will update the draft as PR's are committed to the repository to help keep track of
   the release cadence progress. As PR's are added to sections the ``* Nothing for this release`` is removed.
#. As PR's are added to the draft release notes, pay attention to the contributor. If a new contributor has
   opened a PR, call it out --- "With this release we welcome @[GithubID] as a new contributor (#[PR])"
#. Multiple PR's can be combined on a single bullet line. Documentation updates are often done this way.
#. When we are getting close to crossing one of the release candidate thresholds, the release manager will
   post to the ``#releases-project`` channel in the ``LT.DEV`` section on the Longturn Discord server. The
   main purpose of the post is to alert the developers of a pending cadence threshold and to gauge if we
   need to delay in any way or if we are good to proceed as normal.
#. A release manager or another regular contributor will update the ``vcpkgGitCommitId`` value in the CI/CD
   build file (:file:`.github/workflows/build.yaml`). Grab the Commit ID of the most recent release from
   https://github.com/microsoft/vcpkg/releases. Commit and push a PR. Ensure all of the CI/CD Action runners
   complete successfully.
#. If the new release is a change from ``alpha`` to ``beta`` or ``release candidate`` series, the
   :file:`.github/workflows/release.yaml` file needs an update. In the ``snapcraft`` section, change the
   ``release`` flag -- ``edge`` for Alpha, ``beta`` for Beta and ``candidate`` for the RC's.
#. When it is time, the release manager will finalize the release notes and ask for an editorial review in the
   ``#releases-project`` channel. Updates are made as per review.
#. If the release will be the :strong:`first release candidate` towards a stable release, the release manager
   will:

   #. Delete the existing ``stable`` branch on Github's
      `branches page <https://github.com/longturn/freeciv21/branches>`_.
   #. From the same page, create a new ``stable`` branch from ``master``.
   #. Update :file:`cmake/AutoRevision.txt` with the hash of the last commit in ``master`` and
      ``v[major version].[minor version]-dev.0`` with the version of the :strong:`next stable release`, then
      open a PR for this change to ``master``. This way, development builds from ``master`` will immediately
      use the version number of the next stable.
   #. Update :file:`.github/workflows/release.yaml`. In the ``snapcraft`` section, change the ``release``
      flag to ``stable``.

#. If the release is a :strong:`release candidate` for a :strong:`stable release`, the release manager will
   make sure that the :guilabel:`Target` branch in the release draft is set to ``stable``.
#. The release manager will add a tag to the release notes page and then click :guilabel:`Publish Release`.
   The format of the tag is ``v[major version].[minor version]-[pre-release name].[number]`` for pre-releases
   and ``v[major version].[minor version].[patch version]`` for stable versions. For example:
   ``v3.0-beta.6`` or ``v3.1.0``. :strong:`The format is very important` to the build configuration process.
#. After a few minutes the continuous integration (CI) will open a PR titled
   ``Release Update of AutoRevision.txt``. The release manager will open the PR, click on the
   :guilabel:`Close pull request` button, and then click :guilabel:`Open pull request` button. This is a
   necessary step to handle a GitHub security feature. GitHub requires a human to be involved to merge CI
   created PR's.
#. While inside the ``Release Update of AutoRevision.txt`` PR, the release manager will enable an automatic
   rebase and merge.
#. The release manager will open an issue titled ``Review workarounds after <version> release`` with the
   following text:

      We should review the workarounds in the source code and check that they are still needed. Some
      workarounds are documented here: :doc:`workarounds`.

#. When all the CI actions are complete, the release manager will make a post in the ``#news-channel`` on the
   Longturn.net Discord server.
#. The release manager will download the Windows i686 and x86_64 installer packages and use their Microsoft
   Account to submit the files for Microsoft SmartScreen analysis. The instructions are `provided here
   <https://learn.microsoft.com/en-us/windows/security/threat-protection/microsoft-defender-smartscreen/microsoft-defender-smartscreen-overview#submit-files-to-microsoft-defender-smartscreen-for-review>`_.
   We do this to help our Windows-based users have an easier time downloading the game in the Microsoft Edge
   browser.
#. The release manager mentions user @Corbeau on Discord ``#releases-project`` channel giving the new URL to
   update his blog page once all of the GitHub action runners are complete.
#. The release manager mentions user @panch93 on Discord ``#releases-project`` channel so he can update the
   Arch AUR with the latest release.


Behind the Scenes
=================

This section describes how the Continuous Integration (CI) / Continuous Delivery (CD) is setup for Freeciv21
on GitHub.

GitHub's CI/CD is called `Actions` and is enabled via YAML files in this directory in the repository:
https://github.com/longturn/freeciv21/tree/master/.github/workflows.

There are two files that are integral to the release process: :file:`build.yaml` and :file:`release.yaml`. The
:file:`build.yaml` file is the main CI/CD file. It is what runs all of the action "runners" every time a PR is
opened or updated with a commit to the repository. You can see the status of the runners on the actions page
at: https://github.com/longturn/freeciv21/actions. When a release is published, we have code in the file to
upload the installers generated by the operating system runner. In this file we generate the binary packages
for Windows x86_64 (64 bit), Debian, and macOS. The :file:`release.yaml` file is triggered when we publish a
release. This file generates the :file:`.zip` and :file:`.tar.gz` source archives as well as the Windows i686
(32 bit) installer. All of these files are automatically uploaded and attached to the release notes page as
assets at the bottom.
