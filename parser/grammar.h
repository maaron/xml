#pragma once

#include <assert.h>
#include <string>
#include "parse\parse.h"

namespace xml
{
    using namespace util;

    template <typename unicode_iterator>
    std::string get_string(parse::tree::ast_base<unicode_iterator>& ast)
    {
        std::string ret;
        utf8::utf32to8(ast.start, ast.end, std::back_inserter(ret));
        return ret;
    }

    class parse_exception : public std::exception
    {
        std::string message;

    public:
        template <typename ast_t, typename iterator_t>
        parse_exception(ast_t& ast, iterator_t& end)
        {
            auto next = parse::tree::last_match(ast);
            std::string next_chars;
            size_t count = 0;
            auto stop = next;
            
            while (next != end && count < 100) { stop++; count++; }
            utf8::utf32to8(next, stop, std::back_inserter(message));
        }

        template <typename iterator_t>
        parse_exception(iterator_t& next, iterator_t& end)
        {
            std::ostringstream mstr;
            mstr << "line " << next.get_line() << ", column " << next.get_column() << ": ";
            message += mstr.str();

            std::string next_chars;
            size_t count = 0;
            auto stop = next;
            
            while (next != end && count < 100) { stop++; count++; }
            utf8::utf32to8(next, stop, std::back_inserter(message));
        }

        const char* what() const override
        {
            return message.c_str();
        }
    };

    // This namespace contains a grammar for XML that is a simplified version of the XML spec.
    namespace grammar
	{

		using namespace ::parse;
		using namespace ::parse::operators;
		using namespace ::parse::terminals;

		auto lt = u<'<'>();
		auto gt = u<'>'>();
		auto qmark = u<'?'>();
		auto bslash = u<'\\'>();
		auto fslash = u<'/'>();
		auto dquote = u<'"'>();
		auto squote = u<'\''>();
		auto equal = u<'='>();
		auto space = u<' '>();
		auto colon = u<':'>();
		auto tab = u<'\t'>();
		auto cr = u<'\r'>();
		auto lf = u<'\n'>();
        auto bang = u<'!'>();
        auto dash = u<'-'>();
        auto dot = u<'.'>();
        auto uscore = u<'_'>();

        auto xmlchar = any();

        auto namechar = alpha() | digit() | dot | dash | uscore | colon;

        auto name = (alpha() | uscore) >> *namechar;

        auto ws = +(space | tab | cr | lf);

        auto eq = !ws >> equal >> !ws;

        auto content_char = ~(lt | gt);

        /*
        typedef decltype((squote >> *(~squote) >> squote) | (dquote >> *(~dquote) >> dquote)) qstring_t;
        struct qstring : qstring_t
        {
            template <typename iterator_t>
            struct ast : qstring_t::ast<iterator_t>::type
            {
                typedef ast type;

                std::string value()
                {
                    if ((*this)[_0].matched) return get_string((*this)[_0][_1]);
                    else return get_string((*this)[_1][_1]);
                }
            };
        };
        */

        struct qstring : parser<qstring>
        {
            template <typename iterator_t>
            struct ast : parse::tree::ast_base<iterator_t>
            {
                typedef ast type;

                std::string v;

                std::string value()
                {
                    return v;
                }
            };

            template <typename iterator_t>
            bool parse_internal(iterator_t& it, iterator_t& end, ast<iterator_t>& a)
            {
                auto quote = *it++;
                if (quote == '"')
                {
                    auto value_start = it;
                    while (*it++ != '"' && it != end);
                    a.v = get_string(value_start, it);
                    return true;
                }
                else if (quote == '\'')
                {
                    auto value_start = it;
                    while (*it++ != '\'' && it != end);
                    a.v = get_string(value_start, it);
                    return true;
                }
                else return false;
            }
        };

