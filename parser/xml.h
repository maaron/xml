#pragma once

#include "parse.h"
#include "unicode.h"

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

	class anchor
	{
	protected:
		size_t start;

	public:
        anchor(size_t start) : start(start)
		{
		}
	};

	class element : public anchor
	{
	private:
		parser::element_open::ast< std::iterator<std::random_access_iterator_tag, char> >::type ast;

	public:
		element(std::istream& stream) : anchor(todo)
		{
			parser::element_open parser;
			if (!parser.parse(s, ast)) throw std::exception("parse error");
		}

		std::string name()
		{
			return s.get_string(ast[util::_i1]);
		}

		std::string local_name()
		{
			return s.get_string(ast[util::_i1].second);
		}
	};

	class document : public anchor
	{
	private:

	public:
		document(char_stream& stream) : anchor(stream)
		{
			parser::prolog p;
			parser::prolog::ast ast;
			if (!p.parse(s, ast)) throw std::exception("parse error");
		}

		element root()
		{
			return element(s);
		}
	};

	
}