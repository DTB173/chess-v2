module;
#include <iostream>
#include <bit>
export module Bitboards;
import Types;

export namespace Bitwise{
	template<std::integral T>
	constexpr void set_bit(T& bits, int index) { bits |= 1ULL << index; }
	
	template<std::integral T>
	constexpr void clear_bit(T& bits, int index) { bits &= ~(1ULL << index); }

	template<std::integral T>
	constexpr int pop_lsb(T& bits) {
		int s = std::countr_zero(bits);
		bits &= (bits - 1);
		return s;
	}

	template<std::integral T>
	constexpr int lsb(T bits) {
		return std::countr_zero(bits);
	}

	template<std::integral T>
	constexpr int count(T bits) { return std::popcount(bits); }
}
export namespace Bitboards {

}
