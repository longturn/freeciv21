..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>

Internationalization (I18N)
***************************

Messages and text in general which are shown in the GUI should be translated by using the :code:`_()` macro.
In addition :code:`qInfo()` and some :code:`qWarning()` messages should be translated. In most cases, the
other log levels (:code:`qFatal()`, :code:`qCritical()`, :code:`qDebug()`, :code:`log_debug()`) should NOT be
localised.

See :file:`utility/fciconv.h` for details of how Freeciv21 handles character sets and encoding. Briefly:

* The data_encoding is used in all data files and network transactions. This is UTF-8.

* The internal_encoding is used internally within Freeciv21. This is always UTF-8 at the server, but can be
  configured by the GUI client. When your charset is the same as your GUI library, GUI writing is easier.

* The local_encoding is the one supported on the command line. This is not under our control, and all output
  to the command line must be converted.
