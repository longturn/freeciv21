..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 louis94 <m_louis30@yahoo.com>

Logging
*******

Freeciv21 has built-in support for printing debugging information at runtime. This is very valuable for
developers who want to check their code or a system administrator to monitor their servers. Five levels are
available:

* ``fatal`` reports errors that cause the application to exit immediately.
* ``critical`` is for recoverable errors caused by a problem in the code or an input file.
* ``warning`` corresponds to a couple of errors that may affect the game state. The players are also
  notified using in-game messages.
* ``info`` is used for generic messages aimed at the user.
* ``debug`` messages are typically not shown, but might provide additional hints when there is a problem.

The minimum level for which messages are printed can be set by passing the ``-d <level>`` argument to
Freeciv21. The default is ``info``.

Freeciv21 uses Qt for its messages, so the many Qt configuration options can be used. See
`the Qt documentation <https://doc.qt.io/qt-5/debug.html#warning-and-debugging-messages>`_ for details. The
system can be tweaked to show only the messages you want at run time, for instance by setting
``QT_LOGGING_RULES``. The complete docs are at
`QLoggingCategory <https://doc.qt.io/qt-5/qloggingcategory.html#configuring-categories>`_. The following
categories are currently defined:

======================== ====================================
Name                     Used for
======================== ====================================
``freeciv.stacktrace``   Stack traces in debug builds.
``freeciv.assert``       Assertion errors.
``freeciv.inputfile``    ``spec`` file parser.
``freeciv.bugs``         When the code finds a bug in itself.
``freeciv.timers``       Various timers.
``freeciv.depr``         Deprecation warnings.
``freeciv.cm``           Citizen manager messages.
``freeciv.ruleset``      Ruleset issues.
``freeciv.goto``         Path finding debug messages.
``freeciv.graphics``     FPS counters.
======================== ====================================

For instance, one could disable stack traces and enable Go To ``debug`` message by setting:

.. code-block:: rst

    export QT_LOGGING_RULES="freeciv.stacktrace*=false\nfreeciv.goto.debug=true"


In addition to tweaking categories, system administrators may want to set ``QT_LOGGING_TO_CONSOLE`` to
``0`` to force use of the system logging facilities.

When Freeciv21 is built in ``Debug`` mode, many messages are added to its ``debug`` output and it becomes
very, very verbose. This is not the case for other build types, for example ``Release`` and
``RelWithDebInfo``. Freeciv21 will try and print a detailed stack trace on ``fatal`` and ``critical``
errors, but this is limited by the availability of debugging symbols.
