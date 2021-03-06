//------------------------------------------------------------------------------
/*
  https://github.com/vinniefalco/LuaBridge
  
  Copyright 2019, Dmitry Tarakanov
  Copyright 2012, Vinnie Falco <vinnie.falco@gmail.com>
  Copyright 2007, Nathan Reed

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

#include <LuaBridge/detail/ClassInfo.h>
#include <LuaBridge/detail/Security.h>
#include <LuaBridge/detail/TypeTraits.h>

#include <stdexcept>
#include <string>

namespace luabridge {

/** Provides C++ to Lua registration capabilities.

    This class is not instantiated directly, call `getGlobalNamespace` to start
    the registration process.
*/
class Namespace
{
  Namespace& operator= (Namespace const& other);

  lua_State* const L;
  Namespace* m_parent;
  int mutable m_stackSize;

  //============================================================================
  /**
    Error reporting.

    VF: This function looks handy, why aren't we using it?
  */
#if 0
  static int luaError (lua_State* L, std::string message)
  {
    assert (lua_isstring (L, lua_upvalueindex (1)));
    std::string s;

    // Get information on the caller's caller to format the message,
    // so the error appears to originate from the Lua source.
    lua_Debug ar;
    int result = lua_getstack (L, 2, &ar);
    if (result != 0)
    {
      lua_getinfo (L, "Sl", &ar);
      s = ar.short_src;
      if (ar.currentline != -1)
      {
        // poor mans int to string to avoid <strstrream>.
        lua_pushnumber (L, ar.currentline);
        s = s + ":" + lua_tostring (L, -1) + ": ";
        lua_pop (L, 1);
      }
    }

    s = s + message;

    return luaL_error (L, s.c_str ());
  }
#endif

  void clearStack ()
  {
    assert (m_stackSize <= lua_gettop (L));
    lua_pop (L, m_stackSize);
    m_stackSize = 0;
  }

  /**
    Factored base to reduce template instantiations.
  */
  class ClassBase
  {
    ClassBase& operator= (ClassBase const& other);

  protected:
    lua_State* const L;
    Namespace& m_parent;
    int mutable m_stackSize;

    //--------------------------------------------------------------------------
    /**
      Create the const table.
    */
    void createConstTable (const char* name, bool trueConst = true)
    {
      std::string type_name = trueConst ? "const " : " " + std::string (name);

      // Stack: namespace table (ns)
      lua_newtable (L); // Stack: ns, const table (co)
      lua_pushvalue (L, -1); // Stack: ns, co, co
      lua_setmetatable (L, -2); // co.__metatable = co. Stack: ns, co

      lua_pushboolean (L, 1);
      lua_rawsetp (L, -2, getIdentityKey ()); // co [identityKey] = true. Stack: ns, co

      lua_pushstring (L, type_name.c_str ());
      lua_rawsetp (L, -2, getTypeKey ()); // co [typeKey] = name. Stack: ns, co

      lua_pushcfunction (L, &CFunc::indexMetaMethod);
      rawsetfield (L, -2, "__index");

      lua_pushcfunction (L, &CFunc::newindexObjectMetaMethod);
      rawsetfield (L, -2, "__newindex");

      lua_newtable (L);
      lua_rawsetp (L, -2, getPropgetKey ());

      if (Security::hideMetatables ())
      {
        lua_pushnil (L);
        rawsetfield (L, -2, "__metatable");
      }
    }

    //--------------------------------------------------------------------------
    /**
      Create the class table.

      The Lua stack should have the const table on top.
    */
    void createClassTable (char const* name)
    {
      // Stack: namespace table (ns), const table (co)

      // Class table is the same as const table except the propset table
      createConstTable (name, false); // Stack: ns, co, cl

      lua_newtable (L); // Stack: ns, co, cl, propset table (ps)
      lua_rawsetp (L, -2, getPropsetKey ()); // cl [propsetKey] = ps. Stack: ns, co, cl

      lua_pushvalue (L, -2); // Stack: ns, co, cl, co
      lua_rawsetp(L, -2, getConstKey ()); // cl [constKey] = co. Stack: ns, co, cl

      lua_pushvalue (L, -1); // Stack: ns, co, cl, cl
      lua_rawsetp (L, -3, getClassKey ()); // co [classKey] = cl. Stack: ns, co, cl
    }

