/*************************************************************************
 * Copyright (c) 2015-2020, NVIDIA CORPORATION. All rights reserved.
 *
 * See LICENSE.txt for license information
 ************************************************************************/

#ifndef NCCL_COMMON_KERNEL_H_
#define NCCL_COMMON_KERNEL_H_

#include "devcomm.h"
#include <cstdio>
#include <cstdint>

#include <cuda_runtime.h>
#include <curand_kernel.h>

// Define min for ssize_t
static __device__ int min(int a, ssize_t b) { return (a < b) ? a : b; }

template <typename T>
inline __device__ void loadPtr(void** ptr, T* &v) {
  asm volatile("ld.volatile.global.u64 %0, [%1];"
      : "=l"(v) : "l"(ptr));
}

typedef uint64_t PackType;


template<typename T>
struct FuncDropout {
  __device__ T operator()(const T val, const T biasVal, curandState* randState, const float p, const T residualVal) {
    T out = (curand_uniform(randState) < p ? val + biasVal : (T)0.0f) + residualVal;
    return out;
  }
};

template<>
struct FuncDropout<half> {
  __device__ half2 operator()(const half2 val, const half2 biasVal,curandState* randState, const float p, const half2 residualVal) {
    half2 v = (curand_uniform(randState) < p ? __hadd2_sat(val, biasVal)  : __float2half2_rn(0.0f));
    return (__hadd2_sat(v, residualVal));
  }
};

template<>
struct FuncDropout<half2> {
  __device__ half2 operator()(const half2 val, const half2 biasVal,curandState* randState, const float p, const half2 residualVal) {
    half2 v = (curand_uniform(randState) < p ? __hadd2_sat(val, biasVal) : __float2half2_rn(0.0f));
    return (__hadd2_sat(v, residualVal));
  }
};


template<typename T>
struct FuncDropout2 {
  __device__ T operator()(const T val, const T biasVal, curandState* randState, const float p, const T residualVal) {
    T out = (curand_uniform(randState) < p ? val + biasVal : (T)0.0f) + residualVal;
    return out;
  }
};

template<>
struct FuncDropout2<half> {
  __device__ half operator()(const half val, const half biasVal,curandState* randState, const float p, const half residualVal) {
    half v = (curand_uniform(randState) < p ? __hadd_sat(val, biasVal)  : __float2half_rn(0.0f));
    return (__hadd_sat(v, residualVal));
  }
};

template<>
struct FuncDropout2<half2> {
  __device__ half2 operator()(const half2 val, const half2 biasVal,curandState* randState, const float p, const half2 residualVal) {
    half2 v = (curand_uniform(randState) < p ? __hadd2_sat(val, biasVal)  : __float2half2_rn(0.0f));
    return (__hadd2_sat(v, residualVal));
  }
};

// unpack x and y to elements of type T and apply FUNC to each element
template<class FUNC, typename T>
struct MULTI {
  __device__ PackType operator()(const PackType x, const PackType y) const;
  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) const;
};

template<class FUNC>
struct MULTI<FUNC, int8_t> {
  static_assert(sizeof(PackType) == 2 * sizeof(uint32_t),
      "PackType must be twice the size of uint32_t.");
  union converter {
    PackType storage;
    struct {
      uint32_t a, b;
    };
  };

  __device__ PackType operator()(const PackType x, const PackType y) const {
    converter cx, cy, cr;
    cx.storage = x;
    cy.storage = y;

    // for char, we do these as vector ops
    cr.a = FUNC()(cx.a, cy.a);
    cr.b = FUNC()(cx.b, cy.b);

    return cr.storage;
  }
  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) {/*Not implemented*/}
};

template<class FUNC>
struct MULTI<FUNC, uint8_t> {
  static_assert(sizeof(PackType) == 2 * sizeof(uint32_t),
      "PackType must be twice the size of uint32_t.");
  union converter {
    PackType storage;
    struct {
      uint32_t a, b;
    };
  };

  __device__ PackType operator()(const PackType x, const PackType y) const {
    converter cx, cy, cr;
    cx.storage = x;
    cy.storage = y;

    // for char, we do these as vector ops
    cr.a = FUNC()(cx.a, cy.a);
    cr.b = FUNC()(cx.b, cy.b);

    return cr.storage;
  }
  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) {/*Not implemented*/}
};

