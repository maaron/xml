#pragma once

#include "parse.h"
#include "unicode.h"
#include "stream.h"
#include <codecvt>

namespace xml
{
	namespace parser
	{
		using namespace ::parse;
		using namespace ::parse::operators;
		using namespace ::parse::unicode;

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

		auto name = alpha() >> *(alpha() | digit());
		auto qname = !(name >> colon) >> name;

		auto ws = +(space | tab | cr | lf);

		auto eq = !ws >> equal >> !ws;
		
		struct content_chars : parse::parser<content_chars>
		{
			template <typename stream_t>
			bool parse_internal(stream_t& s)
			{
				auto c = s.read();
				return (
					c != '<' &&
					c != '>');
			}
		};

		typedef decltype((squote >> *(~squote) >> squote) | (dquote >> *(~dquote) >> dquote)) qstring;

		typedef decltype(qname >> eq >> qstring()) attribute;

		typedef decltype(ws >> +(attribute())) attribute_list;

		typedef decltype(lt >> qname >> !attribute_list() >> gt) element_open;

		typedef decltype(lt >> fslash >> qname >> gt) element_close;

		struct element;
		
		typedef decltype(element_open() >> *(reference<element>() | *content_chars()) >> element_close()) element_base;

		struct element : element_base
		{};

		typedef decltype(!ws >> lt >> qmark >> *(~qmark) >> qmark >> gt >> !ws) prolog;
	}

	class document;
	class element;

    std::string get_string(
        parse::tree::ast_base<parser::streams::unicode_iterator>& ast)
    {
        return ast.start.to_string(ast.end);
    }

    class anchor
	{
	protected:
        parser::streams::unicode_iterator iter;

	public:
        anchor(parser::streams::unicode_iterator& start) : iter(start)
		{
		}
	};

	class element : public anchor
	{
	private:
		parse::ast_type<parser::element_open, parser::streams::unicode_iterator>::type ast;

	public:
		element(
            parser::streams::unicode_iterator& start) : anchor(start)
		{
			parser::element_open parser;
			if (!parser.parse_from(start, parser::streams::unicode_iterator(), ast)) throw std::exception("parse error");
		}

		std::string name()
		{
			return get_string(ast[util::_i1]);
		}

		std::string local_name()
		{
			return get_string(ast[util::_i1]);
		}
	};

	class document
	{
	private:
        parser::streams::parse_buffer data;
        parse::ast_type<parser::prolog, parser::streams::unicode_iterator>::type ast;

	public:
		document(std::istream& stream) : data(stream.rdbuf())
		{
		}

		element root()
		{
			parser::prolog p;
            auto iter = data.begin();
			if (!p.parse_from(iter, data.end(), ast)) throw std::exception("parse error");

            return element(iter);
		}
	};

	
}