    //--------------------------------------------------------------------------
    /**
      Create the static table.
    */
    void createStaticTable (char const* name)
    {
      // Stack: namespace table (ns), const table (co), class table (cl)
      lua_newtable (L); // Stack: ns, co, cl, visible static table (vst)
      lua_newtable (L); // Stack: ns, co, cl, st, static metatable (st)
      lua_pushvalue (L, -1); // Stack: ns, co, cl, vst, st, st
      lua_setmetatable (L, -3); // st.__metatable = mt. Stack: ns, co, cl, vst, st
      lua_insert (L, -2); // Stack: ns, co, cl, st, vst
      rawsetfield (L, -5, name); // ns [name] = vst. Stack: ns, co, cl, st

#if 0
      lua_pushlightuserdata (L, this);
      lua_pushcclosure (L, &tostringMetaMethod, 1);
      rawsetfield (L, -2, "__tostring");
#endif
      lua_pushcfunction (L, &CFunc::indexMetaMethod);
      rawsetfield (L, -2, "__index");

      lua_pushcfunction (L, &CFunc::newindexStaticMetaMethod);
      rawsetfield (L, -2, "__newindex");

      lua_newtable (L); // Stack: ns, co, cl, st, proget table (pg)
      lua_rawsetp (L, -2, getPropgetKey ()); // st [propgetKey] = pg. Stack: ns, co, cl, st

      lua_newtable (L); // Stack: ns, co, cl, st, propset table (ps)
      lua_rawsetp (L, -2, getPropsetKey ()); // st [propsetKey] = pg. Stack: ns, co, cl, st

      lua_pushvalue (L, -2); // Stack: ns, co, cl, st, cl
      lua_rawsetp(L, -2, getClassKey()); // st [classKey] = cl. Stack: ns, co, cl, st

      if (Security::hideMetatables ())
      {
        lua_pushnil (L);
        rawsetfield (L, -2, "__metatable");
      }
    }

    //==========================================================================
    /**
      lua_CFunction to construct a class object wrapped in a container.
    */
    template <class Params, class C>
    static int ctorContainerProxy (lua_State* L)
    {
      typedef typename ContainerTraits <C>::Type T;
      ArgList <Params, 2> args (L);
      T* const p = Constructor <T, Params>::call (args);
      UserdataSharedHelper <C, false>::push (L, p);
      return 1;
    }

    //--------------------------------------------------------------------------
    /**
      lua_CFunction to construct a class object in-place in the userdata.
    */
    template <class Params, class T>
    static int ctorPlacementProxy (lua_State* L)
    {
      ArgList <Params, 2> args (L);
      Constructor <T, Params>::call (UserdataValue <T>::place (L), args);
      return 1;
    }

    //--------------------------------------------------------------------------
    explicit ClassBase (Namespace& parent)
      : L (parent.L)
      , m_parent (parent)
      , m_stackSize (0)
    {
    }

    //--------------------------------------------------------------------------
    /**
      Copy Constructor.
    */
    ClassBase (ClassBase const& other)
      : L (other.L)
      , m_parent (other.m_parent)
      , m_stackSize (other.m_stackSize)
    {
      other.m_stackSize = 0;
    }

    ~ClassBase ()
    {
      clearStack ();
    }

    void assertStackState () const
    {
      // Stack: const table (co), class table (cl), static table (st)
      assert (lua_istable (L, -3));
      assert (lua_istable (L, -2));
      assert (lua_istable (L, -1));
    }

    void clearStack ()
    {
      assert (m_stackSize <= lua_gettop (L));
      lua_pop (L, m_stackSize);
      m_stackSize = 0;
    }
  };

