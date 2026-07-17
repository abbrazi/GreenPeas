#ifndef GREENPEAS_POLICIES_COMPUTE_CUDA_HPP
#define GREENPEAS_POLICIES_COMPUTE_CUDA_HPP

/// Standard headers
#include <algorithm>

/// CUDA headers
#include <cooperative_groups.h>
#include <cub/cub.cuh>
#include <thrust/iterator/zip_iterator.h>
#include <thrust/tuple.h>

/// Project headers
#include "GreenPeas/Common.hpp"
#include "GreenPeas/Policies/Compute/CUDAHandlers.hpp"

namespace gp {

template <typename Handler>
GLOBAL void backwardsLayerTraversalKernel(CUDAGraphView graph,
                                          uint32_t numLayers,
                                          uint32_t numNodesPerLayer,
                                          uint32_t numWordsPerNode,
                                          Handler handle) {
  auto grid = cooperative_groups::this_grid();

  const uint32_t workPerLayer = numNodesPerLayer * numWordsPerNode;

  // The nodes in the final layer have zero degree
  for (int layer = numLayers - 2; layer >= 0; --layer) {
    // Grid-stride loop (word-major order)
    for (uint32_t i = grid.thread_rank(); i < workPerLayer; i += grid.size()) {
      const uint32_t k = i / numNodesPerLayer;
      const uint32_t j = i % numNodesPerLayer;

      const uint32_t node = layer * numNodesPerLayer + j;
      const uint64_t neighbors = __ldg(&graph[node]);

      handle(node, k, neighbors);
    }

    // Global sync between layers
    grid.sync();
  }
}

template <typename Handler>
GLOBAL void monolithicTraversalKernel(uint32_t numNodes, Handler handle) {
  const uint32_t node = blockIdx.x * blockDim.x + threadIdx.x;

  if (node < numNodes) {
    handle(node);
  }
}

template <typename ValueT>
GLOBAL void scatterKernel(CUDAMatrixView<ValueT> from,
                          CUDAMatrixView<ValueT> to,
                          CUDAVectorView<uint32_t> newRows,
                          uint32_t numRows,
                          uint32_t numCols) {
  const uint32_t row = blockIdx.x * blockDim.x + threadIdx.x;
  const uint32_t col = blockIdx.y * blockDim.y + threadIdx.y;

  if (row < numRows && col < numCols) {
    to(newRows[row], col) = from(row, col);
  }
}

template <typename ValueT>
GLOBAL void gatherKernel(CUDAMatrixView<ValueT> from,
                         CUDAMatrixView<ValueT> to,
                         CUDAVectorView<uint32_t> oldRows,
                         uint32_t numRows,
                         uint32_t numCols) {
  const uint32_t row = blockIdx.x * blockDim.x + threadIdx.x;
  const uint32_t col = blockIdx.y * blockDim.y + threadIdx.y;

  if (row < numRows && col < numCols) {
    to(row, col) = from(oldRows[row], col);
  }
}

template <typename ValueT>
GLOBAL void scatterKernel(CUDAVectorView<ValueT> from,
                          CUDAVectorView<ValueT> to,
                          CUDAVectorView<uint32_t> newIndices,
                          uint32_t size) {
  const uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;

  if (idx < size) {
    to[newIndices[idx]] = from[idx];
  }
}

template <typename ValueT>
GLOBAL void gatherKernel(CUDAVectorView<ValueT> from,
                         CUDAVectorView<ValueT> to,
                         CUDAVectorView<uint32_t> oldIndices,
                         uint32_t size) {
  const uint32_t idx = blockIdx.x * blockDim.x + threadIdx.x;

  if (idx < size) {
    to[idx] = from[oldIndices[idx]];
  }
}

struct CUDACompute {
  template <typename Handler>
  HOST static void backwardLayerTraversal(CUDAGraphView graph,
                                          uint32_t numLayers,
                                          uint32_t numNodesPerLayer,
                                          uint32_t numWordsPerNode,
                                          Handler handle) {
    dim3 blockSize(32, 8);
    int numSMs = 0;
    cudaDeviceGetAttribute(&numSMs, cudaDevAttrMultiProcessorCount, 0);
    dim3 gridSize(static_cast<unsigned>(numSMs * 2));

    // Pack kernel arguments
    void *kernelArgs[] = {(void *)&graph,
                          (void *)&numLayers,
                          (void *)&numNodesPerLayer,
                          (void *)&numWordsPerNode,
                          (void *)&handle};

    // Launch the kernel with cooperative groups for synchronization
    cudaLaunchCooperativeKernel((void *)backwardsLayerTraversalKernel<Handler>,
                                gridSize,
                                blockSize,
                                kernelArgs);
  }

