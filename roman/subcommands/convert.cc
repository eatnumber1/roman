#define G_LOG_DOMAIN "CDManip"

#include <string>
#include <cstdlib>
#include <cstdio>
#include <ostream>
#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <cerrno>

#include "absl/algorithm/container.h"
#include "absl/strings/str_split.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/container/flat_hash_set.h"
#include "absl/types/span.h"
#include "absl/flags/flag.h"
#include "roman/subcommands/subcommands.h"
#include "roman/util/errors.h"
#include "roman/util/glib.h"
#include "roman/util/cleanup.h"
#include "roman/cdmap/cdmap.pb.h"

#include "mirage/mirage.h"
#include "mirage/writer.h"

ABSL_FLAG(std::vector<std::string>, mirage_debug_masks, {},
          "Emit libmirage debug messages. Use the mask \"HELP\" to list valid "
          "masks.");

namespace roman {

namespace {

absl::Span<const MirageDebugMaskInfo> MirageDebugMasks(GError **error) {
  const MirageDebugMaskInfo *masks;
  int num_masks = -1;
  if (!mirage_get_supported_debug_masks(&masks, &num_masks, error)) return {};
  g_assert(num_masks >= 0);
  return {masks, static_cast<unsigned int>(num_masks)};
}

GObjectPtr<MirageContext> CreateContext(GError **error) {
  auto ctx = NewGObject<MirageContext>(MIRAGE_TYPE_CONTEXT, nullptr);

  mirage_context_set_debug_domain(ctx.Get(), G_LOG_DOMAIN);

  GError *err = nullptr;
  absl::Span<const MirageDebugMaskInfo> supported_masks = MirageDebugMasks(&err);
  if (err != nullptr) {
    g_propagate_error(error, err);
    return nullptr;
  }

  std::vector<std::string> wanted_masks =
      absl::GetFlag(FLAGS_mirage_debug_masks);

  if (!wanted_masks.empty()) {
    constexpr const char kDebugEnvVar[] = "G_MESSAGES_DEBUG";
    absl::string_view old_log_domains_str(std::getenv(kDebugEnvVar));
    absl::flat_hash_set<absl::string_view> log_domains =
        absl::StrSplit(old_log_domains_str, " ", absl::SkipWhitespace());
    log_domains.emplace(G_LOG_DOMAIN);
    std::string new_log_domains_str = absl::StrJoin(log_domains, " ");
    if (setenv(kDebugEnvVar, new_log_domains_str.c_str(), true) != 0) {
      g_set_error(error, CDMANIP_ERROR, roman::ERR_OS,
          "Failed to set environment variable %s: %s", kDebugEnvVar,
          std::strerror(errno));
      return nullptr;
    }
  }

  int masks = 0;
  for (const MirageDebugMaskInfo &mask : supported_masks) {
    auto it = absl::c_find(wanted_masks, mask.name);
    if (it == wanted_masks.end()) continue;
    wanted_masks.erase(it);
    masks |= mask.value;
  }

  if (!wanted_masks.empty()) {
    absl::FPrintF(stderr, "Unsupported mask(s): [%s]\n", absl::StrJoin(wanted_masks, ","));
    absl::FPrintF(stderr, "Supported masks are:\n");
    for (const MirageDebugMaskInfo &mask : supported_masks) {
      absl::FPrintF(stderr, "%s\n", mask.name);
    }
    std::exit(EXIT_SUCCESS);
  }

  mirage_context_set_debug_mask(ctx.Get(), masks);

  return ctx;
}

}  // namespace

bool SubCommand::Convert(
    absl::Span<absl::string_view> args,
    GError **error) {
  if (args.size() != 4) {
    g_set_error(
        error, CDMANIP_ERROR, roman::ERR_USAGE,
        "Usage: %s %s source dest_writer dest", getprogname(), args[0].data());
    return false;
  }
  absl::string_view source_file = args[1];
  absl::string_view writer_name = args[2];
  absl::string_view dest_file = args[3];

  if (!mirage_initialize(error)) return false;
  Cleanup cleanup([]() {
      GError *err = nullptr;
      mirage_shutdown(&err);
      CHECK_OK(err);
  });

  GObjectPtr<MirageContext> ctx = CreateContext(error);
  if (!ctx) return false;

  char *source_file_nullterm[2] = {
    const_cast<char*>(source_file.data()), nullptr
  };
  auto disc = WrapGObject<MirageDisc>(mirage_context_load_image(
        ctx.Get(), source_file_nullterm, error));
  if (!disc) return false;

  auto writer =
    WrapGObject<MirageWriter>(mirage_create_writer(writer_name.data(), error));
  if (!writer) return false;

  mirage_contextual_set_context(MIRAGE_CONTEXTUAL(writer.Get()), ctx.Get());

  // TODO(eatnumber1): Progress reporting.

  std::unique_ptr<GHashTable, GHashTableDestroy> parameters(
    g_hash_table_new(g_str_hash, g_str_equal));
  if (!mirage_writer_convert_image(writer.Get(), dest_file.data(), disc.Get(),
                                   parameters.get(), /*cancellable=*/nullptr,
                                   error)) {
    return false;
  }

  std::vector<const char *> filenames;
  filenames.reserve(args.size());
  for (int i = 1; i < args.size(); i++) filenames.emplace_back(args[i].data());
  filenames.emplace_back(nullptr);

  absl::PrintF("Converted image %s to %s using %s\n", source_file, dest_file,
               writer_name);
  return true;
}

}  // namespace roman
