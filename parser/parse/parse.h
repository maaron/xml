#pragma once

#include <vector>
#include <string>
#include <memory>
#include "tree.h"

namespace parse
{
    namespace placeholders
    {
        template <size_t i>
	    struct index : std::integral_constant<size_t, i> {};

	    static index<-1> _;
        static index<0> _0;
	    static index<1> _1;
	    static index<2> _2;
	    static index<3> _3;
	    static index<4> _4;
	    static index<5> _5;
	    static index<6> _6;
	    static index<7> _7;
	    static index<8> _8;
	    static index<9> _9;
    }

    template <typename parser_t>
    struct debug_tag { static const char* name() { return "unknown"; } };

    template <typename parser_t, typename iterator_t>
    struct ast_type
    {
        typedef typename parser_t::template ast<iterator_t>::type type;
    };

    template <typename parser_t, typename iterator_t>
    struct is_capture
    {
        static const bool value = !std::is_same<typename parser_t::ast<iterator_t>::type, tree::ast_base<iterator_t> >::value;
    };

    template <typename parser_t>
    struct ast_template
    {
        typedef tree::ast_base type;
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
        struct ast { typedef tree::ast_base<iterator_t> type; };

        template <typename iterator_t>
        static
        typename std::enable_if<is_capture<derived_t, iterator_t>::value, bool>::type
        parse_from(iterator_t& start, iterator_t& end, typename ast_type<derived_t, iterator_t>::type& ast)
        {
            auto matchEnd = start;

            if (derived_t::parse_internal(matchEnd, end, ast))
            {
                start = matchEnd;
                return true;
            }
            else return false;
        }

        template <typename iterator_t>
        static
        typename std::enable_if<!is_capture<derived_t, iterator_t>::value, bool>::type
        parse_from(iterator_t& start, iterator_t& end)
        {
            auto matchEnd = start;

            if (derived_t::parse_internal(matchEnd, end))
            {
                start = matchEnd;
                return true;
            }
            else return false;
        }

        template <typename stream_t>
        static
        typename std::enable_if<is_capture<derived_t, typename stream_t::iterator>::value, bool>::type
        parse(stream_t& s, typename ast_type<derived_t, typename stream_t::iterator>::type& ast)
        {
            return parse_from(s.begin(), s.end(), ast);
        }

        template <typename stream_t>
        static bool parse(stream_t& s)
        {
            return parse_from(s.begin(), s.end());
        }
    };

