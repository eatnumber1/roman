#include "cdmanip/cuesheet.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <fstream>
#include <functional> 
#include <iostream>
#include <iterator>
#include <locale>
#include <memory>
#include <ostream>
#include <string>
#include <sys/stat.h>
#include <sysexits.h>
#include <utility>
#include <vector>

#include "absl/types/optional.h"
#include "absl/strings/match.h"
#include "absl/strings/str_split.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_format.h"
#include "absl/strings/numbers.h"
#include "absl/algorithm/container.h"

namespace cdmanip {

using ::absl::SkipEmpty;
using ::absl::StartsWith;
using ::absl::StrCat;
using ::absl::StrSplit;
using ::absl::StrFormat;
using ::absl::make_optional;
using ::absl::optional;
using ::absl::string_view;
using ::std::getline;
using ::std::perror;
using ::std::exit;
using ::std::ifstream;
using ::std::make_shared;
using ::std::shared_ptr;
using ::std::string;
using ::std::vector;
using ::cdmanip::cdmap::CompactDiscMap;
using ::cdmanip::cdmap::Track;
using ::cdmanip::cdmap::File;

namespace {

// Annoyingly, neither ::absl nor ::std contains a facility to trim a string.
// This implementation was grabbed from https://stackoverflow.com/a/217605
// -- begin stackoverflow code

// trim from start
static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
            std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

// trim from end
static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(),
            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

// trim from both ends
static inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}

// -- end stackoverflow code

bool IsEmpty(const Track::Duration &d) {
  return d.minutes() == 0 && d.seconds() == 0 && d.sectors() == 0;
}

int32_t StrToInt32(string_view s) {
  int32_t num = -1;
  if (!SimpleAtoi(s, &num)) {
    std::cerr << "Could not parse \"" << s << "\" as an int" << std::endl;
    exit(EX_DATAERR);
  }
  return num;
}

int64_t DurationToBytes(const Track::Duration &duration, int64_t sector_size) {
  static constexpr const int64_t seconds_per_minute = 60;
  assert(duration.seconds() < seconds_per_minute);
  // Pulled from
  // https://en.wikipedia.org/wiki/Compact_Disc_Digital_Audio#Frames_and_timecode_frames
  static constexpr const int64_t sectors_per_second = 75;
  assert(duration.sectors() < sectors_per_second);

  int64_t bytes = 0;
  bytes += duration.sectors() * sector_size;
  bytes += duration.seconds() * sectors_per_second * sector_size;
  bytes += duration.minutes() * seconds_per_minute * sectors_per_second
    * sector_size;
  return bytes;
}

bool ParseFileType(string_view name, File::Type *value) {
  string myname(StrCat("TYPE_", name));
  if (myname == "TYPE_BINARY") {
    myname = "TYPE_BINARY_BIG_ENDIAN";
  } else if (myname == "TYPE_MOTOROLA") {
    myname = "TYPE_BINARY_LITTLE_ENDIAN";
  } else {
  }
  return File_Type_Parse(myname, value);
}

bool ParseTrackType(string_view name, Track::Type *value) {
  string myname(StrCat("TYPE_", name));
  absl::c_replace(myname, '/', '_');
  return Track_Type_Parse(myname, value);
}

string ParseCatalog(string_view line) {
  vector<string_view> tokens = StrSplit(line, ' ', SkipEmpty());
  assert(tokens.size() == 2);
  assert(tokens.at(0) == "CATALOG");

  return string(tokens.at(1));
}

Track ParseTrack(const File &file, string_view line) {
  vector<string_view> tokens = StrSplit(line, ' ', SkipEmpty());
  assert(tokens.size() == 3);
  assert(tokens.at(0) == "TRACK");

  Track::Type datatype;
  if (!ParseTrackType(tokens.at(2), &datatype)) {
    std::cerr << "Could not parse \"" << tokens[2] << "\" as a Track::Type"
              << std::endl;
    exit(EX_DATAERR);
  }

  int64_t sector_size = 0;
  switch (datatype) {
    case Track::TYPE_CDG:
      sector_size = 2448;
      break;
    case Track::TYPE_MODE1_2048:
      sector_size = 2048;
      break;
    case Track::TYPE_CDI_2336:
    case Track::TYPE_MODE2_2336:
      sector_size = 2336;
      break;
    case Track::TYPE_AUDIO:
    case Track::TYPE_CDI_2352:
    case Track::TYPE_MODE1_2352:
    case Track::TYPE_MODE2_2352:
      sector_size = 2352;
      break;
    default:
      assert(false);
  }

  Track track;
  track.set_datatype(datatype);
  track.mutable_file()->CopyFrom(file);
  track.set_num(StrToInt32(tokens.at(1)));
  track.set_sector_size(sector_size);

  return track;
}

Track::Duration ParseDuration(string_view duration) {
  vector<string_view> tokens = StrSplit(duration, ':', SkipEmpty());
  assert(tokens.size() == 3);

  vector<int32_t> nums;
  absl::c_transform(tokens, std::back_inserter(nums), StrToInt32);

  Track::Duration out;

  out.set_minutes(nums[0]);
  out.set_seconds(nums[1]);
  out.set_sectors(nums[2]);

  return out;
}

Track::Duration ParsePregap(string_view line) {
  vector<string_view> tokens = StrSplit(line, ' ', SkipEmpty());
  assert(tokens.size() == 2);
  assert(tokens.at(0) == "PREGAP");

  return ParseDuration(tokens.at(1));
}

Track::Index ParseIndex(const Track &track, string_view line) {
  vector<string_view> tokens = StrSplit(line, ' ', SkipEmpty());
  assert(tokens.size() == 3);
  assert(tokens.at(0) == "INDEX");

  Track::Index index;
  *index.mutable_start_position() = ParseDuration(tokens.at(2));
  index.set_num(StrToInt32(tokens.at(1)));
  index.set_offset(DurationToBytes(index.start_position(),
                                   track.sector_size()));

  return index;
}

blksize_t BlockSize(const struct stat &st) {
  // Based off of https://unix.stackexchange.com/a/245507
  constexpr const blksize_t io_bufsize = 128 * 1024;
  assert(io_bufsize % st.st_blksize == 0);
  return std::max(st.st_blksize, io_bufsize);
}

File ParseFile(string_view basedir, string_view line) {
  assert(StartsWith(line, "FILE "));

  string myline(line);

  // Remove the 'FILE ' and optionally the opening quote.
  myline.erase(0, 5);
  if (StartsWith(myline, "\"")) myline.erase(0, 1);

  // Reverse the line
  std::reverse(myline.begin(), myline.end());

  // Find the first ' ', before which is the reversed file type.
  auto delim_pos = myline.find(' ');
  assert(delim_pos > 0);
  string filetype(myline.substr(0, delim_pos));
  std::reverse(filetype.begin(), filetype.end());
  myline.erase(0, delim_pos + 1);

  // Optionally strip off the closing quote.
  if (StartsWith(myline, "\"")) myline.erase(0, 1);

  // Everything left is the filename. Reverse it back to normal.
  std::reverse(myline.begin(), myline.end());
  string filename = std::move(myline);

  File::Type parsed_filetype;
  if (!ParseFileType(filetype, &parsed_filetype)) {
    std::cerr << "Could not parse \"" << filetype << "\" as a File::Type"
              << std::endl;
    exit(EX_DATAERR);
  }

  const auto &path = StrCat(basedir, "/", filename);
  struct stat st;
  if (stat(path.data(), &st) == -1) {
    perror(StrFormat("Could not stat file '%s'", path).data());
    exit(EX_NOINPUT);
  }

  File file;
  file.set_name(std::move(filename));
  file.set_basedir(basedir.data());
  file.set_type(parsed_filetype);
  file.set_size(st.st_size);
  file.set_blksize(BlockSize(st));
  return file;
}

void PopulateLength(CompactDiscMap *cuesheet) {
  Track::Index *previous_index = nullptr;
  const File *previous_file = nullptr;
  for (Track &track : *cuesheet->mutable_track()) {
    if (previous_file && previous_file->name() != track.file().name()) {
      assert(previous_index);
      assert(previous_file->size() > previous_index->offset());
      previous_index->set_length(
          previous_file->size() - previous_index->offset());
      previous_index = nullptr;
    }
    for (Track::Index &index : *track.mutable_index()) {
      if (previous_index) {
        assert(index.offset() > previous_index->offset());
        previous_index->set_length(index.offset() - previous_index->offset());
      }
      previous_index = &index;
    }
    previous_file = &track.file();
  }
  assert(previous_index);
  assert(previous_file);
  assert(previous_file->size() > previous_index->offset());
  previous_index->set_length(previous_file->size() - previous_index->offset());
}

}  // namespace

