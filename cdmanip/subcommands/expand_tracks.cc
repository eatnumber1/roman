#include "cdmanip/subcommands/subcommands.h"

#include <errno.h>
#include <algorithm>
#include <sys/uio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
#include "absl/container/fixed_array.h"

namespace cdmanip {
namespace subcommands {

using ::std::exit;
using ::std::perror;
using ::absl::StrFormat;
using ::absl::string_view;
using ::cdmanip::cdmap::CompactDiscMap;
using ::cdmanip::cdmap::Track;
using ::cdmanip::cdmap::File;
using ::absl::FixedArray;
namespace cdmap = ::cdmanip::cdmap;

namespace {

void CopyFileRange(int source_fd, int dest_fd, size_t len, size_t blksize) {
  FixedArray<char> buf(blksize);

  size_t read_remaining = len;
  while (read_remaining != 0) {
    ssize_t nbr = read(source_fd, buf.data(),
                       std::min(buf.memsize(), read_remaining));
    if (nbr == -1) {
      if (errno == EINTR) continue;
      perror("read");
      exit(EX_IOERR);
    }
    assert(nbr != 0);

    size_t write_remaining = nbr;
    while (write_remaining != 0) {
      ssize_t nbw = write(dest_fd, buf.data(), write_remaining);
      if (nbw == -1) {
        if (errno == EINTR) continue;
        perror("write");
        exit(EX_IOERR);
      }
      write_remaining -= nbw;
    }

    read_remaining -= nbr;
  }
}

void ExpandIndex(const Track::Index &index, int source_fd, int dest_fd,
                 int64_t blksize) {
  off_t offset = lseek(source_fd, index.offset(), SEEK_SET);
  if (offset == -1) {
    perror("lseek");
    exit(EX_IOERR);
  }
  assert(offset == index.offset());

  CopyFileRange(source_fd, dest_fd, index.length(), blksize);
}

void ExpandTrack(const Track &track, int dest_dir_fd) {
  const File &source_file = track.file();

  int source_dir_fd = open(source_file.basedir().c_str(), O_RDONLY);
  if (source_dir_fd == -1) {
    perror(StrFormat("Error opening directory \"%s\"",
                     source_file.basedir()).data());
    exit(EX_NOINPUT);
  }

  int source_fd = openat(source_dir_fd, source_file.name().c_str(), O_RDONLY);
  if (source_fd == -1) {
    perror(StrFormat("Error opening source file \"%s\"",
           source_file.name()).data());
    exit(EX_NOINPUT);
  }

  if (close(source_dir_fd) == -1) {
    perror(StrFormat("Error closing directory \"%s\"",
                     source_file.basedir()).data());
    exit(EX_IOERR);
  }

  const auto &dest_name = StrFormat("Track%02d.bin", track.num());
  int dest_fd =
    openat(dest_dir_fd, dest_name.data(), O_WRONLY | O_CREAT | O_EXCL,
           S_IRUSR | S_IWUSR | S_IRGRP);
  if (dest_fd == -1) {
    perror(StrFormat("Error opening dest file \"%s\"", dest_name).data());
    exit(EX_CANTCREAT);
  }

  struct stat dest_stat;
  if (fstat(dest_fd, &dest_stat) == -1) {
    perror(StrFormat("Error fstat'ing dest file \"%s\"", dest_name).data());
    exit(EX_IOERR);
  }
  blksize_t blksize = std::max(static_cast<blksize_t>(source_file.blksize()),
                               dest_stat.st_blksize);

  for (const Track::Index &index : track.index()) {
    ExpandIndex(index, source_fd, dest_fd, blksize);
  }

  if (close(dest_fd) == -1) {
    perror(StrFormat("Error closing dest file \"%s\"", dest_name).data());
    exit(EX_IOERR);
  }

  if (close(source_fd) == -1) {
    perror(StrFormat("Error closing source file \"%s\"",
           source_file.name()).data());
    exit(EX_IOERR);
  }
}

}  // namespace

void ExpandTracks(int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: " << getprogname() << " " << argv[0]
              << " cdmap destdir" << std::endl;
    exit(EX_USAGE);
  }
  string_view cdmap_file(argv[1]);
  string_view destdir(argv[2]);

  int dest_dir_fd = open(destdir.data(), O_RDONLY);
  if (dest_dir_fd == -1) {
    perror(StrFormat("Error opening directory \"%s\"", destdir).data());
    exit(EX_NOINPUT);
  }

  auto cdmap = cdmap::FromFile(cdmap_file);
  for (const Track &track : *cdmap.mutable_track()) {
    ExpandTrack(track, dest_dir_fd);
  }

  if (close(dest_dir_fd) == -1) {
    perror(StrFormat("Error closing directory \"%s\"", destdir).data());
    exit(EX_IOERR);
  }
}

}  // namespace subcommands
}  // namespace cdmanip
