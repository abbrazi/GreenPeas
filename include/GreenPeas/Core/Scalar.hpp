#ifndef GREENPEAS_CORE_SCALAR_HPP
#define GREENPEAS_CORE_SCALAR_HPP

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Policies/Storage/Host.hpp"

namespace gp {

/// @brief Non-owning view over one scalar value.
/// @tparam ValueT Value type.
template <typename ValueT>
struct ScalarView {
  /// @brief Backing C-array (size 1).
  ValueT *data;

  /// @brief Dereference (non-const).
  /// @return Reference to the scalar value.
  HOST DEVICE FORCE_INLINE auto operator*() -> ValueT & { return *data; }

  /// @brief Dereference (const view).
  /// @return Reference to the scalar value.
  HOST DEVICE FORCE_INLINE auto operator*() const -> ValueT & { return *data; }
};

/// @brief Owning scalar with configurable storage policy.
/// @tparam ValueT Value type.
/// @tparam Storage Storage policy for allocation/copy.
template <typename ValueT, typename Storage>
struct Scalar {
  /// @brief Backing C-array (size 1).
  ValueT *data;

  /// @brief Default constructor.
  HOST Scalar() { data = Storage::template allocate<ValueT>(1); }

  /// @brief Destructor.
  HOST ~Scalar() { Storage::deallocate(data); }

  /// @brief Copy constructor.
  /// @param other Scalar to copy from.
  HOST Scalar(const Scalar &other) {
    data = Storage::template allocate<ValueT>(1);
    Storage::copyFrom(data, other.data, 1, Storage{});
  }

  /// @brief Copy assignment.
  /// @param other Scalar to copy from.
  /// @return Reference to `*this`.
  HOST auto operator=(const Scalar &other) -> Scalar & {
    if (this != &other) {
      Storage::deallocate(data);
      data = Storage::template allocate<ValueT>(1);
      Storage::copyFrom(data, other.data, 1, Storage{});
    }
    return *this;
  }

  /// @brief Move constructor.
  /// @param other Scalar to move from.
  HOST Scalar(Scalar &&other) noexcept : data(other.data) {
    other.data = nullptr;
  }

  /// @brief Move assignment.
  /// @param other Scalar to move from.
  /// @return Reference to `*this`.
  HOST auto operator=(Scalar &&other) noexcept -> Scalar & {
    if (this != &other) {
      Storage::deallocate(data);
      data = other.data;
      other.data = nullptr;
    }
    return *this;
  }

  /// @brief Dereference (non-const).
  /// @return Reference to the scalar value.
  HOST auto operator*() -> ValueT & { return *data; }

  /// @brief Dereference (const).
  /// @return Scalar value.
  HOST auto operator*() const -> ValueT { return *data; }

  /// @brief Copy to another scalar.
  /// @tparam DestinationStorage Destination storage policy.
  /// @param other Destination scalar.
  template <typename DestinationStorage>
  HOST void copyTo(Scalar<ValueT, DestinationStorage> &other) const {
    Storage::copyTo(other.data, data, 1, DestinationStorage{});
  }

  /// @brief Copy from another scalar.
  /// @tparam SourceStorage Source storage policy.
  /// @param other Source scalar.
  template <typename SourceStorage>
  HOST void copyFrom(const Scalar<ValueT, SourceStorage> &other) {
    Storage::copyFrom(data, other.data, 1, SourceStorage{});
  }

  /// @brief Copy to a host value.
  /// @param value Destination value.
  HOST void copyTo(ValueT &value) const {
    Storage::copyTo(&value, data, 1, HostStorage{});
  }

  /// @brief Copy from a host value.
  /// @param value Source value.
  HOST void copyFrom(const ValueT &value) {
    Storage::copyFrom(data, &value, 1, HostStorage{});
  }

  /// @brief Set the scalar value.
  HOST void set() { Storage::set(data, 1); }

  /// @brief Clear the scalar value.
  HOST void clear() { Storage::clear(data, 1); }

  /// @brief Get a non-owning view of this scalar.
  /// @return ScalarView sharing this scalar's storage.
  HOST auto getView() -> ScalarView<ValueT> { return {data}; }
};

} // namespace gp

#endif // GREENPEAS_CORE_SCALAR_HPP
