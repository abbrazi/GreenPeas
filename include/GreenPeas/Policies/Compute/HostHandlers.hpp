#ifndef GREENPEAS_POLICIES_COMPUTE_HOSTHANDLERS_HPP
#define GREENPEAS_POLICIES_COMPUTE_HOSTHANDLERS_HPP

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Core/Graph.hpp"
#include "GreenPeas/Core/Matrix.hpp"
#include "GreenPeas/Core/Scalar.hpp"
#include "GreenPeas/Core/Vector.hpp"
#include "GreenPeas/Policies/Data/Layout.hpp"

namespace gp {

using HostGraphView = GraphView;

template <typename ValueT>
using HostMatrixView = MatrixView<uint32_t, ValueT, RowMajorLayout>;

template <typename ValueT>
using HostScalarView = ScalarView<ValueT>;

template <typename ValueT>
using HostVectorView = VectorView<uint32_t, ValueT>;

} // namespace gp

#endif // GREENPEAS_POLICIES_COMPUTE_HOSTHANDLERS_HPP
