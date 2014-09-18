#pragma once

#include <vector>
#include <string>
#include <memory>
#include "tree.h"

namespace parse
{

    template <typename parser_t, typename iterator_t>
    struct ast_type
    {
        typedef typename parser_t::template ast<iterator_t>::type type;
    };

    struct parser_tag {};

    template <bool single>
    struct parser_traits
    {
        static const bool is_single = single;
        typedef parser_tag tag;
    };
      
    // Base class for all parsers.  The main method, parse(stream_t&, ast&) 
    // tracks the start and end positions of the underlying parser's match, and 
    // updates the AST accordingly.
    template <typename derived_t>
    class parser
    {
    public:
        template <typename iterator_t>
        bool parse_from(iterator_t& start, iterator_t& end, typename ast_type<derived_t, iterator_t>::type& ast)
        {
            auto matchEnd = start;

            if (static_cast<derived_t*>(this)->parse_internal(matchEnd, end, ast))
            {
                ast.start = start;
                start = ast.end = matchEnd;
                ast.parsed = true;
                return (ast.matched = true);
            }
            else
            {
                ast.parsed = true;
                return (ast.matched = false);
            }
        }

        template <typename stream_t>
        bool parse(stream_t& s, typename ast_type<derived_t, typename stream_t::iterator>::type& ast)
        {
            return parse_from(s.begin(), s.end(), ast);
        }
    };

    // This parser is useful for collecting series of alternates or sequences
    // into logical groups.  Without using this class, a sequence created with
    // an expression like "a >> b >> c", will generate a class with an AST type
    // that has three children.  If what you really want is "(a >> b) >> c",
    // i.e., a sequence of two elements, you must enclose the first
    // sub-sesquence in a group classm like this: 
    // 
    //   auto elem1 = grouped<decltype(a >> b)>(); // Or group(a >> b), from parse::operators namespace
    //   auto pair = elem1 >> c;
    template <typename parser_t>
    class grouped : public parser< grouped<parser_t> >, public parser_traits<parser_t::is_single>
    {
        parser_t parser;

    public:
        template <typename iterator_t>
        struct ast
        {
            typedef tree::grouped<parser_t, iterator_t> type;
        };

        grouped()
        {
        }

