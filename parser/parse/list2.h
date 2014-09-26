#pragma once

#include "placeholders.h"

namespace list
{
    using namespace placeholders;

    template <size_t i, typename elem_t, typename next_t>
    struct element : next_t
    {
        typedef elem_t head_type;
        typedef element<i, elem_t, next_t> self_type;
        typedef element<i, elem_t, void> self_removed_type;
        typedef next_t tail_type;

        static const size_t idx = i;
        elem_t elem;

        template <size_t i>
        struct has_idx
        {
            static const bool value = (i == idx || next_t::template has_idx<i>::value);
        };

        template <size_t i>
        struct elem_type
        {
            typedef typename next_t::template elem_type<i>::type type;
            static_assert(!std::is_same<type, void>::value, "Element index out of range");
        };

        template <>
        struct elem_type<idx>
        {
            typedef elem_t type;
        };

        template <size_t i>
        struct base_type
        {
            typedef typename next_t::template base_type<i>::type type;
            static_assert(!std::is_same<type, void>::value, "Element index out of range");
        };

        template <> struct base_type<idx> { typedef self_type type; };

        template <size_t i>
        typename elem_type<i>::type& at(const index<i>&)
        {
            return typename base_type<i>::type::elem;
        }
    };

    template <size_t i, typename elem_t>
    struct element<i, elem_t, void>
    {
        typedef element<i, elem_t, void> self_type;

        static const size_t idx = i;
        elem_t elem;

        template <size_t i>
        struct has_idx
        {
            static const bool value = (i == idx);
        };

        template <size_t i>
        struct elem_type
        {
            typedef typename std::conditional<
                i == idx, 
                elem_t, 
                void
            >::type type;
        };

        template <size_t i>
        struct base_type
        {
            typedef typename std::conditional<
                i == idx, 
                self_type, 
                void
            >::type type;
        };

        elem_t& at(const index<idx>&)
        {
            return elem;
        }
    };

    template <size_t i, typename elem_t>
    struct make_list
    {
        typedef element<i, elem_t, void> type;
    };

    template <typename list_t, size_t i, typename elem_t>
    struct push_front
    {
        typedef element<i, elem_t, list_t> type;
    };

    template <typename list_t, size_t i, typename elem_t>
    struct push_back
    {
        typedef typename push_back<typename list_t::tail_type, i, elem_t>::type tail;
        typedef typename push_front<tail, list_t::idx, typename list_t::head_type>::type type;
    };

    template <size_t i_end, typename elem_end_t, size_t i, typename elem_t>
    struct push_back<element<i_end, elem_end_t, void>, i, elem_t>
    {
        typedef typename push_front<element<i, elem_t, void>, i_end, elem_end_t>::type type;
    };

    template <typename list1_t, typename list2_t>
    struct concat
    {
        static_assert(!list1_t::has_idx<list2_t::idx>::value, "Element indeces not unique");
        typedef typename push_back<list1_t, list2_t::idx, typename list2_t::head_type>::type append1;
        typedef typename concat<append1, typename list2_t::tail_type>::type type;
    };

    template <typename list1_t, size_t i, typename elem_t>
    struct concat<list1_t, element<i, elem_t, void> >
    {
        static_assert(!list1_t::has_idx<i>::value, "List already contains an element with the specified index");
        typedef typename push_back<list1_t, i, elem_t>::type type;
    };
}