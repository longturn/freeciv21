.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder
.. role:: advance


Documentation Style Guide
*************************

The Longturn community uses the Python based Sphinx system to generate the documentation available on this
website. Sphinx takes plain text files formatted with a super set of markdown called reStructuredText (ReST).
Sphinx reStructuredText is documented here:
https://www.sphinx-doc.org/en/master/usage/restructuredtext/index.html

This style guide is mostly meant for documentation authors to ensure that we keep a consistent look and feel
between authors.

Headings
========

Headings create chapter breaks between sections of a document as well as provide the title.

Heading 1
    Heading 1 is used to for the title of the page/document. The asterisk is used to denote Heading 1 like
    this:

.. code-block:: rst

    This is an awesome page title
    *****************************


Heading 2
    Heading 2 is used to create chapter markers or other major breaks in a document. The equal sign is used
    to denote Heading 2 like this:

.. code-block:: rst

    This is a chapter marker
    ========================


Heading 3
    Heading 3 is used to break a chapter into sub-chapters in a document. Heading 3 is not shown on the left
    side table of contents since we specifically only show 2 levels in the :file:`conf.py` configuration file.
    The dash is used to denote Heading 3 like this:

.. code-block:: rst

    This is a sub-chapter marker
    ----------------------------


Heading 4
    Heading 4 is used to break a sub-chapter down into a further section. This is useful for large pages, such
    as the :doc:`/Coding/hacking`. The caret is used to denote Heading 4 like this:

.. code-block:: rst

    This is a sub-chapter break
    ^^^^^^^^^^^^^^^^^^^^^^^^^^^


.. _style-attribution:

Attribution
===========

As with our code, the documentation is written by contributors and proper attribution should always be given.
We follow the `SPDX <https://spdx.dev/>`_ standard for attribution in all of our documentation, music, and art
files. For documentation files, we can add the proper SPDX header to the very top of the page. For music and
art we add a text license file along with the primary file. For example, if we have :file:`art.png`, then
there should be a corresponding :file:`art.png.license` file to give proper attribution. We use the GPL v3.0
or later license. Here is what the SPDX header should look like in all scenarios:

.. code-block:: rst

    .. SPDX-License-Identifier: GPL-3.0-or-later
    .. SPDX-FileCopyrightText: [contributor name or handle] <[contributor email address]>


.. note::
    We do not add a date (e.g. year) to our attribution blocks. There is recent commentary that this is not
    needed and leaving the date off makes keeping header blocks up to date easier.

If the file you are working with came from legacy Freeciv, please add this line to the SPDX header for proper
attribution:

.. code-block:: rst

    .. SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors


Interpreted Text Roles
======================

Interpreted text roles are special code blocks that are inserted in line with regular text to create user
interface markup elements to bring attention to something or make it more obvious to the reader what you want
to do. Interpreted text roles are simply a code word surrounded by a colon on both sides and the text you want
to alter is placed inside back-ticks.

* :literal:`:doc:` -- Doc is used to create a hyperlink reference between documents in the documentation
  system.
* :literal:`:ref:` -- Create a cross-reference link to an anchor in another document. This is similar to
  :literal:`:doc:`, except it allows you to go to a specific location within a page, instead of the top of the
  page. To use :literal:`:ref:`, you add an anchor in a page such as :literal:`.. _My Anchor:` and then refer
  to it like this: :literal:`:ref:`My Anchor``. Notice that the anchor has an underscore at the beginning.
  This is required for sphinx to recognize it. Also notice the use of the anchor in :literal:`:ref:` leaves
  the underscore off.
* :literal:`:numref:` -- Create a cross-reference to a named figure.
* :literal:`:table:` -- Create a named table reference. Place an anchor (e.g. :literal:`.. _My Anchor:`) above
  to enable :literal:`:numref:`.
* :literal:`:figure:` -- Create a named figure reference. Place an anchor (e.g. :literal:`.. _My Anchor:`)
  above to enable :literal:`:numref:`.
