#ifndef GREENPEAS_CORE_DOUBLEBUFFER_HPP
#define GREENPEAS_CORE_DOUBLEBUFFER_HPP

/// Standard headers
#include <utility>

/// Project headers
#include "GreenPeas/Common.hpp"

namespace gp {

/// @brief Non-owning pair of buffers for ping-pong read/write.
/// @tparam BufferView Non-owning buffer view type.
template <typename BufferView>
struct DoubleBufferView {
  /// @brief First buffer view.
  BufferView a;

  /// @brief Second buffer view.
  BufferView b;
};

/// @brief Pair of buffers for ping-pong read/write.
/// @tparam Buffer Owning buffer type.
template <typename Buffer>
struct DoubleBuffer {
  /// @brief Non-owning view type for `Buffer`.
  using BufferView = decltype(std::declval<Buffer &>().getView());

  /// @brief First buffer.
  Buffer a;

  /// @brief Second buffer.
  Buffer b;

  /// @brief Default constructor.
  HOST DoubleBuffer() = default;

  /// @brief Construct `a` and `b` with the same arguments.
  /// @param args Arguments forwarded to each `Buffer` constructor.
  template <typename... Args>
  HOST explicit DoubleBuffer(Args &&...args)
      : a(std::forward<Args>(args)...), b(std::forward<Args>(args)...) {}

  /// @brief Get non-owning views of both buffers.
  /// @return DoubleBufferView sharing this double buffer's storage.
  HOST auto getView() -> DoubleBufferView<BufferView> {
    return {a.getView(), b.getView()};
  }

  /// @brief Resize both buffers (requires `Buffer::fitto(...)`).
  template <typename... Args>
  HOST void fitto(Args &&...args) {
    a.fitto(std::forward<Args>(args)...);
    b.fitto(std::forward<Args>(args)...);
  }

  /// @brief Clear both buffers (requires `Buffer::clear()`).
  HOST void clear() {
    a.clear();
    b.clear();
  }
};

} // namespace gp

#endif // GREENPEAS_CORE_DOUBLEBUFFER_HPP
