#pragma once

#include <iterator>
#include <streambuf>
#include <vector>
#include <functional>
#include <codecvt>

#include <algorithm>

namespace xml {
namespace parser {
namespace streams {

        // Buffer class that reads from a stream as necessary, but 
        // stores everything read in memory.
        class parse_buffer;

        // Iterator that generates unicode characters
        class unicode_iterator;

        // Abstract base class that converts characters from some encoding 
        // into UTF-32.
        class converter;

        // Class that translates characters from UTF-8 into UTF-32
        class utf8_converter;

        // Class that translates characters from UTF-16 into UTF-32
        class utf16_converter;

        // Class that translates characters from ASCII into UTF-32
        class ascii_converter;

        class converter
        {
        public:
            virtual char32_t get(parse_buffer* buf, size_t& index) = 0;
            virtual std::string get_string(parse_buffer* buf, size_t start, size_t count) = 0;
        };

        class eof_converter : public converter
        {
        public:
            char32_t get(parse_buffer* buf, size_t& index)
            {
                return std::char_traits<char32_t>::eof();
            }

            std::string get_string(parse_buffer* buf, size_t start, size_t count)
            {
                return "";
            }
        };
        eof_converter eof_conv;

        class unicode_iterator : public std::iterator<std::input_iterator_tag, char32_t>
        {
            size_t i;
            parse_buffer* buf;
            char32_t c;
            converter& cvt;

        public:
            unicode_iterator(parse_buffer* buffer, size_t index, converter& conv) : buf(buffer), i(index), cvt(conv)
            {
                if (buffer == nullptr) i = -1;
                else if ((c = conv.get(buf, i)) == -1)
                {
                    buf = nullptr;
                    i = -1;
                }
            }

            unicode_iterator() : i(-1), buf(nullptr), cvt(eof_conv)
            {
            }

            unicode_iterator(const unicode_iterator& other) : i(other.i), buf(other.buf), cvt(other.cvt) {}

            unicode_iterator& operator=(const unicode_iterator& other)
            {
                i = other.i;
                buf = other.buf;
                cvt = other.cvt;
                c = other.c;
                return *this;
            }

            unicode_iterator& operator++() { increment(); return *this; }

            unicode_iterator operator++(int) { unicode_iterator tmp(*this); operator++(); return tmp; }

            bool operator==(const unicode_iterator& rhs)
            {
                return !operator!=(rhs);
            }

            bool operator!=(const unicode_iterator& rhs)
            {
                return buf != rhs.buf || i != rhs.i;
            }

            char32_t& operator*() { return c; }

            bool eof()
            {
                return buf == nullptr;
            }

            // Returns a string from the current iterator until just before 
            // the "end" iterator.
            std::string to_string(unicode_iterator& end)
            {
                return cvt.get_string(buf, i, end.i);
            }

        protected:

            void increment()
            {
                if (buf != nullptr)
                {
                    i++;
                    if ((c = cvt.get(buf, i)) == std::char_traits<char32_t>::eof())
                    {
                        buf = nullptr;
                        i = -1;
                    }
                }
            }
        };

        class parse_buffer
        {
        private:
            std::vector<char> buf;
            std::streambuf* sbuf;

            class utf8_converter : public converter
            {
                typedef std::codecvt<char32_t, char, std::mbstate_t> facet_type;

                std::locale locale;
                const facet_type& facet;

            public:
                utf8_converter() : facet(std::use_facet<facet_type>(locale))
                {
                }

            protected:
                char32_t get(parse_buffer* buf, size_t& i) override
                {
                    std::mbstate_t state;
                    char32_t wc;
                    char32_t* to = &wc;

                    while (true)
                    {
                        if (!buf->check(i)) return std::char_traits<char32_t>::eof();
                        char c = buf->getc(i++);

                        const char* from = &c;
                        const char* from_next;
                        char32_t* to_next;

                        auto result = facet.in(state, from, from + 1, from_next, to, to + 1, to_next);
                        if (result == facet_type::ok) return wc;
                        else if (result == facet_type::noconv) return wc;
                        else if (result == facet_type::partial) continue;
                        else throw std::exception("unicode conversion error");
                    }
                }

