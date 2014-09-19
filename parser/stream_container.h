#pragma once

#include <assert.h>
#include "util.h"
#include <vector>
#include <streambuf>

namespace util
{
    template <typename streambuf_container>
    class streambuf_iterator 
        : public std::iterator<std::random_access_iterator_tag, typename streambuf_container::value_type>
    {
    public:

    private:
        typedef typename streambuf_container::pos_type pos_type;

        streambuf_container* container;
        pos_type pos;
        value_type value;

        void advance(pos_type n)
        {
            pos += n;
        }

        void get()
        {
            if (container == nullptr)
            {
                pos = streambuf_container::npos;
                return;
            }

            value = (*container)[pos];
            if (value == std::char_traits<value_type>::eof())
                pos = streambuf_container::npos;
        }

        bool eof()
        {
            return pos == streambuf_container::npos;
        }

    public:
        streambuf_iterator() : container(nullptr), pos(streambuf_container::npos)
        {
        }

        streambuf_iterator(streambuf_container* c, pos_type p) : container(c), pos(p)
        {
            get();
        }

        value_type operator * () const
        {
            return value;
        }

        bool operator == (const streambuf_iterator& rhs) const
        {
            return (pos == rhs.pos);
        }

        bool operator != (const streambuf_iterator& rhs) const
        {
            return !(operator == (rhs));
        }

        bool operator< (const streambuf_iterator& rhs) const
        {
            return pos < rhs.pos;
        }

        streambuf_iterator& operator ++ ()
        {
            if (!eof())
            {
                advance(1);
                get();
            }
            return *this;
        }

        streambuf_iterator operator ++ (int)
        {
            streambuf_iterator temp(*this);
            if (!eof())
            {
                advance(1);
                get();
            }
            return temp;
        }

        streambuf_iterator operator+ (pos_type offset)
        {
            return streambuf_iterator(container, pos + offset);
        }

        streambuf_iterator& operator+= (pos_type offset)
        {
            if (!eof())
            {
                advance(offset);
                get();
            }
            return *this;
        }
    };

    template <typename streambuf_t>
    class streambuf_container
    {
        static_assert(std::is_same<char, typename streambuf_t::char_type>::value, "stream_container only supports char streams");

    public:
        typedef streambuf_iterator<streambuf_container<streambuf_t> > iterator;
        typedef typename streambuf_t::char_type value_type;
        typedef size_t pos_type;
        static const pos_type npos = -1;
        typedef size_t int_type;

    private:
        std::vector<value_type> buf;
        streambuf_t* sbuf;

    public:
        streambuf_container(streambuf_t* s) : sbuf(s)
        {
            assert(s != nullptr);
        }

        iterator begin()
        {
            return iterator(this, 0);
        }

        iterator end()
        {
            return iterator(this, npos);
        }

        int_type operator[](pos_type p)
        {
            while (p >= buf.size())
            {
                auto v = sbuf->sbumpc();
                buf.push_back(v);
                if (v == std::char_traits<value_type>::eof())
                    return v;
            }
            return buf[p];
        }
    };
};