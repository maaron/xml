#pragma once

#include <iterator>
#include <type_traits>
#include "util.h"
#include "utf8.h"

namespace unicode
{
    // This class takes an octet_iterator (with an 8-bit value_type, i.e., 
    // char), and converts it into a 16-bit iterator with the specified 
    // endianness.
    template <typename octet_iterator, bool little_endian>
    class char16_iterator : public std::iterator <std::bidirectional_iterator_tag, char16_t>
    { 
        octet_iterator it;
        octet_iterator start;
        octet_iterator end;
        value_type c;

        static_assert(sizeof(typename octet_iterator::value_type) == 1, "Illegal value_type size for octet iterator");
    
    public:
        explicit char16_iterator (const octet_iterator& from, const octet_iterator& to) : it(from), start(from), end(to)
        {
            get();
        }

        octet_iterator base()
        {
            return it;
        }

        value_type operator * () const
        {
            return c;
        }

        bool operator == (const char16_iterator& rhs) const 
        { 
            return (it == rhs.it);
        }

        bool operator != (const char16_iterator& rhs) const
        {
            return !(operator == (rhs));
        }

        char16_iterator& operator ++ () 
        {
            if (it != end)
            {
                std::advance(it, sizeof(value_type));
                get();
            }
            return *this;
        }

        char16_iterator operator ++ (int)
        {
            char16_iterator temp(*this);
            if (it != end)
            {
                std::advance(it, sizeof(value_type));
                get();
            }
            return temp;
        }  

        char16_iterator& operator -- ()
        {
            if (it != start)
            {
                std::advance(it, -sizeof(value_type));
                get();
            }
            return *this;
        }

        char16_iterator operator -- (int)
        {
            iterator temp(*this);
            if (it != start)
            {
                std::advance(it, -sizeof(value_type));
                get();
            }
            return temp;
        }

    protected:
        void get()
        {
            char b1, b2;

            if (it == end)
            {
                c = std::char_traits<char16_t>::eof();
                return;
            }
            else b1 = *it;

            octet_iterator next = it + 1;

            if (next == end)
            {
                c = std::char_traits<char16_t>::eof();
                return;
            }
            else b2 = *next;

            c = little_endian ? 
                ((b2 << 8) & 0xFF00) | (b1 & 0x00FF) :
                ((b1 << 8) & 0xFF00) | (b2 & 0x00FF);
        }

    };

    // This class converts a container with an 8-bit (char) value_type into 
    // one with a 16-bit value_type.
    template <typename octet_container, bool little_endian>
    class char16_container
    {
        typedef char16_iterator<typename octet_container::iterator, little_endian> iterator;

        octet_container container;

    public:
        char16_container(octet_container& c) : container(c)
        {
        }

        iterator begin()
        {
            return iterator(container.begin(), container.end());
        }

        iterator end()
        {
            return iterator(container.end(), container.end());
        }
    };

    // This class converts an 8-bit iterator (char) into a 32-bit iterator.
    template <typename octet_iterator, bool little_endian>
    class char32_iterator : public std::iterator <std::bidirectional_iterator_tag, char32_t>
    { 
        octet_iterator it;
        octet_iterator start;
        octet_iterator end;
        value_type c;

        static_assert(sizeof(typename octet_iterator::value_type) == 1, "Illegal value_type size for octet iterator");
    
    public:
        explicit char32_iterator (const octet_iterator& from, const octet_iterator& to) : it(from), start(from), end(to)
        {
            get();
        }

        octet_iterator base()
        {
            return it;
        }

        value_type operator * () const
        {
            return c;
        }

        bool operator == (const char32_iterator& rhs) const 
        { 
            return (it == rhs.it);
        }

        bool operator != (const char32_iterator& rhs) const
        {
            return !(operator == (rhs));
        }

        char32_iterator& operator ++ () 
        {
            if (it != end)
            {
                std::advance(it, sizeof(value_type));
                get();
            }
            return *this;
        }

        char32_iterator operator ++ (int)
        {
            char32_iterator temp(*this);
            if (it != end)
            {
                std::advance(it, sizeof(value_type));
                get();
            }
            return temp;
        }  

        char32_iterator& operator -- ()
        {
            if (it != start)
            {
                std::advance(it, -sizeof(value_type));
                get();
            }
            return *this;
        }

        char32_iterator operator -- (int)
        {
            char32_iterator temp(*this);
            if (it != start)
            {
                std::advance(it, -sizeof(value_type));
                get();
            }
            return temp;
        }