template<class FUNC>
struct MULTI<FUNC, int32_t> {
  static_assert(sizeof(PackType) == 2 * sizeof(int32_t),
      "PackType must be twice the size of int.");
  union converter {
    PackType storage;
    struct {
      int32_t a, b;
    };
  };

  __device__ PackType operator()(const PackType x, const PackType y) const {
    converter cx, cy, cr;
    cx.storage = x;
    cy.storage = y;

    cr.a = FUNC()(cx.a, cy.a);
    cr.b = FUNC()(cx.b, cy.b);

    return cr.storage;
  }
  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) {/*Not implemented*/}
};

template<class FUNC>
struct MULTI<FUNC, uint32_t> {
  static_assert(sizeof(PackType) == 2 * sizeof(uint32_t),
      "PackType must be twice the size of int.");
  union converter {
    PackType storage;
    struct {
      uint32_t a, b;
    };
  };

  __device__ PackType operator()(const PackType x, const PackType y) const {
    converter cx, cy, cr;
    cx.storage = x;
    cy.storage = y;

    cr.a = FUNC()(cx.a, cy.a);
    cr.b = FUNC()(cx.b, cy.b);

    return cr.storage;
  }
  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) {/*Not implemented*/}
};

template<class FUNC>
struct MULTI<FUNC, half> {
  static_assert(sizeof(PackType) == 4 * sizeof(half),
      "PackType must be four times the size of half.");

  struct PackHalf2 {
    half2 a, b;
  };

  __device__ PackType operator()(const PackType x, const PackType y) const {
    struct PackHalf2 cx, cy, cr;
    cx = *(reinterpret_cast<const struct PackHalf2*>(&x));
    cy = *(reinterpret_cast<const struct PackHalf2*>(&y));

    cr.a = FUNC()(cx.a, cy.a);
    cr.b = FUNC()(cx.b, cy.b);

    return *(reinterpret_cast<PackType*>(&cr));
  }

  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) {
    struct PackHalf2 cx, cb, cr, cy;
    cx = *(reinterpret_cast<const struct PackHalf2*>(&x));
    cb = *(reinterpret_cast<const struct PackHalf2*>(&biasVal));
    cy = *(reinterpret_cast<const struct PackHalf2*>(&residual));

    cr.a = FUNC()(cx.a, cb.a, randState, dropoutProb, cy.a);
    cr.b = FUNC()(cx.b, cb.b, randState, dropoutProb, cy.b);

    return *(reinterpret_cast<PackType*>(&cr));
  }
};

template<class FUNC>
struct MULTI<FUNC, float> {
  static_assert(sizeof(PackType) == 2 * sizeof(float),
      "PackType must be twice the size of float.");
  union converter {
    PackType storage;
    struct {
      float a, b;
    };
  };

  __device__ PackType operator()(const PackType x, const PackType y) const {
    converter cx, cy, cr;
    cx.storage = x;
    cy.storage = y;

    cr.a = FUNC()(cx.a, cy.a);
    cr.b = FUNC()(cx.b, cy.b);

    return cr.storage;
  }

  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) {
    converter cx, cb, cr, cy;
    cx.storage = x;
    cb.storage = biasVal;
    cy.storage = residual;

    cr.a = FUNC()(cx.a, cb.a, randState, dropoutProb, cy.a);
    cr.b = FUNC()(cx.b, cb.b, randState, dropoutProb, cy.b);

    return cr.storage;
  }
};

template<class FUNC>
struct MULTI<FUNC, double> {
  static_assert(sizeof(PackType) == sizeof(double),
      "PackType must be the same size as double.");
  __device__ PackType operator()(const PackType x, const PackType y) const {
    double rv = FUNC()(__longlong_as_double(x), __longlong_as_double(y));
    return __double_as_longlong(rv);
  }

  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) {/*Not implemented*/}
};

template<class FUNC>
struct MULTI<FUNC, uint64_t> {
  static_assert(sizeof(PackType) == sizeof(uint64_t),
      "PackType must be the same size as uint64_t.");
  __device__ PackType operator()(const PackType x, const PackType y) const {
    uint64_t rv = FUNC()(x, y);
    return rv;
  }

  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) {/*Not implemented*/}
};

template<class FUNC>
struct MULTI<FUNC, int64_t> {
  static_assert(sizeof(PackType) == sizeof(int64_t),
      "PackType must be the same size as int64_t.");
  __device__ PackType operator()(const PackType x, const PackType y) const {
    int64_t rv = FUNC()((int64_t)x, (int64_t)y);
    return rv;
  }

