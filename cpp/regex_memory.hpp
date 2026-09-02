#ifndef REGEX_MEMORY_HPP
#define REGEX_MEMORY_HPP

#include <cstddef>
#include <cstring>

#include "oniguruma.h"

inline size_t estimate_pattern_memory(const char* pattern, regex_t* regex) {
  const size_t len = pattern ? std::strlen(pattern) : 0;
  size_t bytes = 4096ull + len * 32ull;
  if (regex) {
    const int captures = onig_number_of_captures(regex);
    if (captures > 0) {
      bytes += static_cast<size_t>(captures) * 128ull;
    }
  }
  return bytes;
}

#endif  // REGEX_MEMORY_HPP
