module;
#include <iostream>
#include <immintrin.h>
#include <bit>
export module Bitboards;
import Types;

export namespace Bitwise{
	template<std::integral T>
	constexpr void set_bit(T& bits, int index) { bits |= 1ULL << index; }
	
	template<std::integral T>
	constexpr void clear_bit(T& bits, int index) { bits &= ~(1ULL << index); }

	template<std::integral T>
	inline int pop_lsb(T& bits) {
		int s = _tzcnt_u64(bits);    // Direct hardware call
		bits = _blsr_u64(bits);      // BMI1 hardware bit reset
		return s;
	}

	template<std::integral T>
	constexpr int lsb(T bits) {
		return static_cast<int>(_tzcnt_u64(bits));
	}

	template<std::integral T>
	constexpr int count(T bits) {
#if defined(_MSC_VER) && defined(__AVX2__)
		return static_cast<int>(__popcnt64(bits));
#else
		return std::popcount(bits); // Clean fallback
#endif
	}
}
export namespace Bitboards {

}
