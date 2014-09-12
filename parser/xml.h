#pragma once

#include "parse.h"
#include "unicode.h"

namespace xml
{
    using namespace util;

    namespace parser
	{

		using namespace ::parse;
		using namespace ::parse::operators;
		using namespace ::parse::terminals;

		auto lt = u<'<'>();
		auto gt = u<'>'>();
		auto qmark = u<'?'>();
		auto bslash = u<'\\'>();
		auto fslash = u<'/'>();
		auto dquote = u<'"'>();
		auto squote = u<'\''>();
		auto equal = u<'='>();
		auto space = u<' '>();
		auto colon = u<':'>();
		auto tab = u<'\t'>();
		auto cr = u<'\r'>();
		auto lf = u<'\n'>();

		auto name = group(alpha() >> *(alpha() | digit()));

		auto qname = group(!(name >> colon) >> name);

		auto ws = +(space | tab | cr | lf);

		auto eq = !ws >> equal >> !ws;

        auto content_char = ~(lt | gt);
		
		typedef decltype((squote >> *(~squote) >> squote) | (dquote >> *(~dquote) >> dquote)) qstring;

		typedef decltype(qname >> eq >> qstring()) attribute;

		typedef decltype(ws >> +(attribute())) attribute_list;

		typedef decltype(lt >> qname >> !attribute_list() >> gt) element_open;

		typedef decltype(lt >> fslash >> qname >> gt) element_close;

		struct element;

        typedef decltype(*(reference<element>() | *content_char)) element_content;
		
		struct element : public decltype(element_open() >> element_content() >> element_close())
        {
        };

		typedef decltype(!ws >> lt >> qmark >> *(~qmark) >> qmark >> gt >> !ws) prolog;

        struct test_referred;

        typedef decltype(reference<test_referred>()) refer;

        struct test_referred : public u<' '> {};
	}

	template <typename unicode_container>
    class document;

    template <typename unicode_iterator>
	class element;

    template <typename unicode_iterator>
    std::string get_string(parse::tree::ast_base<unicode_iterator>& ast)
    {
        std::string ret;
        utf8::utf32to8(ast.start, ast.end, std::back_inserter(ret));
        return ret;
    }

    template <typename unicode_iterator>
    class anchor
	{
	protected:
        unicode_iterator iter;
        unicode_iterator end;

	public:
        anchor(unicode_iterator& start, unicode_iterator& end) : iter(start), end(end)
		{
		}
	};

    template <typename unicode_iterator>
    class attribute
    {
        typedef typename parse::ast_type<parser::attribute, unicode_iterator>::type ast_type;
        
        ast_type& ast;

    public:
        attribute(ast_type& a) : ast(a)
        {
        }

        std::string name()
        {
            return get_string(ast[_0]);
        }

        std::string local_name()
		{
			return get_string(ast[_0][_1]);
		}

        std::string prefix()
        {
            auto& pre = ast[_0][_0].option[_0];
            return pre.matched ? get_string(pre) : "";
        }

        std::string value()
        {
            auto& qstr = ast[_2];
            return qstr[_0].matched ? 
                get_string(qstr[_0][_1]) :
                get_string(qstr[_1][_1]);
        }
    };

    template <typename unicode_iterator>
    class attribute_iterator : public std::iterator<std::forward_iterator_tag, attribute<unicode_iterator> >
    {
        typedef attribute<unicode_iterator> attribute_type;
        typedef typename parse::ast_type<parser::attribute, unicode_iterator>::type ast_type;
        typedef typename std::vector<ast_type>::iterator ast_iterator;

        ast_iterator it;

    public:
        attribute_iterator(const ast_iterator& ast_it) : it(ast_it)
        {
        }

        attribute_iterator(const attribute_iterator& other) : it(other.it)
        {
        }

        attribute_type operator* ()
        {
            return attribute_type(*it);
        }

        bool operator== (const attribute_iterator& other)
        {
            return it == other.it;
        }

        bool operator!= (const attribute_iterator& other)
        {
            return it != other.it;
        }

        attribute_iterator& operator++ ()
        {
            it++;
            return *this;
        }

        attribute_iterator operator++ (int)
        {
            attribute_iterator temp(*this);
            it++;
            return temp;
        }
    };

    template <typename unicode_iterator>
    class attribute_list
    {
    private:
        typedef attribute<unicode_iterator> attribute_type;
        typedef typename parse::ast_type<parser::attribute_list, unicode_iterator>::type ast_type;
        ast_type& ast;

    public:
        typedef attribute_iterator<unicode_iterator> iterator;

        attribute_list(ast_type& a) : ast(a) {}

        iterator begin()
        {
            return ast[_1].matched ? 
                iterator(ast[_1].children.begin()) : 
                end();
        }

        iterator end()
        {
            return ast[_1].children.end();
        }

        std::string operator[](const std::string& name)
        {
            auto it = std::find_if(begin(), end(), [&](decltype(*begin())& a)
            {
                return a.name() == name;
            });

            if (it == end()) throw std::exception("Attribute not present");
            
            return (*it).value();
        }
    };

	template <typename unicode_iterator>
    class element : public anchor<unicode_iterator>
	{
        typedef attribute_list<unicode_iterator> attribute_list_type;
		typename parse::ast_type<parser::element_open, unicode_iterator>::type ast;
        typename parse::ast_type<parser::element_content, unicode_iterator>::type content_ast;

	public:
        attribute_list_type attributes;
        unicode_iterator content_start;

		element(unicode_iterator& start, unicode_iterator& end) : anchor(start, end), attributes(ast[_2].option)
		{
			parser::element_open parser;
			if (!parser.parse_from(start, end, ast)) throw std::exception("parse error");
            content_start = start;
		}

		std::string name()
		{
			return get_string(ast[_1]);
		}

		std::string local_name()
		{
			return get_string(ast[_1].group[_1]);
		}

        std::string prefix()
        {
            auto& pre = ast[_1].group[_0].option[_0];
            return pre.matched ? get_string(pre) : "";
        }

        std::string text()
        {
            parser::element_content parser;
            if (!parser.parse_from(content_start, end, content_ast)) throw std::exception("parse error");
            std::string content;

            for (auto child = content_ast.children.begin();
                child != content_ast.children.end(); child++)
            {
                if ((*child)[_1].matched) content += get_string((*child)[_1]);
            }
            return content;
        }
	};

	template <typename octet_container>
    class document
	{
        typedef typename unicode::unicode_container<octet_container> unicode_container;
	    typedef typename unicode_container::iterator unicode_iterator;
        typedef typename parse::ast_type<parser::prolog, unicode_iterator>::type ast_type;
        typedef element<unicode_iterator> element_type;

        unicode_container data;
        ast_type ast;

	public:
		document(octet_container& c) : data(c)
		{
		}

		element_type root()
		{
			parser::prolog p;
            auto it = data.begin();
            auto end = data.end();
			if (!p.parse_from(it, end, ast)) throw std::exception("parse error");

            return element_type(it, end);
		}
	};

}