  //============================================================================
  //
  // Class
  //
  //============================================================================
  /**
    Provides a class registration in a lua_State.

    After construction the Lua stack holds these objects:
      -1 static table
      -2 class table
      -3 const table
      -4 enclosing namespace table
  */
  template <class T>
  class Class : public ClassBase
  {
  public:
    //==========================================================================
    /**
      Register a new class or add to an existing class registration.
    */
    Class (char const* name, Namespace& parent) : ClassBase (parent)
    {
      assert (lua_istable (L, -1)); // Stack: namespace table (ns)
      rawgetfield (L, -1, name); // Stack: ns, static table (st) | nil
      
      if (lua_isnil (L, -1)) // Stack: ns, nil
      {
        lua_pop (L, 1); // Stack: ns

        createConstTable (name); // Stack: ns, const table (co)
        lua_pushcfunction (L, &CFunc::gcMetaMethod <T>); // Stack: ns, co, function
        rawsetfield (L, -2, "__gc"); // Stack: ns, co
        ++m_stackSize;

        createClassTable (name); // Stack: ns, co, class table (cl)
        lua_pushcfunction (L, &CFunc::gcMetaMethod <T>); // Stack: ns, co, cl, function
        rawsetfield (L, -2, "__gc"); // Stack: ns, co, cl
        ++m_stackSize;

        createStaticTable (name); // Stack: ns, co, cl, st
        ++m_stackSize;

        // Map T back to its tables.
        lua_pushvalue (L, -1); // Stack: ns, co, cl, st, st
        lua_rawsetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::getStaticKey ()); // Stack: ns, co, cl, st
        lua_pushvalue (L, -2); // Stack: ns, co, cl, st, cl
        lua_rawsetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::getClassKey ()); // Stack: ns, co, cl, st
        lua_pushvalue (L, -3); // Stack: ns, co, cl, st, co
        lua_rawsetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::getConstKey ()); // Stack: ns, co, cl, st
      }
      else
      {
        assert (lua_istable (L, -1)); // Stack: ns, st

        // Map T back from its stored tables

        lua_rawgetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::getConstKey ()); // Stack: ns, st, co
        lua_insert (L, -2); // Stack: ns, co, st

        lua_rawgetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::getClassKey ()); // Stack: ns, co, st, cl
        lua_insert (L, -2); // Stack: ns, co, cl, st

        m_stackSize = 3;
      }
    }

    //==========================================================================
    /**
      Derive a new class.
    */
    Class (char const* name, Namespace& parent, void const* const staticKey)
      : ClassBase (parent)
    {
      assert (lua_istable (L, -1)); // Stack: namespace table (ns)

      createConstTable (name); // Stack: ns, const table (co)
      lua_pushcfunction (L, &CFunc::gcMetaMethod <T>); // Stack: ns, co, function
      rawsetfield (L, -2, "__gc"); // Stack: ns, co
      ++m_stackSize;

      createClassTable (name); // Stack: ns, co, class table (cl)
      lua_pushcfunction (L, &CFunc::gcMetaMethod <T>); // Stack: ns, co, cl, function
      rawsetfield (L, -2, "__gc"); // Stack: ns, co, cl
      ++m_stackSize;

      createStaticTable (name); // Stack: ns, co, cl, st
      ++m_stackSize;

      lua_rawgetp (L, LUA_REGISTRYINDEX, staticKey); // Stack: ns, co, cl, st, parent st (pst) | nil
      if (lua_isnil (L, -1)) // Stack: ns, co, cl, st, nil
      {
        ++m_stackSize;
        throw std::runtime_error ("Base class is not registered");
      }

      assert (lua_istable (L, -1)); // Stack: ns, co, cl, st, pst

      lua_rawgetp (L, -1, getClassKey ()); // Stack: ns, co, cl, st, pst, parent cl (pcl)
      assert (lua_istable (L, -1));

      lua_rawgetp (L, -1, getConstKey ()); // Stack: ns, co, cl, st, pst, pcl, parent co (pco)
      assert (lua_istable (L, -1));

      lua_rawsetp (L, -6, getParentKey ()); // co [parentKey] = pco. Stack: ns, co, cl, st, pst, pcl
      lua_rawsetp (L, -4, getParentKey ()); // cl [parentKey] = pcl. Stack: ns, co, cl, st, pst
      lua_rawsetp (L, -2, getParentKey ()); // st [parentKey] = pst. Stack: ns, co, cl, st

      lua_pushvalue (L, -1); // Stack: ns, co, cl, st, st
      lua_rawsetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::getStaticKey ()); // Stack: ns, co, cl, st
      lua_pushvalue (L, -2); // Stack: ns, co, cl, st, cl
      lua_rawsetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::getClassKey ()); // Stack: ns, co, cl, st
      lua_pushvalue (L, -3); // Stack: ns, co, cl, st, co
      lua_rawsetp (L, LUA_REGISTRYINDEX, ClassInfo <T>::getConstKey ()); // Stack: ns, co, cl, st
    }

    //--------------------------------------------------------------------------
    /**
      Continue registration in the enclosing namespace.
    */
    Namespace& endClass ()
    {
      clearStack ();
      return m_parent;
    }

    //--------------------------------------------------------------------------
    /**
      Add or replace a static data member.
    */
    template <class U>
    Class <T>& addStaticData (char const* name, U* pu, bool isWritable = true)
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      lua_pushlightuserdata (L, pu); // Stack: co, cl, st, pointer
      lua_pushcclosure (L, &CFunc::getVariable <U>, 1); // Stack: co, cl, st, getter
      CFunc::addGetter (L, name, -2); // Stack: co, cl, st

      if (isWritable)
      {
        lua_pushlightuserdata (L, pu); // Stack: co, cl, st, ps, pointer
        lua_pushcclosure (L, &CFunc::setVariable <U>, 1); // Stack: co, cl, st, ps, setter
      }
      else
      {
        lua_pushstring (L, name); // Stack: co, cl, st, name
        lua_pushcclosure (L, &CFunc::readOnlyError, 1); // Stack: co, cl, st, error_fn
      }
      CFunc::addSetter (L, name, -2); // Stack: co, cl, st

      return *this;
    }

    //--------------------------------------------------------------------------
    /**
      Add or replace a static property member.

      If the set function is null, the property is read-only.
    */
    template <class U>
    Class <T>& addStaticProperty (char const* name, U (*get) (), void (*set) (U) = 0)
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      typedef U (*get_t) ();
      new (lua_newuserdata (L, sizeof (get))) get_t (get); // Stack: co, cl, st, function ptr
      lua_pushcclosure (L, &CFunc::Call <U (*) (void)>::f, 1); // Stack: co, cl, st, getter
      CFunc::addGetter (L, name, -2); // Stack: co, cl, st

      if (set != 0)
      {
        typedef void (*set_t) (U);
        new (lua_newuserdata (L, sizeof (set))) set_t (set); // Stack: co, cl, st, function ptr
        lua_pushcclosure (L, &CFunc::Call <void (*) (U)>::f, 1); // Stack: co, cl, st, setter
      }
      else
      {
        lua_pushstring (L, name); // Stack: co, cl, st, ps, name
        lua_pushcclosure (L, &CFunc::readOnlyError, 1); // Stack: co, cl, st, error_fn
      }
      CFunc::addSetter (L, name, -2); // Stack: co, cl, st

      return *this;
    }

    //--------------------------------------------------------------------------
    /**
      Add or replace a static member function.
    */
    template <class FP>
    Class <T>& addStaticFunction (char const* name, FP const fp)
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      new (lua_newuserdata (L, sizeof (fp))) FP (fp); // co, cl, st, function ptr
      lua_pushcclosure (L, &CFunc::Call <FP>::f, 1); // co, cl, st, function
      rawsetfield (L, -2, name); // co, cl, st

      return *this;
    }

    //--------------------------------------------------------------------------
    /**
      Add or replace a lua_CFunction.
    */
    Class <T>& addStaticCFunction (char const* name, int (*const fp) (lua_State*))
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      lua_pushcfunction (L, fp); // co, cl, st, function
      rawsetfield (L, -2, name); // co, cl, st

      return *this;
    }

    //--------------------------------------------------------------------------
    /**
      Add or replace a data member.
    */
    template <class U>
    Class <T>& addData (char const* name, const U T::* mp, bool isWritable = true)
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      typedef const U T::*mp_t;
      new (lua_newuserdata (L, sizeof (mp_t))) mp_t (mp); // Stack: co, cl, st, field ptr
      lua_pushcclosure (L, &CFunc::getProperty <T, U>, 1); // Stack: co, cl, st, getter
      lua_pushvalue (L, -1); // Stack: co, cl, st, getter, getter
      CFunc::addGetter (L, name, -5); // Stack: co, cl, st, getter
      CFunc::addGetter (L, name, -3); // Stack: co, cl, st

      if (isWritable)
      {
        new (lua_newuserdata (L, sizeof (mp_t))) mp_t (mp); // Stack: co, cl, st, field ptr
        lua_pushcclosure (L, &CFunc::setProperty <T, U>, 1); // Stack: co, cl, st, setter
        CFunc::addSetter (L, name, -3); // Stack: co, cl, st
      }

      return *this;
    }

    //--------------------------------------------------------------------------
    /**
      Add or replace a property member.
    */
    template <class TG, class TS>
    Class <T>& addProperty (char const* name, TG (T::* get) () const, void (T::* set) (TS))
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      addProperty (name, get); // Add getter

      typedef void (T::* set_t) (TS);
      new (lua_newuserdata (L, sizeof (set_t))) set_t (set); // Stack: co, cl, st, function ptr
      lua_pushcclosure (L, &CFunc::CallMember <set_t>::f, 1); // Stack: co, cl, st, setter
      CFunc::addSetter (L, name, -3); // Stack: co, cl, st

      return *this;
    }

    // read-only
    template <class TG>
    Class <T>& addProperty (char const* name, TG (T::* get) () const)
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      typedef TG (T::*get_t) () const;
      new (lua_newuserdata (L, sizeof (get_t))) get_t (get); // Stack: co, cl, st, funcion ptr
      lua_pushcclosure (L, &CFunc::CallConstMember <get_t>::f, 1); // Stack: co, cl, st, getter
      lua_pushvalue (L, -1); // Stack: co, cl, st, getter, getter
      CFunc::addGetter (L, name, -5); // Stack: co, cl, st, getter
      CFunc::addGetter (L, name, -3); // Stack: co, cl, st

      return *this;
    }

    //--------------------------------------------------------------------------
    /**
      Add or replace a property member, by proxy.

      When a class is closed for modification and does not provide (or cannot
      provide) the function signatures necessary to implement get or set for
      a property, this will allow non-member functions act as proxies.

      Both the get and the set functions require a T const* and T* in the first
      argument respectively.
    */
    template <class TG, class TS = TG>
    Class <T>& addProperty (char const* name, TG (*get) (T const*), void (*set) (T*, TS) = 0)
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      typedef TG (*get_t) (T const*);
      new (lua_newuserdata (L, sizeof (get_t))) get_t (get); // Stack: co, cl, st,, fn ptr
      lua_pushcclosure (L, &CFunc::Call <get_t>::f, 1); // Stack: co, cl, st,, getter
      lua_pushvalue (L, -1); // Stack: co, cl, st,, getter, getter
      CFunc::addGetter (L, name, -5); // Stack: co, cl, st,, getter
      CFunc::addGetter (L, name, -3); // Stack: co, cl, st,

      if (set != 0)
      {
        typedef void (*set_t) (T*, TS);
        new (lua_newuserdata (L, sizeof (set_t))) set_t (set); // Stack: co, cl, st,, fn ptr
        lua_pushcclosure (L, &CFunc::Call <set_t>::f, 1); // Stack: co, cl, st,, setter
        CFunc::addSetter (L, name, -3); // Stack: co, cl, st,
      }

      return *this;
    }

    //--------------------------------------------------------------------------
    /**
        Add or replace a member function.
    */
    template <class MemFn>
    Class <T>& addFunction (char const* name, MemFn mf)
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      static const std::string GC = "__gc";
      if (name == GC)
      {
        throw std::logic_error (GC + " metamethod registration is forbidden");
      }
      CFunc::CallMemberFunctionHelper <MemFn, FuncTraits <MemFn>::isConstMemberFunction>::add (L, name, mf);
      return *this;
    }

    //--------------------------------------------------------------------------
    /**
        Add or replace a member lua_CFunction.
    */
    Class <T>& addCFunction (char const* name, int (T::*mfp) (lua_State*))
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      typedef int (T::*MFP) (lua_State*);
      new (lua_newuserdata (L, sizeof (mfp))) MFP (mfp); // Stack: co, cl, st, function ptr
      lua_pushcclosure (L, &CFunc::CallMemberCFunction <T>::f, 1); // Stack: co, cl, st, function
      rawsetfield (L, -3, name); // Stack: co, cl, st

      return *this;
    }

    //--------------------------------------------------------------------------
    /**
        Add or replace a const member lua_CFunction.
    */
    Class <T>& addCFunction (char const* name, int (T::*mfp) (lua_State*) const)
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      typedef int (T::*MFP) (lua_State*) const;
      new (lua_newuserdata (L, sizeof (mfp))) MFP (mfp);
      lua_pushcclosure (L, &CFunc::CallConstMemberCFunction <T>::f, 1);
      lua_pushvalue (L, -1); // Stack: co, cl, st, function, function
      rawsetfield (L, -4, name); // Stack: co, cl, st, function
      rawsetfield (L, -4, name); // Stack: co, cl, st

      return *this;
    }

    //--------------------------------------------------------------------------
    /**
      Add or replace a primary Constructor.

      The primary Constructor is invoked when calling the class type table
      like a function.

      The template parameter should be a function pointer type that matches
      the desired Constructor (since you can't take the address of a Constructor
      and pass it as an argument).
    */
    template <class MemFn, class C>
    Class <T>& addConstructor ()
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      lua_pushcclosure (L, &ctorContainerProxy <typename FuncTraits <MemFn>::Params, C>, 0);
      rawsetfield (L, -2, "__call");

      return *this;
    }

    template <class MemFn>
    Class <T>& addConstructor ()
    {
      assertStackState (); // Stack: const table (co), class table (cl), static table (st)

      lua_pushcclosure (L, &ctorPlacementProxy <typename FuncTraits <MemFn>::Params, T>, 0);
      rawsetfield (L, -2, "__call");

      return *this;
    }
  };

