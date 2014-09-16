#pragma once

#include <assert.h>
#include "parse.h"
#include "unicode.h"

namespace xml
{
    using namespace util;

    class parse_error : public std::exception
    {
    public:
        parse_error() : std::exception("xml parse error") {}
    };

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

		auto name = alpha() >> *(alpha() | digit());

		typedef decltype(!(group(name) >> colon) >> name) qname_t;
        struct qname : qname_t
        {
            template <typename iterator_t>
            struct ast : qname_t::ast<iterator_t>::type
            {
                typedef ast type;

                std::string prefix()
                {
                    return (*this)[_0].option ?
                        get_string((*this)[_0][_0]) : "";
                }

                std::string name()
                {
                    return get_string(*this);
                }

                std::string local_name()
                {
                    return get_string((*this)[_1]);
                }
            };
        };

		auto ws = +(space | tab | cr | lf);

		auto eq = !ws >> equal >> !ws;

        auto content_char = ~(lt | gt);
		
		typedef decltype((squote >> *(~squote) >> squote) | (dquote >> *(~dquote) >> dquote)) qstring_t;
        struct qstring : qstring_t
        {
            template <typename iterator_t>
            struct ast : qstring_t::ast<iterator_t>::type
            {
                typedef ast type;

                std::string value()
                {
                    if ((*this)[_0].matched) return get_string((*this)[_0][_1]);
                    else return get_string((*this)[_1][_1]);
                }
            };
        };
        

		typedef decltype(group(qname()) >> eq >> qstring()) attribute_t;
        struct attribute : attribute_t
        {
            template <typename iterator_t>
            struct ast : attribute_t::ast<iterator_t>::type
            {
                typedef ast type;

                typename qstring::ast<iterator_t>::type qstring()
                {
                    return (*this)[_2];
                }

                typename qname::ast<iterator_t>::type& key()
                {
                    return (*this)[_0].group;
                }
            };
        };

		typedef decltype(ws >> +(attribute())) attribute_list;

		typedef decltype(lt >> qname() >> !attribute_list() >> gt) element_open;

		typedef decltype(lt >> fslash >> qname() >> gt) element_close;

		struct element;

        typedef reference<element> element_ref;

        typedef decltype(*content_char) textnode;

        typedef decltype(element_ref() | textnode()) childnode;

        typedef decltype(*(childnode())) element_content;

        typedef decltype(lt >> group(qname()) >> !attribute_list() >> ((fslash >> gt) | (gt >> element_content() >> element_close()))) element_base;
		
		struct element : public element_base {};

		typedef decltype(!ws >> lt >> qmark >> *(~qmark) >> qmark >> gt >> !ws) prolog;

        typedef decltype(!prolog() >> element()) document;

        // This parser reads the next element open or close tag, skipping over any preceeding content
        //typedef decltype(*parser::content_char >> (parser::element_open() | parser::element_close())) next_open;

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
            return ast.key().name();
        }

        std::string local_name()
		{
			return ast.key().local_name()
		}

        std::string prefix()
        {
            return ast.key().prefix();
        }

        std::string value()
        {
            return ast.qstring().value();
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
    class element_iterator 
        : public std::iterator<std::forward_iterator_tag, element<unicode_iterator> >
    {
        typedef element<unicode_iterator> element_type;
        typedef typename parse::ast_type<parser::element_content, unicode_iterator>::type content_ast_type;
        typedef typename content_ast_type::container_type::iterator ast_iterator;

        ast_iterator ast_iter;
        ast_iterator ast_end;

    public:
        element_iterator(const ast_iterator& iter, const ast_iterator& end) 
            : ast_iter(iter), ast_end(end)
        {
            get();
        }

        element_type operator* ()
        {
            if (ast_iter == ast_end) throw std::exception("iterator out of bounds");
            auto& elem_ast = *(*ast_iter)[_0].ptr.get();
            return element_type(elem_ast);
        }

        bool operator== (const element_iterator& other)
        {
            return ast_iter == other.ast_iter;
        }

        bool operator!= (const element_iterator& other)
        {
            return ast_iter != other.ast_iter;
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
            if (ast_iter == ast_end) return;
            ast_iter++;
            get();
        }

        void get()
        {
            while (ast_iter != ast_end && !(*ast_iter)[_0].matched)
                ast_iter++;
        }
    };

    template <typename unicode_iterator>
    class element_list
    {
        typedef typename parser::element_content::ast<unicode_iterator>::type ast_type;
        ast_type& ast;

    public:
        typedef element<unicode_iterator> element_type;
        typedef element_iterator<unicode_iterator> iterator;

        element_list(ast_type& a) 
            : ast(a)
        {
        }

        iterator begin()
        {
            return iterator(ast.matches.begin(), ast.matches.end());
        }

        iterator end()
        {
            return iterator(ast.matches.end(), ast.matches.end());
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
    class element
	{
        typedef typename parse::ast_type<parser::element, unicode_iterator>::type ast_type;

        ast_type& ast;

	public:
        typedef attribute_list<unicode_iterator> attribute_list_type;
        typedef attribute<unicode_iterator> attribute_type;
        typedef element_list<unicode_iterator> element_list_type;

        attribute_list_type attributes;
        element_list_type elements;

		element(ast_type& a) 
            : attributes(a[_2].option), 
            elements(a[_3][_1][_1]), 
            ast(a)
		{
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
            std::string ret;

            auto& childnodes = ast[_3][_1][_1].matches;
            
            for (auto child = childnodes.begin();
                child != childnodes.end(); child++)
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
        typedef typename parse::ast_type<parser::document, unicode_iterator>::type ast_type;

        unicode_container data;
        ast_type ast;

	public:
        typedef element<unicode_iterator> element_type;

        document(container& c) : data(c)
		{
			parser::document p;
			if (!p.parse(data, ast)) throw parse_error();
        }

		element_type root()
		{
            return element_type(ast[_1]);
		}
	};

    template <typename container>
    document<container> parse(container& c)
    {
        return document<container>(c);
    }
}