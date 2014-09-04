// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>

#include "parse.h"

int _tmain(int argc, _TCHAR* argv[])
{  
  using namespace parse::operators;

  auto _1 = parse::constant<char32_t, '1'>();
  auto _2 = parse::constant<char32_t, '2'>();
  auto _3 = parse::constant<char32_t, '3'>();
  auto _4 = parse::constant<char32_t, '4'>();

  auto parser = _1 >> _2 >> _3 >> _4;

  std::wstring data(L"1234");

  auto ast = parse::tree::make_ast(parser, data);
  bool valid = parser.parse(data, ast);

  auto& c0 = ast[parse::_i0];
  auto& c1 = ast[parse::_i1];
  auto& c2 = ast[parse::_i2];
  auto& c3 = ast[parse::_i3];
  //auto& c4 = ast[parse::_i4];
  //auto& c5 = ast.get(parse::_i5);
  //auto& c6 = ast.get(parse::_i6);
  //auto& c7 = ast.get(parse::_i7);

  auto size = ast.size;

  return 0;
}

