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

#if 0
namespace ast_tag_test
{
    using namespace parse;
    using namespace parse::operators;
    using namespace parse::terminals;

    auto num = +digit();
    auto lparen = u<'('>();
    auto rparen = u<')'>();

    struct s_expr;

    typedef decltype(num | reference<s_expr>()) elem_t;

    typedef decltype(lparen >> *space() >> elem_t() >> *(+space() >> elem_t()) >> *space() >> rparen) s_expr_t;

    struct s_expr : s_expr_t {};

    void test()
    {
        s_expr p;
        std::string data("(1 (2 33 345) ((((1234)))))");
        auto ast = tree::make_ast(p, data);

        bool valid = p.parse(data, ast);
    }
}
#endif

template <typename container_t>
void read_dump(container_t& c)
{
    xml::reader::document<container_t> doc(c);
    auto root = doc.root();
    dump_element(root, "");
}

template <typename iterator_t>
void dump_element(xml::reader::element<iterator_t>& e, const std::string& indent)
{
    std::cout << indent << "element: " << e.name() << std::endl;
    std::cout << indent << "attributes: ";

    xml::reader::attribute<iterator_t> a = e.next_attribute();
    if (a.is_end()) std::cout << "(none)" << std::endl;
    else
    {
        std::cout << std::endl;

        for (; !a.is_end(); a = a.next_attribute())
            std::cout << indent << "  " << a.name() << "=" << a.value() << std::endl;
    }

    auto nextIndex = indent + "  ";
    std::cout << indent << "childnodes: ";
    xml::reader::node<iterator_t> child = a.next_child();
    if (child.is_end()) std::cout << "(none)" << std::endl;
    else
    {
        std::cout << std::endl;
        for (; !child.is_end(); child = child.next_sibling())
        {
            if (child.is_text())
                std::cout << nextIndex << "textnode: " << child.text() << std::endl;
            else
                dump_element(child.element(), nextIndex);
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

#include "parse\parse2.h"

int _tmain(int argc, _TCHAR* argv[])
{
#if 1
    /* Pruned AST test */
    {
        using namespace parse2;
        using namespace parse2::operators;

        auto a = constant<'a'>();
        auto b = constant<'b'>();

        auto acap = a[parse2::_0];
        auto p = a[parse2::_0] >> b[parse2::_1] >> a[parse2::_2] >> b >> a >> b[parse2::_3];

        typedef decltype(a) a_type;
        typedef decltype(p) p_type;
        typedef decltype(acap) acap_type;

        p_type::left_type::capture_type::get_ast<std::string::iterator>::type left_ast;
        p_type::right_type::capture_type::get_ast<std::string::iterator>::type right_ast;
        p_type::capture_type::get_ast<std::string::iterator>::type ast;
        //auto ast = parse2::make_ast<std::string::iterator>(p);

        typedef p_type::left_type::capture_type left_capture_type;
        typedef p_type::right_type::capture_type right_capture_type;

        acap_type::capture_type acapcap;

        left_capture_type lcap;
        right_capture_type rcap;
        auto id = sequence_impl<p_type::left_type::capture_type, p_type::right_type::capture_type>::id;

        p_type::impl_type::capture_type asdf;

        auto& c0 = ast[parse2::_0];
        auto& c1 = ast[parse2::_1];
        auto& c2 = ast[parse2::_2];

        typedef decltype(ast) ast_type;
        auto ast_size = ast_type::size;

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

    long long t1, t2;

    /* stream performance test */
#if 0
    unicode::unicode_container<std::string> uc(xml_data);
    t1 = time();
    for (auto i = uc.begin(); i != uc.end(); i++);
    t2 = time();
    std::cout << "stream iterate time: " << double(t2 - t1)/10000 << std::endl;
#endif

    /* XML Tree Test */
#if 0
    t1 = time();
    xml::tree::document doc(xml_data);
    t2 = time();
    std::cout << "tree parse time: " << double(t2 - t1)/10000 << std::endl;
#endif

    /* XML Reader Test */
#if 0
    std::cout.setstate(std::ios_base::badbit);
    t1 = time();
    read_dump(xml_data);
    t2 = time();
    std::cout.clear();
    std::cout << "reader parse time: " << t2 - t1 << std::endl;
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