    protected:
        void get()
        {
            char b1, b2, b3, b4;
            octet_iterator next1 = it + 1;
            octet_iterator next2 = it + 2;
            octet_iterator next3 = it + 3;

            if (it == end) { c = std::char_traits<char32_t>::eof(); return; }
            else b1 = *it;

            if (next1 == end) { c = std::char_traits<char32_t>::eof(); return; }
            else b2 = *next1;

            if (next2 == end) { c = std::char_traits<char32_t>::eof(); return; }
            else b3 = *next2;

            if (next3 == end) { c = std::char_traits<char32_t>::eof(); return; }
            else b4 = *next3;

            c = little_endian ? 
                ((b4 << 24) & 0xFF000000) | ((b3 << 16) & 0x00FF0000) | ((b2 << 8) & 0x0000FF00) | (b1 & 0x000000FF) :
                ((b1 << 24) & 0xFF000000) | ((b2 << 16) & 0x00FF0000) | ((b3 << 8) & 0x0000FF00) | (b4 & 0x000000FF);
        }

    };

    // This class converts a container with an 8-bit (char) value_type info 
    // one with a 32-bit value_type.
    template <typename octet_container, bool little_endian>
    class char32_container
    {
        typedef char32_iterator<typename octet_container::iterator, little_endian> iterator;

        octet_container container;

    public:
        char32_container(octet_container& c) : container(c)
        {
        }

        iterator begin()
        {
            return iterator(container.begin(), container.end());
        }

        iterator end()
        {
            return iterator(container.end(), container.end());
        }
    };

    // This function (is probably implemented in some Boost or C++11/14 library) 
    // checks whether a container starts with another container by iteratively 
    // comparing the elements of each.  If 'c' does not start with elements in
    // 'sub' (all of them), the function returns false.
    template <typename container, typename sub_container>
    bool starts_with(container& c, sub_container& sub)
    {
        auto c_it = c.begin();
        auto c_end = c.end();

        auto sub_it = sub.begin();
        auto sub_end = sub.end();

        while (c_it != c_end && sub_it != sub_end)
        {
            if (*c_it != *sub_it) return false;
            c_it++;
            sub_it++;
        }

        return (sub_it == sub_end);
    }

    // UTF encodings supported by unicode_iterator
    enum encoding { utf8, utf16le, utf16be, utf32le, utf32be };

    template <typename octet_iterator, typename enable = void>
    class unicode_iterator
    {
        static_assert(util::always_false<octet_iterator>::value, "unicode_iterator doesn't support the specified container type.");
    };

    // This class implements an iterator that wraps an octet-based (char) 
    // iterator and a specified UTF encoding.  The resulting object then iterates
    // unicode characters (char32_t).
    template <typename octet_iterator>
    class unicode_iterator<octet_iterator, typename std::enable_if<sizeof(typename octet_iterator::value_type) == sizeof(char)>::type>
        : public std::iterator<std::forward_iterator_tag, char32_t>
    {
        octet_iterator current;
        octet_iterator next;
        octet_iterator end;
        value_type c;
        encoding enc;

    public:
        unicode_iterator()
        {
        }

        explicit unicode_iterator (const octet_iterator& from, const octet_iterator& to, encoding e) : current(from), next(from), end(to), enc(e)
        {
            get();
        }

        value_type operator * () const
        {
            return c;
        }

        bool operator == (const unicode_iterator& rhs) const 
        { 
            return (current == rhs.current);
        }

        bool operator != (const unicode_iterator& rhs) const
        {
            return !(operator == (rhs));
        }

        bool operator< (const unicode_iterator& rhs) const
        {
            return current < rhs.current;
        }

        unicode_iterator& operator ++ () 
        {
            if (current != end)
            {
                get();
            }
            return *this;
        }

        unicode_iterator operator ++ (int)
        {
            unicode_iterator temp(*this);
            if (current != end)
            {
                get();
            }
            return temp;
        }

    private:
        void get()
        {
            current = next;

            if (current == end)
            {
                c = std::char_traits<value_type>::eof();
                return;
            }

            switch (enc)
            {
            case utf8:
                c = utf8::next(next, end);
                break;

            case utf16le:
                {
                    unicode::char16_iterator<decltype(next), true> it16(next, end);
                    unicode::char16_iterator<decltype(next), true> end16(end, end);
                    c = utf8::utf16::next(it16, end16);
                    next = it16.base();
                }
                break;

            case utf16be:
                {
                    unicode::char16_iterator<decltype(next), false> it16(next, end);
                    unicode::char16_iterator<decltype(next), false> end16(end, end);
                    c = utf8::utf16::next(it16, end16);
                    next = it16.base();
                }
                break;

            case utf32le:
                {
                    unicode::char32_iterator<decltype(next), true> it32(next, end);
                    c = *it32++;
                    next = it32.base();
                }
                break;

            case utf32be:
                {
                    unicode::char32_iterator<decltype(next), false> it32(next, end);
                    c = *it32++;
                    next = it32.base();
                }
                break;

            default:
                throw std::exception("Unexpected encoding");
            }
        }
    };