                std::string get_string(parse_buffer* buf, size_t start, size_t count) override
                {
                    if (!buf->check(start + count - 1)) throw std::exception("index out of bounds");
                    return std::string(&buf->getc(start), count);
                }
            };

            class utf32_converter : public converter
            {
            public:

            protected:
                char32_t get(parse_buffer* buf, size_t& i) override
                {
                    if (!buf->check(i + 3)) return std::char_traits<char32_t>::eof();
                    return buf->getc32(i);
                }

                std::string get_string(parse_buffer* buf, size_t start, size_t end) override
                {
                    if (!buf->check(end - 1)) throw std::exception("index out of bounds");

                    std::wstring_convert<std::codecvt_utf8<char32_t, 1114111UL, std::little_endian>, char32_t> cvt;
                    const char32_t* first = &buf->getc32(start);
                    const char32_t* last = &buf->getc32(end);
                    return cvt.to_bytes(first, last);
                }
            };

            class utf32rev_converter : public converter
            {
            public:

            protected:
                char32_t get(parse_buffer* buf, size_t& i)
                {
                    if (!buf->check(i + 3)) return std::char_traits<char32_t>::eof();
                    auto& c = buf->getc32(i);
                    return ((c >> 24) & 0x000000FF) |
                           ((c >> 8)  & 0x0000FF00) |
                           ((c << 8)  & 0x00FF0000) |
                           ((c << 24) & 0xFF000000);
                }

                std::string get_string(parse_buffer* buf, size_t start, size_t end) override
                {
                    if (!buf->check(end - 1)) throw std::exception("index out of bounds");

                    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> cvt;
                    const char32_t* first = &buf->getc32(start);
                    const char32_t* last = &buf->getc32(end);
                    return cvt.to_bytes(first, last);
                }
            };

            class utf16_converter : public converter
            {
                typedef std::codecvt<char32_t, char16_t, std::mbstate_t> facet_type;

                std::locale locale;
                const facet_type& facet;

            public:
                utf16_converter() : facet(std::use_facet<facet_type>(locale))
                {
                }

            protected:
                char32_t get(parse_buffer* buf, size_t& i)
                {
                    std::mbstate_t state;
                    char32_t wc;
                    char32_t* to = &wc;

                    while (true)
                    {
                        if (!buf->check(i)) return std::char_traits<char32_t>::eof();
                        char16_t c = buf->getc16(i++);

                        const char16_t* from = &c;

                        auto result = facet.in(state, from, from + 1, from, to, to + 1, to);
                        if (result == facet_type::ok) return wc;
                        else if (result == facet_type::noconv) return wc;
                        else if (result == facet_type::partial) continue;
                        else throw std::exception("unicode conversion error");
                    }
                }

                std::string get_string(parse_buffer* buf, size_t start, size_t end) override
                {
                    if (!buf->check(end - 1)) throw std::exception("index out of bounds");

                    std::wstring_convert<std::codecvt_utf8<char16_t, 1114111UL, std::little_endian>, char16_t> cvt;
                    const char16_t* first = &buf->getc16(start);
                    const char16_t* last = &buf->getc16(end);
                    return cvt.to_bytes(first, last);
                }
            };

            class utf16rev_converter : public converter
            {
                typedef std::codecvt<char32_t, char16_t, std::mbstate_t> facet_type;

                std::locale locale;
                const facet_type& facet;

            public:
                utf16rev_converter() : facet(std::use_facet<facet_type>(locale))
                {
                }

            protected:
                char32_t get(parse_buffer* buf, size_t& i)
                {
                    std::mbstate_t state;
                    char32_t wc;
                    char32_t* to = &wc;

                    while (true)
                    {
                        if (!buf->check(i)) return std::char_traits<char32_t>::eof();
                        char16_t swapped = buf->getc16(i++);
                        char16_t c = ((swapped >> 8) & 0x00FF) |
                                     ((swapped << 8) & 0xFF00);

                        const char16_t* from = &c;

                        auto result = facet.in(state, from, from + 1, from, to, to + 1, to);
                        if (result == facet_type::ok) return wc;
                        else if (result == facet_type::noconv) return wc;
                        else if (result == facet_type::partial) continue;
                        else throw std::exception("unicode conversion error");
                    }
                }

