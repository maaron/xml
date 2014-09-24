#pragma once

namespace parse2
{
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

    template <typename iterator_t, typename left_t, typename right_t>
    struct ast_list
    {
        left_t left;
        right_t right;

        static const size_t size = 2;

		template <size_t i> struct elem;
		template <> struct elem<0> { typedef left_t type; };
		template <> struct elem<1> { typedef right_t type; };

		left_t& operator[](const index<0>&) { return left; }
		right_t& operator[](const index<1>&) { return right; }
    };

    template <typename iterator_t, typename t1, typename t2, typename right_t>
    struct ast_list<iterator_t, ast_list<iterator_t, t1, t2>, right_t>
    {
        typedef ast_list<iterator_t, t1, t2> left_t;

        left_t left;
        right_t right;

        static const size_t size = v::size + 1;

		template <size_t i>
		struct elem
		{
			static_assert(i < size, "Element type index out of range");
			typedef typename left_t::template elem<i>::type type;
		};

		template <>
		struct elem<size - 1> { typedef right_t type; };

		right_t& operator[](const index<size - 1>&) { return right; }

		template <size_t i>
		typename elem<i>::type& operator[](const index<i>& idx)
		{
			static_assert(i < size, "Element index out of range");
			return left[idx];
		}
	};

    template <typename parser_t>
    struct capture;

    template <typename derived_t>
    struct parser
    {
        template <typename iterator_t>
        struct get_ast { typedef void type; };

        capture<derived_t> operator[](size_t i)
        {
            return capture<derived_t>();
        }
    };

    template <typename parser_t>
    struct capture : parser< capture<parser_t> >
    {
        template <typename iterator_t>
        struct get_ast { typedef ast_base<iterator_t> type; };
    };

    // Non-capturing sequence parser
    template <typename t1, typename t2>
    struct sequence
    {
        template <typename iterator_t>
        struct get_ast { typedef void type; };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end);
        }
    };

    // Left-side-capturing sequence parser
    template <typename t1, typename t2>
    struct sequence< capture<t1>, t2 >
    {
        template <typename iterator_t>
        struct get_ast { typedef typename t1::template get_ast<iterator_t>::type type; };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& a)
        {
            return t1::parse_from(start, end, a) &&
                t2::parse_from(start, end);
        }
    };

    // Right-side-capturing sequence parser
    template <typename t1, typename t2>
    struct sequence< t1, capture<t2> >
    {
        template <typename iterator_t>
        struct get_ast { typedef typename capture<t2>::template get_ast<iterator_t>::type type; };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& a)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end, a);
        }
    };

    // Both-sides-capturing sequence parser
    template <typename t1, typename t2>
    struct sequence< capture<t1>, capture<t2> >
    {
        static const int sequence_type = 4;

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename t1::template get_ast<iterator_t>::type t1_ast_type;
            typedef typename t2::template get_ast<iterator_t>::type t2_ast_type;
            typedef ast_list<iterator_t, t1_ast_type, t2_ast_type> type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& a)
        {
            return t1::parse_from(start, end, a.left) &&
                t2::parse_from(start, end, a.right);
        }
    };

    template <char32_t t>
    struct constant : parser< constant<t> >
    {
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

        template <typename t1, typename t2>
        sequence<capture<t1>, t2> operator>> (const capture<t1>& p1, const parser<t2>& p2)
        {
            return sequence<capture<t1>, t2>();
        }

        template <typename t1, typename t2>
        sequence<t1, capture<t2> > operator>> (const parser<t1>& p1, const capture<t2>& p2)
        {
            return sequence<t1, capture<t2> >();
        }

        template <typename t1, typename t2>
        sequence<capture<t1>, capture<t2> > operator>> (const capture<t1>& p1, const capture<t2>& p2)
        {
            return sequence<capture<t1>, capture<t2> >();
        }
    }
}