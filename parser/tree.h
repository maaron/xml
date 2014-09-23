#pragma once

#include <list>
#include <map>
#include "reader.h"

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
                std::string* text;
            } node_ptr;

        public:
            node(element* e)
            {
                node_ptr.element = e;
            }

            node(std::string* s)
            {
                node_ptr.text = s;
            }
        };

        class element
        {
            std::list<node> childnodes;

        public:
            std::string name;
            std::map<std::string, std::string> attributes;
            std::list<element> elements;
            std::list<std::string> textnodes;

            template <typename iterator_t>
            void read(xml::reader::element<iterator_t>& e)
            {
                attributes.clear();
                elements.clear();
                textnodes.clear();

                name = e.name();

                xml::reader::attribute<iterator_t> attr = e.next_attribute();
                for (; !attr.is_end(); 
                    attr = attr.next_attribute())
                {
                    attributes[attr.name()] = attr.value();
                }

                xml::reader::node<iterator_t> child = attr.next_child();
                for (; !child.is_end();
                    child = child.next_sibling())
                {
                    if (child.is_text())
                    {
                        textnodes.push_back(child.text());
                        childnodes.push_back(node(&textnodes.back()));
                    }
                    else
                    {
                        assert(child.is_element());
                        elements.push_back(element());
                        elements.back().read(child.element());
                        childnodes.push_back(node(&elements.back()));
                    }
                }
            }

            std::string text()
            {
                std::string s;
                for (auto i = textnodes.begin(); i != textnodes.end(); i++)
                {
                    s += *i;
                }
                return s;
            }
        };

        class document
        {
        public:
            element root;

            template <typename container_t>
            document(container_t& c)
            {
                xml::reader::document<container_t> doc(c);
                root.read(doc.root());
            }
        };

        template <typename iterator_t>
        bool parse_element(element& e, iterator_t start, iterator_t end)
        {
        }
    }
}