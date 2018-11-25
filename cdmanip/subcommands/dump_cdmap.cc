#include "cdmanip/subcommands/subcommands.h"

#include <stdlib.h>
#include <cstdio>
#include <cstdlib>
#include <sysexits.h>
#include <iostream>
#include <ostream>

#include "cdmanip/cdmap/cdmap.pb.h"
#include "cdmanip/cdmap/cdmap.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"

namespace cdmanip {
namespace subcommands {

using ::std::exit;
using ::std::perror;
using ::absl::string_view;
using ::cdmanip::cdmap::CompactDiscMap;
using ::google::protobuf::TextFormat;
using ::google::protobuf::io::OstreamOutputStream;
namespace cdmap = ::cdmanip::cdmap;

void DumpCompactDiscMap(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << getprogname() << " " << argv[0]
              << " dump_cdmap cdmap_file" << std::endl;
    exit(EX_USAGE);
  }
  string_view cdmap_file(argv[1]);

  auto cdmap = cdmap::FromFile(cdmap_file);

  OstreamOutputStream cout_output_stream(&std::cout);
  if (!TextFormat::Print(cdmap, &cout_output_stream)) {
    perror("Error printing cdmap");
    exit(EX_IOERR);
  }
}

}  // namespace subcommands
}  // namespace cdmanip
