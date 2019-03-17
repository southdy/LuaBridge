// https://github.com/vinniefalco/LuaBridge
//
// Copyright 2019, Dmitry Tarakanov
// SPDX-License-Identifier: MIT

#include "TestBase.h"

#include "LuaBridge/RefCountedPtr.h"


struct RefCountedPtrTests : TestBase
{
  template <class T>
  T variable(const std::string& name)
  {
    runLua("result = " + name);
    return result().cast <T>();
  }
};

namespace {

struct RefCounted : luabridge::RefCountedObject
{
  explicit RefCounted(bool& deleted)
    : deleted (deleted)
  {
    deleted = false;
  }

  ~RefCounted()
  {
    deleted = true;
  }

  bool isDeleted () const
  {
    return deleted;
  }

  bool& deleted;
};

} // namespace

TEST_F (RefCountedPtrTests, Lifetime)
{
  luabridge::getGlobalNamespace (L)
    .beginClass <RefCounted> ("Class")
    .addProperty ("deleted", &RefCounted::isDeleted)
    .endClass ();

  bool deleted = false;

  luabridge::RefCountedObjectPtr <RefCounted> object (new RefCounted (deleted));

  luabridge::setGlobal (L, object, "object");
  runLua("result = object.deleted");
  ASSERT_EQ (true, result ().isBool ());
  ASSERT_EQ (false, result ().cast <bool> ());

  object = nullptr;
  runLua("result = object.deleted");
  ASSERT_EQ(true, result ().isBool ());
  ASSERT_EQ(false, result ().cast <bool>());
  ASSERT_EQ(false, deleted);

  runLua ("result = nil");
  ASSERT_EQ (true, result ().isNil ());
  ASSERT_EQ (true, deleted);
}
