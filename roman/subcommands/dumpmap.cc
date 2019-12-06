#include <string>
#include <cstdlib>
#include <cstdio>
#include <ostream>
#include <iostream>
#include <stdlib.h>

#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "absl/algorithm/container.h"
#include "absl/strings/str_split.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "absl/container/fixed_array.h"
#include "absl/flags/flag.h"
#include "roman/subcommands/subcommands.h"
#include "roman/util/errors.h"
#include "roman/util/glib.h"
#include "roman/util/cleanup.h"
#include "roman/cdmap/cdmap.pb.h"

namespace roman {

bool SubCommand::DumpMap(
    absl::Span<absl::string_view> args,
    GError **error) {
  if (args.size() != 2) {
    g_set_error(
        error, CDMANIP_ERROR, roman::ERR_USAGE,
        "Usage: %s %s map.cdm", getprogname(), args[0].data());
    return false;
  }
  absl::string_view map_file = args[1];

  int map_fd = open(map_file.data(), O_RDONLY);
  struct stat st;
  fstat(map_fd, &st);
  absl::FixedArray<char> buf(st.st_size);
  read(map_fd, buf.data(), st.st_size);
  close(map_fd);
  std::string buf_str(buf.data(), buf.size());

  CompactDiscMap map;
  map.ParseFromString(buf_str);

  absl::PrintF("%s\n", map.DebugString());
  return true;
}

}  // namespace roman
