#pragma once

namespace placeholders
{
    template <size_t i>
	struct index : std::integral_constant<size_t, i> {};

    static index<-1> _;
    static index<0> _0;
	static index<1> _1;
	static index<2> _2;
	static index<3> _3;
	static index<4> _4;
	static index<5> _5;
	static index<6> _6;
	static index<7> _7;
	static index<8> _8;
	static index<9> _9;
}