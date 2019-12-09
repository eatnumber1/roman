#include "roman/hash.h"

namespace roman {

using Type = ::roman::Hash::Type;

Hash::Hash(Type type, std::string_view value)
  : type_(type), value_(std::string(value)) {}

Type Hash::GetType() const { return type_; }
const std::string &Hash::GetValue() const { return value_; }

std::string Hash::TypeToString(Type t) {
  switch (t) {
    case Hash::MD5: return "MD5";
    case Hash::SHA1: return "SHA1";
    case Hash::CRC: return "CRC";
    default: return "UNKNOWN";
  }
}

std::ostream &operator<<(std::ostream &os, const Hash &hash) {
  return os << hash.GetType() << ":" << hash.GetValue();
}

bool Hash::operator!=(const Hash &o) const {
  return !operator==(o);
}
bool Hash::operator==(const Hash &o) const {
  return type_ == o.type_ && value_ == o.value_;
}

}  // namespace roman
