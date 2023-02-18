..
.. SPDX-License-Identifier: GPL-3.0-or-later
.. SPDX-FileCopyrightText: Louis Moureaux <m_louis30@yahoo.com>
.. SPDX-FileCopyrightText: James Robertson <jwrober@gmail.com>

Coding Guidelines
*****************

This page contains a set of guidelines regarding the preferred coding style for new code. Old code follows
different standards and should not be modified for the sole purpose of making it follow the guidelines.

In a nutshell, new or refactored Freeciv21 code is written in modern C++ (the standard version evolves
depending on what the targeted compilers support). This coding style strives to achieve a balance between old
code originally written in C and the new possibilities brought by C++, so that new and old code do not look
too different. At this time we are targetting C++17.

The rules listed on this page are guidelines. They represent the preferred way of writing code for Freeciv21,
but good code that does not follow the guidelines will not be rejected directly. However, you should expect
suggestions for changes.


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

Formatting the code as dictated by ``clang-format`` is :strong:`mandatory`.


Copyright Notice
----------------

New code follows the `SPDX standard <https://spdx.dev/ids/>`_:

.. code-block:: cpp

    /**
     * SPDX-License-Identifier: GPL-3.0-or-later
     * SPDX-FileCopyrightText: Freeciv21 and Freeciv Contributors
     * SPDX-FileCopyrightText: Author Name <how-to-contact@example.com>
     */

You do not need to add your name to the file, but we recommend that you do so. Even if the same information
is stored in the Git history, it may be lost if someone ends up moving the file.


Header Guards
-------------

Use ``#pragma once`` to protect headers against multiple inclusion:

.. code-block:: cpp

    #pragma once

This is equivalent to the ancient ``#ifndef``/``#define``/``#endif`` idiom, but much less error-prone.
The syntax is supported by all mainstream compilers.


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

    // Qt
    #include <QString>
    #include <QWidget>

    // std
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

Two highly recommended Doxygen markup commands to include are ``\file`` and ``\class``. When writing code,
especially a new ``cpp`` file, including a comment with the ``\file`` markup near the top, but below the
`Copyright Notice`_ gives the reader a better understanding of what the file's purpose is. If the ``cpp`` file
contains functions related to a class, then using the ``\class`` markup aids the reader in understanding what
the class is doing.


Variable Declaration
--------------------

Older Freeciv21 code will often have a block of variables defined all at once at the top of a function. The
problem with this style is it makes it very easy to create variables that never get used or initialized.

When encountering this older style or writing new code, it is best to define the variable and give it an
initial value right before it is used.

.. code-block:: cpp

    ... some code
    ... some code

    int i;
    for (i = 0; i < max_item; i++) {
      ... do something in the loop
    }


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
side effects.


The Anonymous Namespace
-----------------------

Symbols that are used in a single file, as support for other functions, should be defined in the anonymous
namespace:

.. code-block:: cpp

    namespace /* anonymous */ {

    const int IMPORTANT_CONSTANT = 5; ///< Very, very important

    /**
     * Calculates a more important value of @c x.
     */
    int some_internal_function(int x)
    {
      return x + IMPORTANT_CONSTANT;
    }

    } // anonymous namespace

The compiler will generate arbitrary names for symbols in the anonymous namespace that will not clash with
symbols defined elsewhere.


Premature Optimization
----------------------

It is often useless to try and optimize a function before proving that it is inefficient by profiling the
execution in an optimized build (``Release`` or ``RelWithDebInfo``). Most functions in Freeciv21 are not
executed in tight loops. Prefer readable code over fast code.


C++ Features
============

C++ is a very complex language, but fortunately Freeciv21 only needs to use a relatively small subset. Qt,
our main dependency, manages very well to minimize user exposure to confusing parts. If all you are doing is
small changes here and there, you will most likely not need to know a lot about C++. As your projects grow in
scale and complexity, you will likely want to learn more about the language. In addition to your preferred
learning resources, it is useful to read guidelines written by C++ experts, for instance the
`C++ Core Guidelines <https://isocpp.github.io/CppCoreGuidelines/>`_ edited by the very founder of C++.

We collect below a list of recommendations that we find useful in the context of Freeciv21.


Pass by Reference
-----------------

When writing a function that takes a complex object (anything larger than a ``long long``), use a constant
reference:

.. code-block:: cpp

    QString foo(const QString &argument);
    int bar(const std::vector<int> &argument);


Use ``const``
-------------

Variables that are not modified should be declared ``const``. While this is more of a personal preference for
variables, it is especially important for functions taking references (see above).

Functions that do not modify their argument should make them ``const``. Class methods that do not modify the
object should also be marked ``const``.


Use Encapsulation
-----------------

Classes that are more complicated than C-like ``struct`` should not have any public variables. Getters and
setters should be provided when needed.


Use ``auto``
------------

The ``auto`` keyword is useful to avoid typing the type of a variable, especially lengthy names used in the
Standard Library. We recommend to use it whenever possible. Do *not* try to use ``auto`` for function
arguments.

.. code-block:: cpp

    const auto &unit = tile->units.front();


Use STL Containers
------------------

Containers in the Standard Library should be preferred over Qt ones:

.. code-block:: cpp

    std::vector<unit *> foo;
    std::map<int, int> bar;

One notable exception is ``QStringList``, which should be preferred over other constructs because it
integrates better with Qt.

The main point here is to avoid using function parameters to return values.


Use ``<algorithm>``
-------------------

The C++ Standard Library provides a set of `basic algorithms <https://en.cppreference.com/w/cpp/algorithm>`_.
Code using the standard algorithms is often more clear than hand-written loops, if only because experienced
programmers will recognize the function name immediately.


Use Range-based ``for``
-----------------------

Avoid using indices to iterate over containers. Prefer the much simpler range-based ``for``:

.. code-block:: cpp

    for (const auto &city : player->cities) {
      // ...
    }


Use Structured Bindings
-----------------------

Structured bindings are very useful when facing a ``std::pair``, for instance when iterating over a map:

.. code-block:: cpp

    for (const auto &[key, value] : map) {

If you do not wish to use one of the variables, use ``_``:

.. code-block:: cpp

    for (const auto &[key, _] : map) {
      // Use the key only


Use Smart Pointers
------------------

Instead of using ``new`` and ``delete``, delegate the task to a smart pointer:

.. code-block:: cpp

    auto result = std::make_unique<cm_result>();

When facing a memory handling bug such as a double free, it is sometimes easier to rewrite the code using
smart pointers than to understand the issue.

Smart pointers are rarely needed with Qt classes. The
`parent-child mechanism <https://doc.qt.io/qt/qobject.html#details>`_ is the preferred way of handling
ownership for classes deriving from ``QObject``. In many other cases, Qt classes are meant to be used
directly on the stack. This is valid for ``QString``, ``QByteArray``, ``QColor``, ``QPixmap``, and many
others. If you are unsure, try to find an example in the Qt documentation.

Qt provides its own smart pointer for ``QObject``, called `QPointer <https://doc.qt.io/qt/qpointer.html>`_.
This pointer tracks the lifetime of the pointed-to object and is reset to ``nullptr`` if the object gets
deleted. This is useful in some situations.


.. todo::

    We would like to include some tips about the following topics in the future:

    * Logging
    * Qt Tips
