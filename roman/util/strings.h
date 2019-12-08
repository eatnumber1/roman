#ifndef ROMAN_UTIL_STRINGS_H_
#define ROMAN_UTIL_STRINGS_H_

#include <cstdint>

#include "absl/types/span.h"

namespace roman {

template <typename T>
absl::Span<T> SpanFromNullTerm(T *ary) {
  std::size_t size = 0;
  for (T *e = ary; e != nullptr && *e != nullptr; e++) size++;
  return absl::MakeSpan(ary, size);
}

}  // namespace roman

#endif  // ROMAN_UTIL_STRINGS_H_
