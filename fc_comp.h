#ifndef __FC_COMP_H__
#define __FC_COMP_H__

#include <bit>
#include <bitset>
#include <concepts>
#include <cstdint>
#include <immintrin.h>
#include <iostream>
#include <type_traits>


namespace frozenca {

template <typename K>
concept CanUseSimd = (sizeof(K) == 4 || sizeof(K) == 8) &&
                     (std::signed_integral<K> || std::floating_point<K>);

using regi = __m512i;
using regf = __m512;
using regd = __m512d;

regi broadcast(std::int32_t key) { return _mm512_set1_epi32(key); }

regi broadcast(std::int64_t key) { return _mm512_set1_epi64(key); }

regf broadcast(float key) { return _mm512_set1_ps(key); }

regd broadcast(double key) { return _mm512_set1_pd(key); }

unsigned int cmp(regi key, const std::int32_t *key_ptr) {
  regi keys_to_comp =
      _mm512_load_si512(reinterpret_cast<const regi *>(key_ptr));
  return _mm512_cmpgt_epi32_mask(key, keys_to_comp);
}

unsigned int cmp(regi key, const std::int64_t *key_ptr) {
  regi keys_to_comp =
      _mm512_load_si512(reinterpret_cast<const regi *>(key_ptr));
  return _mm512_cmpgt_epi64_mask(key, keys_to_comp);
}

unsigned int cmp(regf key, const float *key_ptr) {
  regf keys_to_comp = _mm512_load_ps(key_ptr);
  return _mm512_cmp_ps_mask(key, keys_to_comp, _MM_CMPINT_GT);
}

unsigned int cmp(regd key, const double *key_ptr) {
  regd keys_to_comp = _mm512_load_pd(key_ptr);
  return _mm512_cmp_pd_mask(key, keys_to_comp, _MM_CMPINT_GT);
}

unsigned int cmp(const std::int32_t *key_ptr, regi key) {
  regi keys_to_comp =
      _mm512_load_si512(reinterpret_cast<const regi *>(key_ptr));
  return _mm512_cmpgt_epi32_mask(keys_to_comp, key);
}

unsigned int cmp(const std::int64_t *key_ptr, regi key) {
  regi keys_to_comp =
      _mm512_load_si512(reinterpret_cast<const regi *>(key_ptr));
  return _mm512_cmpgt_epi64_mask(keys_to_comp, key);
}

unsigned int cmp(const float *key_ptr, regf key) {
  regf keys_to_comp = _mm512_load_ps(key_ptr);
  return _mm512_cmp_ps_mask(keys_to_comp, key, _MM_CMPINT_GT);
}

unsigned int cmp(const double *key_ptr, regd key) {
  regd keys_to_comp = _mm512_load_pd(key_ptr);
  return _mm512_cmp_pd_mask(keys_to_comp, key, _MM_CMPINT_GT);
}

template <CanUseSimd K> struct SimdTrait {
  static constexpr int shift = (sizeof(K) == 4) ? 4 : 3;
  static constexpr int mask = (sizeof(K) == 4) ? 0xF : 0x7;
  static constexpr int unit = (sizeof(K) == 4) ? 16 : 8;
};

template <CanUseSimd K, bool less>
inline std::int32_t get_lb_simd(K key, const K *first, const K *last) {
  const auto len = static_cast<std::int32_t>(last - first);
  const K *curr = first;
  auto key_broadcast = broadcast(key);
  int mask = 0;
  int offset = 0;
  while (offset < len) {
    if constexpr (less) {
      mask = ~cmp(key_broadcast, curr);
    } else {
      mask = ~cmp(curr, key_broadcast);
    }
    auto i = __builtin_ffs(mask) - 1;
    if (i < SimdTrait<K>::unit) {
      return offset + i;
    }
    curr += SimdTrait<K>::unit;
    offset += SimdTrait<K>::unit;
  }
  return len;
}

template <CanUseSimd K, bool less>
inline std::int32_t get_ub_simd(K key, const K *first, const K *last) {
  const auto len = static_cast<std::int32_t>(last - first);
  const K *curr = first;
  auto key_broadcast = broadcast(key);
  int mask = 0;
  int offset = 0;
  while (offset < len) {
    if constexpr (less) {
      mask = cmp(curr, key_broadcast);
    } else {
      mask = cmp(key_broadcast, curr);
    }
    auto i = __builtin_ffs(mask) - 1;
    if (i > 0) {
      return offset + i;
    }
    curr += SimdTrait<K>::unit;
    offset += SimdTrait<K>::unit;
  }
  return len;
}

} // namespace frozenca

#endif //__FC_COMP_H__