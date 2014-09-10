#pragma once

#include <iterator>
#include <type_traits>

namespace unicode {

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

}