  __device__ PackType dropoutBias(const PackType x, const PackType biasVal, curandState* randState, float dropoutProb, const PackType residual) {/*Not implemented*/}
};

template<typename T> inline __device__
T vFetch(const volatile T* ptr) {
  return *ptr;
}

template<typename T> inline __device__
void vStore(volatile T* ptr, const T val) {
  *ptr = val;
}

#if CUDART_VERSION < 9000
template<> inline __device__
half vFetch<half>(const volatile half* ptr) {
  half r;
  r.x = ptr->x;
  return r;
}

template<> inline __device__
void vStore<half>(volatile half* ptr, const half val) {
  ptr->x = val.x;
}
#else
template<> inline __device__
half vFetch<half>(const volatile half* ptr) {
  half r;
  r = ((half*)ptr)[0];
  return r;
}

template<> inline __device__
void vStore<half>(volatile half* ptr, const half val) {
  ((half*)ptr)[0] = val;
}
#endif

typedef ulong2 Pack128;

template<class FUNC, typename T>
struct MULTI128 {
  __device__ void operator()(Pack128& x, Pack128& y) {
    x.x = MULTI<FUNC, T>()(x.x, y.x);
    x.y = MULTI<FUNC, T>()(x.y, y.y);
  }
  __device__ void dropoutBias(Pack128& x, const Pack128& biasVal, curandState* randState, float dropoutProb, const Pack128& residualVal) {
    x.x = MULTI<FUNC, T>().dropoutBias(x.x, biasVal.x, randState, dropoutProb, residualVal.x);
    x.y = MULTI<FUNC, T>().dropoutBias(x.y, biasVal.y, randState, dropoutProb, residualVal.y);
  }
};

inline __device__ void Fetch128(Pack128& v, const Pack128* p) {
  asm volatile("ld.volatile.global.v2.u64 {%0,%1}, [%2];" : "=l"(v.x), "=l"(v.y) : "l"(p) : "memory");
}
inline __device__ void Store128(Pack128* p, Pack128& v) {
  asm volatile("st.volatile.global.v2.u64 [%0], {%1,%2};" :: "l"(p), "l"(v.x), "l"(v.y) : "memory");
}

template<class FUNC, typename T, int UNROLL, int MINSRCS, int MAXSRCS, int MINDSTS, int MAXDSTS, int DROPOUT_BIAS_LAYERNORM>
__device__ __forceinline__ void ReduceCopyMulti(const int w, const int nw, const int t,
    int nsrcs, const T** s, int ndsts, T** d, const int elemOffset, const int Nelem, size_t mainBufferOffset, T* bias, int biasSize, curandState* randNumGen, float dropoutProb, T* residual) {
  const int inc = nw * UNROLL * WARP_SIZE;
  int offset = w * UNROLL * WARP_SIZE + t;

  const T* srcs[MAXSRCS];
  for (int i=0; i<MAXSRCS; i++) srcs[i] = s[i]+elemOffset+offset;
  T* dsts[MAXDSTS];
  for (int i=0; i<MAXDSTS; i++) dsts[i] = d[i]+elemOffset+offset;

  while (offset < Nelem) {
    T vals[UNROLL];
    // Load and reduce
    for (int u = 0; u < UNROLL; ++u) vals[u] = vFetch(srcs[0]+u*WARP_SIZE);

    #pragma unroll
    for (int i=1; i<MINSRCS; i++) {
      T vals2[UNROLL];
      for (int u = 0; u < UNROLL; ++u) vals2[u] = vFetch(srcs[i]+u*WARP_SIZE);
      for (int u = 0; u < UNROLL; ++u) vals[u] = FUNC()(vals[u], vals2[u]);
    }
    #pragma unroll
    for (int i=MINSRCS; i<MAXSRCS; i++) {
      if (i<nsrcs) {
        T vals2[UNROLL];
        for (int u = 0; u < UNROLL; ++u) vals2[u] = vFetch(srcs[i]+u*WARP_SIZE);
        for (int u = 0; u < UNROLL; ++u) vals[u] = FUNC()(vals[u], vals2[u]);
      }
    }

    for (int u = 0; u < UNROLL; ++u) {
      if (DROPOUT_BIAS_LAYERNORM) {
        size_t totalOffset = mainBufferOffset + elemOffset+offset;
        size_t biasOffset = totalOffset % biasSize;
        vals[u] = FuncDropout2<T>()(vals[u], bias[biasOffset], randNumGen, dropoutProb, residual[totalOffset]);
      }
      // Store
      #pragma unroll
      for (int i = 0; i < MINDSTS; i++) {
        vStore(dsts[i]+u*WARP_SIZE, vals[u]);
      }
      #pragma unroll
      for (int i=MINDSTS; i<MAXDSTS; i++) {
        if (i<ndsts) {
          vStore(dsts[i]+u*WARP_SIZE, vals[u]);
        }
      }
    }
    for (int i=0; i<MAXSRCS; i++) srcs[i] += inc;
    for (int i=0; i<MAXDSTS; i++) dsts[i] += inc;
    offset += inc;
  }
}

