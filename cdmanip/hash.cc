#include "cdmanip/hash.h"

#include <utility>
#include <sstream>
#include <ios>
#include <iomanip>
#include <cstdlib>

#include <iostream>

#include "openssl/err.h"

namespace cdmanip {

using ::absl::string_view;
using ::std::string;
using ::std::stringstream;

HasherMd5::HasherMd5(MD5_CTX ctx) : ctx_(std::move(ctx)) {}

/*static*/ HasherMd5 HasherMd5::Create() {
  MD5_CTX ctx;
  if (MD5_Init(&ctx) != 1) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  return {std::move(ctx)};
}

string HasherMd5::Finalize() {
  unsigned char digest[MD5_DIGEST_LENGTH];
  if (MD5_Final(digest, &ctx_) != 1) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }

  stringstream ss;
  ss << std::hex << std::setfill('0');
  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    ss << std::setw(2) << static_cast<int>(digest[i]);
  }
  return std::move(ss).str();
}

void HasherMd5::Update(string_view data) {
  if (MD5_Update(&ctx_, data.data(), data.size()) != 1) {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
  }
}

}  // namespace cdmanip
