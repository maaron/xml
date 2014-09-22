#pragma once

#include <vector>
#include <map>

namespace xml
{
    namespace tree
    {
        class document;
        class element;
        class attribute;
        class text;

        class node
        {
            enum { textnode, elementnode } type;
            union
            {
                element* element;
                text* text;
            } node;
        };

        class element
        {
            std::vector<node> childnodes;

        public:
            std::map<std::string, std::string> attributes;
            std::vector<element> elements;
            std::vector<text> textnodes;
        };

        class document
        {
        public:
            element root;

            template <typename container_t>
            document(const container_t& c)
            {
            }
        };

        template <typename iterator_t>
        bool parse_element(element& e, iterator_t start, iterator_t end)
        {
        }
    }
}