/**************************************************************************
 Copyright (c) 1996-2020 Freeciv21 and Freeciv contributors. This file is
 part of Freeciv21. Freeciv21 is free software: you can redistribute it
 and/or modify it under the terms of the GNU  General Public License  as
 published by the Free Software Foundation, either version 3 of the
 License,  or (at your option) any later version. You should have received
 a copy of the GNU General Public License along with Freeciv21. If not,
 see https://www.gnu.org/licenses/.
**************************************************************************/
#pragma once

#include <set>
#include <utility>

/***************************************************************************
  Helper template to connect C and C++ code.

  This class is a helper to create Java-like "listeners" for "events"
  generated in C code. If the C interface defines a callback foo() and
  you want to use it as an event, you first declare a C++ interface:

  ~~~~~{.cpp}
  class foo_listener : public listener<foo_listener>
  {
  public:
    virtual void foo() = 0;
  };
  ~~~~~

  The listener needs some static data. Declaring it is as simple as putting
  a macro in some source file:

  ~~~~~{.cpp}
  FC_CPP_DECLARE_LISTENER(foo_listener)
  ~~~~~

  Then, you call the listeners from the implementation of the C interface:

  ~~~~~{.cpp}
  void foo()
  {
    foo_listener::invoke(&foo_listener::foo);
  }
  ~~~~~

  This will invoke foo() on all foo_listener objects.

  == Listening to events

  Listening to events is done by inheriting from a listener. When your
  object is ready to receive events, it should call the listener's @c listen
  function. This will typically be done in the constructor:

  ~~~~~{.cpp}
  class bar : private foo_listener
  {
  public:
    explicit bar();
    void foo();
  };

  bar::bar()
  {
    // Initialize the object here
    foo_listener::listen();
  }
  ~~~~~

  == Passing arguments

  Passing arguments to the listeners is very simple: you write them after
  the function pointer. For instance, let's say foo() takes an int. It can
  be passed to the listeners as follows:

  ~~~~~{.cpp}
  void foo(int argument)
  {
    foo_listener::invoke(&foo_listener::foo, argument);
  }
  ~~~~~

  As there may be an arbitrary number of listeners, passing mutable data
  through invoke() is discouraged.

  == Technical details

  This class achieves its purpose using the Curiously Recurring Template
  Pattern, hence the weird parent for listeners:

  ~~~~~{.cpp}
  class foo_listener : public listener<foo_listener>
  ~~~~~

  The template argument is used to specialize object storage and member
  function invocation. Compilers should be able to inline calls to invoke(),
  leaving only the overhead of looping on all instances.

  @warning Implementation is not thread-safe.
***************************************************************************/
template <class _type_> class listener {
public:
  // The type a given specialization supports.
  typedef _type_ type_t;

private:
  // All instances of type_t that have called listen().
  static std::set<listener<type_t> *> instances;

protected:
  explicit listener();
  virtual ~listener();

  void listen();

public:
  template <class _member_fct_, class... _args_>
  static void invoke(_member_fct_ function, _args_ &&...args);
};

/***************************************************************************
  Macro to declare the static data needed by listener<> classes
***************************************************************************/
#define FC_CPP_DECLARE_LISTENER(_type_)                                     \
  template <>                                                               \
  std::set<listener<_type_> *> listener<_type_>::instances =                \
      std::set<listener<_type_> *>();

/***************************************************************************
  Constructor
***************************************************************************/
template <class _type_> listener<_type_>::listener() = default;

/***************************************************************************
  Starts listening to events
***************************************************************************/
template <class _type_> void listener<_type_>::listen()
{
  // If you get an error here, your listener likely doesn't inherit from the
  // listener<> correctly. See the class documentation.
  instances.insert(this);
}

/***************************************************************************
  Destructor
***************************************************************************/
template <class _type_> listener<_type_>::~listener()
{
  instances.erase(this);
}

/***************************************************************************
  Invokes a member function on all instances of a listener type. Template
  parameters are meant to be automatically deduced.

  @param function The member function to call
  @param args     Arguments to pass ot the function.
***************************************************************************/
template <class _type_>
template <class _member_fct_, class... _args_>
void listener<_type_>::invoke(_member_fct_ function, _args_ &&...args)
{
  for (auto &instance : instances) {
    (dynamic_cast<type_t *>(instance)->*function)(
        std::forward<_args_>(args)...);
  }
}
