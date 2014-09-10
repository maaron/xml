// test.cpp : Defines the entry point for the console application.
//

// The extensive use of templates causes this "decorated name length too 
// long" warning all over the place.  Since we aren't exporting any of these 
// template instanciations in a library, it is safe to ignore.
#pragma warning( disable : 4503 )

#include "stdafx.h"

/*
#include "parse.h"
#include "xml.h"
#include <string>
#include <sstream>
#include <iostream>
*/

#include <iostream>
#include "utf8.h"
#include "char16_iterator.h"
#include "char32_iterator.h"

namespace unicode
{

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

enum encoding { utf8, utf16le, utf16be, utf32le, utf32be };

// This class implements an iterator that wraps an octet-based (char) 
// iterator and a specified UTF encoding.  The resulting object then iterates
// unicode characters (char32_t).
template <typename octet_iterator>
class unicode_iterator : public std::iterator<std::forward_iterator_tag, char32_t>
{
    octet_iterator it;
    octet_iterator start;
    octet_iterator end;
    value_type c;
    encoding enc;

    static_assert(sizeof(typename octet_iterator::value_type) == 1, "Illegal value_type size for octet iterator");
    
public:
    explicit unicode_iterator (const octet_iterator& from, const octet_iterator& to, encoding e) : it(from), start(from), end(to), enc(e)
    {
        get();
    }

    value_type operator * () const
    {
        return c;
    }

    bool operator == (const unicode_iterator& rhs) const 
    { 
        return (it == rhs.it);
    }

    bool operator != (const unicode_iterator& rhs) const
    {
        return !(operator == (rhs));
    }

    unicode_iterator& operator ++ () 
    {
        if (it != end)
        {
            get();
        }
        return *this;
    }

    unicode_iterator operator ++ (int)
    {
        unicode_iterator temp(*this);
        if (it != end)
        {
            get();
        }
        return temp;
    }

private:
    void get()
    {
        if (it == end) return;

        switch (enc)
        {
        case utf8:
            c = utf8::next(it, end);
            break;

        case utf16le:
            {
                unicode::char16_iterator<decltype(it), true> it16(it, end);
                unicode::char16_iterator<decltype(it), true> end16(end, end);
                c = utf8::utf16::next(it16, end16);
                it = it16.base();
            }
            break;

        case utf16be:
            {
                unicode::char16_iterator<decltype(it), false> it16(it, end);
                unicode::char16_iterator<decltype(it), false> end16(end, end);
                c = utf8::utf16::next(it16, end16);
                it = it16.base();
            }
            break;

        case utf32le:
            {
                unicode::char32_iterator<decltype(it), true> it32(it, end);
                c = *it32++;
                it = it32.base();
            }
            break;

        case utf32be:
            {
                unicode::char32_iterator<decltype(it), false> it32(it, end);
                c = *it32++;
                it = it32.base();
            }
            break;

        default:
            throw std::exception("Unexpected encoding");
        }
    }
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
class unicode_container
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
            try_encoding<utf32le>("\xFF\xFE\x00\x00", 4) ||
            try_encoding<utf32be>("\x00\x00\xFF\xFE", 4) ||
            try_encoding<utf16le>("\xFF\xFE", 2) ||
            try_encoding<utf16be>("\xFE\xFF", 2)))
        {
            // No BOM present, assume UTF-8.  This will also work for ASCII.
            enc = utf8;
            bom_size = 0;
        }
    }

public:
    unicode_container(octet_container& c) : octets(c)
    {
        detect_encoding();
    }

    unicode_iterator<octet_iterator> begin()
    {
        return unicode_iterator<octet_iterator>(octets.begin() + bom_size, octets.end(), enc);
    }

    unicode_iterator<octet_iterator> end()
    {
        return unicode_iterator<octet_iterator>(octets.end(), octets.end(), enc);
    }
};

}

int _tmain(int argc, _TCHAR* argv[])
{  
  //std::string v = "\xEF\xBB\xBFthis is a UTF8-encoded string with a BOM.";
  //std::string v = "this is a UTF8-encoded string without a BOM.";
  std::wstring wv(L"\uFEFFthis is a UTF16-encoded string with a BOM.");
  std::string v((const char*)wv.c_str(), wv.size() * 2);

  unicode::unicode_container<std::string> ustring(v);

  auto i = ustring.begin();
  auto end = ustring.end();

  for (; i != end; i++)
  {
      std::cout << "char: " << (char)*i << std::endl;
  }

  /*
  using namespace parse;
  using namespace parse::operators;

  std::string test_data("<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\">");
  auto p = xml::parser::prolog() >> xml::parser::element_open();
  parse::ast_type<decltype(p), std::string::iterator>::type p_ast;
  bool valid = p.parse(test_data, p_ast);

  std::istringstream xml_data("<?xml encoding='UTF-8'?><nspre:root attribute1=\"value1\"><ns:child1>child1 content<grandchild11></grandchild11></ns:child1><child2>child2 content</child2></nspre:root>");
  xml::document doc(xml_data);

  auto root = doc.root();

  auto name = root.name();
  auto localname = root.local_name();
  */

  return 0;
}

