#ifndef GREENPEAS_POLICIES_STORAGE_HOST_HPP
#define GREENPEAS_POLICIES_STORAGE_HOST_HPP

/// Standard headers
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <type_traits>

/// Project headers
#include "GreenPeas/Common.hpp"

namespace gp {

/// @brief Host (CPU) heap storage.
struct HostStorage {
  /// @brief Allocate a zero-initialized array of `size` elements.
  /// @tparam T Element type.
  /// @param size Number of elements.
  /// @return Pointer to the array; caller must pass it to `deallocate`.
  template <typename T>
  HOST static auto allocate(size_t size) -> T * {
    return new T[size]();
  }

  /// @brief Deallocate an array allocated by `allocate`.
  /// @tparam T Element type.
  /// @param data Pointer returned by `allocate` (or null).
  template <typename T>
  HOST static void deallocate(T *data) {
    delete[] data;
  }

  /// @brief Copy `size` elements from `src` to `dst` (host only).
  /// @tparam T Element type.
  /// @tparam DestinationStorage Must be `HostStorage`.
  /// @param dst Destination buffer.
  /// @param src Source buffer.
  /// @param size Number of `T` elements to copy.
  template <typename T, typename DestinationStorage>
    requires std::is_same_v<DestinationStorage, HostStorage>
  HOST static void
  copyTo(T *dst, const T *src, size_t size, DestinationStorage) {
    std::copy(src, src + size, dst);
  }

  /// @brief Copy `size` elements from `src` to `dst` (host only).
  /// @tparam T Element type.
  /// @tparam SourceStorage Must be `HostStorage`.
  /// @param dst Destination buffer.
  /// @param src Source buffer.
  /// @param size Number of `T` elements to copy.
  template <typename T, typename SourceStorage>
    requires std::is_same_v<SourceStorage, HostStorage>
  HOST static void copyFrom(T *dst, const T *src, size_t size, SourceStorage) {
    std::copy(src, src + size, dst);
  }

  /// @brief Fill `[0, size)` with `0xFF` per byte.
  /// @tparam T Element type.
  /// @param data Buffer to fill.
  /// @param size Number of `T` elements.
  template <typename T>
  HOST static void set(T *data, size_t size) {
    std::memset(data, 0xFF, size * sizeof(T));
  }

  /// @brief Zero-fill `[0, size)`.
  /// @tparam T Element type.
  /// @param data Buffer to clear.
  /// @param size Number of `T` elements.
  template <typename T>
  HOST static void clear(T *data, size_t size) {
    std::memset(data, 0x00, size * sizeof(T));
  }
};

} // namespace gp

#endif // GREENPEAS_POLICIES_STORAGE_HOST_HPP
