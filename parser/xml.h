#pragma once

#include "parse.h"
#include "unicode.h"
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

    // buffer_iterator that supports std::istream
    class streambuf_iterator : public std::iterator<std::input_iterator_tag, char32_t>
    {
    private:
        static const size_t read_size = 8192;
        std::streambuf* sbuf;
        size_t index;
        std::vector<unsigned char>* buf;

    public:
        streambuf_iterator()
            : sbuf(nullptr), buf(nullptr), 
            index(std::streambuf::traits_type::eof())
        {
        }

        streambuf_iterator(std::streambuf* streambuf, std::vector<unsigned char>* buffer)
            : sbuf(streambuf), buf(buffer), index(0)
        {
            char32_t uc;

        }

    protected:
        char32_t operator*()
        {
            return buf->[index];
        };

        streambuf_iterator& operator++()
        {
            advance();
            return *this;
        }

        streambuf_iterator& operator++(int)
        {
            streambuf_iterator tmp = *this;
            ++*this;
            return tmp;
        }

        bool operator==(const streambuf_iterator& rhs)
        {
            return 
                (rhs.sbuf == sbuf == nullptr) ||
                (rhs.sbuf == nullptr && sbuf->sgetc() == std::streambuf::traits_type::eof()) ||
                (sbuf == nullptr && rhs.sbuf->sgetc() == std::streambuf::traits_type::eof()) ||
                index == rhs.index;
        }

        bool operator!=(const streambuf_iterator& rhs)
        {
            return !(*this == rhs);
        }

    private:
        void advance()
        {
            index++;

            if (index == buf->size())
            {
                size_t s = buf.size();
                buf.resize(s + read_size);
                if (sbuf->sgetn(&buf[s], read_size) == 0);
            }
        }
    };

    // Buffer that supports random access, and reads from a std::istream as 
    // necessary.
    class istream_buffer
    {
    private:
        std::streambuf* sbuf;
        std::vector<unsigned char> buffer;

    public:
        typedef streambuf_iterator<unsigned char> iterator;

        istream_buffer(std::istream& stream) : sbuf(stream.rdbuf())
        {
        };

        iterator begin() { return iterator(sbuf, &buffer); }

        iterator end() { return iterator(); }
    };

	class anchor
	{
	protected:
		istream_buffer::iterator start;

	public:
        anchor(istream_buffer::iterator start) : start(start)
		{
		}
	};

	class element : public anchor
	{
	private:
		parse::ast_type<parser::element_open, istream_buffer::iterator>::type ast;

	public:
		element(istream_buffer::iterator start) : anchor(start)
		{
			parser::element_open parser;
			if (!parser.parse_from(start, istream_buffer::iterator(), ast)) throw std::exception("parse error");
		}

		std::string name()
		{
			return ast[util::_i1].to_string();
		}

		std::string local_name()
		{
			return ast[util::_i1].to_string();
		}
	};

	class document
	{
	private:
        istream_buffer data;
        parse::ast_type<parser::prolog, istream_buffer::iterator>::type ast;

	public:
		document(std::istream& stream) : data(stream)
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