template<class FUNC, typename T, int UNROLL, int MINSRCS, int MAXSRCS, int MINDSTS, int MAXDSTS, int DROPOUT_BIAS_LAYERNORM>
__device__ __forceinline__ void ReduceCopy128bMulti(const int w, const int nw, const int t,
    int nsrcs, const T** s, int ndsts, T** d, const int elemOffset, const int Npack, 
    size_t mainBufferOffset, T* bias, int biasSize, curandState* randNumGen, float dropoutProb, T* residual) {
  const int inc = nw * UNROLL * WARP_SIZE;
  int offset = w * UNROLL * WARP_SIZE + t;

  const Pack128* srcs[MAXSRCS];
  for (int i=0; i<MAXSRCS; i++) srcs[i] = ((const Pack128*)(s[i]+elemOffset))+offset;
  Pack128* dsts[MAXDSTS];
  for (int i=0; i<MAXDSTS; i++) dsts[i] = ((Pack128*)(d[i]+elemOffset))+offset;

  while (offset < Npack) {
    Pack128 vals[UNROLL];
    // Load and reduce
    for (int u = 0; u < UNROLL; ++u) Fetch128(vals[u], srcs[0]+u*WARP_SIZE);

    #pragma unroll
    for (int i=1; i<MINSRCS; i++) {
      Pack128 vals2[UNROLL];
      for (int u = 0; u < UNROLL; ++u) Fetch128(vals2[u], srcs[i]+u*WARP_SIZE);
      for (int u = 0; u < UNROLL; ++u) MULTI128<FUNC, T>()(vals[u], vals2[u]);
    }
    #pragma unroll
    for (int i=MINSRCS; i<MAXSRCS; i++) {
      if (i<nsrcs) {
        Pack128 vals2[UNROLL];
        for (int u = 0; u < UNROLL; ++u) Fetch128(vals2[u], srcs[i]+u*WARP_SIZE);
        for (int u = 0; u < UNROLL; ++u) MULTI128<FUNC, T>()(vals[u], vals2[u]);
      }
    }

    // Store
    if (DROPOUT_BIAS_LAYERNORM) {
      for (int u = 0; u < UNROLL; ++u) {
        // Pack128 _vals = vals[u];
        const size_t totalOffset = (mainBufferOffset + elemOffset + (offset + u*WARP_SIZE)*(sizeof(Pack128)/sizeof(T)));
        const size_t biasOffset = totalOffset%biasSize;

        Pack128 biasVal;
        Fetch128(biasVal, (Pack128*)(bias+biasOffset));
        Pack128 residualVal;
        Fetch128(residualVal, (Pack128*)(residual+totalOffset));
        MULTI128<FuncDropout<T>, T>().dropoutBias(vals[u], biasVal, randNumGen, dropoutProb, residualVal);

        #pragma unroll
        for (int i = 0; i < MINDSTS; i++) {
          Store128(dsts[i]+u*WARP_SIZE, vals[u]);
        }

        #pragma unroll
        for (int i=MINDSTS; i<MAXDSTS; i++) {
          if (i<ndsts) {
            Store128(dsts[i]+u*WARP_SIZE, vals[u]);
          }
        }
      }
    } else {
      #pragma unroll
      for (int i = 0; i < MINDSTS; i++) {
        for (int u = 0; u < UNROLL; ++u) Store128(dsts[i]+u*WARP_SIZE, vals[u]);
      }
      #pragma unroll
      for (int i=MINDSTS; i<MAXDSTS; i++) {
        if (i<ndsts) {
          for (int u = 0; u < UNROLL; ++u) Store128(dsts[i]+u*WARP_SIZE, vals[u]);
        }
      }
    }
    for (int i=0; i<MAXSRCS; i++) srcs[i] += inc;
    for (int i=0; i<MAXDSTS; i++) dsts[i] += inc;
    offset += inc;
  }
}