        typedef decltype(ws >> group(name) >> eq >> qstring()) attribute_t;
        struct attribute : attribute_t
        {
            template <typename iterator_t>
            struct ast : attribute_t::ast<iterator_t>::type
            {
                typedef ast type;

                typename qstring::ast<iterator_t>::type qstring()
                {
                    return (*this)[_3];
                }

                typename std::string key()
                {
                    return get_string((*this)[_1].group);
                }
            };
        };

        typedef decltype(*attribute()) attribute_list;

        typedef decltype(lt >> group(name) >> attribute_list() >> gt) element_open;

        typedef decltype(lt >> fslash >> group(name) >> gt) element_close;

        struct element;

        typedef reference<element> element_ref;

        typedef decltype(*content_char) textnode;

        typedef decltype(lt >> bang >> dash >> dash >> *(~dash | (dash >> ~dash)) >> dash >> dash >> gt) comment;

        typedef decltype(element_ref() | comment() | textnode()) childnode;

        typedef decltype(*(childnode())) element_content;

        typedef decltype(lt >> group(name) >> !attribute_list() >> !ws >> ((fslash >> gt) | (gt >> element_content() >> element_close()))) element_base;

        struct element : public element_base {};

        typedef decltype(lt >> qmark >> *(xmlchar - (qmark >> gt)) >> qmark >> gt) pi;

        typedef decltype(comment() | pi() | ws) misc;

        typedef decltype(lt >> qmark >> *(~qmark) >> qmark >> gt) xmldecl;

        typedef decltype(lt >> bang >> *(~gt) >> gt)  doctypedecl;

        typedef decltype(!xmldecl() >> *misc() >> !(doctypedecl() >> *misc())) prolog;

        typedef decltype(group(prolog()) >> element()) document;
    }
}

template <> struct ::parse::debug_tag<xml::grammar::attribute_list> { static const char* name() { return "xml::attribute_list"; } };
template <> struct ::parse::debug_tag<xml::grammar::element_open> { static const char* name() { return "xml::element_open"; } };
template <> struct ::parse::debug_tag<xml::grammar::element_close> { static const char* name() { return "xml::element_close"; } };
template <> struct ::parse::debug_tag<xml::grammar::element_ref> { static const char* name() { return "xml::element_ref"; } };
template <> struct ::parse::debug_tag<xml::grammar::textnode> { static const char* name() { return "xml::textnode"; } };
template <> struct ::parse::debug_tag<xml::grammar::comment> { static const char* name() { return "xml::comment"; } };
template <> struct ::parse::debug_tag<xml::grammar::childnode> { static const char* name() { return "xml::childnode"; } };
template <> struct ::parse::debug_tag<xml::grammar::element_content> { static const char* name() { return "xml::element_content"; } };
template <> struct ::parse::debug_tag<xml::grammar::element_base> { static const char* name() { return "xml::element"; } };
template <> struct ::parse::debug_tag<xml::grammar::pi> { static const char* name() { return "xml::pi"; } };
template <> struct ::parse::debug_tag<xml::grammar::misc> { static const char* name() { return "xml::misc"; } };
template <> struct ::parse::debug_tag<xml::grammar::xmldecl> { static const char* name() { return "xml::xmldecl"; } };
template <> struct ::parse::debug_tag<xml::grammar::doctypedecl> { static const char* name() { return "xml::doctypedecl"; } };
template <> struct ::parse::debug_tag<xml::grammar::prolog> { static const char* name() { return "xml::prolog"; } };
template <> struct ::parse::debug_tag<xml::grammar::document> { static const char* name() { return "xml::document"; } };

using namespace parse::operators;
template <> struct ::parse::debug_tag<decltype(xml::grammar::fslash >> xml::grammar::gt)> { static const char* name() { return "xml::/>"; } };
template <> struct ::parse::debug_tag<decltype(xml::grammar::gt >> xml::grammar::element_content() >> xml::grammar::element_close())> { static const char* name() { return "xml::content + close tag"; } };
