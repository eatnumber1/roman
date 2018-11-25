#include <string>

#include "openssl/md5.h"
#include "absl/strings/string_view.h"

namespace cdmanip {

class HasherMd5 {
public:
  static HasherMd5 Create();

  void Update(absl::string_view data);
  std::string Finalize();

private:
  HasherMd5(MD5_CTX ctx);

  MD5_CTX ctx_;
};

}  // namespace cdmanip