    template <typename parser_t, size_t i>
    struct capture : parser<capture<parser_t, i> >,
        parser_traits<parser_t::is_single>
    {
        template <typename iterator_t>
        struct ast
        {
            //typedef typename ast_type<parser_t, iterator_t>::type parser_ast_type;
            //typedef tree::ast_base<iterator_t> default_ast_type;
            //static const bool void_ast = std::is_void<parser_ast_type>::value;
            //typedef typename std::conditional<void_ast, default_ast_type, parser_ast_type >::type type;
            typedef typename ast_type<parser_t, iterator_t>::type parser_ast_type;
            typedef tree::ast_base<iterator_t> default_ast_type;
            static const bool has_ast = !std::is_same<parser_ast_type, default_ast_type>::value;
            typedef typename std::conditional<has_ast, parser_ast_type, default_ast_type>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& a)
        {
            a.start = start;
            a.parsed = true;
            if (ast<iterator_t>::has_ast)
                a.matched = parser_t::parse_from(start, end, a);
            else
                a.matched = parser_t::parse_from(start, end);
            a.end = start;
            return a.matched;
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
    public:
        template <typename iterator_t>
        struct ast
        {
            typedef tree::grouped<parser_t, iterator_t> type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return parser_t::parse_from(start, end, tree.group);
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
    public:
        typedef typename token_type<first_t, is_single>::type token_type;

        template <typename iterator_t>
        struct ast
        {
            typedef tree::alternate<
                iterator_t,
                typename first_t::template ast<iterator_t>::type,
                typename second_t::template ast<iterator_t>::type> type;
        };

        // This should be called for general parsers
        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return
                first_t::parse_from(start, end, tree.first) ||
                second_t::parse_from(start, end, tree.second);
        }

        // This should only be called if both first and second are single 
        // token parsers.
        template <typename token_t>
        static bool match(token_t t)
        {
            return first_t::match(t) || second_t::match(t);
        }
    };

    // A parser that matches only if both of the given parsers match in 
    // sequence.
    template <typename first_t, typename second_t>
    class sequence : public parser< sequence<first_t, second_t> >,
        public parser_traits<false>
    {
    public:
        template <typename iterator_t>
        struct ast
        {
            typedef tree::sequence<
                iterator_t,
                typename first_t::template ast<iterator_t>::type,
                typename second_t::template ast<iterator_t>::type> type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return first_t::parse_from(start, end, tree.first) &&
                second_t::parse_from(start, end, tree.second);
        }
    };

    // Matches the specified parser zero or more times.  The stream is checked 
    // for eof first, which is still considered a match.
    template <typename parser_t>
    class zero_or_more : public parser< zero_or_more<parser_t> >,
        public parser_traits<false>
    {
    public:
        template <typename iterator_t>
        struct ast
        {
            typedef tree::repetition<parser_t, iterator_t> type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& ast)
        {
            typedef typename parser_t::template ast<iterator_t>::type ast_t;

            while (start != end)
            {
                ast_t partial;
                if (!parser_t::parse_from(start, end, partial) || partial.start == partial.end)
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
    };

    template <typename parser_t, size_t min, size_t max = SIZE_MAX>
    class repetition : public parser< repetition<parser_t, min, max> >,
        public parser_traits<false>
    {
        friend class parser< repetition<parser_t, min, max> >;

    public:
        template <typename iterator_t>
        struct ast
        {
            typedef tree::repetition<parser_t, iterator_t> type;
        };

    protected:
        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
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
    };

    template <typename parser_t>
    class optional : public parser< optional<parser_t> >,
        public parser_traits<false>
    {
        friend class parser< optional<parser_t> >;

    public:
        template <typename iterator_t>
        struct ast
        {
            typedef tree::optional<parser_t, iterator_t> type;
        };

    protected:
        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            if (start != end) parser_t::parse_from(start, end, tree.option);
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
        static bool parse_internal(iterator_t& start, iterator_t& end)
        {
            return start != end && derived_t::match(*start++);
        }

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& a)
        {
            return start != end && derived_t::match(*start++);
        }

        template <size_t i>
        capture< single<derived_t, token_t>, i > operator[](const placeholders::index<i>& ph)
        {
            return capture< single<derived_t, token_t>, i >();
        }
    };

    // A parser that matches a single token that is anything other the what 
    // the parser_t type matches.  This cannot be used with variable-length 
    // (in tokens) parsers.
    template <typename parser_t, typename token_t>
    class complement : public single< complement<parser_t, token_t>, token_t >
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
    class constant : public single< constant<token_t, t>, token_t >
    {
    public:
        static bool match(token_t token)
        {
            return token == t;
        }
    };

    /*
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
    */

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
        static bool parse_internal(iterator_t& s, iterator_t& end, typename ast<iterator_t>::type& tree)
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
        static bool parser_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
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
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            typedef typename ast_type<parser_t, iterator_t>::type p_tree;
            tree.ptr.reset(new p_tree());
            return parser_t::parse_from(start, end, *tree.ptr);
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
    public:
        template <typename iterator_t>
        struct ast
        {
            typedef typename first_t::template ast<iterator_t>::type type;
        };

        template <typename iterator_t>
        static bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            iterator_t tmp = start;
            typename second_t::ast<iterator_t>::type second_tree;
            
            return 
                first_t::parse_from(start, end, tree) && 
                !second_t::parse_from(tmp, end, second_tree);
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
        using namespace parse::placeholders;

        // Generates an alternate<first_t, second_t> parser
        template <typename first_t, typename second_t>
        //typename std::enable_if<std::is_same<parser_tag, typename first_t::tag>::value, alternate<first_t, second_t> >::type
        alternate<first_t, second_t>
        operator| (const parser<first_t>& first, const parser<second_t>& second)
        {
            return alternate<first_t, second_t>();
        }

        // Generates a sequence<first_t, second_t> parser
        template <typename first_t, typename second_t>
        //typename std::enable_if<std::is_same<parser_tag, typename first_t::tag>::value, sequence<first_t, second_t> >::type
        sequence<first_t, second_t>
        operator>> (const parser<first_t>& first, const parser<second_t>& second)
        {
            return sequence<first_t, second_t>();
        }

        // Generates a zero_or_more<parser_t> parser
        template <typename parser_t>
        //typename std::enable_if<std::is_same<parser_tag, typename parser_t::tag>::value, zero_or_more<parser_t> >::type
        zero_or_more<parser_t>
        operator* (parser<parser_t>& parser)
        {
            return zero_or_more<parser_t>();
        }

        // Generates an optional parser
        template <typename parser_t>
        //typename std::enable_if<std::is_same<parser_tag, typename parser_t::tag>::value, optional<parser_t> >::type
        optional<parser_t>
        operator! (const parser<parser_t>& parser)
        {
            return optional<parser_t>();
        }

        // Generates a "one or more" (repetition<parser_t, 1>) parser
        template <typename parser_t>
        //typename std::enable_if<std::is_same<parser_tag, typename parser_t::tag>::value, repetition<parser_t, 1> >::type
        repetition<parser_t, 1>
        operator+ (const parser<parser_t>& parser)
        {
            return repetition<parser_t, 1>();
        }

        // Generates a complement parser (only for single-token sub-parsers)
        template <typename parser_t>
        //typename std::enable_if<parser_t::is_single, complement< parser_t, typename parser_t::token_type > >::type
        complement< parser_t, typename parser_t::token_type >
        operator~ (const parser<parser_t>& parser)
        {
            return complement< parser_t, typename parser_t::token_type >();
        }

        template <typename first_t, typename second_t>
        difference<first_t, second_t> operator- (const parser<first_t>& f, const parser<second_t>& s)
        {
            return difference<first_t, second_t>();
        }

        // This is obviously not a c++ operator, but is included here since 
        // it is intended to be used in operator expressions.  The purpose of 
        // this is to group expressions like "(a >> b) >> c", which have no
        // difference from "a >> b >> c".  Using this function, "a >> b" can
        // be grouped like this: "group(a >> b) >> c".
        template <typename parser_t>
        grouped<parser_t> group(const parser<parser_t>& p)
        {
            return grouped<parser_t>();
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
			static bool match(char32_t t)
			{
				return isdigit(t) != 0;
			}
		};

		struct alpha : single<alpha, char32_t>
		{
			static bool match(char32_t t)
			{
				return isalpha(t) != 0;
			}
		};

        struct space : public single<space, char32_t>
        {
            static bool match(char32_t t)
            {
                return isspace(t) != 0;
            }
        };

        struct any : single<space, char32_t>
        {
            static bool match(char32_t) { return true; }
        };
    
    }

}
