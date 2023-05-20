
#include <climits>

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN ||                   \
    defined(__BIG_ENDIAN__)
#define UAPI_IS_BIG_ENDIAN 1
#else
#define UAPI_IS_BIG_ENDIAN 0
#endif

constexpr size_t getEndianIndex(size_t k, size_t max) noexcept {
#if UAPI_IS_BIG_ENDIAN
  return k;
#else
  return max - 1 - k;
#endif
}

// template <typename T> T swap_endian(T u) {
//   static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");

//   union {
//     T u;
//     unsigned char u8[sizeof(T)];
//   } source, dest;

//   source.u = u;

//   for (size_t k = 0; k < sizeof(T); k++)
//     dest.u8[k] = source.u8[sizeof(T) - k - 1];

//   return dest.u;
// }

// void applySwapEndianness(char *data, size_t s) {
//   for (int i = 0; i < s; i++) {
//     data[i] = swap_endian(data[i]);
//   }
// }
