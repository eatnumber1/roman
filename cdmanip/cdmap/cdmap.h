#include "cdmanip/cdmap/cdmap.pb.h"
#include "absl/strings/string_view.h"

namespace cdmanip {
namespace cdmap {

CompactDiscMap FromFile(absl::string_view path);

}  // namespace cdmap
}  // namespace cdmanip
