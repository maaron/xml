#pragma once

#include <type_traits>

namespace tree
{
	template <typename iterator_t>
    struct ast
    {
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

        template <typename left_t, typename right_t>
        struct branch
            : left_t, right_t
        {
            typedef left_t left_type;
            typedef right_t right_type;
            typedef branch<left_t, right_t> self_type;

            template <size_t i>
            struct has_key
            {
                static const bool value =
                left_type::template has_key<i>::type ||
                right_type::template has_key<i>::value;
            };

            template <size_t i>
            struct get_base_type
            {
                typedef typename left_type::template get_base_type<i>::type left_base;
                typedef typename right_type::template get_base_type<i>::type right_base;
                typedef typename std::conditional<std::is_void<left_base>::value, right_base, left_base>::type type;
            };

            template <typename base_type> struct base_to_value_type { typedef typename base_type::value_type type; };
            template <> struct base_to_value_type<void> { typedef void type; };

            template <size_t i>
            struct get_value_type
            {
                typedef typename base_to_value_type<typename get_base_type<i>::type>::type type;
            };

            template <size_t i>
            typename get_value_type<i>::type& operator[] (const placeholders::index<i>& ph)
            {
                typedef typename get_base_type<i>::type base_type;
                static_assert(!std::is_void<base_type>::value, "Element index out of range");
                return base_type::value;
            }

            left_type& left() { return static_cast<left_type&>(*this); }
            right_type& right() { return static_cast<right_type&>(*this); }
        };

        template <size_t i, typename value_t>
        struct ast_leaf : base
        {
            typedef ast_leaf<i, value_t> self_type;
            typedef value_t value_type;
            typedef ast_leaf<i, value_t> self_type;

            static const size_t idx = i;
            value_type value;

            template <size_t i>
            struct has_key
            {
                static const bool value = i == idx;
            };

            template <size_t i> struct get_base_type { typedef void type; };
            template <> struct get_base_type<idx> { typedef self_type type; };

            template <size_t i>
            struct get_value_type { typedef void type; };
            template <> struct get_value_type<idx> { typedef value_type type; };

            self_type& operator[] (const placeholders::index<idx>&)
            {
                return *this;
            }
        };

        // Specialization for leaf nodes that specify a void value type
        template <size_t i>
        struct ast_leaf<i, void> : ast_base
        {
            typedef ast_leaf<i, void> self_type;
            typedef value_t value_type;
            typedef ast_leaf<i, value_t> self_type;

            static const size_t idx = i;

            template <size_t i>
            struct has_key
            {
                static const bool value = i == idx;
            };

            template <size_t i> struct get_base_type { typedef void type; };
            template <> struct get_base_type<idx> { typedef self_type type; };

            template <size_t i>
            struct get_value_type { typedef void type; };
            template <> struct get_value_type<idx> { typedef value_type type; };

            self_type& operator[] (const placeholders::index<idx>&)
            {
                return *this;
            }
        };

        template <typename branch1_t, typename branch2_t>
        struct is_unique
        {
            static const bool value =
            is_unique<typename branch1_t::left_type, branch2_t>::value &&
            is_unique<typename branch1_t::right_type, branch2_t>::value;
        };

        template <size_t i, typename value_t, typename branch_t>
        struct is_unique<ast_leaf<i, value_t>, branch_t>
        {
            typedef ast_leaf<i, value_t> leaf_t;
            static const bool value =
                is_unique<leaf_t, typename branch_t::left_type>::value &&
                is_unique<leaf_t, typename branch_t::right_type>::value;
        };

        template <typename branch_t, size_t i, typename value_t>
        struct is_unique<branch_t, ast_leaf<i, value_t> >
        {
            typedef ast_leaf<i, value_t> leaf_t;
            static const bool value =
                is_unique<leaf_t, typename branch_t::left_type>::value &&
                is_unique<leaf_t, typename branch_t::right_type>::value;
        };

        template <size_t i1, typename value1_t, size_t i2, typename value2_t>
        struct is_unique<ast_leaf<i1, value1_t>, ast_leaf<i2, value2_t> >
        {
            typedef ast_leaf<i1, value1_t> leaf1_t;
            typedef ast_leaf<i2, value2_t> leaf2_t;
            static const bool value = leaf1_t::idx != leaf2_t::idx;
        };

        template <typename branch1_t, typename branch2_t>
        struct join
        {
            static_assert(is_unique<branch1_t, branch2_t>::value, "Element indeces not unique.");
            typedef typename branch<branch1_t, branch2_t> type;
        };
    };

}