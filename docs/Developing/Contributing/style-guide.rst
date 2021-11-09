Documentation Style Guide
*************************

.. Custom Interpretive Text Roles for longturn.net/Freeciv21
.. role:: unit
.. role:: improvement
.. role:: wonder

The Longturn community uses the Python based Sphinx system to generate the documentation available on this
website and in the :file:`doc` directory in the released tarball. Sphinx takes plain text files formatted
with a superset of markdown called reStructuredText (ReST). Sphinx reStructuredText is documented here:
https://www.sphinx-doc.org/en/master/usage/restructuredtext/index.html

This style guide is mostly meant for documentation authors to ensure that we keep a consistent look and feel
between authors.

Headings
========

Headings create chapter breaks between sections of a document as well as provide the title.

Heading 1
    Heading 1 is used to for the title of the page/document.  The asterisk is used to denote Heading 1 like
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
    side table of contents since we specifcally only show 2 levels in the :file:`conf.py` configuration file.
    The dash is used to denote Heading 3 like this:

.. code-block:: rst

    This is heading 3
    -----------------


Interpreted Text Roles
======================

Interpreted text roles are special code blocks that are inserted in line with regular text to create user
interface markup elements to bring attention to something or make it more obvious to the reader what you
want to do. Interpreted text roles are simply a code word surrounded by a colon on both sides and the text
you want to alter is placed inside back-ticks.

* :literal:`:doc:` -- Doc is used to create a hyperlink reference between documents in the documentation
  system.
* :literal:`:emphasis:` -- Emphasis is used to :emphasis:`bring attention to something`.
* :literal:`:file:` -- File is used for file names and paths such as :file:`~/.local/share/freeciv21/saves`
* :literal:`:guilabel:` -- GUI Label is used to bring attention to someting on the screen like the
  :guilabel:`Next` button on the installer wizard
* :literal:`:literal:` -- Literal is used when you want to note a text element in its raw form. This is equivalent to using four back-ticks: ````text````.
* :literal:`:menuselection:` -- Menu Selection is used to give the path of menu clicks such as
  :menuselection:`Game --> Options --> Local Options`. To create the arrow character in between the options
  you will place a text arrow like this: :literal:`-->` in between the selection items.
* :literal:`:strong:` -- Strong is used to :strong:`bold some text`.
* :literal:`:title-reference:` -- Title Reference is used notate a :title-reference:`title entry` in the
  in-game help or to refer to a page in the documentation without giving an actual hyperlink reference
  (see :literal:`:doc:` above).

The docutils specification allows for custom Interpreted Text Roles and we use this feature. The docutils
documentation on this feature is available here:
https://docutils.sourceforge.io/docs/ref/rst/directives.html#custom-interpreted-text-roles

* :literal:`:unit:` -- This provides an opportunity to highlight a Freeciv21 unit, such as the
  :unit:`Musketeer`
* :literal:`:improvement:` -- This provides an opportunity to highlight a Freeciv21 building or city
  improvement, such as the :improvement:`Granary`.
* :literal:`:wonder:` -- This provides an opportunity to highlight a Freeciv21 small or great wonder, such as
  the :wonder:`Pyramids`.

Admonition Directives
=====================

Admonitions are specially marked "topics" that can appear anywhere an ordinary body element can. Typically,
an admonition is rendered as an offset block in a document, sometimes outlined or shaded, with a title
matching the admonition type. We use some of the standard admonitions in our documentation as well.

* :literal:`.. attention::` -- Use attention to bring a very important high profile item to the reader's
  attention.

.. attention::
    This is a really important message! Don't forget to eat breakfast every day.

* :literal:`.. todo::` -- Use To Do as a reminder for documentation editors to come back and fix things at
  a later date.

.. todo::
    Come back and fix something later.

* :literal:`.. note::` --  Use the "note" as we way to give more information to the reader on a topic.

.. note::
    It's important to note that Freeciv21 is really fun to play with group's of people online.

* :literal:`.. code-block:: rst` -- The code block is an excellent way to display actual code or any
  pre-formatted plain text.

.. code-block:: rst

    This is a code block showing some pre-formatted text.

