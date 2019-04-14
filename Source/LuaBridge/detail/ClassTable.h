// https://github.com/vinniefalco/LuaBridge
//
// Copyright 2019, Dmitry Tarakanov
// SPDX-License-Identifier: MIT

#pragma once

#include <map>
#include <Lua/Lua.5.1.5/src/lauxlib.h>


namespace luabridge {

class ClassTable
{
public:
  static ClassTable* fromStack (lua_State* L, int index)
  {
    assert (lua_isuserdata (L, index));
    return static_cast <ClassTable*> (lua_touserdata(L, -1));
  }

  ClassTable (lua_State* L, const char* type, const ClassTable* baseClass)
    : m_L (L)
    , m_type (type)
    , m_baseClass (baseClass)
    , m_metatableRef  (LUA_NOREF)
  {
  }

  ~ClassTable ()
  {
    luaL_unref (m_L, LUA_REGISTRYINDEX, m_metatableRef);
    clear (m_constMethods);
    clear (m_methods);
    clear (m_getters);
    clear (m_setters);
  }

  const std::string& getType () const
  {
    return m_type;
  }

  const ClassTable* getBaseClass () const
  {
    return m_baseClass;
  }

  void setMetatable ()
  {
    assert (lua_istable (m_L, -1));
    m_metatableRef = luaL_ref (m_L, LUA_REGISTRYINDEX);
  }

  void getMetatable() const
  {
    lua_rawgeti (m_L, LUA_REGISTRYINDEX, m_metatableRef);
  }

  void setConstMethod (const char* name)
  {
    setFunction (m_constMethods, name);
  }

  void setMethod (const char* name)
  {
    setFunction (m_methods, name);
  }

  void setGetter (const char* name)
  {
    setFunction (m_getters, name);
  }

  void setSetter (const char* name)
  {
    setFunction (m_setters, name);
  }

  void getConstMethod (const char* name) const
  {
    if (!findFunction (m_constMethods, name))
    {
      if (m_baseClass)
      {
        m_baseClass->getConstMethod (name);
      }
      else
      {
        lua_pushnil (m_L);
      }
    }
  }

  void getMethod (const char* name) const
  {
    if (!findFunction (m_methods, name))
    {
      if (m_baseClass)
      {
        m_baseClass->getMethod (name);
      }
      else
      {
        lua_pushnil (m_L);
      }
    }
  }

  void getGetter (const char* name) const
  {
    if (!findFunction (m_getters, name))
    {
      if (m_baseClass)
      {
        m_baseClass->getGetter (name);
      }
      else
      {
        lua_pushnil (m_L);
      }
    }
  }

  void getSetter (const char* name) const
  {
    if (!findFunction (m_setters, name))
    {
      if (m_baseClass)
      {
        m_baseClass->getSetter (name);
      }
      else
      {
        lua_pushnil (m_L);
      }
    }
  }

  void getField (const char* name, bool isConstObj) const
  {
    if (!findFunction (m_constMethods, name) &&
      (!isConstObj && !findFunction (m_methods, name)) &&
      !findFunction (m_getters, name))
    {
      if (m_baseClass)
      {
        m_baseClass->getField (name, isConstObj);
      }
      else
      {
        lua_pushnil (m_L);
      }
    }
  }

private:
  typedef std::map <std::string, int> FnList;

  bool findFunction (const FnList& fnList, const char* name) const
  {
    FnList::const_iterator i = fnList.find (name);
    if (i != fnList.end ())
    {
      lua_rawgeti (m_L, LUA_REGISTRYINDEX, i->second);
      return true;
    }
    return false;
  }

  void setFunction (FnList& fnList, const char* name)
  {
    assert (lua_iscfunction (m_L, -1));

    FnList::iterator i = fnList.find (name);
    if (i == fnList.end ())
    {
      fnList [name] = luaL_ref (m_L, LUA_REGISTRYINDEX);
    }
    else
    {
      luaL_unref (m_L, LUA_REGISTRYINDEX, i->second);
      i->second = luaL_ref (m_L, LUA_REGISTRYINDEX);
    }
  }

  void clear (FnList& fnList)
  {
    for (FnList::iterator i = fnList.begin (); i != fnList.end (); ++i)
    {
      luaL_unref (m_L, LUA_REGISTRYINDEX, i->second);
    }
    fnList.clear ();
  }

  lua_State* m_L;
  std::string m_type;
  const ClassTable* m_baseClass;
  int m_metatableRef;
  FnList m_constMethods;
  FnList m_methods;
  FnList m_getters;
  FnList m_setters;
  FnList m_constFunctions;
  FnList m_functions;
};

} // namespace luabridge
