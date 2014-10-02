#pragma once

#include <vector>
#include <string>
#include <memory>
#include "tree.h"
#include "placeholders.h"

namespace parse
{
    template <typename t>
    struct always_false { enum { value = false }; };

    template <typename parser_t>
    struct debug_tag { static const char* name() { return "unknown"; } };

    template <typename parser_t, size_t i>
    struct captured_parser;

    // This meta-function is convenient for creating AST's.  It returns the 
    // type of an AST given the parser and iterator type.
    template <typename parser_t, typename iterator_t>
    struct parser_ast
    {
        typedef typename parser_t::template get_ast<iterator_t>::type type;
    };

    // This meta-function returns true if the specified parser is a 
    // tree::leaf or a tree::branch.  It is used in generating static AST 
    // data structures using sequence and/or alternate expressions (i.e., 
    // ">>", "|" operators).
    template <typename parser_t, typename iterator_t>
    struct has_tree_ast
    {
        typedef typename parser_ast<parser_t, iterator_t>::type ast_type;
        static const bool value = tree::is_tree<ast_type>::value;
    };

    // Base class for all parsers.  Provides parse_from methods that handle 
    // returning the iterator to the original position if the parser doesn't 
    // match.
    template <typename derived_t>
    struct parser
    {
        typedef derived_t derived_type;

        // Most parsers have variable-length matches, but those that have 
        // fixed, length 1 matches can hide this member in order to support 
        // the compliment (~) operator.
        static const bool is_single = false;
        
        // Most parsers don't capture anything by default, but those that do 
        // (e.g., captured_parser) can hide this member with their own 
        // implementation that returns a non-void AST type.
        template <typename iterator_t>
        struct get_ast { typedef void type; };

        // 3-parameter parse_from() only valid if the parser is captured.
        // it is set to the end of the match, which may be a partial if the
        // function returns false.
        template <typename iterator_t, typename ast_t>
        static bool parse_from(iterator_t& it, iterator_t& end, ast_t& a)
        {
            auto start = it;
            if (!derived_t::parse_internal(it, end, a))
            {
                it = start;
                return false;
            }
            else return true;
        }

        // 2-parameter parse_from() always available for parsing without 
        // generating an AST.
        template <typename iterator_t>
        static bool parse_from(iterator_t& it, iterator_t& end)
        {
            auto start = it;
            if (!derived_t::parse_internal(it, end))
            {
                it = start;
                return false;
            }
            else return true;
        }

        // Operator overload for capturing parser result into an AST.
        template <size_t i>
        captured_parser<derived_t, i> operator[] (const placeholders::index<i>& ph)
        {
            return captured_parser<derived_t, i>();
        }
    };

    // This parser captures its matching input into a leaf of an AST
    template <typename parser_t, size_t i>
    struct captured_parser
        : parser<captured_parser<parser_t, i> >
    {
        static const size_t key = i;

        typedef parser<captured_parser<parser_t, i> > base_type;
        typedef parser_t parser_type;

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename parser_ast<parser_t, iterator_t>::type parser_ast_type;
            typedef tree::leaf<i, tree::base<iterator_t, parser_ast_type> > type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return parser_t::parse_from(start, end);
        }

