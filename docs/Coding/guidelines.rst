..
    SPDX-License-Identifier: GPL-3.0-or-later
    SPDX-FileCopyrightText: 2022 Louis Moureaux <m_louis30@yahoo.com>

Coding Guidelines
*****************

This page contains a set of guidelines regarding the preferred coding style for new code. Old code had
different standards and should not be modified for the sole purpose of making it follow the guidelines.

In a nutshell, new Freeciv21 code is written in modern C++ (the standard version evolves depending on what
the targeted compilers support). The coding style strives to achieve a balance between old code originally
written in C and the new possibilities brought by C++, so that new and old code do not look too different.

The rules listed on this page are guidelines. They represent the preferred way of writing code for Freeciv21,
but good code that does not follow the guidelines will not be rejected (though you can expect suggestions for
changes).


General
=======

Code Formatting Rules
---------------------

Freeciv21 uses an automated code formatting tool, ``clang-format`` version 11, to handle the most tedious
parts of code formatting. All code must be formatted with ``clang-format``. This relieves the admins from
checking that you used the correct amount of whitespace after a comma, and you from having to fix it
manually. The downside is that the tool is really picky and will sometimes want to reformat good-looking
code. Even the maintainers are sometimes surprised! You will find more information about ``clang-format``
and detailed instructions in :doc:`/Contributing/pull-request`.

Formatting the code as dictated by ``clang-format`` is mandatory.


Copyright Notices
-----------------

New code follows the `SPDX standard <https://spdx.dev/ids/>`_:

.. code-block:: cpp

    // SPDX-License-Identifier: GPL-3.0-or-later
    // SPDX-FileCopyrightText: 2022 Author Name <how-to-contact@example.com>

You do npt need to add your name to the file, but we recommend that you do so. The same information will be
stored in the Git history, but may be lost if someone ends up moving the file.


Header Guards
-------------

Use ``#pragma once`` to protect headers against multiple inclusion:

.. code-block:: cpp

    #pragma once

This is equivalent to the ancient ``#ifndef``/``#define``/``#endif`` idiom, but also much less error-prone.
The syntax is supported by all modern compilers.


Includes
--------

Included headers coming from the same place should be grouped together: first all Freeciv21 headers grouped
by source directory, then Qt headers, and finally headers from the Standard Library:

.. code-block:: cpp

    // common
    #include "tile.h"
    #include "unit.h"

    // client
    #include "layer.h"

    #include <QString>
    #include <QWidget>

    #include <map>

This order forces Freeciv21 headers to include the Qt and Standard Library headers they need, facilitating
their use in other files.

In ``cpp`` files, the header of the same name should always be included first.


Forward Declarations
--------------------

Whenever possible, use forward declarations in headers instead of adding an ``#include``. This can greatly
speed up compilation:

.. code-block:: cpp

    class QLabel;
    class QLineEdit;

One can use a forward declaration if the class is only used to declare a pointer or reference variable,
possibly as a function argument.


Documentation Comments
----------------------

The function of all public entities (classes, functions, enumerations, enumerators, variables, ...) should
be described in a `Doxygen <https://doxygen.nl/manual/docblocks.html>`_-enabled comment. Classes and
functions should use a multiline comment:

.. code-block:: cpp

    /**
     * Shows a message box greeting the user.
     */
    void greet()
    {
      QMessageBox::information(nullptr, _("Greetings"), _("Hello, user"));
    }

These comments serve two purposes:

* They help the reader understand the code.
* They act as separators between functions in ``cpp`` files.

Single-line comments can be used for very simple methods whose implementation is included in a class
definition, as well as for less complex constructs such as enumerations and variables. The use of Doxygen
`markup commands <https://doxygen.nl/manual/commands.html>`_ to provide more detailed descriptions is
welcome, but in no way mandatory.


Naming Convention
-----------------

The developers have not agreed on a naming convention yet. In the meantime, most code has been following the
former practice of using ``all_lowercase_letters`` in most cases. The only exception to this rule is for
constant values (enumeration values and ``const static`` variables), for which ``UPPERCASE`` is generally
used.

Private member variables should be prefixed with ``m_`` and be placed at the bottom of the class:

.. code-block:: cpp

    class something
    {
    public:
      explicit something();
      virtual ~something();

    private:
      int m_foo;
    };


The ``freeciv`` Namespace
-------------------------

The ``freeciv`` namespace has been used to group classes created during refactoring efforts. This code is
expected to follow higher standards than the rest of the code base, such as encapsulation and having minimal
side-effects.


The Anonymous Namespace
-----------------------

Symbols that are used in a single file, as support for other functions, should be defined in the anonymous
namespace:

.. code-block:: cpp

    namespace /* anonymous */ {

    const int IMPORTANT_CONSTANT = 5; ///< Very, very important

    /**
     * Calculates the square of @c x.
     */
    int some_internal_function(int x)
    {
      return x + IMPORTANT_CONSTANT;
    }

    } // anonymous namespace

The compiler will generate an arbitrary names for symbols in the anonymous namespace that will not clash with
symbols defined elsewhere.


Premature Optimization
----------------------

It is often useless to try and optimize a function before proving that it is inefficient by profiling the
execution in an optimized build (``Release`` or ``RelWithDebInfo``). Most functions in Freeciv21 are not
executed in tight loops. Prefer readable code over fast code.
