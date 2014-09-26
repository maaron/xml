#pragma once

#include "list2.h"

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

    // This type, along with the is_list struct below are used to identify 
    // ast_list templates.
    struct ast_list_tag {};

    template <typename capture_t>
    struct is_list
    {
    private:
        typedef typename capture_t::template get_ast<int>::type ast_type;

        typedef char                      yes;
        typedef struct { char array[2]; } no;

        template<typename C> static std::enable_if<std::is_same<typename C::tag, ast_list_tag>::value, yes> test(typename C::tag*);
        template<typename C> static no  test(...);
    
    public:
        static const bool value = sizeof(test<ast_type>(0)) == sizeof(yes);
    };

    template <typename capture_t1, typename capture_t2, typename enable = void>
    struct ast_list;

    template <typename capture_t1, typename capture_t2>
    struct ast_list<capture_t1, capture_t2, typename std::enable_if<!is_list<capture_t1>::value>::type>
    {
        template <typename iterator_t>
        struct impl
        {
            typedef typename capture_t1::template get_ast<iterator_t>::type left_t;
            typedef typename capture_t2::template get_ast<iterator_t>::type right_t;
            typedef ast_list_tag tag;
            
            left_t left;
            right_t right;

            static const size_t size = 2;

		    template <size_t i> struct elem;
		    template <> struct elem<0> { typedef left_t type; };
		    template <> struct elem<1> { typedef right_t type; };

		    left_t& operator[](const index<0>&) { return left; }
		    right_t& operator[](const index<1>&) { return right; }
        };
    };

    template <typename t>
    struct always_false { enum { value = false }; };

    template <typename capture_t1, typename capture_t2>
    struct ast_list<capture_t1, capture_t2, typename std::enable_if<is_list<capture_t1>::value>::type>
    {
        template <typename iterator_t>
        struct impl
        {
            typedef typename capture_t1::template get_ast<iterator_t>::type left_t;
            typedef typename capture_t2::template get_ast<iterator_t>::type right_t;
            typedef ast_list_tag tag;

            left_t left;
            right_t right;

            static const size_t size = left_t::size + 1;

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
	};

    template <typename parser_t, size_t i>
    struct capture_group;

    template <typename derived_t, typename capture_type>
    struct parser;

    // Capturing parser
    template <typename derived_t, template <typename> class ast_t, size_t i>
    struct parser<derived_t, captured<ast_t, i> >
    {
        static const bool is_captured = true;
        typedef derived_t parser_type;
        typedef captured<ast_t, i> capture_type;

        typedef parser<derived_t, captured<ast_t, -1> > uncaptured_type;

        template <typename iterator_t>
        static bool parse_from(iterator_t& it, iterator_t& end, typename capture_type::template get_ast<iterator_t>::type& a)
        {
            return derived_t::parse_internal(it, end, a);
        }
    };

    // Non-capturing parser
    template <typename derived_t, template <typename> class ast_t>
    struct parser<derived_t, captured<ast_t, -1> >
    {
        static const bool is_captured = false;
        typedef derived_t parser_type;
        typedef captured<ast_t, -1> capture_type;

        template <size_t i>
        struct capture { typedef parser<derived_t, captured<ast_t, i> > type; };

        template <typename iterator_t>
        static bool parse_from(iterator_t& it, iterator_t& end)
        {
            return derived_t::parse_internal(it, end);
        }

        template <size_t i>
        capture_group<derived_t, i> operator[] (const index<i>& ph)
        {
            return capture_group<derived_t, i>();
        }
    };

    // Both branches are captured, as well as the sequence.
    template <typename t1_capture_t, typename t2_capture_t>
    struct sequence_impl;

    // Both branches are captured, but not the sequence.
    template <
        template <typename> class t1_ast_t, size_t t1_i, 
        template <typename> class t2_ast_t, size_t t2_i>
    struct sequence_impl<captured<t1_ast_t, t1_i>, captured<t2_ast_t, t2_i> >
    {
        static const int id = 1;
        typedef captured<t1_ast_t, t1_i> t1_capture_type;
        typedef captured<t2_ast_t, t2_i> t2_capture_type;
        typedef captured<ast_list<t1_capture_type, t2_capture_type>::impl, -1> capture_type;

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast_list<t1_capture_type, t2_capture_type>::template impl<iterator_t>::type& a)
        {
            return t1::parse_from(start, end, a.left) &&
                t2::parse_from(start, end, a.right);
        }
    };

    /*
    // The right branch is captured, as well as the sequence.
    template <typename t1_capture_t, typename t2_capture_t, size_t i>
    struct sequence_impl;

    template <
        template <typename> class t1_ast_t, 
        template <typename> class t2_ast_t, size_t t2_i, 
        size_t i>
    struct sequence_impl<captured<t1_ast_t, -1>, captured<t2_ast_t, t2_i>, i>
    {
        typedef captured<ast_group<t2_ast_t>::impl, i> capture_type;

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast_group<t2_ast_t>::template impl<iterator_t>::type& a)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end, a.ast);
        }
    };
    */

    // The right branch is captured, but not the sequence.
    template <
        size_t t1_i, 
        template <typename> class t2_ast_t, size_t t2_i>
    struct sequence_impl<captured<void_ast, t1_i>, captured<t2_ast_t, t2_i> >
    {
        static const int id = 2;
        typedef captured<t2_ast_t, t2_i> capture_type;

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename t2_ast_t<iterator_t>::type& a)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end, a);
        }
    };

    /*
    // The left branch is captured, as well as the sequence.
    template <typename t1_capture_t, typename t2_capture_t, size_t i>
    struct sequence_impl;

    template <
        template <typename> class t1_ast_t, size_t t1_i,
        template <typename> class t2_ast_t, 
        size_t i>
    struct sequence_impl<captured<t1_ast_t, t1_i>, captured<t2_ast_t, -1>, i>
    {
        typedef captured<ast_group<t1_ast_t>::impl, i> capture_type;

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast_group<t1_ast_t>::template impl<iterator_t>::type& a)
        {
            return t1::parse_from(start, end, a.ast) &&
                t2::parse_from(start, end);
        }
    };
    */

    // The left branch is captured, but not the sequence.
    template <
        template <typename> class t1_ast_t, size_t t1_i,
        size_t t2_i>
    struct sequence_impl<captured<t1_ast_t, t1_i>, captured<void_ast, t2_i> >
    {
        static const int id = 3;
        typedef captured<t1_ast_t, t1_i> capture_type;

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename t1_ast_t<iterator_t>::type& a)
        {
            return t1::parse_from(start, end, a) &&
                t2::parse_from(start, end);
        }
    };

    /*
    // Neither branch is captured, but the sequence is.
    template <typename t1_capture_t, typename t2_capture_t, size_t i>
    struct sequence_impl;

    template <
        template <typename> class t1_ast_t,
        template <typename> class t2_ast_t, 
        size_t i>
    struct sequence_impl<captured<t1_ast_t, -1>, captured<t2_ast_t, -1>, i>
    {
        typedef captured<ast_base, i> capture_type;

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast_base<iterator_t>& a)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end);
        }
    };
    */

    // Nothing is captured.
    template <size_t t1_i, size_t t2_i>
    struct sequence_impl<captured<void_ast, t1_i>, captured<void_ast, t2_i> >
    {
        static const int id = 4;
        typedef captured<void_ast, -1> capture_type;

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end);
        }
    };

    template <typename t1, typename t2>
    struct sequence : parser<sequence<t1, t2>, typename sequence_impl<typename t1::capture_type, typename t2::capture_type>::capture_type>
    {
        typedef typename sequence_impl<typename t1::capture_type, typename t2::capture_type> impl_type;
        typedef parser<sequence<t1, t2>, typename impl_type::capture_type> base_type;
        typedef t1 left_type;
        typedef t2 right_type;
        static const int impl_id = impl_type::id;

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return sequence_impl::parse_internal(start, end);
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename base_type::capture_type::template get_ast<iterator_t>::type& a)
        {
            return sequence_impl::parse_internal(start, end, a);
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
    struct capture_group<parser_t, i> : parser<capture_group<parser_t, i>, typename make_group_capture<typename parser_t::capture_type, i>::type>
    {
        typedef parser<capture_group<parser_t, i>, typename make_group_capture<typename parser_t::capture_type, i>::type> base_type;

        template <typename iterator_t>
        static bool parser_internal(iterator_t& start, iterator_t& end, typename base_type::capture_type::template get_ast<iterator_t>::type& a)
        {
            return parser_t::parse_from(start, end, a.ast);
        }
    };

    template <char32_t t>
    struct constant : parser< constant<t>, captured<void_ast, -1> >
    {
    };

    template <typename iterator_t, typename parser_t>
    typename parser_t::template get_ast<iterator_t>::type make_ast(const parser_t& p)
    {
        return typename parser_t::template get_ast<iterator_t>::type();
    }

    namespace operators
    {
        template <typename t1, typename t2, typename t1_capture, typename t2_capture>
        sequence<parser<t1, t1_capture>, parser<t2, t2_capture> > operator>> (const parser<t1, t1_capture>& p1, const parser<t2, t2_capture>& p2)
        {
            return sequence<parser<t1, t1_capture>, parser<t2, t2_capture> >();
        }
    }
}