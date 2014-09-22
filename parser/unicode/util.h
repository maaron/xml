#pragma once

namespace util
{
    // Template constant that always evaluates to false.  Useful for 
    // static_assert's that shouldn't fire unless the containing template is 
    // instanciated.
    template <typename T>
    struct always_false { enum { value = false }; };
}