  template <typename Handler>
  HOST static void monolithicTraversal(uint32_t numNodes, Handler handle) {
    int threadsPerBlock = 256;
    int blocks = (numNodes + threadsPerBlock - 1) / threadsPerBlock;

    monolithicTraversalKernel<<<blocks, threadsPerBlock>>>(numNodes, handle);
  }

  template <typename ValueT>
  HOST static void scatter(CUDAMatrixView<ValueT> from,
                           CUDAMatrixView<ValueT> to,
                           CUDAVectorView<uint32_t> newRows,
                           uint32_t numRows,
                           uint32_t numCols) {
    dim3 blockSize(32, 8);
    dim3 gridSize((numRows + blockSize.x - 1) / blockSize.x,
                  (numCols + blockSize.y - 1) / blockSize.y);

    scatterKernel<<<gridSize, blockSize>>>(from, to, newRows, numRows, numCols);
  }

  template <typename ValueT>
  HOST static void gather(CUDAMatrixView<ValueT> from,
                          CUDAMatrixView<ValueT> to,
                          CUDAVectorView<uint32_t> oldRows,
                          uint32_t numRows,
                          uint32_t numCols) {
    dim3 blockSize(32, 8);
    dim3 gridSize((numRows + blockSize.x - 1) / blockSize.x,
                  (numCols + blockSize.y - 1) / blockSize.y);

    gatherKernel<<<gridSize, blockSize>>>(from, to, oldRows, numRows, numCols);
  }

  template <typename ValueT>
  HOST static void scatter(CUDAVectorView<ValueT> from,
                           CUDAVectorView<ValueT> to,
                           CUDAVectorView<uint32_t> newIndices,
                           uint32_t size) {
    int threadsPerBlock = 256;
    int blocks = (size + threadsPerBlock - 1) / threadsPerBlock;

    scatterKernel<<<blocks, threadsPerBlock>>>(from, to, newIndices, size);
  }

  template <typename ValueT>
  HOST static void gather(CUDAVectorView<ValueT> from,
                          CUDAVectorView<ValueT> to,
                          CUDAVectorView<uint32_t> oldIndices,
                          uint32_t size) {
    int threadsPerBlock = 256;
    int blocks = (size + threadsPerBlock - 1) / threadsPerBlock;

    gatherKernel<<<blocks, threadsPerBlock>>>(from, to, oldIndices, size);
  }

  template <typename KeyT, typename ValT>
  HOST static void sort(CUDAVectorView<KeyT> &keysA,
                        CUDAVectorView<KeyT> &keysB,
                        CUDAVectorView<ValT> &valsA,
                        CUDAVectorView<ValT> &valsB,
                        uint32_t numItems,
                        CUDAVectorView<uint8_t> scratch,
                        size_t &bytes) {
    // Wrap in double buffers
    cub::DoubleBuffer<KeyT> keys(keysA.data, keysB.data);
    cub::DoubleBuffer<ValT> vals(valsA.data, valsB.data);

    // Perform the radix sort
    cub::DeviceRadixSort::SortPairs(scratch.data, bytes, keys, vals, numItems);

    // Sync keys, result in buffer A
    keysA.data = keys.Current();
    keysB.data = keys.Alternate();

    // Sync vals, result in buffer A
    valsA.data = vals.Current();
    valsB.data = vals.Alternate();
  }

