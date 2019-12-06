#include <string>
#include <cstdlib>
#include <cstdio>
#include <ostream>
#include <iostream>
#include <stdlib.h>

#include "absl/algorithm/container.h"
#include "absl/strings/str_split.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_join.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "absl/flags/flag.h"
#include "roman/subcommands/subcommands.h"
#include "roman/util/errors.h"
#include "roman/util/glib.h"
#include "roman/util/cleanup.h"
#include "mirage/mirage.h"
#include "roman/cdmap/cdmap.pb.h"

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

std::vector<absl::string_view> ToStringViews(char **strings) {
  std::vector<absl::string_view> ret;
  for (char **e = strings; *e != nullptr; e++) ret.emplace_back(*e);
  return ret;
}

GObjectPtr<MirageContext> CreateContext(GError **error) {
  auto ctx = NewGObject<MirageContext>(MIRAGE_TYPE_CONTEXT, nullptr);

  mirage_context_set_debug_domain(ctx.get(), "CDMANIP");
  mirage_context_set_debug_name(ctx.get(), "roman");

  GError *err = nullptr;
  absl::Span<const MirageDebugMaskInfo> supported_masks = MirageDebugMasks(&err);
  if (err != nullptr) {
    g_propagate_error(error, err);
    return nullptr;
  }

  std::vector<std::string> wanted_masks =
      absl::GetFlag(FLAGS_mirage_debug_masks);

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

  mirage_context_set_debug_mask(ctx.get(), masks);

  return ctx;
}

bool ImportFragment(
    MirageFragment *fragment, absl::string_view data_file,
    absl::string_view subchannel_file, GError **error) {

#if 0
  int length_sectors = mirage_fragment_get_length(fragment);

gboolean
mirage_fragment_read_main_data (MirageFragment *self,
                                gint address,
                                guint8 **buffer,
                                gint *length,
                                GError **error);

gboolean
mirage_fragment_read_subchannel_data (MirageFragment *self,
                                      gint address,
                                      guint8 **buffer,
                                      gint *length,
                                      GError **error);
#endif

  return true;
}

bool ImportTrack(MirageTrack *in_track,
                 CompactDiscMap::Track *out_track, GError **error) {
  int track_number = mirage_track_layout_get_track_number(in_track);

  out_track->set_number(track_number);

  string data_filename = absl::StrFormat("Track%02d.bin", track_number);
  string subchannel_filename = absl::StrFormat("Track%02d.sub", track_number);
  for (int i = 0; i < mirage_track_get_number_of_fragments(in_track); i++) {
    MirageFragment *fragment = mirage_track_get_fragment_by_index(in_track, i, error);
    if (!fragment) return false;
    if (ImportFragment(fragment, data_file, subchannel_file, error)) return false;
  }

  return true;
}

bool ImportSession(MirageSession *in_session,
                   CompactDiscMap::Session *out_session, GError **error) {
  using Session = ::roman::CompactDiscMap::Session;

  if (const char *mcn = mirage_session_get_mcn(in_session)) {
    out_session->set_mcn(mcn);
  }

  switch (mirage_session_get_session_type(in_session)) {
    case MIRAGE_SESSION_CDDA:
      out_session->set_type(Session::TYPE_CDDA);
    case MIRAGE_SESSION_CDROM:
      out_session->set_type(Session::TYPE_CDROM);
    case MIRAGE_SESSION_CDI:
      out_session->set_type(Session::TYPE_CDI);
    case MIRAGE_SESSION_CDROM_XA:
      out_session->set_type(Session::TYPE_CDROM_XA);
    default:
      out_session->set_type(Session::TYPE_UNKNOWN);
  }

  for (int i = 0; i < mirage_session_get_number_of_tracks(in_session); i++) {
    MirageTrack *track = mirage_session_get_track_by_index(in_session, i, error);
    if (!track) return false;
    if (ImportTrack(track, out_session->add_track(), error)) return false;
  }

  return true;
}

bool Import(MirageDisc *disc, GError **error) {
  CompactDiscMap map;

  for (int i = 0; i < mirage_disc_get_number_of_sessions(disc); i++) {
    MirageSession *session = mirage_disc_get_session_by_index(disc, i, error);
    if (!session) return false;
    if (ImportSession(session, map.add_session(), error)) return false;
  }

  absl::PrintF("Imported cdmap:\n%s\n", map.DebugString());

  return true;
}

}  // namespace

bool SubCommand::Import(
    absl::Span<absl::string_view> args,
    GError **error) {
  if (args.size() < 1) {
    g_set_error(
        error, CDMANIP_ERROR, roman::ERR_USAGE,
        "Usage: %s %s source", getprogname(), args[0].data());
    return false;
  }

  if (!mirage_initialize(error)) return false;
  Cleanup cleanup([]() {
      GError *err = nullptr;
      mirage_shutdown(&err);
      CHECK_OK(err);
  });

  GObjectPtr<MirageContext> ctx = CreateContext(error);
  if (!ctx) return false;

  std::vector<const char *> filenames;
  filenames.reserve(args.size());
  for (int i = 1; i < args.size(); i++) filenames.emplace_back(args[i].data());
  filenames.emplace_back(nullptr);

  GObjectPtr<MirageDisc> disc(mirage_context_load_image(
        ctx.get(), const_cast<char **>(filenames.data()), error));
  if (!disc) return false;

  return roman::Import(disc.get(), &map, error);
}

}  // namespace roman
