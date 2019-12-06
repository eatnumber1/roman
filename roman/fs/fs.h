#ifndef CDMANIP_FS_FS_H_
#define CDMANIP_FS_FS_H_

#include <string>
#include <cstddef>
#include <functional>
#include <memory>

#include "absl/time/time.h"
#include "absl/container/flat_hash_map.h"
#include "rhutil/status.h"

namespace roman {

struct FileInfo {
  std::string path;
  std::string name;
  std::size_t size = 0;
  absl::Time mtime = absl::InfinitePast();

  enum class Type {
    UNKNOWN,
    FILE,
    DIRECTORY
  }
  Type type = Type::UNKNOWN;

  absl::flat_hash_map<std::string, std::string> hashes;
};

struct StatOptions {
  bool require_mtime = false;
  bool require_hash = false;

  enum class Filter {
    NONE,
    FILES_ONLY
    DIRS_ONLY
  }
  Filter filter = Filter::NONE;
};

rhutil::Status StatRecursive(
    std::string_view path,
    std::function<rhutil::Status(std::unique_ptr<FileInfo>)> callback,
    const StatOptions &opts);

}  // namespace roman

#endif  // CDMANIP_FS_FS_H_
