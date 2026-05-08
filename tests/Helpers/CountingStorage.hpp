#ifndef GREENPEAS_TESTS_HELPERS_COUNTING_STORAGE_HPP
#define GREENPEAS_TESTS_HELPERS_COUNTING_STORAGE_HPP

/// Standard headers
#include <algorithm>
#include <cstddef>
#include <cstring>

struct CountingStorage {
  static inline size_t allocations = 0;
  static inline size_t deallocations = 0;

  template <typename T>
  static auto allocate(size_t size) -> T * {
    ++allocations;
    return new T[size]();
  }

  template <typename T>
  static void deallocate(T *data) {
    if (data != nullptr) {
      ++deallocations;
    }
    delete[] data;
  }

  template <typename T, typename DestinationStorage>
  static void copyTo(T *dst, const T *src, size_t size, DestinationStorage) {
    std::copy(src, src + size, dst);
  }

  template <typename T, typename SourceStorage>
  static void copyFrom(T *dst, const T *src, size_t size, SourceStorage) {
    std::copy(src, src + size, dst);
  }

  template <typename T>
  static void set(T *data, size_t size) {
    std::memset(data, 0xFF, size * sizeof(T));
  }

  template <typename T>
  static void clear(T *data, size_t size) {
    std::memset(data, 0x00, size * sizeof(T));
  }
};

inline void resetCountingStorage() {
  CountingStorage::allocations = 0;
  CountingStorage::deallocations = 0;
}

#endif // GREENPEAS_TESTS_HELPERS_COUNTING_STORAGE_HPP
