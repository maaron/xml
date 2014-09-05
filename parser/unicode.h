#pragma once

#include <ctype.h>
#include "parse.h"

namespace parse {
	namespace unicode {

		template <char32_t t>
		class u : public constant<unsigned int, t>
		{
		};

		struct digit : public single<digit, char32_t>
		{
			bool match(char32_t t)
			{
				return isdigit(t) != 0;
			}
		};

		struct alpha : public single<alpha, char32_t>
		{
			bool match(char32_t t)
			{
				return isalpha(t) != 0;
			}
		};

	}
}
