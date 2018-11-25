#include <vector>
#include <string>
#include <sysexits.h>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <iostream>
#include <ostream>

#include "cdmanip/hash.h"
#include "cdmanip/subcommands/subcommands.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_cat.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/fixed_array.h"

using ::std::printf;
using ::std::fprintf;
using ::std::exit;
using ::std::string;
using ::std::perror;
using ::absl::string_view;
using ::absl::StrCat;
using ::absl::flat_hash_map;
using ::absl::StrFormat;
using ::absl::FixedArray;
using ::cdmanip::subcommands::ParseCuesheet;
using ::cdmanip::subcommands::DumpCompactDiscMap;
using ::cdmanip::subcommands::ExpandTracks;

namespace cdmanip {

#if 0
constexpr const int64_t USER_BYTES_PER_SECTOR = 2048;
constexpr const int64_t BYTES_PER_CDROM_MODE1_SECTOR = 2352;

stat Stat(string_view file) {
  stat buf;
  if (stat(file.data(), &buf) == -1) {
    perror(StrFormat("Could not stat file '%s'", file).data());
    exit(EX_NOINPUT);
  }
  return buf;
}

void IdentifyCdImage(string_view file) {
  stat image_info = Stat(file);
  auto image_size = image_info.st_size;

  // The GCD of USER_BYTES_PER_SECTOR and BYTES_PER_CDROM_MODE1_SECTOR is 16, so
  // files with at least one sector are guaranteed to never be ambiguous.
  if (image_size < USER_BYTES_PER_SECTOR) {
    fprintf(stderr, "File '%s' is too small to be a CD image\n", file.data());
    exit(EX_DATAERR);
  }

  if (image_size % USER_BYTES_PER_SECTOR == 0) {
    int64_t num_sectors = image_size / USER_BYTES_PER_SECTOR;
    printf("%s: user data consisting of %lld sectors\n", file.data(),
           num_sectors);
  } else if (image_size % BYTES_PER_CDROM_MODE1_SECTOR == 0) {
    int64_t num_sectors = image_size / BYTES_PER_CDROM_MODE1_SECTOR;
    printf("%s: %lld sectors\n", file.data(), num_sectors);
  } else {
    printf("%s: unknown\n", file.data());
  }
}

void CalculateMD5(Cuesheet *cuesheet) {
  flat_hash_map<string, string> hashes;
  for (Track &track : *cuesheet->mutable_track()) {
    auto &file = *track.mutable_file();
    const auto &filepath = file.path();
    if (!file.md5sum().empty()) {
      hashes.emplace(filepath, file.md5sum());
      continue;
    }

    if (hashes.find(filepath) == hashes.end()) {
      HasherMd5 hasher = HasherMd5::Create();

      int fd = open(filepath.c_str(), O_RDONLY);
      if (fd == -1) {
        perror(StrFormat("Could not open file '%s'", filepath).data());
        exit(EX_DATAERR);
      }

      FixedArray<char> buf(track.file().blksize());
      ssize_t nb = -1;
      do {
        nb = read(fd, buf.data(), buf.memsize());
        if (nb == -1) {
          if (errno == EINTR) continue;
          perror(StrFormat("Could not read file '%s'", filepath).data());
          exit(EX_DATAERR);
        }

        hasher.Update({buf.data(), static_cast<size_t>(nb)});
      } while (nb != 0);

      if (close(fd) == -1) {
        perror(StrFormat("Could not close file '%s'", filepath).data());
        exit(EX_DATAERR);
      }

      hashes.emplace(filepath, hasher.Finalize());
    }

    file.set_md5sum(hashes[filepath]);
  }
}

bool PrintCuesheet(string_view file) {
  Cuesheet cuesheet = ParseCuesheet(file);
  CalculateMD5(&cuesheet);
  std::cout << cuesheet.DebugString() << std::endl;

  return true;
}
#endif

}  // namespace cdmanip

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << getprogname() << " subcommand [args]" << std::endl;
    exit(EX_USAGE);
  }

  // Remove the program name.
  argv++;
  argc--;

  string_view subcommand(argv[0]);

  //IdentifyCdImage(argv[1]);
  //PrintCuesheet(image_file);

  if (subcommand == "parse_cuesheet") {
    ParseCuesheet(argc, argv);
  } else if (subcommand == "dump_cdmap") {
    DumpCompactDiscMap(argc, argv);
  } else if (subcommand == "expand_tracks") {
    ExpandTracks(argc, argv);
  } else {
    std::cerr << "Unknown subcommand " << subcommand << std::endl;
    exit(EX_USAGE);
  }

  return EXIT_SUCCESS;
}
