// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "parse.h"
#include "xml.h"
#include <string>
#include <iostream>

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

  auto& c0 = ast[util::_i0];
  auto& c1 = ast[util::_i1];
  auto& c2 = ast[util::_i2];
  auto& c3 = ast[util::_i3];
  //auto& c4 = ast[parse::_i4];

  std::cout << "c0=" << c0.to_string() << std::endl;
  std::cout << "c1=" << c1.to_string() << std::endl;
  std::cout << "c2=" << c2.to_string() << std::endl;
  std::cout << "c3=" << c3.to_string() << std::endl;

  auto size = ast.size;

  std::string xml_data("<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\"><ns:child1>child1 content<grandchild11></grandchild11></ns:child1><child2>child2 content</child2></nspre:root>");
  xml::document doc(xml_data);

  auto root = doc.root();

  auto name = root.name();
  auto localname = root.local_name();

  return 0;
}

