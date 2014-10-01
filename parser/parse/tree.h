#pragma once

#include "list.h"
#include <memory>

namespace parse
{

	namespace tree
	{

        // This class contains the information common to all AST leaf 
        // nodes.  It is used to track the start/end positions of a 
        // parser's match.
        template <typename iterator_t, typename base_t = void>
        struct base : base_t
	    {
            typedef iterator_t iterator;

		    iterator_t start;
		    iterator_t end;
		    bool matched;
            bool parsed;

		    base() : matched(false), parsed(false)
		    {
		    }
	    };

        template <typename iterator_t>
        struct base<iterator_t, void>
	    {
            typedef iterator_t iterator;

		    iterator_t start;
		    iterator_t end;
		    bool matched;
            bool parsed;

		    base() : matched(false), parsed(false)
		    {
		    }
	    };

        // A branch is a container for two other branch/leaf nodes.
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
            struct get_leaf_type
            {
                typedef typename left_type::template get_leaf_type<i>::type left_base;
                typedef typename right_type::template get_leaf_type<i>::type right_base;
                typedef typename std::conditional<std::is_void<left_base>::value, right_base, left_base>::type type;
            };

            template <size_t i>
            typename get_leaf_type<i>::type::value_type& operator[] (const placeholders::index<i>& ph)
            {
                typedef typename get_leaf_type<i>::type leaf_type;
                static_assert(!std::is_void<leaf_type>::value, "Element index out of range");
                return leaf_type::value;
            }

            left_type& left() { return static_cast<left_type&>(*this); }
            right_type& right() { return static_cast<right_type&>(*this); }
        };

        // A leaf node in an AST.
        template <size_t i, typename value_t>
        struct leaf
        {
            typedef leaf<i, value_t> self_type;
            typedef value_t value_type;

            value_type value;

            static const size_t idx = i;

            template <size_t i>
            struct has_key
            {
                static const bool value = i == idx;
            };

            template <size_t i> struct get_leaf_type { typedef void type; };
            template <> struct get_leaf_type<idx> { typedef self_type type; };

            value_type& operator[] (const placeholders::index<idx>&)
            {
                return value;
            }
        };

        // This meta-function returns true if the supplied type is a branch 
        // or a leaf.
        template <typename t>
        struct is_tree { static const bool value = false; };

        template <typename t1, typename t2>
        struct is_tree<branch<t1, t2> > { static const bool value = true; };

        template <size_t i, typename t>
        struct is_tree<leaf<i, t> > { static const bool value = true; };

        // This meta-function returns a bool indicating whether the branch 
        // contains a given key.
        template <typename branch_t, size_t key>
        struct contains_key;

        template <typename left_t, typename right_t, size_t key>
        struct contains_key<branch<left_t, right_t>, key>
        {
            static const bool value = 
                contains_key<left_t, key>::value ||
                contains_key<right_t, key>::value;
        };
        template <size_t i, typename value_t, size_t key>
        struct contains_key<leaf<i, value_t>, key>
        {
            static const bool value = i == key;
        };

        // This meta-function is used to determine whether two AST's have 
        // common indeces.
        template <typename branch1_t, typename branch2_t>
        struct is_unique
        {
            static const bool value =
            is_unique<typename branch1_t::left_type, branch2_t>::value &&
            is_unique<typename branch1_t::right_type, branch2_t>::value;
        };
        template <size_t i, typename value_t, typename branch_t>
        struct is_unique<leaf<i, value_t>, branch_t>
        {
            static const bool value = !contains_key<branch_t, i>::value;
        };
        template <typename branch_t, size_t i, typename value_t>
        struct is_unique<branch_t, leaf<i, value_t> >
        {
            static const bool value = !contains_key<branch_t, i>::value;
        };
        template <size_t i1, typename value1_t, size_t i2, typename value2_t>
        struct is_unique<leaf<i1, value1_t>, leaf<i2, value2_t> >
        {
            static const bool value = i1 != i2;
        };

        // This meta-function creates a new AST type by joining to AST's 
        // into a branch.  It also verifies that the indeces are unique.
        template <typename branch1_t, typename branch2_t>
        struct join
        {
            static_assert(is_unique<branch1_t, branch2_t>::value, "Element indeces not unique.");
            typedef typename branch<branch1_t, branch2_t> type;
        };

        template <typename parser_ast_t, typename iterator_t>
        struct repetition
        {
            typedef std::vector<parser_ast_t> container_type;
            container_type matches;
            parser_ast_t partial;
        };

        template <typename parser_t, typename iterator_t>
        struct optional
        {
            typename parser_t::template get_ast<iterator_t>::type option;
        };

        template <typename parser_t, typename iterator_t>
        struct reference
        {
            std::shared_ptr<typename parser_t::template get_ast<iterator_t>::type> ptr;
        };

        // This function looks for a terminal that didn't match at a given 
        // location.
        template <typename t1, typename iterator_t>
        iterator_t last_match(optional<t1, iterator_t>& opt)
        {
            assert(opt.parsed);
            return last_match(opt.option);
        }

        template <typename t1, typename iterator_t>
        iterator_t last_match(reference<t1, iterator_t>& ref)
        {
            assert(ref.parsed);
            return ref.ptr.get() == nullptr ? ref.start : last_match(*ref.ptr);
        }

        template <typename t1, typename iterator_t>
        iterator_t last_match(repetition<t1, iterator_t>& rep)
        {
            assert(rep.parsed);
            if (rep.partial.parsed) return last_match(rep.partial);
            else if (rep.matches.size() > 0) return last_match(rep.matches.back());
            else return rep.start;
        }

        template <typename branch_t>
        struct branch_iterator
        {
            typedef typename branch_iterator<typename branch_t::left_type>::type type;
        };

        template <size_t i, typename value_t>
        struct branch_iterator<leaf<i, value_t> >
        {
            typedef typename value_t::iterator type;
        };

        template <typename t1, typename t2>
        typename branch_iterator<t1>::type last_match(branch<t1, t2>& b)
        {
            return max(last_match(b.left()), last_match(b.right()));
        }

        template <size_t i, typename t1>
        typename branch_iterator<leaf<i, t1> >::type last_match(leaf<i, t1>& b)
        {
            return last_match(b.value);
        }

        template <typename iterator_t, typename base_t>
        iterator_t last_match(base<iterator_t, base_t>& b)
        {
            return b.matched ? b.end : b.start;
        }

        template <typename parser_t, typename stream_t>
		typename parser_t::template ast< typename stream_t::iterator >::type make_ast(parser_t& p, stream_t& s)
		{
			return typename parser_t::ast<typename stream_t::iterator>::type();
		}

	}

}