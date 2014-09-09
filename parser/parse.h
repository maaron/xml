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

    // Base class for all parsers.  The main method, parse(stream_t&, ast&) 
    // tracks the start and end positions of the underlying parser's match, and 
    // updates the AST accordingly.
    template <typename derived_t>
    class parser
    {
    public:
        template <typename iterator_t, typename ast_t>
        bool parse_from(iterator_t& start, iterator_t& end, ast_t& ast)
        {
            auto matchEnd = start;

            if (static_cast<derived_t*>(this)->parse_internal(matchEnd, end, ast))
            {
                ast.start = start;
                start = ast.end = matchEnd;
                return (ast.matched = true);
            }
            else
            {
                return (ast.matched = false);
            }
        }

        template <typename stream_t, typename ast_t>
        bool parse(stream_t& s, ast_t& ast)
        {
            return parse_from(s.begin(), s.end(), ast);
        }
    };

    // A parser that matches if either of the two supplied parsers match.  The 
    // second parser won't be tried if the first matches.
    template <typename first_t, typename second_t>
    class alternate : public parser< alternate<first_t, second_t> >
    {
        first_t first;
        second_t second;

    public:
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

        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            return
                first.parse_from(start, end, tree.first) ||
                second.parse_from(start, end, tree.second);
        }
    };

    // A parser that matches only if both of the given parsers match in 
    // sequence.
    template <typename first_t, typename second_t>
    class sequence : public parser< sequence<first_t, second_t> >
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
    class zero_or_more : public parser< zero_or_more<parser_t> >
    {
        parser_t elem;

    public:
        zero_or_more()
        {
        }

        zero_or_more(const parser_t& parser) : parser(parser)
        {
        }

        template <typename iterator_t>
        struct ast : public tree::ast_base<iterator_t>
        {
            typedef ast type;
            std::vector<typename ast_type<parser_t, iterator_t>::type> children;
        };

        template <typename iterator_t, typename ast_t>
        bool parse_internal(iterator_t& start, iterator_t& end, ast_t& ast)
        {
            typename ast_type<parser_t, iterator_t>::type elem_tree;
            while (!s.eof() && elem.parse_from(start, end, elem_tree))
            {
                tree.children.push_back(elem_tree);

                // This is neccessary to prevent endless looping when a 
                // parser_t can have a valid, zero-length match.
                if (elem_tree.start == elem_tree.end) break;
            }
            return true;
        }
    };

    template <typename parser_t, size_t min, size_t max = SIZE_MAX>
    class repetition : public parser< repetition<parser_t, min, max> >
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
            typedef ast type;
            std::vector<typename ast_type<parser_t, iterator_t>::type> children;
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
                if (!parser.parse(start, end, child)) return false;
                tree.children.push_back(child);
            }

            for (; i < max; i++)
            {
                typename ast_type<parser_t, iterator_t>::type child;
                if (s.eof() || !parser.parse(start, end, child)) break;
                tree.children.push_back(child);
            }
            return true;
        }
    };

    template <typename parser_t>
    class optional : public parser< optional<parser_t> >
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
            typedef ast type;
            typename ast_type<parser_t, iterator_t>::type child;
        };

        parser_t parser;

    protected:
        template <typename iterator_t>
        bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            parser.parse(start, end, tree.child);
            return true;
        }
    };

    // This is the base class for all parsers that match a single token.  This 
    // class handles reading the single token, and calls the derived class's 
    // match() method to determine whether the token matches.  The token type, 
    // token_t, can be any type with a parameterless constructor.  Also, the 
    // stream_t must return values that are assignable to token_t.
    template <typename derived_t, typename token_t>
    class single : public parser< single<derived_t, token_t> >
    {
    public:
        template <typename iterator_t>
        struct ast : public tree::ast_base<iterator_t>
        {
            typedef ast type;
            token_t token;

            ast() : token(0)
            {
            }
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
    public:
        parser_t parser;

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

    template <typename parser_t>
    class reference : public parser< reference<parser_t> >
    {
    public:
        template <typename iterator_t>
        struct ast : public tree::ast_base<iterator_t>
        {
            std::shared_ptr<typename ast_type<parser_t, iterator_t>::type> ptr;
        };

        template <typename iterator_t>
        bool parse(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
        {
            typedef typename ast_type<parser_t, iterator_t>::type p_tree;
            parser_t parser;
            ast.ptr.reset(new p_tree());
            return parser.parse(s, *ast.ptr);
        }
    };

    // This namespace defines operators that are useful in cosntructing parsers 
    // using a convenient syntax (shorter than a complicated, nested template 
    // instantiation).
    namespace operators
    {
        // Generates an alternate<first_t, second_t> parser
        template <typename first_t, typename second_t>
        alternate<first_t, second_t> operator| (const first_t& first, const second_t& second)
        {
            return alternate<first_t, second_t>(first, second);
        }

        // Generates a sequence<first_t, second_t> parser
        template <typename first_t, typename second_t>
        sequence<first_t, second_t> operator>> (const first_t& first, const second_t& second)
        {
            return sequence<first_t, second_t>(first, second);
        }

        // Generates a zero_or_more<parser_t> parser
        template <typename parser_t>
        zero_or_more<parser_t> operator* (parser_t& parser)
        {
            return zero_or_more<parser_t>(parser);
        }

        template <typename parser_t>
        optional<parser_t> operator! (const parser_t& parser)
        {
            return optional<parser_t>(parser);
        }

        template <typename parser_t>
        repetition<parser_t, 1> operator+ (const parser_t& parser)
        {
            return repetition<parser_t, 1>(parser);
        }

        template <typename parser_t, typename token_t>
        complement< parser_t, token_t > operator~ (const single<parser_t, token_t>& parser)
        {
            return complement< parser_t, token_t >(parser);
        }

    }

}