* :literal:`:emphasis:` -- Emphasis is used to :emphasis:`bring attention to something`.
* :literal:`:file:` -- File is used for file names and paths such as :file:`~/.local/share/freeciv21/saves`.
* :literal:`:guilabel:` -- GUI Label is used to bring attention to something on the screen like the
  :guilabel:`Next` button on the installer wizard.
* :literal:`:literal:` -- Literal is used when you want to note a text element in its raw form. This is
  equivalent to using two back-ticks: ````text````.
* ``math`` and ``.. math::`` -- Used to insert mathematics, see `Formulas`_.
* :literal:`:menuselection:` -- Menu Selection is used to give the path of menu clicks such as
  :menuselection:`Game --> Local Options`. To denote submenus, use a test arrow like this: :literal:`-->`
  between the selection items.
* :literal:`:strong:` -- Strong is used to :strong:`bold some text`. A good use of :literal:`:strong:` is to
  highlight game elements.
* :literal:`:title-reference:` -- Title Reference is used notate a :title-reference:`title entry` in the
  in-game help or to refer to a page in the documentation without giving an actual hyperlink reference
  (see :literal:`:doc:` above).
* :literal:`.. versionadded::` -- Used at the paragraph level to document the first version in which a feature
  was added.

The docutils specification allows for custom Interpreted Text Roles and we use this feature. The docutils
documentation on this feature is available here:
https://docutils.sourceforge.io/docs/ref/rst/directives.html#custom-interpreted-text-roles

* :literal:`:unit:` -- This provides an opportunity to highlight a Freeciv21 unit, such as the
  :unit:`Musketeer`
* :literal:`:improvement:` -- This provides an opportunity to highlight a Freeciv21 building or city
  improvement, such as the :improvement:`Granary`.
* :literal:`:wonder:` -- This provides an opportunity to highlight a Freeciv21 small or great wonder, such as
  the :wonder:`Pyramids`.
* :literal:`:advance:` -- This provides an opportunity to highlight a Freeciv21 technology advance, such as
  :advance:`Ceremonial Burial`.

Admonition Directives
=====================

Admonitions are specially marked "topics" that can appear anywhere an ordinary body element can. Typically, an
admonition is rendered as an offset block in a document, sometimes outlined or shaded, with a title matching
the admonition type. We use some of the standard admonitions in our documentation as well.

* :literal:`.. attention::` -- Use Attention to bring a very important high profile item to the reader's
  attention.

.. attention::
    This is a really important message! Do not forget to eat breakfast every day.

* :literal:`.. todo::` -- Use To Do as a reminder for documentation editors to come back and fix things at
  a later date.

.. todo::
    Come back and fix something later.

* :literal:`.. note::` --  Use the Note as the way to give more information to the reader on a topic.

.. note::
    It is important to note that Freeciv21 is really fun to play with groups of people online.

* :literal:`.. code-block:: rst` -- The code block is an excellent way to display actual code or any
  pre-formatted plain text. The tag ``rst`` can be replaces by ``sh``, ``cpp``, and ``ini`` as well to give
  different types of markup for shell commands, C++ code, and ini file formatting.

.. code-block:: rst

    This is a code block showing some pre-formatted text.


Language Usage Elements
=======================

The documentation is written mostly in US English (en_US), however elements of Queen's English (e.g. en_GB)
are also found in the documentation. The two forms of English are close enough that we do not worry too much
if one author uses "color" and another uses "colour". Any reader or language translator will be able to figure
out what the author is trying to say. However, there are some standards that documentation authors do need to
adhere to, so the documentation is consistently formatted and certain language elements are always used the
same way.

The Oxford Comma
    The Oxford Comma is the usage of a comma when listing multiple items and placing a comma before the "and"
    or "or" at the end of the list. For example: You need to follow these steps: Click on :guilabel:`Menu`,
    then click on :guilabel:`Options`, and finally click on :guilabel:`Interface`. Notice the comma usage
    before the word "and", that is the Oxford comma and its usage is expected in our documentation.

