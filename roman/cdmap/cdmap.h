#include "roman/cdmap/cdmap.pb.h"
#include "absl/strings/string_view.h"

namespace roman {
namespace cdmap {

CompactDiscMap FromFile(absl::string_view path);

}  // namespace cdmap
}  // namespace roman
