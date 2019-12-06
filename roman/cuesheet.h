#include "absl/strings/string_view.h"
#include "roman/cdmap/cdmap.pb.h"

namespace roman {

::roman::cdmap::CompactDiscMap ParseCuesheet(absl::string_view path);

}  // namespace roman