        grouped(const parser_t& p) : parser(p)
        {
        }

        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return parser.parse_from(start, end, tree.group);
        }
    };

    // This meta-function is used to get the token_type of an alternate 
    // parser.  If the alternate is not a single token itself, it resolves to 
    // void.
    template <typename parser_t, bool single>
    struct token_type { typedef void type; };

    template <typename parser_t>
    struct token_type<parser_t, true> { typedef typename parser_t::token_type type; };

    // A parser that matches if either of the two supplied parsers match.  The 
    // second parser won't be tried if the first matches.  This class supports
    // general parser alternates, and also has special support for single 
    // token alternates.
    template <typename first_t, typename second_t>
    class alternate : public parser< alternate<first_t, second_t> >, 
        public parser_traits<first_t::is_single && second_t::is_single>
    {
        first_t first;
        second_t second;

    public:
        typedef typename token_type<first_t, is_single>::type token_type;

        template <typename iterator_t>
        struct ast
        {
            typedef typename tree::template ast_list<iterator_t>::template alternate<
            typename first_t::template ast<iterator_t>::type,
            typename second_t::template ast<iterator_t>::type> type;
        };

        alternate()
        {
        }
        
        alternate(const first_t& first, const second_t& second) : first(first), second(second)
        {
        }

        // This should be called for general parsers
        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return
                first.parse_from(start, end, tree.first) ||
                second.parse_from(start, end, tree.second);
        }

        // This should only be called if both first and second are single 
        // token parsers.
        template <typename token_t>
        bool match(token_t t)
        {
            return first.match(t) || second.match(t);
        }
    };

    // A parser that matches only if both of the given parsers match in 
    // sequence.
    template <typename first_t, typename second_t>
    class sequence : public parser< sequence<first_t, second_t> >,
        public parser_traits<false>
    {
        first_t first;
        second_t second;

    public:
        sequence()
        {
        }

        sequence(const first_t& first, const second_t& second) : first(first), second(second)
        {
        }

        template <typename iterator_t>
        struct ast
        {
            typedef typename tree::template ast_list<iterator_t>::template sequence<
                typename first_t::template ast<iterator_t>::type,
                typename second_t::template ast<iterator_t>::type> type;
        };

        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return first.parse_from(start, end, tree.first) &&
                second.parse_from(start, end, tree.second);
        }
    };

    // Matches the specified parser zero or more times.  The stream is checked 
    // for eof first, which is still considered a match.
    template <typename parser_t>
    class zero_or_more : public parser< zero_or_more<parser_t> >,
        public parser_traits<false>
    {
        parser_t elem;

    public:
        zero_or_more()
        {
        }

        zero_or_more(const parser_t& parser) : elem(parser)
        {
        }

        template <typename iterator_t>
        struct ast : public tree::ast_base<iterator_t>
        {
            typedef tree::zero_or_more<parser_t, iterator_t> type;
        };

        template <typename iterator_t, typename ast_t>
        bool parse_internal(iterator_t& start, iterator_t& end, ast_t& ast)
        {
            while (start != end)
            {
                typename ast_type<parser_t, iterator_t>::type elem_tree;

                if (!elem.parse_from(start, end, elem_tree)) break;

                // This is neccessary to prevent endless looping when a 
                // parser_t can have a valid, zero-length match.
                if (elem_tree.start == elem_tree.end) break;

                ast.matches.push_back(elem_tree);
            }
            return true;
        }
    };

    template <typename parser_t, size_t min, size_t max = SIZE_MAX>
    class repetition : public parser< repetition<parser_t, min, max> >,
        public parser_traits<false>
    {
        friend class parser< repetition<parser_t, min, max> >;

    public:
        repetition()
        {
        }

        repetition(const parser_t& parser) : parser(parser)
        {
        }

        template <typename iterator_t>
        struct ast : public tree::ast_base<iterator_t>
        {
            typedef tree::repetition<parser_t, iterator_t> type;
        };

    protected:
        parser_t parser;

        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            size_t i;
            for (i = 0; i < min; i++)
            {
                typename ast_type<parser_t, iterator_t>::type child;
                if (!parser.parse_from(start, end, child)) return false;
                tree.matches.push_back(child);
            }

            for (; i < max; i++)
            {
                typename ast_type<parser_t, iterator_t>::type child;
                if (start == end || !parser.parse_from(start, end, child)) break;
                tree.matches.push_back(child);
            }
            return true;
        }
    };

    template <typename parser_t>
    class optional : public parser< optional<parser_t> >,
        public parser_traits<false>
    {
        friend class parser< optional<parser_t> >;

    public:
        optional()
        {
        }

        optional(const parser_t& parser) : parser(parser)
        {
        }

        template <typename iterator_t>
        struct ast : public tree::ast_base<iterator_t>
        {
            typedef tree::optional<parser_t, iterator_t> type;
        };

        parser_t parser;

    protected:
        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            if (start != end) parser.parse_from(start, end, tree.option);
            return true;
        }
    };

    // This is the base class for all parsers that match a single token.  This 
    // class handles reading the single token, and calls the derived class's 
    // match() method to determine whether the token matches.  The token type, 
    // token_t, can be any type with a parameterless constructor.  Also, the 
    // stream_t must return values that are assignable to token_t.
    template <typename derived_t, typename token_t>
    class single : public parser< single<derived_t, token_t> >,
        public parser_traits<true>
    {
    public:
        typedef token_t token_type;

        template <typename iterator_t>
        struct ast : public tree::ast_base<iterator_t>
        {
            typedef tree::single<token_t, iterator_t> type;
        };

        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return start != end && static_cast<derived_t*>(this)->match(tree.token = *start++);
        }
    };

    // A parser that matches a single token that is anything other the what 
    // the parser_t type matches.  This cannot be used with variable-length 
    // (in tokens) parsers.
    template <typename parser_t, typename token_t>
    class complement : public single< complement<parser_t, token_t>, token_t >
    {
        parser_t parser;

    public:
        complement(const parser_t& p) : parser(p) {}

        complement() {}

        bool match(char c)
        {
            return !parser.match(c);
        }
    };

    // A parser that matches a single token, in cases where the token_t type is 
    // integral (e.g., a char, wchar_t, etc., representing a character).
    template <typename token_t, token_t t>
    class constant : public single< constant<token_t, t>, token_t >
    {
    public:
        bool match(token_t token)
        {
            return token == t;
        }
    };

    // Similar to constant, but the character is passed to the constructor at 
    // runtime, instead of the template at compile-time.
    template <typename token_t>
    class character : public single< character<token_t>, token_t >
    {
    private:
        token_t t;

    public:
        character(token_t token) : t(token)
        {
        }

        bool match(token_t token)
        {
            return token == t;
        }
    };

    // A zero-length parser that matches only at the end of the stream.
    class end : public parser<end>
    {
    public:
        template <typename iterator_t>
        struct ast
        {
            typedef tree::ast_base<iterator_t> type;
        };

        template <typename iterator_t>
        bool parse_internal(iterator_t& s, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return s.eof();
        }
    };

    // A zero-length parser that matches only at the beginning of the stream.
    class start : public parser<start>
    {
    public:
        template <typename iterator_t>
        struct ast
        {
            typedef tree::ast_base<iterator_t> type;
        };

        template <typename iterator_t>
        bool parser_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return s.pos() == 0;
        }
    };

    // This parser is used to make recursive parsers that refer to 
    // themselves, either directly or via some other sub-parser.
    template <typename parser_t>
    class reference : public parser< reference<parser_t> >,
        public parser_traits<false>
    {
    public:
        template <typename iterator_t>
        struct ast
        {
            typedef tree::reference<parser_t, iterator_t> type;
        };

        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            typedef typename ast_type<parser_t, iterator_t>::type p_tree;
            parser_t parser;
            tree.ptr.reset(new p_tree());
            return parser.parse_from(start, end, *tree.ptr);
        }
    };

    // This parser matches only if the first_t parser matches and the 
    // second_t doesn't (from the same location).  This supports single
    // token parsing if both types are also single.
    template <typename first_t, typename second_t>
    struct difference 
        : public parser< difference<first_t, second_t> >,
        public parser_traits< first_t::is_single && second_t::is_single >
    {
        first_t first;
        second_t second;

    public:
        template <typename iterator_t>
        struct ast
        {
            typedef typename first_t::ast<iterator_t>::type type;
        };

        difference() {}

        difference(const first_t& f, const second_t& s) : first(f), second(s) {}

        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            iterator_t tmp = start;
            typename second_t::ast<iterator_t>::type second_tree;
            
            return 
                first.parse_from(start, end, tree) && 
                !second.parse_from(tmp, end, second_tree);
        }

        template <typename token_t>
        bool match(token_t t)
        {
            return first.match(t) && !second.match(t);
        }
    };

    // This namespace defines operators that are useful in cosntructing parsers 
    // using a convenient syntax (shorter than a complicated, nested template 
    // instantiation).
    namespace operators
    {
        // Generates an alternate<first_t, second_t> parser
        template <typename first_t, typename second_t>
        typename std::enable_if<std::is_same<parser_tag, typename first_t::tag>::value, alternate<first_t, second_t> >::type
        operator| (const first_t& first, const second_t& second)
        {
            return alternate<first_t, second_t>(first, second);
        }

        // Generates a sequence<first_t, second_t> parser
        template <typename first_t, typename second_t>
        typename std::enable_if<std::is_same<parser_tag, typename first_t::tag>::value, sequence<first_t, second_t> >::type
        operator>> (const first_t& first, const second_t& second)
        {
            return sequence<first_t, second_t>(first, second);
        }

        // Generates a zero_or_more<parser_t> parser
        template <typename parser_t>
        typename std::enable_if<std::is_same<parser_tag, typename parser_t::tag>::value, zero_or_more<parser_t> >::type
        operator* (parser_t& parser)
        {
            return zero_or_more<parser_t>(parser);
        }

        // Generates an optional parser
        template <typename parser_t>
        typename std::enable_if<std::is_same<parser_tag, typename parser_t::tag>::value, optional<parser_t> >::type
        operator! (const parser_t& parser)
        {
            return optional<parser_t>(parser);
        }

        // Generates a "one or more" (repetition<parser_t, 1>) parser
        template <typename parser_t>
        typename std::enable_if<std::is_same<parser_tag, typename parser_t::tag>::value, repetition<parser_t, 1> >::type
        operator+ (const parser_t& parser)
        {
            return repetition<parser_t, 1>(parser);
        }

        // Generates a complement parser (only for single-token sub-parsers)
        template <typename parser_t>
        typename std::enable_if<parser_t::is_single, complement< parser_t, typename parser_t::token_type > >::type
            operator~ (const parser_t& parser)
        {
            return complement< parser_t, typename parser_t::token_type >(parser);
        }

        template <typename first_t, typename second_t>
        difference<first_t, second_t> operator- (const first_t& f, const second_t& s)
        {
            return difference<first_t, second_t>(f, s);
        }

        // This is obviously not a c++ operator, but is included here since 
        // it is intended to be used in operator expressions.  The purpose of 
        // this is to group expressions like "(a >> b) >> c", which have no
        // difference from "a >> b >> c".  Using this function, "a >> b" can
        // be grouped like this: "group(a >> b) >> c".
        template <typename parser_t>
        grouped<parser_t> group(const parser_t& p)
        {
            return grouped<parser_t>(p);
        }
    }

    namespace terminals
    {

        template <char32_t t>
		struct u : constant<unsigned int, t>
		{
		};

		struct digit : single<digit, char32_t>
		{
			bool match(char32_t t)
			{
				return isdigit(t) != 0;
			}
		};

		struct alpha : single<alpha, char32_t>
		{
			bool match(char32_t t)
			{
				return isalpha(t) != 0;
			}
		};

        struct any : single<any, char32_t>
        {
            bool match(char32_t) { return true; }
        };

    }

}
