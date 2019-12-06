#include <string>

#include <glib.h>

#include "openssl/md5.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace cdmanip {

class HasherMd5 {
public:
  HasherMd5(const HasherMd5 &o) = delete;
  HasherMd5(HasherMd5 &&o) = default;

  bool Update(absl::string_view data, GError **error);
  absl::optional<string> HasherMd5::Finalize(GError **error);

private:
  MD5_CTX *GetContext(GError **error);

  absl::optional<MD5_CTX> ctx_;  // non-copyable
};

}  // namespace cdmanip
