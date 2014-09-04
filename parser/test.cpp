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

  std::string data("1234");

  auto ast = parse::tree::make_ast(parser, data);
  bool valid = parser.parse(data, ast);

  auto& c0 = ast[parse::_i0];
  auto& c1 = ast[parse::_i1];
  auto& c2 = ast[parse::_i2];
  auto& c3 = ast[parse::_i3];
  //auto& c4 = ast[parse::_i4];

  std::cout << "c0=" << c0.to_string() << std::endl;
  std::cout << "c1=" << c1.to_string() << std::endl;
  std::cout << "c2=" << c2.to_string() << std::endl;
  std::cout << "c3=" << c3.to_string() << std::endl;

  auto size = ast.size;

  return 0;
}