                std::string get_string(parse_buffer* buf, size_t start, size_t end) override
                {
                    if (!buf->check(end - 1)) throw std::exception("index out of bounds");

                    std::wstring_convert<std::codecvt_utf8<char16_t>, char16_t> cvt;
                    const char16_t* first = &buf->getc16(start);
                    const char16_t* last = &buf->getc16(end);
                    return cvt.to_bytes(first, last);
                }
            };
            
            utf8_converter utf8;
            utf16_converter utf16;
            utf16rev_converter utf16rev;
            utf32_converter utf32;
            utf32rev_converter utf32rev;
            converter* encoding;
            size_t start;

        public:
            parse_buffer(std::streambuf* sbuf) : sbuf(sbuf), encoding(&utf8), start(0)
            {
                // Determine the appropriate iterator based on the BOM
                if (starts_with((char)0xEF, (char)0xBB, (char)0xBF)) { encoding = &utf8; start = 2; }
                else if (starts_with((char32_t)0x0000FEFF)) { encoding = &utf32; start = 3; }
                else if (starts_with((char32_t)0xFFFE0000)) { encoding = &utf32rev; start = 3; }
                else if (starts_with((char16_t)0xFFFE)) { encoding = &utf16; start = 2; }
                else if (starts_with((char16_t)0xFEFF)) { encoding = &utf16rev; start = 2; }
                else { encoding = &utf8; start = 0; }
            }

            bool read()
            {
                auto c = sbuf->sbumpc();
                if (c != std::char_traits<char>::eof())
                    buf.push_back(c);
            }

            bool check(size_t i)
            {
                while (i >= buf.size())
                {
                    auto c = sbuf->sbumpc();
                    if (c == std::char_traits<char>::eof()) return false;
                    else buf.push_back(c);
                }
                return true;
            }

            char& getc(size_t i)
            {
                return buf[i];
            }

            char16_t& getc16(size_t i)
            {
                return *(char16_t*)&buf[i];
            }

            char32_t& getc32(size_t i)
            {
                return *(char32_t*)&buf[i];
            }

            unicode_iterator begin()
            {
                return unicode_iterator(this, start, *encoding);
            }

            unicode_iterator end()
            {
                return unicode_iterator();
            }

            bool starts_with(char16_t c)
            {
                return (check(1) && getc16(0) == c);    
            }

            bool starts_with(char b1, char b2, char b3)
            {
                return (check(2) &&
                    getc(0) == b1 &&
                    getc(1) == b2 &&
                    getc(2) == b3);
            }

            bool starts_with(char32_t c)
            {
                return (check(3) && getc32(0) == c);
            }
        };

        /*
        class parse_buffer_iterator : public std::iterator<std::input_iterator_tag, char>
        {
            size_t i;
            parse_buffer* buffer;

        public:
            parse_buffer_iterator() : i(std::char_traits<char>::eof()), buffer(nullptr) {}

            parse_buffer_iterator(parse_buffer* buffer) : i(0), buffer(buffer)
            {
                check();
            }
            
            parse_buffer_iterator(const parse_buffer_iterator& other) : i(other.i), buffer(other.buffer) {}
            
            parse_buffer_iterator& operator++() { increment(); return *this; }
            
            parse_buffer_iterator operator++(int) { parse_buffer_iterator tmp(*this); operator++(); return tmp; }
            
            bool operator==(const parse_buffer_iterator& rhs)
            {
                return !operator==(rhs);
            }
            
            bool operator!=(const parse_buffer_iterator& rhs)
            {
                return buffer != rhs.buffer || i != rhs.i;
            }
            
            char& operator*() { return buffer->getc(i); }

            bool eof()
            {
                return buffer == nullptr;
            }

        private:
            void check()
            {
                if (!buffer->check(i))
                {
                    i = std::char_traits<char>::eof();
                    buffer = nullptr;
                }
            }

            void increment()
            {
                if (buffer != nullptr)
                {
                    i++;
                    check();
                }
            }
        };
        */

}}}