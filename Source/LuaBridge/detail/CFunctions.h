//------------------------------------------------------------------------------
/*
  https://github.com/vinniefalco/LuaBridge

  Copyright 2012, Vinnie Falco <vinnie.falco@gmail.com>

  License: The MIT License (http://www.opensource.org/licenses/mit-license.php)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
//==============================================================================

#pragma once

#include <LuaBridge/detail/ClassTable.h>

#include <string>


namespace luabridge {

// We use a structure so we can define everything in the header.
//
struct CFunc
{
  //----------------------------------------------------------------------------
  /**
      __index metamethod for a namespace or class static members.

      This handles:
        Retrieving functions and class static methods, stored in the metatable.
        Reading global and class static data, stored in the __propget table.
        Reading global and class properties, stored in the __propget table.
  */
  static int indexMetaMethod (lua_State* L)
  {
    ClassTable* classTable = ClassTable::fromStack (L, -1);
    classTable->getField (lua_tostring (L, lua_upvalueindex (1)), false);
    return 1;
  }

  //----------------------------------------------------------------------------
  /**
      __newindex metamethod for a namespace or class static members.

      The __propset table stores proxy functions for assignment to:
        Global and class static data.
        Global and class properties.
  */
  static int newindexMetaMethod (lua_State* L)
  {
    ClassTable* classTable = static_cast <ClassTable*> (lua_touserdata (L, -1));
    assert (classTable);
    classTable->getSetter (lua_tostring (L, lua_upvalueindex (1)));
    return 1;
  }

  //----------------------------------------------------------------------------
  /**
      lua_CFunction to report an error writing to a read-only value.

      The name of the variable is in the first upvalue.
  */
  static int readOnlyError (lua_State* L)
  {
    return luaL_error (L, "'%s' is read-only", lua_tostring (L, lua_upvalueindex (1)));
  }

  //----------------------------------------------------------------------------
  /**
      lua_CFunction to get a variable.

      This is used for global variables or class static data members.

      The pointer to the data is in the first upvalue.
  */
  template <class T>
  static int getVariable (lua_State* L)
  {
    assert (lua_islightuserdata (L, lua_upvalueindex (1)));
    T const* ptr = static_cast <T const*> (lua_touserdata (L, lua_upvalueindex (1)));
    assert (ptr != 0);
    Stack <T>::push (L, *ptr);
    return 1;
  }

  //----------------------------------------------------------------------------
  /**
      lua_CFunction to set a variable.

      This is used for global variables or class static data members.

      The pointer to the data is in the first upvalue.
  */
  template <class T>
  static int setVariable (lua_State* L)
  {
    assert (lua_islightuserdata (L, lua_upvalueindex (1)));
    T* ptr = static_cast <T*> (lua_touserdata (L, lua_upvalueindex (1)));
    assert (ptr != 0);
    *ptr = Stack <T>::get (L, 1);
    return 0;
  }

  //----------------------------------------------------------------------------
  /**
      lua_CFunction to call a function with a return value.

      This is used for global functions, global properties, class static methods,
      and class static properties.

      The function pointer is in the first upvalue.
  */
  template <class FnPtr,
    class ReturnType = typename FuncTraits <FnPtr>::ReturnType>
    struct Call
  {
    typedef typename FuncTraits <FnPtr>::Params Params;
    static int f (lua_State* L)
    {
      assert (isfulluserdata (L, lua_upvalueindex (1)));
      FnPtr const& fnptr = *static_cast <FnPtr const*> (lua_touserdata (L, lua_upvalueindex (1)));
      assert (fnptr != 0);
      try
      {
        ArgList <Params> args (L);
        Stack <typename FuncTraits <FnPtr>::ReturnType>::push (L, FuncTraits <FnPtr>::call (fnptr, args));
      }
      catch (const std::exception& e)
      {
        luaL_error (L, e.what ());
      }
      return 1;
    }
  };

  //----------------------------------------------------------------------------
  /**
      lua_CFunction to call a function with no return value.

      This is used for global functions, global properties, class static methods,
      and class static properties.

      The function pointer is in the first upvalue.
  */
  template <class FnPtr>
  struct Call <FnPtr, void>
  {
    typedef typename FuncTraits <FnPtr>::Params Params;
    static int f (lua_State* L)
    {
      assert (isfulluserdata (L, lua_upvalueindex (1)));
      FnPtr const& fnptr = *static_cast <FnPtr const*> (lua_touserdata (L, lua_upvalueindex (1)));
      assert (fnptr != 0);
      try
      {
        ArgList <Params> args (L);
        FuncTraits <FnPtr>::call (fnptr, args);
      }
      catch (const std::exception& e)
      {
        luaL_error (L, e.what ());
      }
      return 0;
    }
  };

  //----------------------------------------------------------------------------
  /**
      lua_CFunction to call a class member function with a return value.

      The member function pointer is in the first upvalue.
      The class userdata object is at the top of the Lua stack.
  */
  template <class MemFnPtr,
    class ReturnType = typename FuncTraits <MemFnPtr>::ReturnType>
    struct CallMember
  {
    typedef typename FuncTraits <MemFnPtr>::ClassType T;
    typedef typename FuncTraits <MemFnPtr>::Params Params;

    static int f (lua_State* L)
    {
      assert (isfulluserdata (L, lua_upvalueindex (1)));
      T* const t = Userdata::get <T> (L, 1, false);
      MemFnPtr const& fnptr = *static_cast <MemFnPtr const*> (lua_touserdata (L, lua_upvalueindex (1)));
      assert (fnptr != 0);
      try
      {
        ArgList <Params, 2> args (L);
        Stack <ReturnType>::push (L, FuncTraits <MemFnPtr>::call (t, fnptr, args));
      }
      catch (const std::exception& e)
      {
        luaL_error (L, e.what ());
      }
      return 1;
    }
  };

  template <class MemFnPtr,
    class ReturnType = typename FuncTraits <MemFnPtr>::ReturnType>
    struct CallConstMember
  {
    typedef typename FuncTraits <MemFnPtr>::ClassType T;
    typedef typename FuncTraits <MemFnPtr>::Params Params;

    static int f (lua_State* L)
    {
      assert (isfulluserdata (L, lua_upvalueindex (1)));
      T const* const t = Userdata::get <T> (L, 1, true);
      MemFnPtr const& fnptr = *static_cast <MemFnPtr const*> (lua_touserdata (L, lua_upvalueindex (1)));
      assert (fnptr != 0);
      try
      {
        ArgList <Params, 2> args (L);
        Stack <ReturnType>::push (L, FuncTraits <MemFnPtr>::call (t, fnptr, args));
      }
      catch (const std::exception& e)
      {
        luaL_error (L, e.what ());
      }
      return 1;
    }
  };

  //----------------------------------------------------------------------------
  /**
      lua_CFunction to call a class member function with no return value.

      The member function pointer is in the first upvalue.
      The class userdata object is at the top of the Lua stack.
  */
  template <class MemFnPtr>
  struct CallMember <MemFnPtr, void>
  {
    typedef typename FuncTraits <MemFnPtr>::ClassType T;
    typedef typename FuncTraits <MemFnPtr>::Params Params;

    static int f (lua_State* L)
    {
      assert (isfulluserdata (L, lua_upvalueindex (1)));
      T* const t = Userdata::get <T> (L, 1, false);
      MemFnPtr const& fnptr = *static_cast <MemFnPtr const*> (lua_touserdata (L, lua_upvalueindex (1)));
      assert (fnptr != 0);
      try
      {
        ArgList <Params, 2> args (L);
        FuncTraits <MemFnPtr>::call (t, fnptr, args);
      }
      catch (const std::exception& e)
      {
        luaL_error (L, e.what ());
      }
      return 0;
    }
  };

  template <class MemFnPtr>
  struct CallConstMember <MemFnPtr, void>
  {
    typedef typename FuncTraits <MemFnPtr>::ClassType T;
    typedef typename FuncTraits <MemFnPtr>::Params Params;

    static int f (lua_State* L)
    {
      assert (isfulluserdata (L, lua_upvalueindex (1)));
      T const* const t = Userdata::get <T> (L, 1, true);
      MemFnPtr const& fnptr = *static_cast <MemFnPtr const*> (lua_touserdata (L, lua_upvalueindex (1)));
      assert (fnptr != 0);
      try
      {
        ArgList <Params, 2> args (L);
        FuncTraits <MemFnPtr>::call (t, fnptr, args);
      }
      catch (const std::exception& e)
      {
        luaL_error (L, e.what ());
      }
      return 0;
    }
  };

  //--------------------------------------------------------------------------
  /**
      lua_CFunction to call a class member lua_CFunction.

      The member function pointer is in the first upvalue.
      The class userdata object is at the top of the Lua stack.
  */
  template <class T>
  struct CallMemberCFunction
  {
    static int f (lua_State* L)
    {
      assert (isfulluserdata (L, lua_upvalueindex (1)));
      typedef int (T::*MFP) (lua_State* L);
      T* const t = Userdata::get <T> (L, 1, false);
      MFP const& fnptr = *static_cast <MFP const*> (lua_touserdata (L, lua_upvalueindex (1)));
      assert (fnptr != 0);
      return (t->*fnptr) (L);
    }
  };

  template <class T>
  struct CallConstMemberCFunction
  {
    static int f (lua_State* L)
    {
      assert (isfulluserdata (L, lua_upvalueindex (1)));
      typedef int (T::*MFP) (lua_State* L);
      T const* const t = Userdata::get <T> (L, 1, true);
      MFP const& fnptr = *static_cast <MFP const*> (lua_touserdata (L, lua_upvalueindex (1)));
      assert (fnptr != 0);
      return (t->*fnptr) (L);
    }
  };

  //--------------------------------------------------------------------------

  // SFINAE Helpers

  template <class MemFnPtr, bool isConst>
  struct CallMemberFunctionHelper
  {
    static void add (lua_State* L, ClassTable& classTable, char const* name, MemFnPtr mf)
    {
      new (lua_newuserdata (L, sizeof (MemFnPtr))) MemFnPtr (mf);
      lua_pushcclosure (L, &CallConstMember <MemFnPtr>::f, 1);
      lua_pushvalue (L, -1);
      classTable.setConstMethod (name);
      classTable.setMethod (name);
    }
  };

  template <class MemFnPtr>
  struct CallMemberFunctionHelper <MemFnPtr, false>
  {
    static void add (lua_State* L, ClassTable& classTable, char const* name, MemFnPtr mf)
    {
      new (lua_newuserdata (L, sizeof (MemFnPtr))) MemFnPtr (mf);
      lua_pushcclosure (L, &CallMember <MemFnPtr>::f, 1);
      classTable.setMethod (name);
    }
  };

  //--------------------------------------------------------------------------
  /**
      __gc metamethod for a class.
  */
  template <class C>
  static int gcMetaMethod (lua_State* L)
  {
    Userdata* const ud = Userdata::getExact <C> (L, 1);
    ud->~Userdata ();
    return 0;
  }

  //--------------------------------------------------------------------------
  /**
      lua_CFunction to get a class data member.

      The pointer-to-member is in the first upvalue.
      The class userdata object is at the top of the Lua stack.
  */
  template <class C, typename T>
  static int getProperty (lua_State* L)
  {
    C* const c = Userdata::get <C> (L, 1, true);
    T C::** mp = static_cast <T C::**> (lua_touserdata (L, lua_upvalueindex (1)));
    try
    {
      Stack <T&>::push (L, c->**mp);
    }
    catch (const std::exception& e)
    {
      luaL_error (L, e.what ());
    }
    return 1;
  }

  //--------------------------------------------------------------------------
  /**
      lua_CFunction to set a class data member.

      The pointer-to-member is in the first upvalue.
      The class userdata object is at the top of the Lua stack.
  */
  template <class C, typename T>
  static int setProperty (lua_State* L)
  {
    C* const c = Userdata::get <C> (L, 1, false);
    T C::** mp = static_cast <T C::**> (lua_touserdata (L, lua_upvalueindex (1)));
    try
    {
      c->**mp = Stack <T>::get (L, 2);
    }
    catch (const std::exception& e)
    {
      luaL_error (L, e.what ());
    }
    return 0;
  }
};

} // namespace luabridge
