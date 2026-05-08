#ifndef GREENPEAS_CORE_ARRAY_HPP
#define GREENPEAS_CORE_ARRAY_HPP

/// Project headers
#include "GreenPeas/Common.hpp"

namespace gp {

/// @brief Fixed-size hashable array.
///
/// @tparam ValueT Value type. Must be castable to uint64_t and uint32_t.
/// @tparam size The size of the array.
template <typename ValueT, size_t size>
struct Array {
  /// @brief Backing C-array.
  ValueT data[size];

  /// @brief Subscript access (non-const).
  /// @param index Zero-based index in [0, size).
  /// @return Reference to the element at index.
  HOST DEVICE FORCE_INLINE auto operator[](size_t index) -> ValueT & {
    return data[index];
  }

  /// @brief Subscript access (const).
  /// @param index Zero-based index in [0, size).
  /// @return Const reference to the element at index.
  HOST DEVICE FORCE_INLINE auto operator[](size_t index) const
      -> const ValueT & {
    return data[index];
  }

  /// @brief FNV-1a 32-bit hash of the array contents.
  /// @return 32-bit hash value; equal arrays produce equal hashes.
  HOST DEVICE FORCE_INLINE constexpr auto hash32() const -> uint32_t {
    // 32-bit FNV offset basis
    uint32_t hash = 0x811c9dc5;
    // 32-bit FNV prime
    const uint32_t prime = 0x01000193;

#pragma unroll
    for (uint32_t i = 0; i < size; ++i) {
      hash ^= static_cast<uint32_t>(data[i]);
      hash *= prime;
    }

    return hash;
  }

  /// @brief FNV-1a 64-bit hash of the array contents.
  /// @return 64-bit hash value; equal arrays produce equal hashes.
  HOST DEVICE FORCE_INLINE constexpr auto hash64() const -> uint64_t {
    // 64-bit FNV offset basis
    uint64_t hash = 0xcbf29ce484222325;
    // 64-bit FNV prime
    const uint64_t prime = 0x100000001b3;

#pragma unroll
    for (uint32_t i = 0; i < size; ++i) {
      hash ^= static_cast<uint64_t>(data[i]);
      hash *= prime;
    }

    return hash;
  }
};

} // namespace gp

#endif // GREENPEAS_CORE_ARRAY_HPP
