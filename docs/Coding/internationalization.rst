..  SPDX-License-Identifier: GPL-3.0-or-later
..  SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
..  SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
..  SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

Internationalization
********************

Internationalization, or i18n for short, refers to making sure software is ready to be adapted for use in a
language and country other than the original. Freeciv21 uses American English (``en-US``) as the primary
target language, but can be localized to other regions.

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
library (defined in ``utility/fcintl.h``). The most simple one returns a translated version of a string:

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


Not Translated
--------------

While most text should be translated, there are a few cases where this is not wanted:

* :strong:`File Formats:` It must be possible to load a saved game independently of the locale that was in
  use when producing it. The same stands for rulesets, tilesets, and other file formats used by Freeciv21.
* :strong:`Fatal Error Messages:` If the game encounter such a critical error that it needs to abort, it may
  not have the resources to produce a translation.


Helper Comments
---------------

When translators work on strings, they are provided with a list taken out of context. They can see the
original text in English and sometimes the source code, but most translators cannot read code. In some
cases, the lack of context makes translation very hard. The string "Close", for instance, can have many
meanings, with the correct one being inferred from context: "near", "closed", "close (a door)", "stop", etc.
Each meaning calls for a different translation. One can add special comments to the code to help translators
identify the correct variant:

.. code-block:: cpp

    // TRANS: Close the current window
    close_button->setText(_("Close"));

The comment will be picked up if it is on the line before the translated text. These comments are typically
very useful when building text from different parts using placeholders (``%1``, ``%2``, etc.). In such cases,
a comment should be added to explain what the final string looks like:

.. code-block:: cpp

    // TRANS: <Unit> (<Home city>)
    text += QString::format(_("%1 (%2)")).arg(unit_type_name).arg(home_city_name);

    // TRANS: "HP: 5/20 MP: 5/5" in unit description. Keep short
    text += QString::format(_("HP: %1/%2 MP: %3/%4"))
                .arg(hp)
                .arg(max_hp)
                .arg(mp)
                .arg(max_mp);

In complex cases, adding an example or a short explanation also makes the code easier to read.


Character Encodings
-------------------

The way characters are encoded into strings has long been a hot topic of internationalization, and
language-specific character encodings are still around on some systems. Freeciv21 always uses UTF-8 for data
files and internal communication (e.g. in the network protocol).  The recommended way of storing text is with
Qt's ``QString`` class, which uses UTF-16 internally.

The ``QString`` constructor performs the conversion from UTF-8 automatically when passed a ``char *``
argument. In the opposite direction, ``qUtf8Printable()`` takes a ``QString`` and returns a *temporary*
``char *`` encoded in UTF-8, which is deleted automatically at the next semicolon.

Text can be converted to the system encoding using ``qPrintable`` or ``QString::toLocal8Bit``. This should
be rarely, if ever, needed.


Common Difficulties
-------------------

Every language is different, and there is no reason for the order of words or even sentences to be the same
as in English. When possible, it is thus preferable to provide the translators with full sentences or
paragraphs. If you can speak several languages, it is also useful to think about how to translate your text:
you may find a way to simplify it and facilitate its translation. In this section, we describe a few issues
that we have encountered.

Freeciv21 cannot handle more than one plural in the same string. Imagine the following text::

    %1 units, %2 buildings, %3 wonders

Since the ``PL_()`` macro takes a single numeric parameter, only one of the words can be pluralized
correctly. There is currently no fully satisfactory solution to this problem. A slightly better version would
be an enumeration::

    Units: %1, buildings: %2, wonders: %3

In English and some other languages, this form is correct even if there is only one unit.

Another difficulty shows up when dynamically inserting words in a sentence. This works extremely well in
English, but in many other languages this leads to incorrect grammar, with genders and declension being
common culprits. For example, consider the following simplified version of the "unit lost" message:

    Legion lost to an attack by a Greek Catapult.

This message has three dynamic parts: the unit types and the nationality of the attacker. Let us now look at
the correct French translation:

    Légion perdue dans une attaque d'une Catapulte grecque.

If you look closely enough, you will notice that this is pretty close: "perdue" is "lost" and the order of
"Greek" and "Catapult" needs to be swapped. But what if the attacker is a Cannon and the defender a
Musketeer?

    Mousquetaire :strong:`perdu` dans une attaque :strong:`d'un` Canon :strong:`grec`.

The words in bold in the main sentence had to be changed to match the new units. There is currently no real
solution to this problem in Freeciv21, and translators resort to use incorrect grammar.