Capitalization
    For consistent formatting, the following should always use
    `"Title Case" rules <https://www.grammarly.com/blog/capitalization-rules/>`_:

    * Page and section headings (e.g. the 4 documented above).
    * Image captions, when they act as a title to the image.
    * The names of specific game items such as units, city improvements, technologies, wonders, etc. Some of
      them even have special text roles (:literal:`:unit:`, :literal:`:improvement:`, and :literal:`:wonder:`).
      :doc:`See here for a list. <capitalized-terms>`

      This is particularly useful with words that are used ambiguously in the game, such as "granary" which is
      both the amount of food a city needs before growing and an improvement in many rulesets. Another
      example is "transport" which covers both the movement of units on a ship and the particular unit type of
      :unit:`Transport`.

    When describing elements of the user interface, use the same capitalization as in the game and wrap the
    text inside markup elements with the :literal:`:guilabel:` or :literal:`:menuselection:` roles. They are
    rendered as follows: "the :guilabel:`Turn Done` button", "select :menuselection:`Help --> Overview` in
    the menu".

    .. Get rid of the "WARNING: document isn't included in any toctree"
    .. toctree::
      :hidden:

      capitalized-terms

Language Contractions
    Language Contractions are when two words are combined together with an apostrophe ( ``'`` ). For example,
    the word "don't" is a contraction of "do not". Not all language translators, and especially non-native
    English speakers can get confused if contractions are used. To aid the readability of our documentation,
    :strong:`the usage of contractions is not advised` and should be used sparingly.

The Use of Person
    In English there are three types of person: first, second, and third. First person is possessive -- "I
    took a walk down the street". Second person is about speaking to someone -- "You took a walk down the
    street". Third person is non-specific -- "They took a walk down the street". In our documentation we use
    the second person form. We want to be conversational with our readers and speak to them about the game,
    features, actions, etc.

    This page provides a good overview of the use person for US English:
    https://www.grammar-monster.com/glossary/person.htm

Double Negatives / Negations
    To aid the readability of our documentation, we want to stay away from using double negatives. A double
    negative is where two negative words are combined together that end with a positive. For example:
    "The guidelines are not bad". The last two words are negative -- "not bad". It is better to use positive
    language. For example the first sentence is better written as: "The guidelines are good".

Figure Numbers
    Diagrams, Screenshots, and Tables are :strong:`expected` to be numbered using the :literal:`numfig`
    feature of Sphinx. For example see this code block for a figure:

    .. code-block:: rst

        .. _Start Screen:
        .. figure:: /_static/images/gui-elements/start-screen.png
          :scale: 65%
          :align: center
          :alt: Freeciv21 Start Screen
          :figclass: align-center

          Start Screen with NightStalker Theme


    The first line ``.. _Start Screen:`` is a label for the figure. The ``numfig`` feature of Sphinx will
    automatically give the figure a number in the order they are found in the page. You can then provide a
    link to the figure in your text with :literal:`:numfig:\`Label\``


Formulas
========

The Freeciv21 documentation supports inserting mathematics. This feature should be used sparingly, ideally
only on technical pages or in sections that less math-savvy users can skip. When math formulas are used on
non-technical pages (such as any one of the manuals), the reasoning should be relatively simple following
`elementary algebra <https://en.wikipedia.org/wiki/Elementary_algebra>`_. Contrary to ordinary math
textbooks, it is best to avoid single-letter symbols in the documentation. Full-text names should be used
instead, wrapping them with ``\text{}``:

.. math::
  \text{happy} \ge \text{unhappy} + 2 \times \text{angry}.

There may be exceptions to this rule on primarily technical pages: quantities that exist as variables in the
code could be typeset in monospace with ``\texttt{}``, or defining a few symbols may come handy when writing
a long reasoning. The main guideline for formulas is to take your time to make them as readable as possible.

Formulas use the ``:math:`` role or the ``.. math::`` directive. These blocks support most of the LaTeX
`mathematics syntax <https://en.wikibooks.org/wiki/LaTeX/Mathematics>`_. The ``:math:`` role is used for
inline math in a paragraph. For instance, ``:math:`a+b=1``` becomes :math:`a+b=1`. The directive is used for
longer or more important formulas that come on their own line:

.. math::
  a+b=1.

This is rendered using an ``align`` environment, so alignment directives (``&``) can be used.

.. warning::
  When editing formulas, checking both the HTML and the PDF output is heavily recommended.
