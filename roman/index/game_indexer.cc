#include "roman/index/game_indexer.h"

namespace roman {

using ::rhutil::Status;
using ::rhutil::StatusOr;
using ::rhutil::OkStatus;
using ::rhutil::InvalidArgumentError;
using ::rhutil::InvalidArgumentErrorBuilder;
using ::rhutil::NotFoundErrorBuilder;
using ::dat2pb::RomDat;

namespace {

absl::flat_hash_map<
  Hash, std::vector<std::pair<const RomDat::Game*, const RomDat::Game::Rom*>>>
RomsByHash(Hash::Type type, const RomDat &dat) {
  absl::flat_hash_map<
        Hash,
        std::vector<std::pair<const RomDat::Game*, const RomDat::Game::Rom*>>>
      roms_by_hash;
  for (const RomDat::Game &game : dat.game()) {
    for (const RomDat::Game::Rom &rom : game.rom()) {
      std::string_view hval;
      switch (type) {
        case Hash::MD5:
          hval = rom.md5();
          break;
        case Hash::SHA1:
          hval = rom.sha1();
          break;
        case Hash::CRC:
          hval = rom.crc();
          break;
        default:
          CHECK(false);
      }
      roms_by_hash[Hash(type, hval)].emplace_back(std::make_pair(&game, &rom));
    }
  }

  return roms_by_hash;
}

}  // namespace

GameIndexer::GameIndexer(Hash::Type hash_type, const RomDat &dat)
  : hash_type_(hash_type), roms_by_hash_(RomsByHash(hash_type, dat))
{}

Status GameIndexer::AddFile(absl::string_view path, const Hash &hash) {
  if (hash.GetType() != hash_type_) {
    return InvalidArgumentErrorBuilder()
        << "Hash for path " << path << " has type "
        << Hash::TypeToString(hash.GetType())
        << ", but this indexer only supports "
        << Hash::TypeToString(hash_type_);
  }

  const std::vector<std::pair<const RomDat::Game*, const RomDat::Game::Rom*>>
      *games_and_roms;
  if (auto it = roms_by_hash_.find(hash); it == roms_by_hash_.end()) {
    return NotFoundErrorBuilder()
        << "A rom with hash " << hash
        << " was not found in the database. The unknown file's path is "
        << path;
  } else {
    games_and_roms = &it->second;
  }

  for (const auto &p : *games_and_roms) {
    const RomDat::Game *game = p.first;
    const RomDat::Game::Rom *rom = p.second;

    GameIndex::Game *igame;
    if (auto it = indexed_games_.find(game); it == indexed_games_.end()) {
      igame = index_.add_game();
      *igame->mutable_dat() = *game;
      indexed_games_[game] = igame;
    } else {
      igame = it->second;
    }

    GameIndex::Game::Rom *irom;
    auto key = std::make_pair(game, rom);
    if (auto it = indexed_roms_.find(key); it == indexed_roms_.end()) {
      GameIndex::Game::Rom *irom = igame->add_rom();
      *irom->mutable_dat() = *rom;
      irom->set_path(std::string(path));
      indexed_roms_[key] = irom;
    } else {
      irom = it->second;
    }

    (void)irom;
  }

  return OkStatus();
}

StatusOr<GameIndex> GameIndexer::GetIndex() {
  return std::move(index_);
}

}  // namespace roman
