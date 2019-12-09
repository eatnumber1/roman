#ifndef ROMAN_GAME_INDEXER_H_
#define ROMAN_GAME_INDEXER_H_

#include <vector>
#include <string_view>
#include <utility>

#include "absl/container/flat_hash_map.h"
#include "dat2pb/romdat.pb.h"
#include "roman/index/game_index.pb.h"
#include "roman/hash.h"
#include "rhutil/status.h"

namespace roman {

class GameIndexer {
 public:
  GameIndexer(Hash::Type hash_type, const dat2pb::RomDat &dat);

  rhutil::Status AddFile(absl::string_view path, const Hash &hash);

  rhutil::StatusOr<GameIndex> GetIndex();

 private:
  Hash::Type hash_type_;

  GameIndex index_;
  absl::flat_hash_map<const dat2pb::RomDat::Game*,
                      GameIndex::Game*>
      indexed_games_;
  absl::flat_hash_map<std::pair<const dat2pb::RomDat::Game*,
                                const dat2pb::RomDat::Game::Rom*>,
                      GameIndex::Game::Rom*>
      indexed_roms_;

  const absl::flat_hash_map<
    Hash,
    std::vector<std::pair<const dat2pb::RomDat::Game*,
                          const dat2pb::RomDat::Game::Rom*>>>
      roms_by_hash_;
};

}  // namespace roman

#endif  // ROMAN_GAME_INDEXER_H_
