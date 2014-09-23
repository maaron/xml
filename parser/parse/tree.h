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
            typedef iterator_t iterator;

			iterator_t start;
			iterator_t end;
			bool matched;
            bool parsed;
            const char* tag;

			ast_base() : matched(false), parsed(false)
			{
			}

            std::basic_string<typename iterator_t::value_type> to_string()
            {
                return std::basic_string<typename iterator_t::value_type>(start, end);
            }
		};

		template <typename iterator_t, typename t1, typename t2>
		struct sequence :
			util::list<sequence, iterator_t, t1, t2>,
			ast_base<iterator_t>
		{
            typedef t1 first_t;
            typedef t2 second_t;
		};

		template <typename iterator_t, typename t1, typename t2>
		struct alternate :
            util::list<alternate, iterator_t, t1, t2>,
			ast_base<iterator_t>
		{
            typedef t1 first_t;
            typedef t2 second_t;
		};

        template <typename parser_t, typename iterator_t>
        struct grouped : ast_base<iterator_t>
        {
            typename parser_t::template ast<iterator_t>::type group;
        };

        template <typename parser_t, typename iterator_t>
        struct repetition : public ast_base<iterator_t>
        {
            typedef typename parser_t::template ast<iterator_t>::type parser_ast_t;
            typedef std::vector<parser_ast_t> container_type;
            container_type matches;
            parser_ast_t partial;
        };

        template <typename token_t, typename iterator_t>
        struct single : public tree::ast_base<iterator_t>
        {
            token_t token;
        };

        template <typename parser_t, typename iterator_t>
        struct optional : public tree::ast_base<iterator_t>
        {
            typename parser_t::template ast<iterator_t>::type option;
        };

        template <typename parser_t, typename iterator_t>
        struct reference : public tree::ast_base<iterator_t>
        {
            std::shared_ptr<typename parser_t::template ast<iterator_t>::type> ptr;
        };

        // This function looks for a terminal that didn't match at a given 
        // location.
        template <typename iterator_t, typename t1>
        iterator_t last_match(grouped<t1, iterator_t>& g)
        {
            assert(g.parsed);
            auto name = g.tag;
            return last_match(g.group);
        }

        template <typename iterator_t, typename t1>
        iterator_t last_match(optional<t1, iterator_t>& opt)
        {
            assert(opt.parsed);
            auto name = opt.tag;
            return last_match(opt.option);
        }

        template <typename iterator_t, typename t1>
        iterator_t last_match(reference<t1, iterator_t>& ref)
        {
            assert(ref.parsed);
            auto name = ref.tag;
            return ref.ptr.get() == nullptr ? ref.start : last_match(*ref.ptr);
        }

        template <typename iterator_t, typename t1, typename t2>
        iterator_t last_match(sequence<iterator_t, t1, t2>& seq)
        {
            assert(seq.parsed);
            auto name = seq.tag;
            return seq.first.matched ?
                max(last_match(seq.first), last_match(seq.second)) :
                last_match(seq.first);
        }

        template <typename t1, typename t2, typename iterator_t>
        iterator_t last_match(alternate<iterator_t, t1, t2>& alt)
        {
            assert(alt.parsed);
            auto name = alt.tag;
            if (alt.second.parsed) 
                return std::max(last_match(alt.first), last_match(alt.second));
            else
                return last_match(alt.first);
        }

        template <typename t1, typename iterator_t>
        iterator_t last_match(repetition<t1, iterator_t>& rep)
        {
            assert(rep.parsed);
            auto name = rep.tag;
            if (rep.partial.parsed) return last_match(rep.partial);
            else if (rep.matches.size() > 0) return last_match(rep.matches.back());
            else return rep.start;
        }

        template <typename iterator_t, typename token_t>
        iterator_t last_match(single<token_t, iterator_t>& s)
        {
            assert(s.parsed);
            auto name = s.tag;
            return s.start;
        }

        template <typename parser_t, typename stream_t>
		typename parser_t::template ast< typename stream_t::iterator >::type make_ast(parser_t& p, stream_t& s)
		{
			return typename parser_t::ast<typename stream_t::iterator>::type();
		}

	}

}