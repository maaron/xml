#pragma once

#include "grammar.h"

namespace xml
{
    namespace reader
    {
        template <typename container_t>
        class document;

        template <typename iterator_t>
        class element;

        template <typename iterator_t>
        class attribute;

        template <typename container_t>
        class document
        {
            typedef typename container_t::iterator iterator_t;

            element root;

        public:
            document(const container_t& c)
            {
                parser::prolog prolog_p;
                parse::ast_type<prolog_p, iterator_t>::type prolog_a;
                auto begin = c.begin();
                auto end = c.end();

                if (!prolog_p.parse_from(begin, end, prolog_a)) 
                    throw parse_error(parse::tree::last_match(prolog_a));

                
            }
        };
    }
}