        template <typename iterator_t>
        static typename std::enable_if<!std::is_void<typename parser_ast<parser_t, iterator_t>::type>::value, bool>::type
            parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& a)
        {
            a.value.start = a.value.end = start;
            a.value.matched = parser_t::parse_from(start, end, a.value);
            a.value.end = start;
            a.value.parsed = true;
            return a.value.matched;
        }

        template <typename iterator_t>
        static typename std::enable_if<std::is_void<typename parser_ast<parser_t, iterator_t>::type>::value, bool>::type
            parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& a)
        {
            a.value.start = a.value.end = start;
            a.value.matched = parser_t::parse_from(start, end);
            a.value.end = start;
            a.value.parsed = true;
            return a.value.matched;
        }
    };

    // This meta-function is used to get the token_type of an alternate 
    // parser.  If the alternate is not a single token itself, it resolves to 
    // void.
    template <typename parser_t, bool single>
    struct token_type { typedef void type; };

    template <typename parser_t>
    struct token_type<parser_t, true> { typedef typename parser_t::token_type type; };

    // Creates an AST branch by joining the AST's from two parsers
    template <typename t1, typename t2, typename iterator_t>
    struct joined_ast
    {
        typedef typename tree::join<
            typename parser_ast<t1, iterator_t>::type,
            typename parser_ast<t2, iterator_t>::type
        >::type type;
    };

    // Creates an AST branch given two types.  If they are both branches 
    // (leaves or branches), returns a joined branch.  Otherwise, returns 
    // one of the two arguments that is a branch (or leaf) or void if 
    // neither is a branch.
    template <typename t1, typename t2, typename iterator_t, typename enable = void>
    struct make_branch_ast;

    template <typename t1, typename t2, typename iterator_t>
    struct make_branch_ast<t1, t2, iterator_t, typename std::enable_if<has_tree_ast<t1, iterator_t>::value && has_tree_ast<t2, iterator_t>::value>::type>
    {
        typedef typename joined_ast<t1, t2, iterator_t>::type type;
    };

    template <typename t1, typename t2, typename iterator_t>
    struct make_branch_ast<t1, t2, iterator_t, typename std::enable_if<!has_tree_ast<t1, iterator_t>::value && has_tree_ast<t2, iterator_t>::value>::type>
    {
        typedef typename parser_ast<t2, iterator_t>::type type;
    };

    template <typename t1, typename t2, typename iterator_t>
    struct make_branch_ast<t1, t2, iterator_t, typename std::enable_if<has_tree_ast<t1, iterator_t>::value && !has_tree_ast<t2, iterator_t>::value>::type>
    {
        typedef typename parser_ast<t1, iterator_t>::type type;
    };

    template <typename t1, typename t2, typename iterator_t>
    struct make_branch_ast<t1, t2, iterator_t, typename std::enable_if<!has_tree_ast<t1, iterator_t>::value && !has_tree_ast<t2, iterator_t>::value>::type>
    {
        typedef void type;
    };

    // A parser that matches if either of the two supplied parsers match.  The 
    // second parser won't be tried if the first matches.  This class supports
    // general parser alternates, and also has special support for single 
    // token alternates.
    template <typename t1, typename t2>
    struct alternate
        : parser<alternate<t1, t2> >
    {
        typedef t1 left_type;
        typedef t2 right_type;

        static const bool is_single = t1::is_single && t2::is_single;

        typedef typename token_type<t1, is_single>::type token_type;

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename make_branch_ast<t1, t2, iterator_t>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return t1::parse_from(start, end) ||
                t2::parse_from(start, end);
        }

        template <typename iterator_t, typename ast_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, ast_t& a)
        {
            return parse_internal_map<
                iterator_t, 
                has_tree_ast<t1, iterator_t>::value,
                has_tree_ast<t2, iterator_t>::value
            >::parse_internal(start, end, a);
        }

        template <typename iterator_t, bool left, bool right>
        struct parse_internal_map;

        template <typename iterator_t>
        struct parse_internal_map<iterator_t, true, true>
        {
            template <typename iterator_t>
            static typename std::enable_if<has_tree_ast<t1, iterator_t>::value && has_tree_ast<t2, iterator_t>::value, bool>::type
                parse_internal(iterator_t& start, iterator_t& end, typename joined_ast<t1, t2, iterator_t>::type& a)
            {
                return t1::parse_from(start, end, a.left()) ||
                    t2::parse_from(start, end, a.right());
            }
        };

        template <typename iterator_t>
        struct parse_internal_map<iterator_t, true, false>
        {
            template <typename iterator_t>
            static bool parse_internal(iterator_t& start, iterator_t& end, typename parser_ast<t1, iterator_t>::type& a)
            {
                return t1::parse_from(start, end, a) ||
                    t2::parse_from(start, end);
            }
        };

        template <typename iterator_t>
        struct parse_internal_map<iterator_t, false, true>
        {
            template <typename iterator_t>
            static bool parse_internal(iterator_t& start, iterator_t& end, typename parser_ast<t2, iterator_t>::type& a)
            {
                return t1::parse_from(start, end) ||
                    t2::parse_from(start, end, a);
            }
        };

        // This should only be called if both first and second are single 
        // token parsers.
        template <typename token_t>
        static bool match(token_t t)
        {
            return t1::match(t) || t2::match(t);
        }
    };

    // A parser that matches only if both of the given parsers match in 
    // sequence.
    template <typename t1, typename t2>
    struct sequence
        : parser<sequence<t1, t2> >
    {
        typedef t1 left_type;
        typedef t2 right_type;

        static const int sequencetype = 1;

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename make_branch_ast<t1, t2, iterator_t>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return t1::parse_from(start, end) &&
                t2::parse_from(start, end);
        }

        template <typename iterator_t, typename ast_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, ast_t& a)
        {
            return parse_internal_map<
                iterator_t, 
                has_tree_ast<t1, iterator_t>::value,
                has_tree_ast<t2, iterator_t>::value
            >::parse_internal(start, end, a);
        }

        template <typename iterator_t, bool left, bool right>
        struct parse_internal_map;

        template <typename iterator_t>
        struct parse_internal_map<iterator_t, true, true>
        {
            template <typename iterator_t>
            static bool parse_internal(iterator_t& start, iterator_t& end, typename joined_ast<t1, t2, iterator_t>::type& a)
            {
                return t1::parse_from(start, end, a.left()) &&
                    t2::parse_from(start, end, a.right());
            }
        };

        template <typename iterator_t>
        struct parse_internal_map<iterator_t, true, false>
        {
            template <typename iterator_t>
            static bool parse_internal(iterator_t& start, iterator_t& end, typename parser_ast<t1, iterator_t>::type& a)
            {
                return t1::parse_from(start, end, a) &&
                    t2::parse_from(start, end);
            }
        };

        template <typename iterator_t>
        struct parse_internal_map<iterator_t, false, true>
        {
            template <typename iterator_t>
            static bool parse_internal(iterator_t& start, iterator_t& end, typename parser_ast<t2, iterator_t>::type& a)
            {
                return t1::parse_from(start, end) &&
                    t2::parse_from(start, end, a);
            }
        };
    };

    // Matches the specified parser zero or more times.  The stream is checked 
    // for eof first, which is still considered a match.  If the unerlying 
    // parser returns a zero-length match, the iteration is stopped.  This 
    // assumes that subsequent parse attempts would continue to produce 
    // zero-length matches and result in an infinite loop.  If state-ful 
    // parsers are to be supported, this behavior could be changed.
    template <typename parser_t>
    struct zero_or_more : public parser< zero_or_more<parser_t> >
    {
        template <typename iterator_t, typename parser_ast_t>
        struct create_ast
        {
            typedef tree::repetition<parser_ast_t, iterator_t> type;
        };

        template <typename iterator_t>
        struct create_ast<iterator_t, void>
        {
            typedef void type;
        };

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename create_ast<iterator_t, typename parser_ast<parser_t, iterator_t>::type>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& ast)
        {
            typedef typename parser_ast<parser_t, iterator_t>::type ast_t;

            while (start != end)
            {
                ast_t partial;
                auto tmp = start;
                if (start == end || !parser_t::parse_from(start, end, partial) || start == tmp)
                {
                    ast.partial = partial;
                    break;
                }
                else
                {
                    ast.matches.push_back(partial);
                }
            }
            return true;
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            while (start != end)
            {
                auto tmp = start;
                if (start == end || !parser_t::parse_from(start, end) || start == tmp)
                    break;
            }
            return true;
        }
    };

    // This parser matches if the underlying parser matches a number of 
    // times that is between a constant minimum and maximum.
    template <typename parser_t, size_t min, size_t max = SIZE_MAX>
    struct repetition : public parser< repetition<parser_t, min, max> >
    {
        template <typename iterator_t, typename parser_ast_t>
        struct create_ast
        {
            typedef tree::repetition<parser_ast_t, iterator_t> type;
        };

        template <typename iterator_t>
        struct create_ast<iterator_t, void>
        {
            typedef void type;
        };

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename create_ast<iterator_t, typename parser_ast<parser_t, iterator_t>::type>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& tree)
        {
            typedef typename parser_t::template ast<iterator_t>::type ast_t;

            size_t i;
            for (i = 0; i < min; i++)
            {
                ast_t partial;
                if (!parser_t::parse_from(start, end, partial))
                {
                    tree.partial = partial;
                    return false;
                }
                tree.matches.push_back(partial);
            }

            for (; i < max; i++)
            {
                ast_t partial;
                if (start == end) break;
                if (!parser_t::parse_from(start, end, partial) || partial.start == partial.end)
                {
                    tree.partial = partial;
                    break;
                }
                else tree.matches.push_back(partial);
            }
            return true;
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            size_t i;
            for (i = 0; i < min; i++)
            {
                if (!parser_t::parse_from(start, end))
                    return false;
            }

            for (; i < max; i++)
            {
                if (start == end) break;
                auto tmp = start;
                if (!parser_t::parse_from(start, end) || tmp == start)
                    break;
            }
            return true;
        }
    };

    // This parser tries to match the underlying parser, but returns true 
    // even if it doesn't.
    template <typename parser_t>
    struct optional : public parser< optional<parser_t> >
    {
        template <typename iterator_t>
        struct get_ast
        {
            typedef typename parser_ast<parser_t, iterator_t>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            if (start != end) parser_t::parse_from(start, end);
            return true;
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& tree)
        {
            if (start != end) parser_t::parse_from(start, end, tree);
            return true;
        }
    };

    // This is the base class for all parsers that match a single token.  This 
    // class handles reading the single token, and calls the derived class's 
    // match() method to determine whether the token matches.  The token type, 
    // token_t, can be any type with a parameterless constructor.  Also, the 
    // stream_t must return values that are assignable to token_t.
    template <typename derived_t, typename token_t>
    struct single : public parser< single<derived_t, token_t> >
    {
        typedef token_t token_type;

        static const bool is_single = true;

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return start != end && derived_t::match(*start++);
        }

        static bool match(token_t t) { return derived_t::match(t); }
    };

    // A parser that matches a single token that is anything other the what 
    // the parser_t type matches.  This cannot be used with variable-length 
    // (in tokens) parsers.
    template <typename parser_t, typename token_t>
    struct complement : public single< complement<parser_t, token_t>, token_t >
    {
    public:
        static bool match(char c)
        {
            return !parser_t::match(c);
        }
    };

    // A parser that matches a single token, in cases where the token_t type is 
    // integral (e.g., a char, wchar_t, etc., representing a character).
    template <typename token_t, token_t t>
    struct constant : public single< constant<token_t, t>, token_t >
    {
    public:
        static bool match(token_t token)
        {
            return token == t;
        }
    };

    // This parser is used to make recursive parsers that refer to 
    // themselves, either directly or via some other sub-parser.  The 
    // parser_t parameter can be an incomplete type.
    template <typename parser_t>
    struct reference : public parser< reference<parser_t> >
    {
        template <typename iterator_t>
        struct get_ast
        {
            typedef typename tree::reference<parser_t, iterator_t> type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return parser_t::parse_from(start, end);
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& tree)
        {
            typedef typename parser_ast<parser_t, iterator_t>::type p_tree;
            return parser_t::parse_from(start, end, tree.get());
        }
    };

    // This parser matches only if the first_t parser matches and the 
    // second_t doesn't (from the same location).  This supports single
    // token parsing if both types are also single.
    template <typename t1, typename t2>
    struct difference 
        : public parser< difference<t1, t2> >
    {
        static const bool is_single = t1::is_single && t2::is_single;

        template <typename iterator_t>
        struct get_ast
        {
            typedef typename t1::template get_ast<iterator_t>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename get_ast<iterator_t>::type& tree)
        {
            iterator_t tmp = start;
            typename t2::ast<iterator_t>::type second_tree;
            
            return 
                t1::parse_from(start, end, tree) && 
                !t2::parse_from(tmp, end, second_tree);
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            iterator_t tmp = start;
            return 
                t1::parse_from(start, end) && 
                !t2::parse_from(tmp, end);
        }

        template <typename token_t>
        static bool match(token_t t)
        {
            return first.match(t) && !second.match(t);
        }
    };

    // This namespace defines operators that are useful in cosntructing parsers 
    // using a convenient syntax (shorter than a complicated, nested template 
    // instantiation).
    namespace operators
    {
        using namespace placeholders;

        // Generates an alternate<first_t, second_t> parser
        template <typename first_t, typename second_t>
        alternate<first_t, second_t> operator| (const parser<first_t>& first, const parser<second_t>& second)
        {
            return alternate<first_t, second_t>();
        }

        // Generates a sequence<first_t, second_t> parser
        template <typename first_t, typename second_t>
        sequence<first_t, second_t> operator>> (const parser<first_t>& first, const parser<second_t>& second)
        {
            return sequence<first_t, second_t>();
        }

        // Generates a zero_or_more<parser_t> parser
        template <typename parser_t>
        zero_or_more<parser_t> operator* (parser<parser_t>& parser)
        {
            return zero_or_more<parser_t>();
        }

        // Generates an optional parser
        template <typename parser_t>
        optional<parser_t> operator! (const parser<parser_t>& parser)
        {
            return optional<parser_t>();
        }

        // Generates a "one or more" (repetition<parser_t, 1>) parser
        template <typename parser_t>
        repetition<parser_t, 1> operator+ (const parser<parser_t>& parser)
        {
            return repetition<parser_t, 1>();
        }

        // Generates a complement parser (only for single-token sub-parsers)
        template <typename parser_t>
        complement< parser_t, typename parser_t::token_type > operator~ (const parser<parser_t>& parser)
        {
            return complement< parser_t, typename parser_t::token_type >();
        }

        // Generates a difference parser
        template <typename first_t, typename second_t>
        difference<first_t, second_t> operator- (const parser<first_t>& f, const parser<second_t>& s)
        {
            return difference<first_t, second_t>();
        }
    }

    // This namespace contains parsers that match single unicode characters.
    namespace terminals
    {

        // Matches a specific unicode character
        template <char32_t t>
		struct u : constant<unsigned int, t>
		{
		};

		// Matches any character that would be considered a digit by isdigit().
        struct digit : single<digit, char32_t>
		{
			static bool match(char32_t t)
			{
				return isdigit(t) != 0;
			}
		};

		// Matches any character that would be considered an alpha by isalpha().
        struct alpha : single<alpha, char32_t>
		{
			static bool match(char32_t t)
			{
				return isalpha(t) != 0;
			}
		};

        // Matches any character that would be considered whitespace by isspace().
        struct space : public single<space, char32_t>
        {
            static bool match(char32_t t)
            {
                return isspace(t) != 0;
            }
        };

        // Matches any character
        struct any : single<space, char32_t>
        {
            static bool match(char32_t) { return true; }
        };
    
    }

}
