#ifndef GREENPEAS_POLICIES_STORAGE_CUDA_HPP
#define GREENPEAS_POLICIES_STORAGE_CUDA_HPP

/// Standard headers
#include <cstddef>
#include <stdexcept>
#include <type_traits>

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

namespace gp {

/// @brief Device (GPU) heap storage.
struct CUDAStorage {
  /// @brief Allocate an array of `size` elements in device memory.
  /// @tparam T Element type.
  /// @param size Number of elements.
  /// @return Pointer to the array; caller must pass it to `deallocate`.
  template <typename T>
  HOST static auto allocate(size_t size) -> T * {
    T *data = nullptr;

    if (cudaMalloc(&data, size * sizeof(T)) != cudaSuccess) {
      throw std::runtime_error("CUDAStorage: Failed to allocate memory.");
    }

    return data;
  }

  /// @brief Deallocate an array allocated by `allocate`.
  /// @tparam T Element type.
  /// @param data Pointer returned by `allocate` (or null).
  template <typename T>
  HOST static void deallocate(T *data) {
    if (data) {
      cudaFree(data);
    }
  }

  /// @brief Copy `size` elements from `src` to `dst` (device or host).
  /// @tparam T Element type.
  /// @tparam DestinationStorage Must be `CUDAStorage` or `HostStorage`.
  /// @param dst Destination buffer.
  /// @param src Source buffer.
  /// @param size Number of `T` elements to copy.
  template <typename T, typename DestinationStorage>
  requires std::is_same_v<DestinationStorage, CUDAStorage> ||
      std::is_same_v<DestinationStorage, HostStorage>
          HOST static void
          copyTo(T *dst, const T *src, size_t size, DestinationStorage) {
    constexpr auto kind = std::is_same_v<DestinationStorage, CUDAStorage>
                              ? cudaMemcpyDeviceToDevice
                              : cudaMemcpyDeviceToHost;
    cudaMemcpy(dst, src, size * sizeof(T), kind);
  }

  /// @brief Copy `size` elements from `src` (device or host) to `dst`.
  /// @tparam T Element type.
  /// @tparam SourceStorage Must be `CUDAStorage` or `HostStorage`.
  /// @param dst Destination buffer.
  /// @param src Source buffer.
  /// @param size Number of `T` elements to copy.
  template <typename T, typename SourceStorage>
  requires std::is_same_v<SourceStorage, CUDAStorage> ||
      std::is_same_v<SourceStorage, HostStorage>
          HOST static void
          copyFrom(T *dst, const T *src, size_t size, SourceStorage) {
    constexpr auto kind = std::is_same_v<SourceStorage, CUDAStorage>
                              ? cudaMemcpyDeviceToDevice
                              : cudaMemcpyHostToDevice;
    cudaMemcpy(dst, src, size * sizeof(T), kind);
  }

  /// @brief Fill `[0, size)` with `0xFF` per byte.
  /// @tparam T Element type.
  /// @param data Buffer to fill.
  /// @param size Number of `T` elements.
  template <typename T>
  HOST static void set(T *data, size_t size) {
    cudaMemset(data, 0xFF, size * sizeof(T));
  }

  /// @brief Zero-fill `[0, size)`.
  /// @tparam T Element type.
  /// @param data Buffer to clear.
  /// @param size Number of `T` elements.
  template <typename T>
  HOST static void clear(T *data, size_t size) {
    cudaMemset(data, 0x00, size * sizeof(T));
  }
};

} // namespace gp

#endif // GREENPEAS_POLICIES_STORAGE_CUDA_HPP