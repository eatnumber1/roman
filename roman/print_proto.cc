#include "roman/print_proto.h"

#include "rhutil/status.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"

ABSL_FLAG(bool, binary, false,
          "Output using the binary format instead of text");

namespace roman {

using ::rhutil::Status;
using ::rhutil::OkStatus;
using ::rhutil::UnknownError;
using ::google::protobuf::Message;
using ::google::protobuf::io::OstreamOutputStream;
using ::google::protobuf::TextFormat;

Status PrintProto(const Message &message, std::ostream *out) {
  if (absl::GetFlag(FLAGS_binary)) {
    if (!message.SerializeToOstream(out)) {
      return UnknownError("Failed to write romdat to stdout");
    }
  } else {
    OstreamOutputStream strm(out);
    if (!TextFormat::Print(message, &strm)) {
      return UnknownError("Failed to write romdat textproto to stdout");
    }
  }
  return OkStatus();
}

}  // namespace roman
