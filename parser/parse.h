#pragma once

#include <vector>
#include <functional>

namespace parse
{

  template <size_t i>
  struct index {};

  index<0> _i0;
  index<1> _i1;
  index<2> _i2;
  index<3> _i3;
  index<4> _i4;
  index<5> _i5;
  index<6> _i6;
  index<7> _i7;
  index<8> _i8;
  index<9> _i9;

  // This meta-class is used to create list structures similar to runtime 
  // linked-lists.  This list structure consists of an item<t1, t2> type, where 
  // t1 may be an item<t11, t12> itself.  The inner-most t1 (i.e., t111...1) is
  // a list of two elements, and each t2 parameter appends to the end of the 
  // list.  Thus, list_t<t1, t2> repesents an element 't2' that is appended to the
  // end of the list 't1'.  The first parameter, t1, is only considered a list if 
  // it is an instance of the list_t template.  Otherwise, the result is just a 
  // pair (2 element list).
  //
  // Examples:
  //
  //  // Create a 2-element list (int, char*).
  //  static_list<mylist_t>::item<int, char*> mypair;
  //
  //  // Create a 3-element list (int, char*, char)
  //  static_list<mylist_t>::item<decltype(mypair), char> mylist;
  //
  //  // Modify the second element of mylist
  //  mylist[_1] = "hello";
  
  // Creates a two element list
  template <template <typename,typename> class derived_t, typename first_t, typename second_t>
  struct list
  {
    first_t first;
    second_t second;

    static const size_t size = 2;
    static const size_t lastindex = 1;

    template <size_t i> struct elem;
    template <> struct elem<0> { typedef first_t type; };
    template <> struct elem<1> { typedef second_t type; };

    first_t& operator[](const index<0>&) { return first; }
    second_t& operator[](const index<1>&) { return second; }
  };

  // Creates an N+1-element list by appending second_t to the N-element list 
  // list_t<t1, t2>.
  template <template <typename,typename> class derived_t, typename t1, typename t2, typename second_t>
  struct list< derived_t, derived_t<t1, t2>, second_t >
  {
    typedef derived_t<t1, t2> first_t;

    first_t first;
    second_t second;

    static const size_t size = first_t::size + 1;

    template <size_t i> struct elem
    {
      static_assert(i < size, "Element type index out of range");
      typedef typename first_t::elem<i>::type type;
    };
    template <> struct elem<size-1> { typedef second_t type; };

    second_t& operator[](const index<size-1>&) { return second; }

    template <size_t i>
    typename elem<i>::type& operator[](const index<i>& idx)
    {
      static_assert(i < size, "Element index out of range");
      return first[idx];
    }
  };

  namespace tree
  {
    // Base class for all AST's.  Contains information about where in the token 
    // stream the parser matched and the size, in tokens.
    template <typename iterator_t>
    struct ast_base
    {
      iterator_t start;
      iterator_t end;
      bool matched;

      ast_base() : matched(false)
      {
      }
    };

    template <typename iterator_t>
    struct ast_list
    {
      template <typename t1, typename t2>
      struct sequence : 
        list<sequence, t1, t2>,
        ast_base<iterator_t>
      {
      };

      template <typename t1, typename t2>
      struct alternate : 
        list<alternate, t1, t2>,
        ast_base<iterator_t>
      {
      };
    };

    template <typename parser_t, typename stream_t>
    typename parser_t::ast<typename stream_t::iterator>::type make_ast(parser_t&, stream_t&)
    {
      return typename parser_t::ast<typename stream_t::iterator>::type();
    }

  }


  // Base class for all parsers.  The main method, parse(stream_t&, ast&) 
  // tracks the start and end positions of the underlying parser's match, and 
  // updates the AST accordingly.
  template <typename derived_t>
  class parser
  {
  public:
    template <typename iterator_t, typename ast_t>
    bool parse_from(iterator_t& start, iterator_t& end, ast_t& ast)
    {
      auto matchEnd = start;

      if (static_cast<derived_t*>(this)->parse_internal(matchEnd, end, ast))
      {
        ast.start = start;
        start = ast.end = matchEnd;
        return (ast.matched = true);
      }
      else
      {
        return (ast.matched = false);
      }
    }

    template <typename stream_t, typename ast_t>
    bool parse(stream_t& s, ast_t& ast)
    {
      return parse_from(s.begin(), s.end(), ast);
    }
  };

  // Matches the specified parser zero or more times.  The stream is checked 
  // for eof first, which is still considered a match.
  template <typename parser_t>
  class zero_or_more : public parser< zero_or_more<parser_t> >
  {
    parser_t elem;

