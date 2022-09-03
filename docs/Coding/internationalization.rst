..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 1996-2021 Freeciv Contributors
    SPDX-FileCopyrightText: 2022 James Robertson <jwrober@gmail.com>
    SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>

Internationalization
********************

Internationalization, or i18n for short, refers to making sure software is ready to be adapted for use in a
language and country other than the original. Freeciv21 uses American English as the primary target language,
but can be localized to other regions.

You should keep internationalization in mind when writing code that interacts with the user. Here are a few
things to take into account:

* Text should be translated to the language of the user. Freeciv21 has facilities to handle this, see below.
* Widgets should resize automatically to fit the text. In a language like Russian, text is often twice as
  long as in English.
* The visual arrangement of items on screen should match the natural direction of the language. Arabic and
  Hebrew, for instance, write text from right to left. This is in many cases handled automatically by Qt.

Paying attention to these three points is sufficient to get internationalization right in the vast majority
of the cases.

Translating Text
----------------

Translating user-facing text is the most work-intensive part of internationalization. As a developer, you
only need to make sure that strings are marked for translation when appropriate. In Freeciv21, this is done
using a set of macros based on the `gettext <https://www.gnu.org/software/gettext/manual/gettext.html>`_
library. The most simple one returns a translated version of a string:

.. code-block:: cpp

    const char *translation = _("some text");

In this example, "some text" will be collected automatically and handed over to the translators. If there is
a translation for the locale used by the user, it will be used automatically instead of the original English
text.

Dealing with plurals requires a different function. Indeed, pluralization rules are very complicated in some
languages, so this is handled in another macro:

.. code-block:: cpp

    int count = ...;
    const char *translation = PL("%1 translation", "%1 translations", count);
    const QString with_number = QString(translation).arg(count);

Notice how you still need to replace ``%1`` by yourself, since this is not handled by the macro. In actual
code, the intermediate variable would be simplified away:

.. code-block:: cpp

    const QString with_number =
        QString(PL("%1 translation", "%1 translations", count)).arg(count);

In some cases, one needs to disambiguate between different uses of the same string. For instance, ``None`` is
used in different places to mean "no technology" and "no wonder". This is handled by ``Q_``:

.. code-block:: cpp

    const char *translation = Q_("?wonder:None");

Finally, there is also a macro ``N_()`` that marks a string for translation without doing the translation (it
returns the original). This is useful in static contexts.

Writing Strings for Translation
-------------------------------

In addition :code:`qInfo()` and some :code:`qWarning()` messages should be translated. In most cases, the
other log levels (:code:`qFatal()`, :code:`qCritical()`, :code:`qDebug()`, :code:`log_debug()`) should NOT be
localised.

See :file:`utility/fciconv.h` for details of how Freeciv21 handles character sets and encoding. Briefly:

* The data_encoding is used in all data files and network transactions. This is UTF-8.

* The internal_encoding is used internally within Freeciv21. This is always UTF-8 at the server, but can be
  configured by the GUI client. When your charset is the same as your GUI library, GUI writing is easier.

* The local_encoding is the one supported on the command line. This is not under our control, and all output
  to the command line must be converted.
