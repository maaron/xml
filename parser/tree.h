#pragma once

#include "list.h"

namespace parse
{

	namespace tree
	{

		// Base class for all AST's.  Contains information about where in the token 
		// stream the parser matched and the size, in tokens.
		template <typename iterator_t>
		struct ast_base
		{
			iterator_t start;
			iterator_t end;
			bool matched;

			ast_base() : matched(false)
			{
			}

			std::basic_string<typename iterator_t::value_type> to_string()
			{
				return std::basic_string<typename iterator_t::value_type>(start, end);
			}
		};

		template <typename iterator_t>
		struct ast_list
		{
			template <typename t1, typename t2>
			struct sequence :
				util::list<sequence, t1, t2>,
				ast_base<iterator_t>
			{
			};

			template <typename t1, typename t2>
			struct alternate :
                util::list<alternate, t1, t2>,
				ast_base<iterator_t>
			{
			};
		};

		template <typename parser_t, typename stream_t>
		typename parser_t::template ast< typename stream_t::iterator >::type make_ast(parser_t& p, stream_t& s)
		{
			return typename parser_t::ast<typename stream_t::iterator>::type();
		}

	}

}