private:
  //----------------------------------------------------------------------------
  /**
      Open the global namespace for registrations.
  */
  explicit Namespace (lua_State* L_)
    : L (L_)
    , m_parent (NULL)
    , m_stackSize (1)
  {
    lua_getglobal (L, "_G");
  }

  //----------------------------------------------------------------------------
  /**
      Open a namespace for registrations.

      The namespace is created if it doesn't already exist.
      The parent namespace is at the top of the Lua stack.
  */
  Namespace (char const* name, Namespace& parent)
    : L (parent.L)
    , m_parent (&parent)
    , m_stackSize (0)
  {
    assert (lua_istable (L, -1)); // Stack: parent namespace (pns)

    rawgetfield (L, -1, name); // Stack: pns, namespace (ns) | nil

    if (lua_isnil (L, -1)) // Stack: pns, nil
    {
      lua_pop (L, 1); // Stack: pns

      lua_newtable (L); // Stack: pns, ns
      lua_pushvalue (L, -1); // Stack: pns, ns, ns

      // na.__metatable = ns
      lua_setmetatable (L, -2); // Stack: pns, ns

      // ns.__index = indexMetaMethod
      lua_pushcfunction (L, &CFunc::indexMetaMethod);
      rawsetfield (L, -2, "__index"); // Stack: pns, ns

      // ns.__newindex = newindexMetaMethod
      lua_pushcfunction (L, &CFunc::newindexStaticMetaMethod);
      rawsetfield (L, -2, "__newindex"); // Stack: pns, ns

      lua_newtable (L); // Stack: pns, ns, propget table (pg)
      lua_rawsetp (L, -2, getPropgetKey ()); // ns [propgetKey] = pg. Stack: pns, ns

      lua_newtable (L); // Stack: pns, ns, propset table (ps)
      lua_rawsetp (L, -2, getPropsetKey ()); // ns [propsetKey] = ps. Stack: pns, ns

      // pns [name] = ns
      lua_pushvalue (L, -1); // Stack: pns, ns, ns
      rawsetfield (L, -3, name); // Stack: pns, ns
#if 0
      lua_pushcfunction (L, &tostringMetaMethod);
      rawsetfield (L, -2, "__tostring");
#endif
    }

    ++m_stackSize;
  }

