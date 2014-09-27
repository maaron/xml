#pragma once

#include "placeholders.h"

namespace util
{

	// Creates a two element list
	template <template <typename, typename, typename> class derived_t, typename iterator_t, typename first_t, typename second_t>
	struct list
	{
		first_t first;
		second_t second;

		static const size_t size = 2;

		template <size_t i> struct elem;
		template <> struct elem<0> { typedef first_t type; };
		template <> struct elem<1> { typedef second_t type; };

		first_t& operator[](const placeholders::index<0>&) { return first; }
        second_t& operator[](const placeholders::index<1>&) { return second; }
	};

	// Creates an N+1-element list by appending second_t to the N-element list 
	// list_t<t1, t2>.
	template <template <typename, typename, typename> class derived_t, typename iterator_t, typename t1, typename t2, typename second_t>
	struct list< derived_t, iterator_t, derived_t<iterator_t, t1, t2>, second_t >
	{
		typedef derived_t<iterator_t, t1, t2> first_t;

		first_t first;
		second_t second;

		static const size_t size = first_t::size + 1;

		template <size_t i>
		struct elem
		{
			static_assert(i < size, "Element type index out of range");
			typedef typename first_t::template elem<i>::type type;
		};

		template <>
		struct elem<size - 1> { typedef second_t type; };

        second_t& operator[](const placeholders::index<size - 1>&) { return second; }

		template <size_t i>
        typename elem<i>::type& operator[](const placeholders::index<i>& idx)
		{
			static_assert(i < size, "Element index out of range");
			return first[idx];
		}
	};

}
