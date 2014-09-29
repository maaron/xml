#pragma once

#include "tree2.h"

namespace parse2
{
    using namespace placeholders;

    template <template <typename> class ast_t, size_t i>
    struct captured
    {
        template <typename iterator_t>
        struct get_ast { typedef ast_t<iterator_t> type; };
        static const size_t i;
        static const bool is_captured = true;
    };

    template <template <typename> class ast_t>
    struct captured<ast_t, -1>
    {
        template <typename iterator_t>
        struct get_ast { typedef ast_t<iterator_t> type; };
        static const size_t i;
        static const bool is_captured = false;
    };

    template <typename t>
    struct void_ast
    {
    };

    template <typename iterator_t>
	struct ast_base
	{
        typedef iterator_t iterator;

		iterator_t start;
		iterator_t end;
		bool matched;
        bool parsed;

		ast_base() : matched(false), parsed(false)
		{
		}
	};

    template <template <typename> class ast_t>
    struct ast_group
    {
        template <typename iterator_t> 
        struct impl : ast_base<iterator_t>
        {
            ast_t<iterator_t> ast;
        };
    };

    template <typename t>
    struct always_false { enum { value = false }; };

    template <typename parser_t, size_t i>
    struct capture_group;

    template <typename parser_t, typename iterator_t>
    struct parser_ast
    {
        typedef typename parser_t::template get_ast<iterator_t>::type type;
    };

    template <typename derived_t>
    struct parser
    {
        typedef derived_t parser_type;

        template <typename iterator_t>
        struct get_ast { typedef void type; };

        // 3-parameter parse_from() only valid if the parser is captured.
        template <typename iterator_t, typename ast_t>
        static bool parse_from(iterator_t& it, iterator_t& end, ast_t& a)
        {
            return derived_t::parse_internal(it, end, a);
        }

        // 2-parameter parse_from() always available for parsing without 
        // generating an AST.
        template <typename iterator_t>
        static bool parse_from(iterator_t& it, iterator_t& end)
        {
            return derived_t::parse_internal(it, end);
        }

        // Operator overload for capturing parser result into an AST.
        template <size_t i>
        capture_group<derived_t, i> operator[] (const index<i>& ph)
        {
            return capture_group<derived_t, i>();
        }
    };

    template <typename t1, typename t2, typename enable = void>
    struct sequence { static_assert(always_false<enable>::value, "wht"); };

    template <typename t1, typename t2>
    struct sequence<t1, t2, typename std::enable_if<t1::is_captured && t2::is_captured>::type >
        : parser<sequence<t1, t2> >
    {
        static const int sequencetype = 1;
        static const bool is_captured = true;

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename parser_ast<t1, iterator_t>::type t1_ast_type;
            typedef typename parser_ast<t2, iterator_t>::type t2_ast_type;
            typedef typename ast::join<t1_ast_type, t2_ast_type>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end);
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& a)
        {
            return t1::parse_from(start, end, a.left()) &&
                t2::parse_from(start, end, a.right());
        }
    };

    template <typename t1, typename t2>
    struct sequence<t1, t2, typename std::enable_if<t1::is_captured && !t2::is_captured>::type >
        : parser<sequence<t1, t2> >
    {
        static const int sequencetype = 2;
        static const bool is_captured = true;

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename parser_ast<t1, iterator_t>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end);
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& a)
        {
            return t1::parse_from(start, end, a) &&
                t2::parse_from(start, end);
        }
    };

    template <typename t1, typename t2>
    struct sequence<t1, t2, typename std::enable_if<!t1::is_captured && t2::is_captured>::type >
        : parser<sequence<t1, t2> >
    {
        static const int sequencetype = 3;
        static const bool is_captured = true;

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename parser_ast<t2, iterator_t>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end);
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& a)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end, a);
        }
    };

    template <typename t1, typename t2>
    struct sequence<t1, t2, typename std::enable_if<!t1::is_captured && !t2::is_captured>::type >
        : parser<sequence<t1, t2> >
    {
        static const int sequencetype = 4;
        static const bool is_captured = false;
        template <typename iterator_t>
        struct get_ast
        {
            typedef typename parser_ast<t2, iterator_t>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end);
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& a)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end);
        }
    };

    template <typename capture_t, size_t new_i>
    struct make_group_capture;

    template <template <typename> class ast_t, size_t i, size_t new_i>
    struct make_group_capture<captured<ast_t, i>, new_i>
    {
        typedef captured<typename ast_group<ast_t>::impl, new_i> type;
    };

    template <typename parser_t, size_t i>
    struct capture_group : parser<capture_group<parser_t, i> >
    {
        static const size_t key = i;
        static const bool is_captured = true;

        typedef parser<capture_group<parser_t, i> > base_type;

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename parser_ast<parser_t, iterator_t>::type parser_ast_type;
            typedef typename std::conditional<std::is_void<parser_ast_type>::value, ast_base<iterator_t>, parser_ast_type>::type nonvoid;
            typedef typename ast::ast_leaf<i, nonvoid> type;
        };

        template <typename iterator_t>
        static bool parser_internal(iterator_t& start, iterator_t& end, typename parser_ast<parser_t, iterator_t>::type& a)
        {
            return parser_t::parse_from(start, end, a.ast);
        }
    };

    template <char32_t t>
    struct constant : parser<constant<t> >
    {
        static const bool is_captured = false;
    };

    template <typename iterator_t, typename parser_t>
    typename parser_t::template get_ast<iterator_t>::type make_ast(const parser_t& p)
    {
        return typename parser_t::template get_ast<iterator_t>::type();
    }

    namespace operators
    {
        template <typename t1, typename t2>
        sequence<t1, t2> operator>> (const parser<t1>& p1, const parser<t2>& p2)
        {
            return sequence<t1, t2>();
        }
    }
}