  public:
    template <typename iterator_t>
    struct ast : public tree::ast_base<iterator_t>
    {
      typedef ast type;
      std::vector<typename parser_t::ast> children;
    };

    template <typename iterator_t, typename ast_t>
    bool parse_internal(iterator_t& start, iterator_t& end, ast_t& ast)
    {
      typename parser_t::ast elem_tree;
      while (!s.eof() && elem.parse_from(s, elem_tree))
      {
        tree.children.push_back(elem_tree);
      }
      return true;
    }
  };

  // A parser that matches if either of the two supplied parsers match.  The 
  // second parser won't be tried if the first matches.
  template <typename first_t, typename second_t>
  class alternate : public parser< alternate<first_t, second_t> >
  {
    first_t first;
    second_t second;

  public:
    template <typename iterator_t>
    struct ast
    {
      typedef typename tree::ast_list<iterator_t>::alternate<
        typename first_t::ast<iterator_t>::type,
        typename second_t::ast<iterator_t>::type> type;
    };

    template <typename iterator_t>
    bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
    {
      return (tree.first_matched = first.parse_from(s, tree.first)) || 
        second.parse_from(s, tree.second);
    }
  };

  // A parser that matches only if both of the given parsers match in 
  // sequence.
  template <typename first_t, typename second_t>
  class sequence : public parser< sequence<first_t, second_t> >
  {
    first_t first;
    second_t second;

  public:
    template <typename iterator_t>
    struct ast
    {
      typedef typename tree::ast_list<iterator_t>::sequence<
        typename first_t::ast<iterator_t>::type, 
        typename second_t::ast<iterator_t>::type> type;
    };

    template <typename iterator_t>
    bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
    {
      return first.parse_from(start, end, tree.first) && 
        second.parse_from(start, end, tree.second);
    }
  };

  // This is the base class for all parsers that match a single token.  This 
  // class handles reading the single token, and calls the derived class's 
  // match() method to determine whether the token matches.  The token type, 
  // token_t, can be any type with a parameterless constructor.  Also, the 
  // stream_t must return values that are assignable to token_t.
  template <typename derived_t, typename token_t>
  class single : public parser< single<derived_t, token_t> >
  {
  public:
    template <typename iterator_t>
    struct ast : public tree::ast_base<iterator_t>
    {
      typedef ast type;
      token_t token;

      ast() : token(0)
      {
      }
    };

    template <typename iterator_t>
    bool parse_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
    {
      return start != end && static_cast<derived_t*>(this)->match(tree.token = *start++);
    }
  };

  // A parser that matches a single token, in cases where the token_t type is 
  // integral (e.g., a char, wchar_t, etc., representing a character).
  template <typename token_t, token_t t>
  class constant : public single< constant<token_t, t>, token_t >
  {
  public:
    bool match(token_t token)
    {
      return token == t;
    }
  };

  // A zero-length parser that matches only at the end of the stream.
  class end : public parser<end>
  {
  public:
    template <typename iterator_t>
    struct ast
    {
      typedef tree::ast_base<iterator_t> type;
    };

    template <typename iterator_t>
    bool parse_internal(iterator_t& s, iterator_t& end, typename ast<iterator_t>::type& tree)
    {
      return s.eof();
    }
  };

  // A zero-length parser that matches only at the beginning of the stream.
  class start : public parser<start>
  {
  public:
    template <typename iterator_t>
    struct ast
    {
      typedef tree::ast_base<iterator_t> type;
    };

    template <typename iterator_t>
    bool parser_internal(iterator_t& start, iterator_t& end, typename ast<iterator_t>::type& tree)
    {
      return s.pos() == 0;
    }
  };

  // This namespace defines operators that are useful in cosntructing parsers 
  // using a convenient syntax (shorter than a complicated, nested template 
  // instantiation).
  namespace operators
  {
    // Generates an alternate<first_t, second_t> parser
    template <typename first_t, typename second_t>
    alternate<first_t, second_t> operator| (const first_t& first, const second_t& second)
    {
      return alternate<first_t, second_t>();
    }

    // Generates a sequence<first_t, second_t> parser
    template <typename first_t, typename second_t>
    sequence<first_t, second_t> operator>> (const first_t& first, const second_t& second)
    {
      return sequence<first_t, second_t>();
    }

    // Generates a zero_or_more<parser_t> parser
    template <typename parser_t>
    zero_or_more<parser_t> operator* (parser_t& e)
    {
      return zero_or_more<parser_t>();
    }

  }

}
