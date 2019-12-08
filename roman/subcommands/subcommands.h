#ifndef ROMAN_SUBCOMMANDS_SUBCOMMANDS_H_
#define ROMAN_SUBCOMMANDS_SUBCOMMANDS_H_

#include <string_view>
#include <string>
#include <functional>

#include "absl/types/span.h"
#include "rhutil/status.h"
#include "absl/synchronization/mutex.h"
#include "absl/container/flat_hash_map.h"

namespace roman {

class SubCommands {
 public:
  static SubCommands *Instance();

  using Handler = std::function<rhutil::Status(absl::Span<std::string_view>)>;

  rhutil::Status Call(absl::Span<std::string_view> args) const;

  void Add(std::string_view name, Handler handler);

 private:
  SubCommands() = default;
  ~SubCommands() = delete;

  mutable absl::Mutex handlers_mu_;
  absl::flat_hash_map<std::string, Handler> handlers_ GUARDED_BY(handlers_mu_);
};

}  // namespace roman

#endif  // ROMAN_SUBCOMMANDS_SUBCOMMANDS_H_