  template <typename KeyT, typename ValT, typename ExtT, typename Handler>
  HOST static void reduce(CUDAVectorView<KeyT> &keysA,
                          CUDAVectorView<KeyT> &keysB,
                          CUDAVectorView<ValT> &valsA,
                          CUDAVectorView<ValT> &valsB,
                          CUDAVectorView<ExtT> &extsA,
                          CUDAVectorView<ExtT> &extsB,
                          CUDAScalarView<uint32_t> numRuns,
                          Handler handle,
                          uint32_t numItems,
                          CUDAVectorView<uint8_t> scratch,
                          size_t &bytes) {
    // Create I/O iterators
    auto zipi =
        thrust::make_zip_iterator(thrust::make_tuple(valsA.data, extsA.data));
    auto zipo =
        thrust::make_zip_iterator(thrust::make_tuple(valsB.data, extsB.data));

    // Perform the reduction
    cub::DeviceReduce::ReduceByKey(scratch.data,
                                   bytes,
                                   keysA.data,
                                   keysB.data,
                                   zipi,
                                   zipo,
                                   numRuns.data,
                                   handle,
                                   numItems);
  }

  template <typename Worldview>
  HOST static auto getScratchpadSize(Worldview view,
                                     uint32_t numNodes,
                                     size_t &sortBytes,
                                     size_t &reduceBytes) -> uint32_t {
    CUDAVectorView<uint8_t> nullview{};

    sort(view.error.hashes.a,
         view.error.hashes.b,
         view.error.indices.a,
         view.error.indices.b,
         numNodes,
         nullview,
         sortBytes);

    reduce(view.error.hashes.a,
           view.error.hashes.b,
           view.error.indices.a,
           view.error.indices.b,
           view.error.probabilities.b,
           view.error.probabilities.a,
           view.error.numClasses,
           CUDAErrorClassReducer{},
           numNodes,
           nullview,
           reduceBytes);

    return static_cast<uint32_t>(std::max(sortBytes, reduceBytes));
  }

  template <typename Worldview>
  HOST static void scatterInitialSensitivities(Worldview view,
                                               uint32_t numMeasurements,
                                               uint32_t numWordsPerNode) {
    scatter(view.sense.map.vals,
            view.sense.matrix,
            view.sense.map.keys,
            numMeasurements,
            numWordsPerNode);
  }

  template <typename Worldview>
  HOST static void genSensitivities(Worldview view,
                                    uint32_t numLayers,
                                    uint32_t numNodesPerLayer,
                                    uint32_t numWordsPerNode) {
    backwardLayerTraversal(view.graph,
                           numLayers,
                           numNodesPerLayer,
                           numWordsPerNode,
                           CUDASensitivityGenerator(view.sense.matrix));
  }

  template <typename Worldview>
  HOST static void
  genErrorClasses(Worldview view, uint32_t numNodes, uint32_t numWordsPerNode) {
    monolithicTraversal(numNodes,
                        CUDAErrorClassGenerator<>(view.sense.matrix,
                                                  view.error.classes.a,
                                                  view.error.hashes.a,
                                                  view.error.indices.a,
                                                  numWordsPerNode));
  }

  template <typename Worldview>
  HOST static void
  sortErrorClasses(Worldview &view, uint32_t numNodes, size_t sortBytes) {
    sort(view.error.hashes.a,
         view.error.hashes.b,
         view.error.indices.a,
         view.error.indices.b,
         numNodes,
         view.scratchpad,
         sortBytes);
  }

  template <typename Worldview>
  HOST static void permuteErrorProbabilities(Worldview view,
                                             uint32_t numNodes) {
    gather(view.error.probabilities.a,
           view.error.probabilities.b,
           view.error.indices.a,
           numNodes);
  }

  template <typename Worldview>
  HOST static void
  reduceErrorClasses(Worldview view, uint32_t numNodes, size_t reduceBytes) {
    reduce(view.error.hashes.a,
           view.error.hashes.b,
           view.error.indices.a,
           view.error.indices.b,
           view.error.probabilities.b,
           view.error.probabilities.a,
           view.error.numClasses,
           CUDAErrorClassReducer{},
           numNodes,
           view.scratchpad,
           reduceBytes);
  }

  template <typename Worldview>
  HOST static void gatherFinalErrorClasses(Worldview view,
                                           uint32_t numClasses,
                                           uint32_t maxClassSize) {
    gather(view.error.classes.a,
           view.error.classes.b,
           view.error.indices.b,
           numClasses,
           maxClassSize);
  }
};

} // namespace gp

#endif // GREENPEAS_POLICIES_COMPUTE_CUDA_HPP
