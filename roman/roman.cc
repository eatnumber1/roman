#include <utility>
#include <vector>
#include <string>
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
#include <stdlib.h>
#include <string_view>

#include "absl/types/span.h"
#include "roman/subcommands/subcommands.h"
#include "rhutil/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/match.h"
#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/flags/usage_config.h"
#include "rhutil/module_init.h"

namespace roman {

using ::rhutil::Status;
using ::rhutil::InvalidArgumentError;

Status Main(absl::Span<std::string_view> args) {
  return SubCommands::Instance()->Call(args);
}

}  // namespace roman

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage(
      absl::StrCat(
        "Manipulate CD images.\nUsage: ", getprogname(), " subcommand [args]"));
  absl::FlagsUsageConfig config;
  config.contains_help_flags = [](std::string_view path) {
    return absl::StartsWith(path, "roman/");
  };
  absl::SetFlagsUsageConfig(std::move(config));
  std::vector<char*> cargs = absl::ParseCommandLine(argc, argv);

  rhutil::ModuleInit::InitializeAll();

  std::vector<std::string_view> args;
  args.reserve(argc);
  for (char *arg : cargs) args.emplace_back(arg);

  if (args.size() < 2) {
    std::cerr << absl::ProgramUsageMessage() << std::endl;
    return static_cast<int>(rhutil::StatusCode::kInvalidArgument);
  }

  if (auto err = roman::Main(absl::MakeSpan(args).subspan(1)); !err.ok()) {
    std::cerr << err << std::endl;
    return static_cast<int>(err.code());
  }
  return EXIT_SUCCESS;
}
