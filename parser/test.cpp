// test.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

// The extensive use of templates causes this "decorated name length too 
// long" warning all over the place.  Since we aren't exporting any of these 
// template instanciations in a library, it is safe to ignore.
#pragma warning( disable : 4503 )

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>

#include "stream_container.h"
#include "parse\parse.h"
#include "tree.h"
#include "reader.h"

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
#if 0
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
#endif

#if 1
namespace ast_tag_test
{
    using namespace parse;
    using namespace parse::operators;
    using namespace parse::terminals;

    auto one = u<'1'>();
    auto num = +digit();
    auto lparen = u<'('>();
    auto rparen = u<')'>();
    auto ws = +space();

    struct s_expr;

    typedef decltype(num[_0] | reference<s_expr>()[_1]) elem_t;

    typedef decltype(lparen >> *space() >> elem_t()[_0] >> (*(+space() >> elem_t()))[_1] >> *space() >> rparen) s_expr_t;

    struct s_expr : s_expr_t {};

    typedef decltype(num[_0] >> ws >> s_expr()[_1]) parser;

    void test()
    {
        typedef std::string::iterator it_t;
        std::string data("((1) 2 3 (2 33 345) ((((1234)))))");
        typedef parse::parser_ast<parser, std::string::iterator>::type ast_t;
        ast_t ast;
        typedef parser p_type;

        bool valid = parser::parse_from(data.begin(), data.end(), ast);
    }
}
#endif

template <typename container_t>
void read_dump(container_t& c)
{
    xml::reader::document<container_t> doc(c);
    auto root = doc.root();
    dump_element(root, 0);
}

template <typename iterator_t>
void dump_element(xml::reader::element<iterator_t>& e, int indent)
{
    //std::cout << indent << "element: " << e.name() << std::endl;
    //std::cout << indent << "attributes: ";

    xml::reader::attribute<iterator_t> a = e.next_attribute();
    if (a.is_end())
    {
        //std::cout << "(none)" << std::endl;
    }
    else
    {
        //std::cout << std::endl;

        for (; !a.is_end(); a = a.next_attribute())
        {
            //std::cout << indent << "  " << a.name() << "=" << a.value() << std::endl;
        }
    }

    auto nextIndent = indent + 2;
    //std::cout << indent << "childnodes: ";
    xml::reader::node<iterator_t> child = a.next_child();
    if (child.is_end())
    {
        //std::cout << "(none)" << std::endl;
    }
    else
    {
        //std::cout << std::endl;
        for (; !child.is_end(); child = child.next_sibling())
        {
            if (child.is_text())
            {
                //std::cout << nextIndex << "textnode: " << child.text() << std::endl;
            }
            else
                dump_element(child.element(), nextIndent);
        }
    }
}

#include <Windows.h>
long long time()
{
    /* Windows */
    FILETIME ft;
    LARGE_INTEGER li;

    /* Get the amount of 100 nano seconds intervals elapsed since January 1, 1601 (UTC) and copy it
    * to a LARGE_INTEGER structure. */
    GetSystemTimeAsFileTime(&ft);
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    return li.QuadPart;
}

template <typename t> struct map_f { typedef t* type; };

