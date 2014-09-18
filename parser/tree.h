#pragma once

#include "list.h"
#include <boost\rational.hpp>

namespace parse
{

	namespace tree
	{
        // This structure stores information that is used for analyzing parse 
        // failures.
        template <iterator_t>
        struct ast_analysis
        {
            ast_base<iterator_t>* best_partial;

            ast_analysis() : best_partial(nullptr)
            {
            }

            template <typename t1, typename t2>
            static void analyze(sequence<t1, t2>& sequence)
            {
                analyze(first);
                analyze(second);

                best_partial = better_partial(
                    first.analysis.best_partial,
                    second.analysis.best_partial);
            }
        };

		// Base class for all AST's.  Contains information about where in the token 
		// stream the parser matched and the size, in tokens.
		template <typename iterator_t>
		struct ast_base
		{
			iterator_t start;
			iterator_t end;
			bool matched;
            bool parsed;

            ast_analysis analysis;

			ast_base() : matched(false), parsed(false)
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
                size_t get_size()
                {
                    return find_last_match(*this).get_size();
                }

                size_t get_match_size()
                {
                    return find_last_match(*this).get_match_size();
                }
			};
		};

        template <typename parser_t, typename iterator_t>
        struct grouped : ast_base<iterator_t>
        {
            typename parser_t::template ast<iterator_t>::type group;

            size_t get_size() { return group.get_size(); }
        };

        template <typename parser_t, typename iterator_t>
        struct repetition : public ast_base<iterator_t>
        {
            typedef std::vector< typename parser_t::template ast<iterator_t>::type > container_type;
            container_type matches;
            container_type partial;

            size_t get_size()
            {
                size_t size;
                for (auto it = matches.begin(); it != matches.end(); it++)
                    size += it->get_size();

                for (auto it = partial.begin(); it != partial.end(); it++)
                    size += it->get_size();

                return size;
            }

            size_t get_match_size()
            {
                size_t size;
                for (auto it = matches.begin(); it != matches.end(); it++)
                    size += it->get_match_size();

                for (auto it = partial.begin(); it != partial.end(); it++)
                    size += it->get_match_size();

                return size;
            }
        };

        template <typename token_t, typename iterator_t>
        struct single : public tree::ast_base<iterator_t>
        {
            token_t token;

            size_t get_size() { return 1; }

            size_t get_match_size() { return matched ? 1 : 0; }
        };

        template <typename parser_t, typename iterator_t>
        struct optional : public tree::ast_base<iterator_t>
        {
            typename parser_t::template ast<iterator_t>::type option;

            size_t get_size() { return option.matched ? option.get_size() : 0; }
        };

        template <typename parser_t, typename iterator_t>
        struct reference : public tree::ast_base<iterator_t>
        {
            std::shared_ptr<typename parser_t::template ast<iterator_t>::type> ptr;

            size_t get_size()
            {
                return ptr.get() == nullptr ? 0 : (*ptr).get_size();
            }
        };

        // This function selects the best partial among two choices.  If 
        // either is a full match, it returns the other one (if both, it
        // returns an arbitrary selection).
        template <typename iterator_t>
        ast_base* better_partial(ast_base<iterator_t>* a1, ast_base<iterator_t>* a2)
        {
            assert(a1 != nullptr && a1->matched);
            assert(a2 != nullptr && a2->matched);
            
            if (a1 == nullptr) return a2;
            else if (a2 == nullptr) return a1;
            else
            {
                // Choose the partial with the highest ratio of match size to 
                // total size.  TODO: this can overflow, so probably need a 
                // better numerical method.
                return a1->size * a2->match_size > a2->size * a1->match_size ?
                    a1 : a2;
            }
        }

        // These functions attempt to pick the "best" match among the 
        // possibilities for the token following the last successfull match.
        // The return value is a partially matching AST if one exists, or a 
        // fully matching one otherwise.
        
        template <typename t1, typename t2>
        ast_base& find_best_partial(sequence<t1, t2>& sequence)
        {
            // Take the best partial among those returned by all the elements.
            auto& partial1 = find_best_partial(first);
            auto& partial2 = find_best_partial(second);

            return better_partial(partial1, partial2);
        }

        template <typename t1, typename t2>
        ast_base& find_best_partial(alternate<t1, t2>& alternate)
        {
            // Take the best partial among those returned by all the alternates.
            auto& partial1 = find_best_partial(first);
            auto& partial2 = find_best_partial(second);

            return better_match(partial1, partial2);
        }

        template <typename parser_t, typename stream_t>
		typename parser_t::template ast< typename stream_t::iterator >::type make_ast(parser_t& p, stream_t& s)
		{
			return typename parser_t::ast<typename stream_t::iterator>::type();
		}

	}

}