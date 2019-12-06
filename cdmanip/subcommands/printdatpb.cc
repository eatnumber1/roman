#include <string_view>
#include <string>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <fstream>

#include "rhutil/file.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "rhutil/module_init.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/types/span.h"
#include "cdmanip/subcommands/subcommands.h"
#include "rhutil/status.h"
#include "dat2pb/romdat.pb.h"
#include "dat2pb/parser.h"

namespace cdmanip {
namespace {

using ::dat2pb::RomDat;
using ::rhutil::StatusOr;
using ::rhutil::Status;
using ::rhutil::OkStatus;
using ::rhutil::UnknownErrorBuilder;
using ::rhutil::UnknownError;
using ::rhutil::InvalidArgumentError;
using ::rhutil::InvalidArgumentErrorBuilder;
using ::rhutil::OpenInputFile;
using ::google::protobuf::io::OstreamOutputStream;
using ::google::protobuf::TextFormat;

Status SubCommandPrintDatPB(absl::Span<std::string_view> args) {
  if (args.size() != 2) {
    return InvalidArgumentError("Usage: cdmanip printdatpb datpb");
  }
  std::string_view datpb(args[1]);

  RomDat dat;
  {
    ASSIGN_OR_RETURN(std::ifstream datpb_in, OpenInputFile(datpb));
    if (!dat.ParseFromIstream(&datpb_in)) {
      return UnknownErrorBuilder() << "Failed to read romdat from " << datpb;
    }
  }

  OstreamOutputStream strm(&std::cout);
  if (!TextFormat::Print(dat, &strm)) {
    return UnknownError("Failed to write romdat textproto to stdout");
  }

  return OkStatus();
}

static void Initialize() {
  SubCommands::Instance()->Add("printdatpb", &SubCommandPrintDatPB);
}
rhutil::ModuleInit module_init(&Initialize);

}  // namespace
}  // namespace cdmanip
