#ifndef ROMAN_PRINT_PROTO_H_
#define ROMAN_PRINT_PROTO_H_

#include <ostream>

#include "absl/flags/flag.h"
#include "rhutil/status.h"
#include "google/protobuf/message.h"

ABSL_DECLARE_FLAG(bool, binary);

namespace roman {

rhutil::Status PrintProto(const google::protobuf::Message &message,
                          std::ostream *out);

}  // namespace roman

#endif  // ROMAN_PRINT_PROTO_H_
