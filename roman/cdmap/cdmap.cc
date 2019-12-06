#include "roman/cdmap/cdmap.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstdio>
#include <cstdlib>
#include <sysexits.h>

#include "absl/strings/str_format.h"

namespace roman {
namespace cdmap {

using ::std::exit;
using ::std::perror;
using ::absl::StrFormat;
using ::absl::string_view;

CompactDiscMap FromFile(absl::string_view path) {
  CompactDiscMap cdmap;

  int fd = open(path.data(), O_RDONLY);
  if (fd == -1) {
    perror(StrFormat("Error opening file \"%s\"", path).data());
    exit(EX_NOINPUT);
  }

  if (!cdmap.ParseFromFileDescriptor(fd)) {
    perror(StrFormat("Error writing to file \"%s\"", path).data());
    exit(EX_IOERR);
  }

  if (close(fd) == -1) {
    perror(StrFormat("Error closing file \"%s\"", path).data());
    exit(EX_IOERR);
  }

  return cdmap;
}

}  // namespace cdmap
}  // namespace roman
