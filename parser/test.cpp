// test.cpp : Defines the entry point for the console application.
//

// The extensive use of templates causes this "decorated name length too 
// long" warning all over the place.  Since we aren't exporting any of these 
// template instanciations in a library, it is safe to ignore.
#pragma warning( disable : 4503 )

#include "stdafx.h"

#include <boost\foreach.hpp>

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "parse.h"
#include "xml.h"

namespace test
{
    using namespace parse::operators;
    using namespace parse::terminals;

    struct grammar
    {
        static u<'1'> one;
        static u<'2'> two;

        typedef decltype(one >> two) onetwo_t;

        void parse()
        {
            onetwo_t onetwo;
            std::string data("12");
            parse::ast_type<decltype(onetwo), std::string::iterator>::type ast;
            onetwo.parse(data, ast);

            parse::ast_type<decltype(one), std::string::iterator>::type ast1;
            one.parse(data, ast1);
        }
    };
}

int _tmain(int argc, _TCHAR* argv[])
{
    /* Unicode tests
  //std::string v = "\xEF\xBB\xBFthis is a UTF8-encoded string with a BOM.";
  //std::string v = "this is a UTF8-encoded string without a BOM.";
  std::wstring wv(L"\uFEFFthis is a UTF16-encoded string with a BOM.");
  std::string v((const char*)wv.c_str(), wv.size() * 2);

  unicode::unicode_container<std::string> ustring(v);

  auto i = ustring.begin();
  auto end = ustring.end();

  for (; i != end; i++)
  {
      std::cout << "char: " << (char)*i << std::endl;
  }
  */

    /* XML parser test
    using namespace parse;
    using namespace parse::operators;

    std::string test_data("<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\">");
    auto p = xml::parser::prolog() >> xml::parser::element_open();
    parse::ast_type<decltype(p), std::string::iterator>::type p_ast;
    bool valid = p.parse(test_data, p_ast);
    */

    // XML reader test
    
    // UTF-8 encoding
    //std::string xml_data("\xEF\xBB\xBF<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\"><ns:child1>child1 content<grandchild11></grandchild11></ns:child1><child2>child2 content</child2></nspre:root>");
    
    // UTF-16 encoding
    std::wstring wxml_data(L"\uFEFF<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\"><ns:child1>child1 content<grandchild11></grandchild11></ns:child1><child2>child2 content</child2></nspre:root>");
    std::string xml_data((const char*)wxml_data.c_str(), wxml_data.size() * 2);

    xml::document<std::string> doc(xml_data);

    auto root = doc.root();

    auto name = root.name();
    auto localname = root.local_name();
    auto prefix = root.prefix();

    auto& attributes = root.attributes;
    // TODO: The long template instantiation to get the concrete 
    // xml::attribute type is very cumbersome... look for a better way to do 
    // this.
    //std::for_each(attributes.begin(), attributes.end(), [](xml::attribute<unicode::unicode_iterator<std::string::iterator> >& a)

    // This is a little easier, and could be wrapped up in a convenient macro.  Could also probably use Boost.Foreach.
    //std::for_each(attributes.begin(), attributes.end(), [](decltype(*attributes.begin())& a)

#define foreach(var, container) std::for_each(container.begin(), container.end(), [](decltype(*container.begin())& var)

    foreach(a, root.attributes)
    {
        std::cout << "root attribute: " << a.name() << "=" << a.value() << std::endl;
    });

    auto attribute1 = root.attributes["attribute1"];

    return 0;
}

