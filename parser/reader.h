#pragma once

#include "unicode\unicode.h"
#include "grammar.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

INT64 node_ctr_low = MAXINT64;
INT64 node_ctr_high = MININT64;

struct profile
{
    LARGE_INTEGER t1;
    long long& low;
    long long& high;

    profile(long long& l, long long& h) : high(h), low(l)
    {
        QueryPerformanceCounter(&t1);
    }

    ~profile()
    {
        LARGE_INTEGER t2;
        QueryPerformanceCounter(&t2);

        auto t = t2.QuadPart - t1.QuadPart;

        if (t < low) low = t;
        if (t > high) high = t;
    }
};

namespace xml
{
    // This namespace contains classes that can be used to parse XML data in 
    // a "pull" fashion.  The data is only parsed as needed as the user calls 
    // the appropriate next_xyz() methods.
    namespace reader
    {
        using namespace xml::grammar;
        using namespace parse;
        using namespace util;
        using namespace parse::operators;

        template <typename container_t>
        class document;

        template <typename iterator_t>
        class element;

        template <typename iterator_t>
        class attribute;

        enum node_type { text_node, element_node, invalid };

        // This class is used to represent children of element nodes, which 
        // can themselves by either an element or a text node.  An instance of
        // this class may also represent the end of a node list.
        template <typename iterator_t>
        class node
        {
            enum parser_state_type { element_node, text_node, end_of_nodes } parser_state;

            node(parser_state_type p, iterator_t it, iterator_t end)
                : parser_state(p), it(it), end(end)
            {
            }

        protected:
            iterator_t it, end;
            std::string text_or_tag;

        public:
            typedef xml::reader::node<iterator_t> node_type;
            typedef xml::reader::element<iterator_t> element_type;

            node(iterator_t i, iterator_t e)
                : it(i), end(e)
            {
                profile(node_ctr_low, node_ctr_high);

                typedef decltype( (lt >> fslash >> name[_0] >> gt) | ((lt >> name[_1]) | comment() | textnode()[_2]) ) parser;
                typedef typename parse::parser_ast<parser, iterator_t>::type ast;

                while (true)
                {
                    ast a;

                    if (!parser::parse_from(it, end, a))
                        throw parse_exception(it, end);

                    if (a[_0].matched)
                    {
                        assert(a[_0].matched);
                        parser_state = end_of_nodes;
                        return;
                    }
                    else if (a[_1].matched)
                    {
                        parser_state = element_node;
                        text_or_tag = get_string(a[_1]);
                        return;
                    }
                    else if (a[_2].matched)
                    {
                        parser_state = text_node;
                        text_or_tag = get_string(a[_2]);
                        return;
                    }
                    // else must be a comment, so keep parsing
                }
            }

            // Creates
            static node_type end_node(iterator_t it, iterator_t end)
            {
                return node_type(end_of_nodes, it, end);
            }

            // Returns true only if the current node is an element
            bool is_element() { return parser_state == element_node; }

            // Returns true only if the current node is a text node
            bool is_text() { return parser_state == text_node; }

            // Returns true only if the object represents the end of the list 
            // of nodes (i.e., it conceptually points to the close tag).  In 
            // this case, the user should call next_parent() to get the next 
            // node following the closing tag.
            bool is_end() { return parser_state == end_of_nodes; }
            
            // Returns an element object representing the current node.  Only 
            // valid if the current node is an element (i.e., is_element() 
            // returns true).
            element_type element()
            {
                assert(is_element());
                return element_type(*this);
            }

            // Returns the content of a the current node.  Only valid if the 
            // current node is a text node (i.e., is_text() returns true).
            std::string text()
            {
                assert(is_text());
                return text_or_tag;
            }

            // Returns the node that follows the the current one.  Not valid 
            // if is_end() would return true.
            node_type next_sibling()
            {
                assert(is_element() || is_text());
                
                return is_element() ? 
                    element().next_sibling() :
                    node_type(it, end);
            }

            // Returns the next node following a the end of a list of 
            // elements.  Only valid if is_end() would return true.
            node_type next_parent()
            {
                assert(is_end());
                return node_type(it, end);
            }
        };

        template <typename iterator_t>
        class element : public node<iterator_t>
        {
            typedef element<iterator_t> element_type;
            typedef node<iterator_t> node_type;

        public:
            typedef xml::reader::attribute<iterator_t> attribute_type;

            element(const node_type& n) : node_type(n)
            {
            }

            element(iterator_t it, iterator_t end) : node_type(it, end)
            {
                if (!is_element()) throw parse_exception(it, end);
            }

