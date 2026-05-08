#ifndef GREENPEAS_CORE_WORDS_HPP
#define GREENPEAS_CORE_WORDS_HPP

/// Standard headers
#include <cstdint>

/// Project headers
#include "GreenPeas/Common.hpp"

namespace gp {

/// @brief Extracts the least significant 32 bits of a 64-bit word.
/// @param word Packed 64-bit value (e.g. two packed uint32_t values).
/// @return The lower 32 bits as uint32_t.
HOST DEVICE FORCE_INLINE constexpr auto getLower(uint64_t word) -> uint32_t {
  return static_cast<uint32_t>(word);
}

/// @brief Extracts the most significant 32 bits of a 64-bit word.
/// @param word Packed 64-bit value (e.g. two packed uint32_t values).
/// @return The upper 32 bits as uint32_t.
HOST DEVICE FORCE_INLINE constexpr auto getUpper(uint64_t word) -> uint32_t {
  return static_cast<uint32_t>(word >> 32);
}

/// @brief Sets the least significant 32 bits of a 64-bit word.
/// @param word Packed 64-bit value to modify in-place.
/// @param lower Value to store in the lower 32 bits.
HOST DEVICE FORCE_INLINE constexpr void setLower(uint64_t &word,
                                                 uint32_t lower) {
  word = (word & 0xFFFFFFFF00000000ULL) | static_cast<uint64_t>(lower);
}

/// @brief Sets the most significant 32 bits of a 64-bit word.
/// @param word Packed 64-bit value to modify in-place.
/// @param upper Value to store in the upper 32 bits.
HOST DEVICE FORCE_INLINE constexpr void setUpper(uint64_t &word,
                                                 uint32_t upper) {
  word = (word & 0x00000000FFFFFFFFULL) | (static_cast<uint64_t>(upper) << 32);
}

/// @brief Number of 64-bit words needed to store n bits (ceiling of n / 64).
/// @tparam T Integer type for the bit count (e.g. uint32_t).
/// @param n Number of bits.
/// @return Smallest number of 64-bit words that can hold n bits.
template <typename T>
HOST DEVICE FORCE_INLINE constexpr auto numWordsForBits(T n) -> uint32_t {
  return static_cast<uint32_t>((static_cast<uint64_t>(n) + 63) / 64);
}

} // namespace gp

#endif // GREENPEAS_CORE_WORDS_HPP