CompactDiscMap ParseCuesheet(string_view path) {
  string basedir(path);
  basedir.erase(basedir.find_last_of('/'));

  CompactDiscMap cuesheet;

  optional<File> current_file;
  optional<Track> current_track;
  ifstream cuefile(path.data());
  if (!cuefile) {
    std::cerr << "Could not open file '" << path << "'" << std::endl;
    exit(EX_DATAERR);
  }
  for (string line; getline(cuefile, line); ) {
    trim(line);
    if (line.empty()) continue;

    if (StartsWith(line, "CATALOG ")) {
      cuesheet.set_catalog(ParseCatalog(line));
      continue;
    }

    if (StartsWith(line, "FILE ")) {
      current_file = make_optional<File>(ParseFile(basedir, line));
      continue;
    }
    assert(current_file);

    if (StartsWith(line, "TRACK ")) {
      if (current_track) {
        *cuesheet.add_track() = std::move(*current_track);
      }
      current_track = make_optional<Track>(ParseTrack(*current_file, line));
      continue;
    }
    assert(current_track);

    if (StartsWith(line, "PREGAP ")) {
      if (!IsEmpty(current_track->pregap())) {
        std::cerr << "Multiple pregaps within a single track is forbidden."
                  << std::endl;
        exit(EX_DATAERR);
      }
      *current_track->mutable_pregap() = ParsePregap(line);
      continue;
    }

    if (StartsWith(line, "INDEX ")) {
      *current_track->add_index() = ParseIndex(*current_track, line);
      continue;
    }

    std::cerr << "Could not parse line: \"" << line << "\"" << std::endl;
  }

  PopulateLength(&cuesheet);

  return cuesheet;
}

}  // namespace cdmanip
