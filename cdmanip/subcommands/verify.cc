#include <string_view>
#include <string>
#include <sstream>
#include <cstdlib>
#include <memory>
#include <fstream>
#include <optional>

#include "rhutil/module_init.h"
#include "nlohmann/json.hpp"
#include "rhutil/file.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_split.h"
#include "absl/types/span.h"
#include "cdmanip/subcommands/subcommands.h"
#include "rhutil/status.h"
#include "dat2pb/romdat.pb.h"
#include "dat2pb/parser.h"
#include "rcrc/rclone.h"
#include "rcrc/remote.h"
#include "absl/flags/flag.h"
#include "absl/container/flat_hash_map.h"
#include "absl/container/flat_hash_set.h"
#include "cdmanip/common_flags.h"

using ::rhutil::CurlURL;

ABSL_FLAG(CurlURL, rclone_url, CurlURL::FromStringOrDie("http://url.invalid"),
          "The URL used to connect to RClone. Authorization is required.");
ABSL_FLAG(bool, recursive, false,
          "Whether to check recursively.");

namespace cdmanip {
namespace {

using ::dat2pb::RomDat;
using ::rhutil::StatusOr;
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

StatusOr<RomDat> ReadRomDat(absl::string_view path) {
  ASSIGN_OR_RETURN(std::ifstream in, OpenInputFile(path));
  RomDat dat;
  if (!dat.ParseFromIstream(&in)) {
    return UnknownErrorBuilder() << "Failed to read romdat from " << path;
  }
  return dat;
}

class RemoteHashReader {
 public:
  struct Options {
    RClone::Options rclone_opts;
    bool recurse = false;
  };

  RemoteHashReader() = default;
  RemoteHashReader(const Options &opts) : opts_(opts) {}

  Status Read(absl::string_view path,
              std::function<Status(json)> callback) {
    ASSIGN_OR_RETURN(rcrc::Remote rc, rcrc::Remote::Create(opts_.rclone_opts.remote));

    std::pair<std::string, std::string_view> p =
        absl::StrSplit(path, absl::MaxSplits(':', 1));
    auto [fs, remote] = p;
    if (fs.empty() || remote.empty()) {
      return InvalidArgumentErrorBuilder() << "Invalid remote " << path;
    }
    fs += ":";

    return rc.OperationsList({
          {"fs", fs},
          {"remote", remote},
          {"opt", {
            {"showHash", true},
            {"recurse", opts_.recurse},
            {"filesOnly", true},
            {"noModTime", true}
          }}
        }, std::move(callback));
  }

 private:
  Options opts_;
};

class GameIndexer {
 public:
  absl::flat_hash_map<const RomDat::Game*, const RomDat::Game::Rom*>
  static RomsByGame(const RomDat &dat) {
    absl::flat_hash_map<const RomDat::Game*, const RomDat::Game::Rom*>
      roms_by_game;
    for (const RomDat::Game &game : dat.game()) {
      for (const RomDat::Game::Rom &rom : game.rom()) {
        roms_by_game.emplace(&game, &rom);
      }
    }

    return roms_by_game;
  }

  enum class HashType {
    MD5, SHA1, CRC
  };

  absl::flat_hash_map<std::string_view, const RomDat::Game::Rom*>
  static RomsByHash(HashType type, const RomDat &dat) {
    absl::flat_hash_map<std::string_view, const RomDat::Game::Rom*>
      roms_by_hash;
    for (const RomDat::Game &game : dat.game()) {
      for (const RomDat::Game::Rom &rom : game.rom()) {
        std::string_view hash;
        switch (type) {
          case HashType::MD5:
            hash = rom.md5();
            break;
          case HashType::SHA1:
            hash = rom.sha1();
            break;
          case HashType::CRC:
            hash = rom.crc();
            break;
        }
        roms_by_hash.emplace(hash, &rom);
      }
    }

    return roms_by_hash;
  }
};

Status SubCommandVerify(absl::Span<std::string_view> args) {
  if (args.size() != 3) {
    return InvalidArgumentError("Usage: cdmanip verify datpb fs:path");
  }
  std::string_view datpb(args[1]);
  std::string_view fspath(args[2]);

  RETURN_IF_ERROR(rcrc::InitializeGlobals());

  std::cout << "Reading DAT" << std::endl;
  ASSIGN_OR_RETURN(RomDat dat, ReadRomDat(datpb));

  RemoteHashReader::Options opts;
  opts.rclone_opts.remote.url = absl::GetFlag(FLAGS_rclone_url);
  opts.rclone_opts.remote.verbose = absl::GetFlag(FLAGS_verbose);
  opts.recurse = absl::GetFlag(FLAGS_recursive);
  RemoteHashReader hash_reader(opts);

  std::cout << "Reticulating splines" << std::endl;
  int count = 0, matches = 0, missing = 0;
  absl::flat_hash_set<std::string> founds;
  absl::flat_hash_map<std::string_view, const RomDat::Game::Rom*> md5_roms =
    GameIndexer::RomsByHash(GameIndexer::HashType::MD5, dat);
  std::cout << "Checking against " << md5_roms.size() << " roms" << std::endl;
  std::cout << "Reading hashes" << std::endl;
  RETURN_IF_ERROR(hash_reader.Read(fspath, [&](json file) -> Status {
    count++;
    auto md5 = file["Hashes"]["MD5"].get<std::string_view>();
    if (auto it = md5_roms.find(md5); it != md5_roms.end()) {
      const RomDat::Game::Rom *rom = it->second;
      matches++;
      founds.emplace(md5);
      std::cout << file["Path"].get<std::string_view>() << std::endl
                << rom->name() << std::endl << std::endl;
    }
    return OkStatus();
  }));
  for (auto [md5, rom] : md5_roms) {
    if (founds.find(md5) == founds.end()) missing++;
  }
  std::cout << "Checked " << count << " files" << std::endl;
  std::cout << "Found " << matches << " matches" << std::endl;
  std::cout << "Missing " << missing << " roms" << std::endl;

  //std::cout << hashes.dump(2) << std::endl;

  return OkStatus();
}

static void Initialize() {
  SubCommands::Instance()->Add("verify", &SubCommandVerify);
}
rhutil::ModuleInit module_init(&Initialize);

}  // namespace
}  // namespace cdmanip
