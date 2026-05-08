#ifndef GREENPEAS_CORE_VECTOR_HPP
#define GREENPEAS_CORE_VECTOR_HPP

/// Standard headers
#include <stdexcept>

/// Project headers
#include "GreenPeas/Common.hpp"

namespace gp {

/// @brief Non-owning view over vector data.
/// @tparam IndexT Index type.
/// @tparam ValueT Value type.
template <typename IndexT, typename ValueT>
struct VectorView {
  /// @brief Number of elements.
  IndexT size;

  /// @brief Backing C-array.
  ValueT *data;

  /// @brief Default constructor.
  HOST VectorView() : size(0), data(nullptr) {}

  /// @brief Construct from size and data pointer.
  /// @param size Number of elements.
  /// @param data Pointer to backing storage.
  HOST VectorView(IndexT size, ValueT *data) : size(size), data(data) {}

  /// @brief Subscript access (non-const).
  /// @param index Zero-based index in `[0, size)`.
  /// @return Reference to the element at index.
  HOST DEVICE FORCE_INLINE auto operator[](IndexT index) -> ValueT & {
    return data[index];
  }

  /// @brief Subscript access (const view).
  /// @param index Zero-based index in `[0, size)`.
  /// @return Reference to the element at index.
  HOST DEVICE FORCE_INLINE auto operator[](IndexT index) const -> ValueT & {
    return data[index];
  }
};

/// @brief Owning vector with configurable storage policy.
/// @tparam IndexT Index type.
/// @tparam ValueT Value type.
/// @tparam Storage Storage policy for allocation/copy.
template <typename IndexT, typename ValueT, typename Storage>
struct Vector {
  /// @brief Maximum size. Invariant: size <= maxSize.
  IndexT maxSize;

  /// @brief Current size. Must be <= maxSize.
  IndexT size;

  /// @brief Backing C-array.
  ValueT *data;

  /// @brief Constructs a vector with given size.
  /// @param size Initial size (also max size).
  HOST explicit Vector(IndexT size) : maxSize(size), size(size) {
    data = Storage::template allocate<ValueT>(maxSize);
  }

  /// @brief Destructor.
  HOST ~Vector() { Storage::deallocate(data); }

  /// @brief Copy constructor.
  /// @param other Vector to copy from.
  HOST Vector(const Vector &other) : maxSize(other.size), size(other.size) {
    data = Storage::template allocate<ValueT>(maxSize);
    Storage::copyFrom(data, other.data, size, Storage{});
  }

  /// @brief Copy assignment.
  /// @param other Vector to copy from.
  /// @return Reference to `*this`.
  HOST auto operator=(const Vector &other) -> Vector & {
    if (this != &other) {
      Storage::deallocate(data);
      maxSize = other.size;
      size = other.size;
      data = Storage::template allocate<ValueT>(maxSize);
      Storage::copyFrom(data, other.data, size, Storage{});
    }
    return *this;
  }

  /// @brief Move constructor.
  /// @param other Vector to move from.
  HOST Vector(Vector &&other) noexcept
      : maxSize(other.maxSize), size(other.size), data(other.data) {
    other.maxSize = 0;
    other.size = 0;
    other.data = nullptr;
  }

  /// @brief Move assignment.
  /// @param other Vector to move from.
  /// @return Reference to `*this`.
  HOST auto operator=(Vector &&other) noexcept -> Vector & {
    if (this != &other) {
      Storage::deallocate(data);
      maxSize = other.maxSize;
      size = other.size;
      data = other.data;
      other.maxSize = 0;
      other.size = 0;
      other.data = nullptr;
    }
    return *this;
  }

  /// @brief Fit new size (must be <= maxSize).
  /// @param newSize New size.
  HOST void fitto(IndexT newSize) {
    if (newSize > maxSize) {
      throw std::runtime_error("Vector: newSize exceeds maxSize.");
    }
    size = newSize;
  }

  /// @brief Subscript access (non-const).
  /// @param index Zero-based index in `[0, size)`.
  /// @return Reference to the element at index.
  HOST auto operator[](IndexT index) -> ValueT & { return data[index]; }

  /// @brief Subscript access (const).
  /// @param index Zero-based index in `[0, size)`.
  /// @return Value at index.
  HOST auto operator[](IndexT index) const -> ValueT { return data[index]; }

  /// @brief Copy to another vector.
  /// @tparam DestinationStorage Destination storage policy.
  /// @param other Destination vector (must have same size).
  template <typename DestinationStorage>
  HOST void copyTo(Vector<IndexT, ValueT, DestinationStorage> &other) const {
    if (size != other.size) {
      throw std::runtime_error("Vector: size mismatch in copyTo.");
    }
    Storage::copyTo(other.data, data, size, DestinationStorage{});
  }

  /// @brief Copy from another vector.
  /// @tparam SourceStorage Source storage policy.
  /// @param other Source vector (must have same size).
  template <typename SourceStorage>
  HOST void copyFrom(const Vector<IndexT, ValueT, SourceStorage> &other) {
    if (size != other.size) {
      throw std::runtime_error("Vector: size mismatch in copyFrom.");
    }
    Storage::copyFrom(data, other.data, size, SourceStorage{});
  }

  /// @brief Set all elements.
  HOST void set() { Storage::set(data, size); }

  /// @brief Clear all elements.
  HOST void clear() { Storage::clear(data, size); }

  /// @brief Get a non-owning view of the vector.
  /// @return VectorView sharing this vector's storage.
  HOST auto getView() -> VectorView<IndexT, ValueT> { return {size, data}; }
};

} // namespace gp

#endif // GREENPEAS_CORE_VECTOR_HPP
