#include "roman/subcommands/subcommands.h"

#include <utility>

namespace roman {

using ::rhutil::NotFoundErrorBuilder;

SubCommands *SubCommands::Instance() {
  static SubCommands *instance = []() { return new SubCommands(); }();
  return instance;
}

void SubCommands::Add(std::string_view name, Handler handler) {
  absl::MutexLock lock(&handlers_mu_);
  handlers_.emplace(std::string(name), std::move(handler));
}

rhutil::Status SubCommands::Call(absl::Span<std::string_view> args) const {
  const Handler *handler = nullptr;
  {
    absl::ReaderMutexLock lock(&handlers_mu_);
    if (handlers_.find(args[0]) == handlers_.end()) {
      return NotFoundErrorBuilder() << "Unknown subcommand " << args[0];
    }
    handler = &handlers_.at(args[0]);
  }
  return (*handler)(args);
}

}  // namespace roman
