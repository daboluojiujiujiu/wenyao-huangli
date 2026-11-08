#pragma once
#include <stddef.h>
#include <stdint.h>

namespace TextUtf8 {
inline int charBytes(const char* s) {
  const auto* p = reinterpret_cast<const uint8_t*>(s);
  if (!p[0]) return 0;
  if (p[0] < 0x80) return 1;
  int n = p[0] >= 0xC2 && p[0] <= 0xDF ? 2
        : p[0] >= 0xE0 && p[0] <= 0xEF ? 3
        : p[0] >= 0xF0 && p[0] <= 0xF4 ? 4 : 0;
  if (!n) return 1;
  for (int i = 1; i < n; ++i) if ((p[i] & 0xC0) != 0x80) return 1;
  if ((n == 3 && ((p[0] == 0xE0 && p[1] < 0xA0) || (p[0] == 0xED && p[1] >= 0xA0))) ||
      (n == 4 && ((p[0] == 0xF0 && p[1] < 0x90) || (p[0] == 0xF4 && p[1] >= 0x90)))) return 1;
  return n;
}
}
