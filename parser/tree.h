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
            std::string _name;
            std::map<std::string, std::string> _attributes;
            std::list<node> _childnodes;
            std::list<element> _elements;
            std::list<std::string> _textnodes;

        public:
            typedef std::map<std::string, std::string> attribute_list;
            typedef std::list<element> element_list;

            typedef std::list<node>::iterator node_iterator;
            typedef std::list<element>::iterator element_iterator;
            typedef std::list<std::string>::iterator textnode_iterator;
            typedef std::map<std::string, std::string>::iterator attribute_iterator;

            template <typename ast_t>
            void read(ast_t& ast)
            {
                _attributes.clear();
                _elements.clear();
                _textnodes.clear();

                name = get_string(ast[_0]);

                auto& attlist_ast = ast[_1].matches;
                for (auto attr = attlist_ast.begin(); attr != attlist_ast.end(); attr++)
                {
                    _attributes[get_string((*attr)[_0])] = qstring_value((*attr)[_1]);
                }

                auto& childlist_ast = ast[_3].matches;
                for (auto it = childlist_ast.begin(); it != childlist_ast.end(); it++)
                {
                    auto& child = *it;
                    if (child[_0].matched)
                    {
                        _elements.push_back(element());
                        _elements.back().read(child[_0].get());
                        _childnodes.push_back(node(&elements.back()));
                    }
                    else if (child[_1].matched)
                    {
                        _textnodes.push_back(get_string(child[_1]));
                        _childnodes.push_back(node(&textnodes.back()));
                    }
                }
            }

            template <typename iterator_t>
            void read(xml::reader::element<iterator_t>& e)
            {
                _attributes.clear();
                _elements.clear();
                _textnodes.clear();

                _name = e.name();

                xml::reader::attribute<iterator_t> attr = e.next_attribute();
                for (; !attr.is_end(); 
                    attr = attr.next_attribute())
                {
                    _attributes[attr.name()] = attr.value();
                }

                xml::reader::node<iterator_t> child = attr.next_child();
                for (; !child.is_end();
                    child = child.next_sibling())
                {
                    if (child.is_text())
                    {
                        _textnodes.push_back(child.text());
                        _childnodes.push_back(node(&textnodes.back()));
                    }
                    else
                    {
                        assert(child.is_element());
                        _elements.push_back(element());
                        _elements.back().read(child.element());
                        _childnodes.push_back(node(&elements.back()));
                    }
                }
            }

            // Returns the tag name of the element
            std::string& name() { return _name; }

            // Returns a string -> string map of attributes.
            attribute_list& attributes() { return _attributes; }

            // Returns a read-only list of child elements.
            const element_list& elements() { return _elements; }

            node_list& nodes() { return _childnodes.begin(); }

            std::string text()
            {
                std::string s;
                for (auto i = _textnodes.begin(); i != _textnodes.end(); i++)
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