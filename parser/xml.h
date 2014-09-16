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
		
		typedef decltype((squote >> *(~squote) >> squote) | (dquote >> *(~dquote) >> dquote)) qstring_t;
        struct qstring : public qstring_t
        {
            template <typename iterator_t>
            struct ast : public qstring_t::ast<iterator_t>
            {

            };
        };

		typedef decltype(qname >> eq >> qstring()) attribute;

		typedef decltype(ws >> +(attribute())) attribute_list;

		typedef decltype(lt >> qname >> !attribute_list() >> gt) element_open;

		typedef decltype(lt >> fslash >> qname >> gt) element_close;

		struct element;

        typedef decltype(*(reference<element>() | *content_char)) element_content;

        typedef decltype(element_open() >> element_content() >> element_close()) element_base;
		
		struct element : public element_base
        {
        };

		typedef decltype(!ws >> lt >> qmark >> *(~qmark) >> qmark >> gt >> !ws) prolog;

        // This parser reads the next element open or close tag, skipping over any preceeding content
        typedef decltype(*parser::content_char >> (parser::element_open() | parser::element_close())) next_open;

	}

    template <typename container>
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
        anchor(const unicode_iterator& start, const unicode_iterator& end) : iter(start), end(end)
		{
		}

        void set_anchor(const unicode_iterator& start)
        {
            iter = start;
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

    template <template <typename> class node, template <typename> class ast_templ, typename unicode_iterator>
    class node_iterator : public std::iterator<std::forward_iterator_tag, node<unicode_iterator> >
    {
        typedef node<unicode_iterator> node_type;
        typedef typename ast_templ<unicode_iterator>::type ast_type;
        typedef typename std::vector<ast_type>::iterator ast_iterator;

        ast_iterator it;

    public:
        node_iterator(const ast_iterator& ast_it) : it(ast_it)
        {
        }

        node_iterator(const node_iterator& other) : it(other.it)
        {
        }

        node_type operator* ()
        {
            return node_type(*it);
        }

        bool operator== (const node_iterator& other)
        {
            return it == other.it;
        }

        bool operator!= (const node_iterator& other)
        {
            return it != other.it;
        }

        node_iterator& operator++ ()
        {
            it++;
            return *this;
        }

        node_iterator operator++ (int)
        {
            node_iterator temp(*this);
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
        typedef node_iterator<attribute, parser::attribute::ast, unicode_iterator> iterator;

        attribute_list(ast_type& a) : ast(a) {}

        iterator begin()
        {
            return ast[_1].matched ? 
                iterator(ast[_1].matches.begin()) : 
                end();
        }

        iterator end()
        {
            return ast[_1].matches.end();
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
    class element_iterator : public std::iterator<std::forward_iterator_tag, element<unicode_iterator> >,
        public anchor<unicode_iterator>
    {
        typedef element<unicode_iterator> element_type;
        typedef typename parser::next_open::ast<unicode_iterator>::type open_ast;

        open_ast open;

    public:
        element_iterator(const unicode_iterator& start, const unicode_iterator& end) 
            : anchor(start, end)
        {
            get();
        }

        element_iterator(const element_iterator& other) : open(other.open), anchor(other)
        {
        }

        element_type operator* ()
        {
            if (iter == end) throw std::exception("iterator out of bounds");
            return element_type(open[_1][_0], end);
        }

        bool operator== (const element_iterator& other)
        {
            return iter == other.iter;
        }

        bool operator!= (const element_iterator& other)
        {
            return iter != other.iter;
        }

        element_iterator& operator++ ()
        {
            advance();
            return *this;
        }

        element_iterator operator++ (int)
        {
            element_iterator temp(*this);
            advance();
            return temp;
        }

    private:
        void advance()
        {
            if (iter == end) return;

            using namespace parse::operators;

            // Parse the remainder of the content, plus the closing tag.
            typedef decltype(parser::element_content() >> parser::element_close()) content_parser;
            typedef typename content_parser::ast<unicode_iterator>::type content_ast;

            content_parser content;
            content_ast ast;
            if (!content.parse_from(iter, end, ast)) throw std::exception("parse error");

            get();
        }

        void get()
        {
            if (iter == end) return;

            parser::next_open p;
            if (!p.parse_from(iter, end, open)) throw std::exception("parse error");

            // If we get an element close, that means we are finished 
            // iterating.
            if (open[_1][_1].matched) iter == end;
        }
    };

    template <typename unicode_iterator>
    class element_list : public anchor<unicode_iterator>
    {

    public:
        typedef element<unicode_iterator> element_type;
        typedef element_iterator<unicode_iterator> iterator;

        element_list(const unicode_iterator& start, const unicode_iterator& end) 
            : anchor(iter, end)
        {
        }

        iterator begin()
        {
            return iterator(iter, anchor<unicode_iterator>::end);
        }

        iterator end()
        {
            return iterator(anchor<unicode_iterator>::end, anchor<unicode_iterator>::end);
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
        typedef typename parse::ast_type<parser::element_open, unicode_iterator>::type open_ast;
        typedef typename parse::ast_type<parser::element_content, unicode_iterator>::type content_ast;
        unicode_iterator content_start;

        open_ast open;
        content_ast content;

	public:
        typedef attribute_list<unicode_iterator> attribute_list_type;
        typedef attribute<unicode_iterator> attribute_type;

        typedef element_list<unicode_iterator> element_list_type;

        attribute_list_type attributes;
        element_list_type elements;

		element(unicode_iterator& start, unicode_iterator& end) : anchor(start, end), attributes(open[_2].option), elements(end, end)
		{
			parser::element_open parser;
			if (!parser.parse_from(start, end, open)) throw std::exception("parse error");
            content_start = start;
            elements.set_anchor(start);
		}

        element(open_ast& a, unicode_iterator& end) : open(a), content_start(a.end), anchor(a.start, end), attributes(a[_2].option), elements(a.end, end)
        {
        }

		std::string name()
		{
			return get_string(open[_1]);
		}

		std::string local_name()
		{
			return get_string(open[_1].group[_1]);
		}

        std::string prefix()
        {
            auto& pre = open[_1].group[_0].option[_0];
            return pre.matched ? get_string(pre) : "";
        }

        std::string text()
        {
            parser::element_content parser;
            if (!parser.parse_from(content_start, end, content)) throw std::exception("parse error");
            std::string ret;
            
            for (auto child = content.matches.begin();
                child != content.matches.end(); child++)
            {
                auto& text_ast = (*child)[_1];
                if (text_ast.matched) ret += get_string(text_ast);
            }
            return ret;
        }
	};

	template <typename container>
    class document
	{
        typedef typename unicode::unicode_container<container> unicode_container;
	    typedef typename unicode_container::iterator unicode_iterator;
        typedef typename parse::ast_type<parser::prolog, unicode_iterator>::type ast_type;

        unicode_container data;
        ast_type ast;

	public:
        typedef element<unicode_iterator> element_type;

        document(container& c) : data(c)
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

    template <typename container>
    document<container> parse(container& c)
    {
        return document<container>(c);
    }
}