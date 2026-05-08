#ifndef GREENPEAS_COMMON_HPP
#define GREENPEAS_COMMON_HPP

#if defined(GP_HAS_CUDA) && defined(__CUDACC__)
#define HOST __host__
#define DEVICE __device__
#define GLOBAL __global__
#define FORCE_INLINE __forceinline__
#else
#define HOST
#define DEVICE
#define GLOBAL
#define FORCE_INLINE
#endif // defined(GP_HAS_CUDA) && GREEN_PEAS_WITH_CUDA

#endif // GREENPEAS_COMMON_HPP
