#ifndef GREENPEAS_POLICIES_COMPUTE_HOST_HPP
#define GREENPEAS_POLICIES_COMPUTE_HOST_HPP

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Policies/Compute/HostHandlers.hpp"

namespace gp {

/// @brief Host (CPU) compute backend.
struct HostCompute {
  /// @brief
  /// @tparam Worldview
  /// @param view
  /// @param sortBytes
  /// @param reduceBytes
  /// @return
  template <typename Worldview>
  HOST static auto getScratchpadSize(Worldview view,
                                     uint32_t numNodes,
                                     size_t &sortBytes,
                                     size_t &reduceBytes) -> uint32_t {
    (void)view;
    (void)numNodes;
    sortBytes = 0;
    reduceBytes = 0;
    return 0;
  }

  /// @brief
  /// @tparam Worldview
  /// @param view
  /// @param numMeasurements
  /// @param numWordsPerNode
  /// @return
  template <typename Worldview>
  HOST static void scatterInitialSensitivities(Worldview view,
                                               uint32_t numMeasurements,
                                               uint32_t numWordsPerNode) {
    (void)view;
    (void)numMeasurements;
    (void)numWordsPerNode;
  }

  /// @brief
  /// @tparam Worldview
  /// @param view
  /// @param numLayers
  /// @param numNodesPerLayer
  /// @param numWordsPerNode
  /// @return
  template <typename Worldview>
  HOST static void genSensitivities(Worldview view,
                                    uint32_t numLayers,
                                    uint32_t numNodesPerLayer,
                                    uint32_t numWordsPerNode) {
    (void)view;
    (void)numLayers;
    (void)numNodesPerLayer;
    (void)numWordsPerNode;
  }

  /// @brief
  /// @tparam Worldview
  /// @param view
  /// @param numNodes
  /// @param numWordsPerNode
  /// @return
  template <typename Worldview>
  HOST static void genErrorClasses(Worldview view,
                                   uint32_t numNodes,
                                   uint32_t numWordsPerNode) {
    (void)view;
    (void)numNodes;
    (void)numWordsPerNode;
  }

  /// @brief
  /// @tparam Worldview
  /// @param view
  /// @param numNodes
  /// @param sortBytes
  /// @return
  template <typename Worldview>
  HOST static void sortErrorClasses(Worldview &view,
                                    uint32_t numNodes,
                                    size_t sortBytes) {
    (void)view;
    (void)numNodes;
    (void)sortBytes;
  }

  /// @brief
  /// @tparam Worldview
  /// @param view
  /// @param numNodes
  /// @return
  template <typename Worldview>
  HOST static void permuteErrorProbabilities(Worldview view,
                                             uint32_t numNodes) {
    (void)view;
    (void)numNodes;
  }

  /// @brief
  /// @tparam Worldview
  /// @param view
  /// @param numNodes
  /// @param reduceBytes
  /// @return
  template <typename Worldview>
  HOST static void reduceErrorClasses(Worldview view,
                                      uint32_t numNodes,
                                      size_t reduceBytes) {
    (void)view;
    (void)numNodes;
    (void)reduceBytes;
  }

  /// @brief
  /// @tparam Worldview
  /// @param view
  /// @param numClasses
  /// @param maxClassSize
  /// @return
  template <typename Worldview>
  HOST static void gatherFinalErrorClasses(Worldview view,
                                           uint32_t numClasses,
                                           uint32_t maxClassSize) {
    (void)view;
    (void)numClasses;
    (void)maxClassSize;
  }
};

} // namespace gp

#endif // GREENPEAS_POLICIES_COMPUTE_HOST_HPP
