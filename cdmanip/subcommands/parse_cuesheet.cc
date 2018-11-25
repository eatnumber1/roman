#include "cdmanip/subcommands/subcommands.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <sysexits.h>
#include <iostream>
#include <ostream>

#include "cdmanip/cdmap/cdmap.pb.h"
#include "cdmanip/cuesheet.h"
#include "absl/strings/str_format.h"
#include "absl/strings/string_view.h"

namespace cdmanip {
namespace subcommands {

using ::std::exit;
using ::std::perror;
using ::absl::StrFormat;
using ::absl::string_view;
using ::cdmanip::cdmap::CompactDiscMap;

void ParseCuesheet(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << getprogname() << " " << argv[0]
              << " source_cuesheet destination_cdmap" << std::endl;
    exit(EX_USAGE);
  }
  string_view cuesheet_file(argv[1]);
  string_view cdmap_file(argv[2]);

  CompactDiscMap cdmap = ::cdmanip::ParseCuesheet(cuesheet_file);

  int fd = open(cdmap_file.data(), O_WRONLY | O_CREAT | O_EXCL,
                S_IRUSR | S_IWUSR | S_IRGRP);
  if (fd == -1) {
    perror(StrFormat("Error opening file \"%s\"", cdmap_file).data());
    exit(EX_CANTCREAT);
  }

  if (!cdmap.SerializeToFileDescriptor(fd)) {
    perror(StrFormat("Error writing to file \"%s\"", cdmap_file).data());
    exit(EX_IOERR);
  }

  if (close(fd) == -1) {
    perror(StrFormat("Error closing file \"%s\"", cdmap_file).data());
    exit(EX_IOERR);
  }
}

}  // namespace subcommands
}  // namespace cdmanip
