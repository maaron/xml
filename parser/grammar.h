#pragma once

#include <assert.h>
#include <string>
#include "parse\parse.h"
#include <algorithm>

namespace xml
{

    template <typename unicode_iterator>
    class match_string
    {
        unicode_iterator s, e;

    public:
        match_string(unicode_iterator start, unicode_iterator end)
            : s(start), e(end) {}

        operator std::string() const
        {
            std::string ret;
            utf8::utf32to8(s, e, std::back_inserter(ret));
            return ret;
        }

        bool operator== (const std::string& rhs) const
        {
            auto b1 = s;
            auto e1 = e;
            auto b2 = rhs.begin();
            auto e2 = rhs.end();
            while (b1 != e1 && b2 != e2)
            {
                if (*b1++ != *b2++) return false;
            }
            return (b1 == e1 && b2 == e2)
        }

        bool operator!= (const std::string& rhs) const { return !(*this==rhs); }
    };

    template <typename iterator_t>
    std::ostream& operator<< (std::ostream& lhs, const match_string<iterator_t>& rhs)
    {
        return lhs << std::string(rhs);
    }

    template <typename unicode_iterator>
    match_string<unicode_iterator> get_string(parse::tree::base<unicode_iterator>& ast)
    {
        return match_string<unicode_iterator>(ast.start, ast.end);
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
            //mstr << "line " << next.get_line() << ", column " << next.get_column() << ": ";
            //message += mstr.str();

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

        typedef decltype((squote >> (*(~squote))[_0] >> squote) | (dquote >> (*(~dquote))[_1] >> dquote)) qstring;
        
        typedef decltype(ws >> name[_0] >> eq >> qstring()[_1]) attribute;

        typedef decltype(*attribute()) attribute_list;

        typedef decltype(lt >> name[_0] >> attribute_list()[_1] >> gt) element_open;

        typedef decltype(lt >> fslash >> name >> gt) element_close;

        struct element;

        typedef reference<element> element_ref;

        typedef decltype(*content_char) textnode;

        typedef decltype(lt >> bang >> dash >> dash >> *(~dash | (dash >> ~dash)) >> dash >> dash >> gt) comment;

        typedef decltype(element_ref()[_0] | comment() | textnode()[_1]) childnode;

        typedef decltype(*(childnode())) element_content;

        typedef decltype(lt >> name[_0] >> !attribute_list()[_1] >> !ws >> ((fslash >> gt)[_2] | (gt >> element_content()[_3] >> element_close()))) element_base;

        struct element : public element_base {};

        typedef decltype(lt >> qmark >> *(xmlchar - (qmark >> gt)) >> qmark >> gt) pi;

        typedef decltype(comment() | pi() | ws) misc;

        typedef decltype(lt >> qmark >> *(~qmark) >> qmark >> gt) xmldecl;

        typedef decltype(lt >> bang >> *(~gt) >> gt)  doctypedecl;

        typedef decltype(!xmldecl() >> *misc() >> !(doctypedecl() >> *misc())) prolog;

        typedef decltype(prolog() >> element()[_0]) document;
    }

    template <typename ast_t>
    auto qstring_value(ast_t& ast) -> decltype(get_string(ast[_0]))
    {
        return ast[_0].matched ? get_string(ast[_0]) : get_string(ast[_1]);
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