int _tmain(int argc, _TCHAR* argv[])
{
    ast_tag_test::test();

#if 0
    /* Pruned AST test */
    {
        using namespace placeholders;
        using namespace parse;
        using namespace parse::operators;
        using namespace parse::terminals;

        auto a = u<'a'>();
        auto b = u<'b'>();

        auto p = (a | b) >> a[_0] >> b[_1] >> (a | b)[_3];

        typedef decltype(p) p_type;

        typedef parse::parser_ast<p_type, std::string::iterator>::type ast_type;
        ast_type ast;

        std::string data("baba");

        bool valid = p.parse_from(data.begin(), data.end(), ast);

        auto& m0 = ast[_0];
        auto& m1 = ast[_1];
        auto& m3 = ast[_3];

        std::cout << std::endl;
    }
#endif

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

    // String stream parsing
    //std::stringstream stream_data("<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\"  attribute2='value2'>root content part 1<ns:child1>child1 content<grandchild11 a='123' /></ns:child1><child2>child2 content</child2></nspre:root>");
    //util::streambuf_container<std::streambuf> xml_data(stream_data.rdbuf());

    // File parsing
    std::ifstream ifs;
    ifs.open("test\\cfg_test.cfg", std::ios::in | std::ios::binary);
    std::string xml_data(std::istreambuf_iterator<char>(ifs.rdbuf()), std::istreambuf_iterator<char>());
    //util::streambuf_container<std::streambuf> xml_data(ifs.rdbuf());

    typedef decltype(xml_data) data_type;

    long long t1, t2;

    /* stream performance test */
#if 1
    for (int i = 0; i < 2; i++)
    {
        unicode::unicode_container<std::string> uc(xml_data);
        t1 = time();
        auto begin = uc.begin();
        auto end = uc.end();
        for (auto i = begin; i != end; i++);
        t2 = time();
        std::cout << "stream iterate time: " << double(t2 - t1)/10000 << std::endl;

        std::string copy;
        copy.resize(xml_data.size());
        t1 = time();
        utf8::utf32to8(uc.begin(), uc.end(), std::back_inserter(copy));
        t2 = time();
        std::cout << "stream copy time: " << double(t2 - t1)/10000 << std::endl;
    }
#endif

    /* XML Tree Test */
#if 1
    t1 = time();
    xml::tree::document doc(xml_data);
    t2 = time();
    std::cout << "tree parse elapsed time: " << double(t2 - t1)/10000 << std::endl;
#endif

    /* XML Reader Test */
#if 1
    std::cout.setstate(std::ios_base::badbit);
    t1 = time();
    read_dump(xml_data);
    t2 = time();
    std::cout.clear();
    std::cout << "reader parse time: " << double(t2 - t1)/10000 << std::endl;
#endif

#if 1
    /* XML parse-only test */
    {
        t1 = time();
        auto start = xml_data.begin();
        bool valid = xml::grammar::document::parse_from(start, xml_data.end());
        t2 = time();
        std::cout << "parse-only time: " << double(t2 - t1)/10000 << ", valid=" << std::boolalpha << valid << std::endl;
    }
#endif

#if 1
    {
        t1 = time();
        auto start = xml_data.begin();
        parse::parser_ast<xml::grammar::document, data_type::iterator>::type ast;
        bool valid = xml::grammar::document::parse_from(start, xml_data.end(), ast);
        t2 = time();
        std::cout << "parse-only with AST time: " << double(t2 - t1)/10000 << ", valid=" << std::boolalpha << valid << std::endl;
        auto l = parse::tree::last_match(ast);
        std::cout << "last match: " << (int)xml_data.begin()._Ptr << ", " << (int)l._Ptr << std::endl;

        auto& root = ast[_0][_3].matches[23][_0].ptr->operator[](_3).matches[165][_0].ptr->operator[](_1).matches[0][_1][_1];
        std::cout << "root element: " << xml::get_string(root) << std::endl;

        auto& tmp = ast[_0][_1];
    }
#endif

    /* XML Tree Test */
    /*
    // Some typedefs for convenience
    typedef xml::document<decltype(xml_data)> document;
    typedef document::element_type element;
    typedef element::attribute_type attribute;

    try
    {
        auto start = GetTimeMs64();
        document doc(xml_data);
        auto end = GetTimeMs64();

        std::cout << "Parse time: " << (end - start) << std::endl;

        typedef unicode::unicode_container<decltype(xml_data)>::iterator iterator_t;

        std::cout << "document AST (string iterator) size: " << sizeof(parse::ast_type<xml::parser::element, std::string::iterator>::type) << std::endl;
        std::cout << "document AST (unicode_iterator) size: " << sizeof(parse::ast_type<xml::parser::document, iterator_t>::type) << std::endl;
        std::cout << "document parser size: " << sizeof(xml::parser::document) << std::endl;
        std::cout << "element AST size: " << sizeof(parse::ast_type<xml::parser::element, iterator_t>::type) << std::endl;
        std::cout << "unicode iterator size: " << sizeof(iterator_t) << std::endl;
        std::cout << "string iterator size: " << sizeof(std::string::iterator) << std::endl;

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
    }
    catch (xml::parse_exception& e)
    {
        std::cout << e.what() << std::endl;
    }
    */

    return 0;
}
