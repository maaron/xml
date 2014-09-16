// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// The extensive use of templates causes this "decorated name length too 
// long" warning all over the place.  Since we aren't exporting any of these 
// template instanciations in a library, it is safe to ignore.
#pragma warning( disable : 4503 )

#include <string>
#include <sstream>
#include <iostream>
#include <algorithm>

#include "stream_container.h"
#include "parse.h"
#include "xml.h"

// This demonstrates using a custom AST type to have more type-safe access to 
// elements.  Instead of using the operator[] overloads directly, these AST 
// classes add named accessor methods to retrieve the various parts of the 
// underlying AST.  Note that the get accessor's themselves are using the
// "unsafe" operator[]'s, but once they are written, any code that uses those
// AST types can use the custom methods, which won't compile if the grammar is
// modified.  The operator[]'s on the other hand, will generally just silently
// start grabbing the wrong part of the AST.
// A downside to this method is that we seem to get some strange errors in 
// xlocnum.h, associated with bringing the parse::operators namespace in to 
// scope alongside the accessors.

namespace custom_ast_test
{
    using namespace parse::terminals;
    using namespace parse::operators;

    static u<'1'> uone;
    static u<'2'> utwo;
    static u<'3'> uthree;

    typedef decltype(uone >> utwo >> uthree) base_t;

    struct parser_t : public base_t
    {
        template <typename iterator_t>
        struct ast : public parse::ast_type<base_t, iterator_t>::type
        {
            typedef ast type;
            
            typename parse::ast_type<decltype(uone), iterator_t>::type& get_one()
            {
                return (*this)[util::_0];
            }

            typename parse::ast_type<decltype(utwo), iterator_t>::type& get_two()
            {
                return (*this)[util::_1];
            }

            typename parse::ast_type<decltype(uthree), iterator_t>::type& get_three()
            {
                return (*this)[util::_2];
            }
        };
    };

    typedef decltype(utwo >> uthree >> parser_t()) base2_t;

    struct parser2_t : public base2_t
    {
        template <typename iterator_t>
        struct ast : public base2_t::ast<iterator_t>::type
        {
            typedef ast type;
            
            typename parse::ast_type<parser_t, iterator_t>::type& get_one()
            {
                return (*this)[util::_2];
            }

            typename parse::ast_type<decltype(utwo), iterator_t>::type& get_two()
            {
                return (*this)[util::_0];
            }

            typename parse::ast_type<decltype(uthree), iterator_t>::type& get_three()
            {
                return (*this)[util::_1];
            }
        };
    };

    void foo()
    {
        parser2_t p;
        std::string data("23123");
        parse::ast_type<parser2_t, std::string::iterator>::type ast;
        p.parse(data, ast);

        auto& c1 = ast.get_one();
        auto& c2 = ast.get_two();
        auto& c3 = ast.get_three();

        auto& c1c1 = c1.get_one();
        auto& c1c2 = c1.get_two();
        auto& c1c3 = c1.get_three();
    }
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
    //std::string xml_data("\xEF\xBB\xBF<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\">root content part 1<ns:child1>child1 content<grandchild11></grandchild11></ns:child1><child2>child2 content</child2></nspre:root>");
    
    // UTF-16 encoding
    //std::wstring wxml_data(L"\uFFFE<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\">root content part 1<ns:child1>child1 content<grandchild11></grandchild11></ns:child1><child2>child2 content</child2></nspre:root>");
    //std::string xml_data((const char*)wxml_data.c_str(), wxml_data.size() * 2);

    // UTF-16 native string (with and without BOM)
    //std::wstring xml_data(L"<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\">root content part 1<ns:child1>child1 content<grandchild11/></ns:child1><child2>child2 content</child2></nspre:root>");
    //std::wstring xml_data(L"\uFFFE<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\">root content part 1<ns:child1>child1 content<grandchild11></grandchild11></ns:child1><child2>child2 content</child2></nspre:root>");

    // Stream parsing
    std::stringstream stream_data("<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\">root content part 1<ns:child1>child1 content<grandchild11/></ns:child1><child2>child2 content</child2></nspre:root>");
    util::streambuf_container<std::streambuf> xml_data(stream_data.rdbuf());

    // Some typedefs for convenience
    typedef xml::document<decltype(xml_data)> document;
    typedef document::element_type element;
    typedef element::attribute_type attribute;

    document doc(xml_data);

    auto root = doc.root();

    auto name = root.name();
    auto localname = root.local_name();
    auto prefix = root.prefix();

    auto& attributes = root.attributes;
    
    std::for_each(attributes.begin(), attributes.end(), [](attribute& a)
    {
        std::cout << "root attribute: " << a.name() << "=" << a.value() << std::endl;
    });

    auto attribute1 = root.attributes["attribute1"];

    std::for_each(root.elements.begin(), root.elements.end(), [](element& e)
    {
        std::cout << "child element: " << e.name() << std::endl;
    });

    auto text = root.text();

    auto child = *std::find_if(root.elements.begin(), root.elements.end(), [](element& e)
    {
        return e.local_name() == "child1";
    });

    auto gchild = *std::find_if(child.elements.begin(), child.elements.end(), [](element& e)
    {
        return e.name() == "grandchild11";
    });

    auto gchild_text = gchild.text();
    auto gchild_name = gchild.name();

    auto hasAttribs = gchild.attributes.begin() != gchild.attributes.end();

    return 0;
}
