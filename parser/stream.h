#include <iterator>
#include <streambuf>
#include <vector>
#include <functional>

namespace parser
{
    namespace streams
    {

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
        };

        class eof_converter : public converter
        {
        public:
            char32_t get(parse_buffer* buf, size_t& index)
            {
                return std::char_traits<char32_t>::eof();
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

            unicode_iterator& operator++() { increment(); return *this; }

            unicode_iterator operator++(int) { unicode_iterator tmp(*this); operator++(); return tmp; }

            bool operator==(const unicode_iterator& rhs)
            {
                return !operator==(rhs);
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
                char32_t get(parse_buffer* buf, size_t& i)
                {
                    std::mbstate_t state;
                    char32_t wc;
                    char32_t* to = &wc;

                    while (true)
                    {
                        if (!buf->check(i)) return std::char_traits<char32_t>::eof();
                        char c = buf->get(i++);

                        const char* from = &c;

                        auto result = facet.in(state, from, from + 1, from, to, to + 1, to);
                        if (result == facet_type::ok) return wc;
                        else if (result == facet_type::noconv) return wc;
                        else if (result == facet_type::partial) continue;
                        else throw std::exception("unicode conversion error");
                    }
                }
            };

            class utf32le_converter : public converter
            {
            public:

            protected:
                char32_t get(parse_buffer* buf, size_t& i)
                {
                    if (!buf->check(i + 3)) return std::char_traits<char32_t>::eof();
                    return buf->getc32(i);
                }
            };

            class utf32be_converter : public converter
            {
            public:

            protected:
                char32_t get(parse_buffer* buf, size_t& i)
                {
                    if (!buf->check(i + 3)) return std::char_traits<char32_t>::eof();
                    return buf->getc32(i);
                }
            };

            class utf16le_converter : public converter
            {
                typedef std::codecvt<char32_t, char, std::mbstate_t> facet_type;

                std::locale locale;
                const facet_type& facet;

            public:
                utf8_converter() : facet(std::use_facet<facet_type>(locale))
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
                        char c = buf->get(i++);

                        const char* from = &c;

                        auto result = facet.in(state, from, from + 1, from, to, to + 1, to);
                        if (result == facet_type::ok) return wc;
                        else if (result == facet_type::noconv) return wc;
                        else if (result == facet_type::partial) continue;
                        else throw std::exception("unicode conversion error");
                    }
                }
            };

            class utf16be_converter : public converter
            {
                typedef std::codecvt<char32_t, char, std::mbstate_t> facet_type;

                std::locale locale;
                const facet_type& facet;

            public:
                utf8_converter() : facet(std::use_facet<facet_type>(locale))
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
                        char c = buf->get(i++);

                        const char* from = &c;

                        auto result = facet.in(state, from, from + 1, from, to, to + 1, to);
                        if (result == facet_type::ok) return wc;
                        else if (result == facet_type::noconv) return wc;
                        else if (result == facet_type::partial) continue;
                        else throw std::exception("unicode conversion error");
                    }
                }
            };
            
            utf8_converter utf8;
            utf16le_converter utf16le;
            utf16be_converter utf16be;
            utf32le_converter utf32le;
            utf32be_converter utf32be;

        public:
            parse_buffer(std::streambuf* sbuf) : sbuf(sbuf)
            {
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
                // Determine the appropriate iterator based on the BOM
                if (starts_with(0xEF, 0xBB, 0xBF)) return unicode_iterator(this, 3, utf8);
                else if (starts_with(0xFF, 0xFE, 0x00, 0x00)) return unicode_iterator(this, 4, utf32le);
                else if (starts_with(0xFE, 0xFF, 0x00, 0x00)) return unicode_iterator(this, 4, utf32be);
                else if (starts_with(0xFF, 0xFE)) return unicode_iterator(this, 4, utf16le);
                else if (starts_with(0xFE, 0xFF)) return unicode_iterator(this, 4, utf16be);
                else return unicode_iterator(this, 0, utf8);
            }

            unicode_iterator end()
            {

            }

            bool starts_with(char b1, char b2)
            {
                return (check(1) && 
                    get(0) == b1 && 
                    get(1) == b2);
            }

            bool starts_with(char b1, char b2, char b3)
            {
                return (check(2) &&
                    get(0) == b1 &&
                    get(1) == b2 &&
                    get(2) == b3);
            }

            bool starts_with(char b1, char b2, char b3, char b4)
            {
                return (check(3) &&
                    get(0) == b1 &&
                    get(1) == b2 &&
                    get(2) == b3 &&
                    get(3) == b4);
            }
        };

        

        

        class utf16_unicode_iterator : public unicode_iterator
        {
            typedef std::codecvt<char32_t, wchar_t, std::mbstate_t> facet_type;

            std::locale locale;
            const facet_type& facet;


        protected:
            char32_t get(parse_buffer* buf, size_t& i)
            {
                std::mbstate_t state;
                char32_t wc;
                char32_t* to = &wc;

                while (true)
                {
                    wchar_t c;
                    
                    if (!buf->check(i)) return std::char_traits<char32_t>::eof();
                    c = buf->get(i++);

                    if (!buf->check(i)) return std::char_traits<char32_t>::eof();
                    c |= buf->get(i++);

                    const wchar_t* from = &c;

                    auto result = facet.in(state, from, from + 1, from, to, to + 1, to);
                    if (result == facet_type::ok) return wc;
                    else if (result == facet_type::noconv) return wc;
                    else if (result == facet_type::partial) continue;
                    else throw std::exception("unicode conversion error");
                }
            }
        };

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
            
            char& operator*() { return buffer->get(i); }

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

        

    }
}