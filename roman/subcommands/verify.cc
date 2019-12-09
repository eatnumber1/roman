#include <string_view>
#include <string>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <fstream>
#include <optional>
#include <vector>

#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "absl/flags/flag.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/types/span.h"
#include "dat2pb/romdat.pb.h"
#include "nlohmann/json.hpp"
#include "rcrc/rclone.h"
#include "rcrc/remote.h"
#include "rhutil/file.h"
#include "rhutil/module_init.h"
#include "rhutil/status.h"
#include "roman/common_flags.h"
#include "roman/hash.h"
#include "roman/index/game_indexer.h"
#include "roman/subcommands/subcommands.h"

namespace roman {
namespace {

using ::dat2pb::RomDat;
using ::rhutil::StatusOr;
using ::rhutil::IsNotFound;
using ::rhutil::Status;
using ::rhutil::OkStatus;
using ::rhutil::UnknownErrorBuilder;
using ::rhutil::CurlHandleDeleter;
using ::rhutil::CurlEasySetopt;
using ::rhutil::CurlEasyInit;
using ::rhutil::CurlEasyPerform;
using ::rhutil::CurlEasySetWriteCallback;
using ::rhutil::InvalidArgumentError;
using ::rhutil::InvalidArgumentErrorBuilder;
using ::rhutil::OpenInputFile;
using ::rcrc::RClone;
using json = ::nlohmann::json;

StatusOr<GameIndex> ReadIndex(absl::string_view path) {
  ASSIGN_OR_RETURN(std::ifstream in, OpenInputFile(path));
  GameIndex index;
  if (!index.ParseFromIstream(&in)) {
    return UnknownErrorBuilder() << "Failed to read game index from " << path;
  }
  return index;
}

constexpr char kUsageMessage[] = R"(Usage: roman verify [options] datpb fs:path

Verify the files in a directory against a datpb.

Verify requires a running RClone instance with its remote control interface
enabled. See https://rclone.org/.

Arguments:
  datpb - A datpb file describing valid ROMs.
  fs:path - A RClone-style path specifier. The fs portion specifies the RClone
    filesystem to access, path specifies the path on the remote.

The --rclone_url flag is required, and describes how to connect to RClone. It
must specify both the URL to RClone, along with the username and password needed
to perform authenticated operations. For example,
--rclone_url='http://user:pass@localhost:5572'

Also see rclone --help for additional options.

Example usage:
$ rclone rcd --rc-user=user --rc-pass=pass &
$ roman verify \
    --rclone_url='http://user:pass@localhost:5572' \
    --recursive \
    pce.datpb 'gdrive:/Games/PC Engine')";

Status SubCommandVerify(absl::Span<std::string_view> args) {
  if (args.size() != 2) {
    return InvalidArgumentError(kUsageMessage);
  }
  std::string_view indexpb(args[1]);

  std::cout << "Reading index" << std::endl;
  ASSIGN_OR_RETURN(GameIndex index, ReadIndex(indexpb));

  std::cout << "Reticulating splines" << std::endl;
  std::vector<const GameIndex::Game*> complete_games;
  std::vector<const GameIndex::Game*> incomplete_games;
  std::vector<const GameIndex::Game*> missing_games;
  for (const GameIndex::Game &game : index.game()) {
    std::vector<const GameIndex::Game*> *gamevec;
    if (game.rom_size() == 0) {
      gamevec = &missing_games;
    } else if (game.rom_size() != game.dat().rom_size()) {
      gamevec = &incomplete_games;
    } else {
      gamevec = &complete_games;
    }
    gamevec->emplace_back(&game);
  }

  std::cout << "Found " << complete_games.size() << " complete games" << std::endl;
  std::cout << "Found " << incomplete_games.size() << " incomplete games" << std::endl;
  std::cout << "Found " << missing_games.size() << " missing games" << std::endl;

  //std::cout << index.DebugString() << std::endl;

  return OkStatus();
}

static void Initialize() {
  SubCommands::Instance()->Add("verify", &SubCommandVerify);
}
rhutil::ModuleInit module_init(&Initialize);

}  // namespace
}  // namespace roman
