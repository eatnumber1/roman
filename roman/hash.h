#ifndef ROMAN_HASH_H_
#define ROMAN_HASH_H_

#include <string>
#include <string_view>
#include <ostream>

namespace roman {

class Hash {
 public:
  enum Type {
    UNKNOWN = 0,
    MD5 = 1,
    SHA1 = 2,
    CRC = 3
  };

  Hash(Type type, std::string_view value);

  Type GetType() const;
  const std::string &GetValue() const;

  template <typename H>
  friend H AbslHashValue(H h, const Hash &s) {
    return H::combine(std::move(h), s.type_, s.value_);
  }

  friend std::ostream &operator<<(std::ostream &os, const Hash &hash);

  bool operator==(const Hash &o) const;
  bool operator!=(const Hash &o) const;

  static std::string TypeToString(Type t);

 private:
  Type type_;
  std::string value_;
};

}  // namespace hash

#endif  // ROMAN_HASH_H_
