// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <string>
#include <iostream>

#include "parse.h"

template <typename parser_t, typename stream_t>
typename parser_t::ast<typename stream_t::iterator>::type
parse_expression(parser_t& parser, stream_t& data)
{
  auto ast = parse::tree::make_ast(parser, data);
  if (!parser.parse(data, ast)) throw std::exception("parse error");
  return ast;
}

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

  using namespace parse;

  auto ast2 = parse_expression(_1 >> _2 >> _3 >> _4, data);

  std::cout << "c0=" << ast2[_i0].to_string() << std::endl;
  std::cout << "c1=" << ast2[_i1].to_string() << std::endl;
  std::cout << "c2=" << ast2[_i2].to_string() << std::endl;
  std::cout << "c3=" << ast2[_i3].to_string() << std::endl;

  auto a = character<char>('a');
  auto b = character<char>('b');
  auto c = character<char>('c');

  std::string data2("d");
  auto abc = a | b | c;
  auto abc_ast = tree::make_ast(abc, data2);
  valid = abc.parse(data2, abc_ast);

  struct dec : std::identity<decltype(_1 >> _2 >> _3)>::type
  {
  };

  dec_ast = tree::make_ast(dec(), data2);
  valid = abc.parse(data2, abc_ast);

  return 0;
}

