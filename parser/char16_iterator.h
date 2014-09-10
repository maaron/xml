#pragma once

#include <iterator>
#include <type_traits>

namespace unicode {

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

}