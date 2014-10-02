#pragma once

#include "placeholders.h"

namespace list
{
    // The element<...> class implements compile-time functionality similar 
    // in nature to std::map<int, ...>, except that the type of each element 
    // can be different.  A list is constructed as an ineheritance chain, 
    // where the most derived class represents the front of the list, and the 
    // base class represents the back.
    template <size_t i, typename elem_t, typename next_t>
    struct element : next_t
    {
        typedef elem_t head_type;
        typedef element<i, elem_t, next_t> self_type;
        typedef element<i, elem_t, void> self_removed_type;
        typedef next_t tail_type;

        static const size_t key = i;
        elem_t elem;

        // Meta-function that returns whether or not the key is contained in 
        // the list.
        template <size_t i>
        struct has_key
        {
            static const bool value = (i == key || next_t::template has_key<i>::value);
        };

        // Meta-function that returns the element type identified by the specified key.
        template <size_t i>
        struct elem_type
        {
            typedef typename next_t::template elem_type<i>::type type;
            static_assert(!std::is_same<type, void>::value, "Element index out of range");
        };
        template <> struct elem_type<key> { typedef elem_t type; };

        // Returns the element<...> class identified by the specified integer key.
        template <size_t i>
        struct base_type
        {
            typedef typename next_t::template base_type<i>::type type;
            static_assert(!std::is_same<type, void>::value, "Element index out of range");
        };
        template <> struct base_type<key> { typedef self_type type; };

        // Returns a reference to the element identified by the index<i> 
        // placeholder argument.
        template <size_t i>
        typename elem_type<i>::type& at(const placeholders::index<i>&)
        {
            return typename base_type<i>::type::elem;
        }
    };

    // Specialization used for the back of a list.
    template <size_t i, typename elem_t>
    struct element<i, elem_t, void>
    {
        typedef element<i, elem_t, void> self_type;
        typedef elem_t head_type;

        static const size_t key = i;
        elem_t elem;

        template <size_t i>
        struct has_key
        {
            static const bool value = (i == key);
        };

        template <size_t i>
        struct elem_type
        {
            typedef typename std::conditional<
                i == key, 
                elem_t, 
                void
            >::type type;
        };

        template <size_t i>
        struct base_type
        {
            typedef typename std::conditional<
                i == key, 
                self_type, 
                void
            >::type type;
        };

        elem_t& at(const placeholders::index<key>&)
        {
            return elem;
        }
    };

    // Meta-function that creates a 1-element list type
    template <size_t i, typename elem_t>
    struct make_list
    {
        typedef element<i, elem_t, void> type;
    };

    // Meta-function that adds a new element with the given integer key to 
    // the front of the list.
    template <typename list_t, size_t i, typename elem_t>
    struct push_front
    {
        typedef element<i, elem_t, list_t> type;
    };

    // Meta-function that adds a new element with the given integer key to 
    // the back of the list.
    template <typename list_t, size_t i, typename elem_t>
    struct push_back
    {
        typedef typename push_back<typename list_t::tail_type, i, elem_t>::type tail;
        typedef typename push_front<tail, list_t::key, typename list_t::head_type>::type type;
    };
    template <size_t i_end, typename elem_end_t, size_t i, typename elem_t>
    struct push_back<element<i_end, elem_end_t, void>, i, elem_t>
    {
        typedef typename push_front<element<i, elem_t, void>, i_end, elem_end_t>::type type;
    };

    // Concatenates two lists together.  A static assertion is the indeces 
    // for each list aren't unique.
    template <typename list1_t, typename list2_t>
    struct concat
    {
        static_assert(!list1_t::template has_key<list2_t::key>::value, "Element indeces not unique");
        typedef typename push_back<list1_t, list2_t::key, typename list2_t::head_type>::type append1;
        typedef typename concat<append1, typename list2_t::tail_type>::type type;
    };
    template <typename list1_t, size_t i, typename elem_t>
    struct concat<list1_t, element<i, elem_t, void> >
    {
        static_assert(!list1_t::template has_key<i>::value, "List already contains an element with the specified index");
        typedef typename push_back<list1_t, i, elem_t>::type type;
    };

    // Meta-function that returns a new list type with all the element types 
    // modified using the specified meta-function.
    template <typename list_t, template <typename> class type_function>
    struct map
    {
        typedef typename type_function<typename list_t::head_type>::type first_mapped;
        typedef typename map<typename list_t::tail_type, type_function>::type tail_mapped;
        typedef typename push_front<tail_mapped, list_t::key, first_mapped>::type type;
    };
    template <size_t i, typename elem_t, template <typename> class type_function>
    struct map< element<i, elem_t, void>, type_function >
    {
        typedef typename type_function<elem_t>::type mapped_type;
        typedef typename make_list<i, mapped_type>::type type;
    };

    // Meta-function that returns true/false, indicating whether the 
    // specified type is a list.
    template <typename not_a_list_t>
    struct is_list : std::false_type {};

    template <typename size_t i, typename elem_t, typename next_t>
    struct is_list< element<i, elem_t, next_t> > : std::true_type {};

}