template <typename T>
__device__ int ptrAlign128(T* ptr) { return (uint64_t)ptr % alignof(Pack128); }

#define PACKELEMS (sizeof(Pack128) / sizeof(T))

template<int UNROLL, class FUNC, typename T, int MINSRCS, int MAXSRCS, int MINDSTS, int MAXDSTS, int DROPOUT_BIAS_LAYERNORM>
__device__ __forceinline__ void ReduceOrCopyMulti(const int tid, const int nthreads,
    int nsrcs, const T** srcs, int ndsts, T** dsts,
    int N, size_t mainBufferOffset, T* bias, int biasSize, curandState* randNumGen, float dropoutProb, T *residual) {
  int Nrem = N;
  if (Nrem <= 0) return;

  int w = tid / WARP_SIZE;       // Warp number
  int nw = nthreads / WARP_SIZE; // Number of warps
  int t = tid % WARP_SIZE;       // Thread (inside the warp)

  // Check that all is 16B aligned. If not don't use 16B load/stores.
  int align = 0;
  #pragma unroll
  for (int i=0; i<MINSRCS; i++) align |= ptrAlign128(srcs[i]);
  for (int i=MINSRCS; i<MAXSRCS && i<nsrcs; i++) align |= ptrAlign128(srcs[i]);
  #pragma unroll
  for (int i=0; i<MINDSTS; i++) align |= ptrAlign128(dsts[i]);
  for (int i=MINDSTS; i<MAXDSTS && i<ndsts; i++) align |= ptrAlign128(dsts[i]);

  if (bias)
    align |= ptrAlign128(bias);

  int offset = 0;
  if (align == 0) {
    // fast path: use 128b loads/stores to do the bulk of the work,
    // assuming the pointers we have are all 128-bit aligned.

    // main loop
    int Npack = (Nrem / (PACKELEMS*UNROLL*WARP_SIZE)) * (UNROLL*WARP_SIZE); // round down
    int Nelem = Npack * PACKELEMS;

    ReduceCopy128bMulti<FUNC, T, UNROLL, MINSRCS, MAXSRCS, MINDSTS, MAXDSTS, DROPOUT_BIAS_LAYERNORM>(w, nw, t, nsrcs, srcs, ndsts, dsts, offset, Npack, mainBufferOffset + offset*PACKELEMS, bias, biasSize, randNumGen, dropoutProb, residual);

    Nrem -= Nelem;
    if (Nrem == 0) return;
    offset += Nelem;

    // slightly less optimized for section when we don't have full unrolling
    Npack = Nrem / PACKELEMS;
    Nelem = Npack * PACKELEMS;

    ReduceCopy128bMulti<FUNC, T, 1, MINSRCS, MAXSRCS, MINDSTS, MAXDSTS, DROPOUT_BIAS_LAYERNORM>(w, nw, t, nsrcs, srcs, ndsts, dsts, offset, Npack, mainBufferOffset + offset*PACKELEMS, bias, biasSize, randNumGen, dropoutProb, residual);

    Nrem -= Nelem;
    if (Nrem == 0) return;
    offset += Nelem;
  }

  // unrolled, by-type (mostly for unaligned buffers)
  int Nelem = (Nrem / (UNROLL*PACKELEMS/2*WARP_SIZE)) * (UNROLL*PACKELEMS/2*WARP_SIZE); // round down

  ReduceCopyMulti<FUNC, T, UNROLL*PACKELEMS/2, MINSRCS, MAXSRCS, MINDSTS, MAXDSTS, DROPOUT_BIAS_LAYERNORM>(w, nw, t, nsrcs, srcs, ndsts, dsts, offset, Nelem, mainBufferOffset + offset*PACKELEMS, bias, biasSize, randNumGen, dropoutProb, residual);

  Nrem -= Nelem;
  if (Nrem == 0) return;
  offset += Nelem;

  // no unroll, by type. Should finish what's remaining.
  ReduceCopyMulti<FUNC, T, 1, MINSRCS, MAXSRCS, MINDSTS, MAXDSTS, DROPOUT_BIAS_LAYERNORM>(w, nw, t, nsrcs, srcs, ndsts, dsts, offset, Nrem, mainBufferOffset + offset*PACKELEMS, bias, biasSize, randNumGen, dropoutProb, residual);
}

#endif // COMMON_KERNEL_H_
