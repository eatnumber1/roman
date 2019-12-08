#include "roman/hash.h"

#include <utility>
#include <sstream>
#include <ios>
#include <iomanip>
#include <cstdlib>

#include <iostream>

#include "openssl/err.h"

#include "roman/util/errors.h"

namespace roman {

MD5_CTX *HasherMd5::GetContext(GError **error) {
  if (ctx_) return ctx_.operator->();

  MD5_CTX ctx;
  if (MD5_Init(&ctx) != 1) {
    g_set_error(
        error, ROMAN_ERROR, roman::ERR_OPENSSL,
        "MD5_Init: %s",
        // TODO(eatnumber1): This is not thread safe. Make it so using
        // ERR_error_string_n
        ERR_error_string(ERR_get_error(), /*buf=*/nullptr));
    return nullptr;
  }
  ctx_.emplace(std::move(ctx));

  return ctx_.operator->();
}

absl::optional<std::string> HasherMd5::Finalize(GError **error) {
  MD5_CTX *ctx = GetContext(error);
  if (!ctx) return absl::nullopt;

  unsigned char digest[MD5_DIGEST_LENGTH];
  if (MD5_Final(digest, GetContext()) != 1) {
    g_set_error(
        error, ROMAN_ERROR, roman::ERR_OPENSSL,
        "MD5_Final: %s",
        // TODO(eatnumber1): This is not thread safe. Make it so using
        // ERR_error_string_n
        ERR_error_string(ERR_get_error(), /*buf=*/nullptr));
    return absl::nullopt;
  }

  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    ss << std::setw(2) << static_cast<int>(digest[i]);
  }
  return std::move(ss).str();
}

bool HasherMd5::Update(absl::string_view data, GError **error) {
  MD5_CTX *ctx = GetContext(error);
  if (!ctx) return false;

  if (MD5_Update(ctx, data.data(), data.size()) != 1) {
    g_set_error(
        error, ROMAN_ERROR, roman::ERR_OPENSSL,
        "MD5_Update: %s",
        // TODO(eatnumber1): This is not thread safe. Make it so using
        // ERR_error_string_n
        ERR_error_string(ERR_get_error(), /*buf=*/nullptr));
    return false;
  }

  return true;
}

}  // namespace roman
