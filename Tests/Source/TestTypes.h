#pragma once

#include <gtest/gtest.h>

#include <string>


using TestTypes = ::testing::Types <
  bool,
  char,
  unsigned char,
  short,
  unsigned short,
  int,
  unsigned int,
  long,
  unsigned long,
  long long,
  unsigned long long,
  float,
  double
>;

template <class T>
struct TypeTraits;

template <>
struct TypeTraits <bool>
{
  static constexpr bool VALUES [] = {true, false, true};
  static std::string list () {return "true, false, true";}
};

template <>
struct TypeTraits <char>
{
  static constexpr char VALUES [] = {'a', 'b', 'c'};
  static std::string list () {return "'a', 'b', 'c'";}
};

template <>
struct TypeTraits <unsigned char>
{
  static constexpr unsigned char VALUES [] = {1, 2, 3};
  static std::string list () {return "1, 2, 3";}
};

template <>
struct TypeTraits <short>
{
  static constexpr short VALUES [] = {1, -2, 3};
  static std::string list () {return "1, -2, 3";}
};

template <>
struct TypeTraits <unsigned short>
{
  static constexpr unsigned short VALUES [] = {1, 2, 3};
  static std::string list () {return "1, 2, 3";}
};

template <>
struct TypeTraits <int>
{
  static constexpr int VALUES [] = {1, -2, 3};
  static std::string list () {return "1, -2, 3";}
};

template <>
struct TypeTraits <unsigned int>
{
  static constexpr unsigned int VALUES [] = {1, 2, 3};
  static std::string list () {return "1, 2, 3";}
};

template <>
struct TypeTraits <long>
{
  static constexpr long VALUES [] = {1, -2, 3};
  static std::string list () {return "1, -2, 3";}
};

template <>
struct TypeTraits <unsigned long>
{
  static constexpr unsigned long VALUES [] = {1, 2, 3};
  static std::string list () {return "1, 2, 3";}
};

template <>
struct TypeTraits <long long>
{
  static constexpr long long VALUES [] = {1, -2, 3};
  static std::string list () {return "1, -2, 3";}
};

template <>
struct TypeTraits <unsigned long long>
{
  static constexpr unsigned long long VALUES [] = {1, 2, 3};
  static std::string list () {return "1, 2, 3";}
};

template <>
struct TypeTraits <float>
{
  static constexpr float VALUES [] = {1.2f, -2.5f, 3.14f};
  static std::string list () {return "1.2, -2.5, 3.14";}
};

template <>
struct TypeTraits <double>
{
  static constexpr double VALUES [] = {1.2, -2.5, 3.14};
  static std::string list () {return "1.2, -2.5, 3.14";}
};
