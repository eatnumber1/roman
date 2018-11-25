#include "absl/strings/string_view.h"
#include "cdmanip/cdmap/cdmap.pb.h"

namespace cdmanip {

::cdmanip::cdmap::CompactDiscMap ParseCuesheet(absl::string_view path);

}  // namespace cdmanip