            // Returns the tag name of the element.
            std::string name()
            {
                return text_or_tag;
            }

            // Returns the first attribute of the element, or an attribute 
            // representing the end of the attribute list, if there are none.
            // Only valid if is_end() would return false.
            attribute_type next_attribute()
            {
                return attribute_type(it, end);
            }

            // Returns the first child of the element node, or a node 
            // representing the end of the list of child nodes, if there 
            // aren't any.  Only valid if is_end() would return false.
            node_type next_child()
            {
                return attribute_type(it, end).next_child();
            }

            // Returns the node that immediately follows this one, or a node 
            // representing the end of the list of child nodes, if there 
            // aren't any.  Only valid if is_end() would return false.
            node_type next_sibling()
            {
                return attribute_type(it, end).next_sibling();
            }
        };

        template <typename iterator_t>
        class attribute
        {
            std::string _name;
            std::string _value;
            iterator_t it, end;
            enum { attribute_node, end_of_attributes, end_of_element } parser_state;

        public:
            typedef node<iterator_t> node_type;
            typedef attribute<iterator_t> attribute_type;

            attribute(iterator_t i, iterator_t e)
                : it(i), end(e)
            {
                typedef decltype( (!ws >> grammar::name[_0] >> eq >> qstring()[_1]) | (!ws >> !fslash[_2] >> gt) ) parser;
                typedef typename parse::parser_ast<parser, iterator_t>::type ast;

                ast a;
                
                if (!parser::parse_from(it, end, a))
                    throw parse_exception(a, end);
                
                if (a[_0].matched)
                {
                    _name = get_string(a[_0]);
                    auto& qstr = a[_1];
                    _value = qstr[_0].matched ? get_string(qstr[_0]) : get_string(qstr[_1]);
                    parser_state = attribute_node;
                }
                else
                {
                    parser_state = a[_2].matched ?
                        end_of_element : end_of_attributes;
                }
            }

            // Returns the name of the attribute, i.e., "name='value'".  Only 
            // valid if is_end() would return false.
            std::string name()
            {
                assert(parser_state == attribute_node);
                return _name;
            }
            
            // Returns the value of the attribute, i.e., "name='value'".  Only 
            // valid if is_end() would return false.
            std::string value()
            {
                assert(parser_state == attribute_node);
                return _value;
            }

            // Returns the attribute immediately following this one, or an 
            // attribute object representing the end of the attribute list, 
            // if there are no more.  Only valid if is_end() would return 
            // false.
            attribute_type next_attribute()
            {
                assert(parser_state == attribute_node);
                return attribute(it, end);
            }

            // Returns the first child of the element this attribute is 
            // associated with, or a node object representing the end of the 
            // list of nodes if there aren't any.
            node_type next_child()
            {
                if (parser_state == attribute_node)
                {
                    auto a = attribute(it, end);
                    while (!a.is_end()) a = a.next_attribute();
                    return a.next_child();
                }
                else if (parser_state == end_of_attributes)
                {
                    return node_type(it, end);
                }
                else //if (parser_state == end_of_element)
                {
                    assert(parser_state == end_of_element);
                    return node_type::end_node(it, end);
                }
            }

            // Returns the node immediately following the node this attribute 
            // is associated with, or a node object representing the end of 
            // the list of nodes if there isn't one.
            node_type next_sibling()
            {
                if (parser_state == attribute_node)
                {
                    auto a = attribute(it, end);
                    while (!a.is_end()) a = a.next_attribute();
                    return a.next_sibling();
                }
                else if (parser_state == end_of_attributes)
                {
                    auto child = node_type(it, end);
                    while (!child.is_end()) child = child.next_sibling();
                    return child.next_parent();
                }
                else //if (parser_state == end_of_element)
                {
                    assert(parser_state == end_of_element);
                    return node_type(it, end);
                }
            }

            // Returns true only if there are no more attributes in the 
            // list.  Conceptually, the current object is associated with the 
            // end of the open tag of an element.
            bool is_end() { return parser_state == end_of_attributes || parser_state == end_of_element; }
        };

        template <typename container_t>
        class document
        {
            typedef typename unicode::unicode_container<container_t> unicode_container;
	        typedef typename unicode_container::iterator iterator_t;
            
            unicode_container data;
            iterator_t it, end;

        public:
            typedef element<iterator_t> element_type;

            document(container_t& c)
                : data(c), it(data.begin()), end(data.end())
            {
                if (!grammar::prolog::parse_from(it, end)) 
                    throw parse_exception(it, end);
            }

            // Returns the element object representing the root of the 
            // document.
            element_type root()
            {
                return element_type(it, end);
            }
        };
    }
}