public:
  //----------------------------------------------------------------------------
  /**
      Copy Constructor.

      Ownership of the stack is transferred to the new object. This happens
      when the compiler emits temporaries to hold these objects while chaining
      registrations across namespaces.
  */
  Namespace (Namespace const& other)
    : L (other.L)
    , m_parent (other.m_parent)
    , m_stackSize (other.m_stackSize)
  {
    other.m_stackSize = 0;
  }

  //----------------------------------------------------------------------------
  /**
      Closes this namespace registration.
  */
  ~Namespace ()
  {
    clearStack ();
  }

  //----------------------------------------------------------------------------
  /**
      Open the global namespace.
  */
  static Namespace getGlobalNamespace (lua_State* L)
  {
    return Namespace (L);
  }

  //----------------------------------------------------------------------------
  /**
      Open a new or existing namespace for registrations.
  */
  Namespace beginNamespace (char const* name)
  {
    return Namespace (name, *this);
  }

  //----------------------------------------------------------------------------
  /**
      Continue namespace registration in the parent.

      Do not use this on the global namespace.
  */
  Namespace& endNamespace ()
  {
    if (!m_parent)
    {
      throw std::logic_error ("endNamespace () called on global namespace");
    }

    clearStack ();
    return *m_parent;
  }

  //----------------------------------------------------------------------------
  /**
      Add or replace a variable.
  */
  template <class T>
  Namespace& addVariable (char const* name, T* pt, bool isWritable = true)
  {
    if (!m_parent)
    {
      throw std::logic_error ("addVariable () called on global namespace");
    }

    assert (lua_istable (L, -1)); // Stack: namespace table (ns)

    lua_pushlightuserdata (L, pt); // Stack: ns, pointer
    lua_pushcclosure (L, &CFunc::getVariable <T>, 1); // Stack: ns, getter
    CFunc::addGetter (L, name, -2); // Stack: ns

    if (isWritable)
    {
      lua_pushlightuserdata (L, pt); // Stack: ns, pointer
      lua_pushcclosure (L, &CFunc::setVariable <T>, 1); // Stack: ns, setter
    }
    else
    {
      lua_pushstring (L, name); // Stack: ns, ps, name
      lua_pushcclosure (L, &CFunc::readOnlyError, 1); // Stack: ns, error_fn
    }
    CFunc::addSetter (L, name, -2); // Stack: ns

    return *this;
  }
  
  //----------------------------------------------------------------------------
  /**
      Add or replace a property.

      If the set function is omitted or null, the property is read-only.
  */
  template <class TG, class TS = TG>
  Namespace& addProperty (char const* name, TG (*get) (), void (*set) (TS) = 0)
  {
    if (!m_parent)
    {
      throw std::logic_error ("addProperty () called on global namespace");
    }

    assert (lua_istable (L, -1)); // Stack: namespace table (ns)

    typedef TG (*get_t) ();
    new (lua_newuserdata (L, sizeof (get_t))) get_t (get); // Stack: ns, field ptr
    lua_pushcclosure (L, &CFunc::Call <TG (*) (void)>::f, 1); // Stack: ns, getter
    CFunc::addGetter (L, name, -2);

    if (set != 0)
    {
      typedef void (*set_t) (TS);
      new (lua_newuserdata (L, sizeof (set_t))) set_t (set);
      lua_pushcclosure (L, &CFunc::Call <void (*) (TS)>::f, 1);
    }
    else
    {
      lua_pushstring (L, name);
      lua_pushcclosure (L, &CFunc::readOnlyError, 1);
    }
    CFunc::addSetter (L, name, -2);

    return *this;
  }

  //----------------------------------------------------------------------------
  /**
      Add or replace a free function.
  */
  template <class FP>
  Namespace& addFunction (char const* name, FP const fp)
  {
    assert (lua_istable (L, -1)); // Stack: namespace table (ns)

    new (lua_newuserdata (L, sizeof (fp))) FP (fp);
    lua_pushcclosure (L, &CFunc::Call <FP>::f, 1); // Stack: ns, function
    rawsetfield (L, -2, name); // Stack: ns

    return *this;
  }

  //----------------------------------------------------------------------------
  /**
      Add or replace a lua_CFunction.
  */
  Namespace& addCFunction (char const* name, int (*const fp) (lua_State*))
  {
    assert (lua_istable (L, -1)); // Stack: namespace table (ns)

    lua_pushcfunction (L, fp); // Stack: ns, function
    rawsetfield (L, -2, name); // Stack: ns

    return *this;
  }

  //----------------------------------------------------------------------------
  /**
      Open a new or existing class for registrations.
  */
  template <class T>
  Class <T> beginClass (char const* name)
  {
    return Class <T> (name, *this);
  }

  //----------------------------------------------------------------------------
  /**
      Derive a new class for registrations.

      To continue registrations for the class later, use beginClass ().
      Do not call deriveClass () again.
  */
  template <class Derived, class Base>
  Class <Derived> deriveClass (char const* name)
  {
    return Class <Derived> (name, *this, ClassInfo <Base>::getStaticKey ());
  }
};

//------------------------------------------------------------------------------
/**
    Retrieve the global namespace.

    It is recommended to put your namespace inside the global namespace, and
    then add your classes and functions to it, rather than adding many classes
    and functions directly to the global namespace.
*/
inline Namespace getGlobalNamespace (lua_State* L)
{
  return Namespace::getGlobalNamespace (L);
}

} // namespace luabridge
