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
                type = elementnode;
                node_ptr.element = e;
            }

            node(std::string* s)
            {
                type = textnode;
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

            template <typename ast_t>
            void read(ast_t& ast)
            {
                attributes.clear();
                elements.clear();
                textnodes.clear();

                name = get_string(ast[_0]);

                auto& attlist_ast = ast[_1].matches;
                for (auto attr = attlist_ast.begin(); attr != attlist_ast.end(); attr++)
                {
                    attributes[get_string((*attr)[_0])] = qstring_value((*attr)[_1]);
                }

                auto& childlist_ast = ast[_3].matches;
                for (auto it = childlist_ast.begin(); it != childlist_ast.end(); it++)
                {
                    auto& child = *it;
                    if (child[_0].matched)
                    {
                        elements.push_back(element());
                        elements.back().read(*(child[_0].ptr));
                        childnodes.push_back(node(&elements.back()));
                    }
                    else if (child[_1].matched)
                    {
                        textnodes.push_back(get_string(child[_1]));
                        childnodes.push_back(node(&textnodes.back()));
                    }
                }
            }

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
                typedef typename unicode::unicode_container<container_t> unicode_container;
	            typedef typename unicode_container::iterator iterator;
            
                unicode_container data(c);
                parse::parser_ast<xml::grammar::document, iterator>::type ast;
                
                xml::grammar::document::parse_from(data.begin(), data.end(), ast);
                root.read(ast[_0]);
            }
        };
    }
}