    // This class is the same as above, but appropriate for wchar_t/char16_t 
    // iterators.  It is used in conjunction with the wchar_t unicode_iterator.
    template <typename wchar_iterator>
    class unicode_iterator<wchar_iterator, typename std::enable_if<sizeof(typename wchar_iterator::value_type) == sizeof(char16_t)>::type>
        : public std::iterator<std::forward_iterator_tag, char32_t>
    {
        wchar_iterator current;
        wchar_iterator next;
        wchar_iterator end;
        value_type c;
        bool swap_bytes;

    public:
        unicode_iterator() : swap_bytes(false), c(std::char_traits<value_type>::eof())
        {
        }

        explicit unicode_iterator(const wchar_iterator& from, const wchar_iterator& to, bool swap) : current(from), next(from), end(to), swap_bytes(swap)
        {
            get();
        }

        value_type operator * () const
        {
            return c;
        }

        bool operator == (const unicode_iterator& rhs) const
        {
            return (current == rhs.current);
        }

        bool operator != (const unicode_iterator& rhs) const
        {
            return !(operator == (rhs));
        }

        unicode_iterator& operator ++ ()
        {
            if (current != end)
            {
                get();
            }
            return *this;
        }

        unicode_iterator operator ++ (int)
        {
            unicode_iterator temp(*this);
            if (current != end)
            {
                get();
            }
            return temp;
        }

    private:
        void get()
        {
            current = next;

            if (current == end)
            {
                c = std::char_traits<value_type>::eof();
                return;
            }

            c = utf8::utf16::next(next, end);

            if (swap_bytes) c =
                ((c >> 8) & 0x00FF) |
                ((c << 8) & 0xFF00);
        }
    };

    template <typename octet_container, typename Enable = void>
    class unicode_container
    {
        static_assert(util::always_false<octet_container>::value, "unicode_container doesn't support the specified container type.");
    };

    // This class implements a container that wraps other STL-style containers 
    // with begin()/end() methods that return iterators.  The wrapped container
    // must contain single byte elements (i.e., char) type.  Examples of such 
    // STL containers are std::string, std::vector<char>, etc.  This class treats
    // the underlying container as housing a UTF-encoded string with an optional 
    // BOM (mandatory for UTF-16 and UTF-32 strings).  The begin() method returns
    // an iterator that points to the first unicode character following the BOM.
    // The value_type for the iterator is char32_t, which is large enough to hold
    // any unicode character.
    template <typename octet_container>
    class unicode_container<octet_container, typename std::enable_if<sizeof(typename octet_container::value_type) == sizeof(char)>::type>
    {
        typedef typename octet_container::iterator octet_iterator;

        encoding enc;
        octet_container& octets;
        size_t bom_size;

        template <encoding e>
        bool try_encoding(const char* bom, size_t size)
        {
            bool matches = starts_with(octets, std::string(bom, size));
            if (matches) { enc = e; bom_size = size; }
            return matches;
        }

        void detect_encoding()
        {
            if (!(
                try_encoding<utf8>("\xEF\xBB\xBF", 3) ||
                try_encoding<utf32be>("\xFF\xFE\x00\x00", 4) ||
                try_encoding<utf32le>("\x00\x00\xFE\xFF", 4) ||
                try_encoding<utf16be>("\xFF\xFE", 2) ||
                try_encoding<utf16le>("\xFE\xFF", 2)))
            {
                // No BOM present, assume UTF-8.  This will also work for ASCII.
                enc = utf8;
                bom_size = 0;
            }
        }

    public:
        typedef unicode_iterator<octet_iterator> iterator;

        unicode_container(octet_container& c) : octets(c)
        {
            detect_encoding();
        }

        iterator begin()
        {
            return iterator(octets.begin() + bom_size, octets.end(), enc);
        }

        iterator end()
        {
            return iterator(octets.end(), octets.end(), enc);
        }
    };

    // This class does the same as the one above, but appropriate for 
    // containers that have 2-byte value types.  It assumes that each 2-byte 
    // value is a UTF-16 encoded character.  This is mainly useful in cases 
    // where the user is dealing with a std::wstring (or wchar_t/char16_t 
    // array, etc).  Currently, this class doesn't support UTF-32.
    template <typename wchar_container>
    class unicode_container<wchar_container, typename std::enable_if<sizeof(typename wchar_container::value_type) == sizeof(char16_t)>::type>
    {
        typedef typename wchar_container::iterator wchar_iterator;
        
        wchar_container& container;
        size_t bom_size;
        bool swap_bytes;

        void detect_encoding()
        {
            if (starts_with(container, std::wstring(L"\uFEFF", 1)))
            {
                swap_bytes = true;
                size_t bom_size = 1;
            }
            else if (starts_with(container, std::wstring(L"\uFFFE", 1)))
            {
                swap_bytes = false;
                bom_size = 1;
            }
            else
            {
                swap_bytes = false;
                bom_size = 0;
            }
        }

    public:
        typedef unicode_iterator<wchar_iterator> iterator;

        unicode_container(wchar_container& c) : container(c)
        {
            detect_encoding();
        }

        iterator begin()
        {
            return iterator(container.begin() + bom_size, container.end(), swap_bytes);
        }

        iterator end()
        {
            return iterator(container.end(), container.end(), swap_bytes);
        }
    };

}