#include "cdmanip/cdmap/mirage/cdm_writer.h"

#include <memory>
#include <string>
#include <utility>

#include <glib.h>
#include <glib/gprintf.h>

#include <FLAC++/metadata.h>

#include "absl/types/span.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/types/span.h"
#include "cdmanip/util/strings.h"
#include "cdmanip/util/errors.h"
#include "cdmanip/util/glib.h"
#include "cdmanip/cdmap/cdmap.pb.h"
#include "cdmanip/cdmap/mirage/flac_filter.h"

#include "mirage/stream.h"
#include "mirage/context.h"

#include "mirage/contextual.h"
#include "mirage/disc.h"
#include "mirage/writer.h"
#include "mirage/fragment.h"
#include "mirage/stream.h"
#include "mirage/language.h"
#include "mirage/session.h"
#include "mirage/track.h"
#include "mirage/debug.h"
#include "mirage/index.h"

using namespace ::cdmanip;

namespace {

absl::string_view Basename(absl::string_view path) {
  std::vector<absl::string_view> file_parts = absl::StrSplit(path, "/");
  return file_parts[file_parts.size() - 1];
}

absl::string_view Dirname(absl::string_view path) {
  std::vector<absl::string_view> file_parts = absl::StrSplit(path, "/");
  file_parts.pop_back();
  return absl::StrJoin(file_parts, "/");
}

}  // namespace

struct MirageWriterCdmPrivate {
 public:
  MirageWriterCdmPrivate(MirageWriterCdm *cdm)
      : cdm_(cdm) {
    mirage_writer_generate_info(MIRAGE_WRITER(cdm_), "WRITER-CDM",
                                "CDMap Image Writer");
  }

  void Dispose() {}

  bool OpenImage(MirageDisc *disc, GError **error) {
    absl::Span<char *> filenames =
        SpanFromNullTerm(mirage_disc_get_filenames(disc));
    if (filenames.size() != 1) {
      g_set_error(
          error, CDMANIP_ERROR, cdmanip::ERR_USAGE,
          "Invalid number of filenames provided. Expected 1, got %d.",
          static_cast<int>(filenames.size()));
      return false;
    }
    image_filename_ = filenames[0];

    return true;
  }

  MirageFragment *CreateFragment(
      MirageTrack *track, MirageFragmentRole role, GError **error) {
    auto fragment = NewGObject<MirageFragment>(MIRAGE_TYPE_FRAGMENT);

    std::string file_suffix = "bin";
    int session_num = mirage_track_layout_get_session_number(track);
    int track_num = mirage_track_layout_get_track_number(track);
    std::vector<std::string> filename_parts = {image_filename_};
    if (session_num != 1) {
      filename_parts.emplace_back(absl::StrCat("s", session_num));
    }
    filename_parts.emplace_back(absl::StrCat("t", track_num));
    if (role == MIRAGE_FRAGMENT_PREGAP) {
      filename_parts.emplace_back("pregap");
    }

    MirageSectorType sector_type = mirage_track_get_sector_type(track);
    switch (sector_type) {
      case MIRAGE_SECTOR_AUDIO:
        mirage_fragment_main_data_set_size(fragment.Get(), 2352);
        mirage_fragment_main_data_set_format(fragment.Get(),
                                             MIRAGE_MAIN_DATA_FORMAT_AUDIO);
        file_suffix = "flac";
        break;
      // TODO(eatnumber1): Check that this is the behavior that we want.
      case MIRAGE_SECTOR_MODE1:
      case MIRAGE_SECTOR_MODE2_FORM1:
        mirage_fragment_main_data_set_size(fragment.Get(), 2048);
        mirage_fragment_main_data_set_format(
            fragment.Get(), MIRAGE_MAIN_DATA_FORMAT_DATA);
        break;
      // TODO(eatnumber1): Check that this is the behavior that we want.
      case MIRAGE_SECTOR_MODE2:
      case MIRAGE_SECTOR_MODE2_MIXED:
        mirage_fragment_main_data_set_size(fragment.Get(), 2048);
        mirage_fragment_main_data_set_format(
            fragment.Get(), MIRAGE_MAIN_DATA_FORMAT_DATA);
        break;
      default:
        g_set_error(
            error, CDMANIP_ERROR, cdmanip::ERR_UNIMPL,
            "Sector type %d not supported.", sector_type);
        return nullptr;
    }
    filename_parts.emplace_back(file_suffix);

    std::string out_file = absl::StrJoin(filename_parts, ".");
    MIRAGE_DEBUG(cdm_, MIRAGE_DEBUG_WRITER, "Creating stream to write to %s",
                 out_file.c_str());

    auto stream = WrapGObject<MirageStream>(
        mirage_contextual_create_output_stream(
          MIRAGE_CONTEXTUAL(cdm_), out_file.c_str(),
          /*filter_chain=*/nullptr, error));
    if (!stream) return nullptr;

    if (sector_type == MIRAGE_SECTOR_AUDIO) {
      auto outer_stream =
        NewGObject<MirageFilterStreamFlacfile>(MIRAGE_TYPE_FILTER_STREAM_FLACFILE);
      mirage_contextual_set_context(
          MIRAGE_CONTEXTUAL(outer_stream.Get()), context());
      if (!mirage_filter_stream_open(
            MIRAGE_FILTER_STREAM(outer_stream.Get()), stream.Get(), /*writable=*/true, error)) {
        return nullptr;
      }
      stream.Reset(MIRAGE_STREAM(outer_stream.Release()));
    }

    mirage_fragment_main_data_set_stream(fragment.Get(), stream.Get());

    mirage_fragment_subchannel_data_set_format(
        fragment.Get(), MIRAGE_SUBCHANNEL_DATA_FORMAT_EXTERNAL);

    return fragment.Release();
  }

  bool WriteCDM(const CompactDiscMap &map, GError **error) {
    std::string cdmap_filename = absl::StrCat(image_filename_, ".cdm");
    auto stream = WrapGObject<MirageStream>(
        mirage_contextual_create_output_stream(
          MIRAGE_CONTEXTUAL(cdm_),
          cdmap_filename.c_str(),
          /*filter_chain=*/nullptr, error));
    if (!stream) return false;

    std::string map_buf;
    if (!map.SerializeToString(&map_buf)) {
        g_set_error(
            error, CDMANIP_ERROR, cdmanip::ERR_UNKNOWN,
            "Unknown error serializing CompactDiscMap:\n%s\n",
            map.DebugString().c_str());
        return false;
    }

    if (mirage_stream_write(stream.Get(), map_buf.c_str(), map_buf.size(), error) != map_buf.size()) {
      g_set_error(
          error, CDMANIP_ERROR, cdmanip::ERR_UNKNOWN,
          "Unknown error writing CompactDiscMap to stream.");
      return false;
    }

    MIRAGE_DEBUG(
        cdm_, MIRAGE_DEBUG_WRITER,
        "Wrote a cdmap to %s", cdmap_filename.c_str());

    return true;
  }

  bool FinalizeImage(MirageDisc *mirage_disc, GError **error) {
    CompactDiscMap map_disc;

    if (!WriteMediumType(mirage_disc, &map_disc, error)) return false;
    if (!WriteSessions(mirage_disc, &map_disc, error)) return false;

    if (!WriteFLACMetadata(mirage_disc, error)) return false;

    if (!WriteCDM(map_disc, error)) return false;

    return true;
  }

 private:
  template <typename E>
  class EnumerateCallback {
   public:
    EnumerateCallback(std::function<bool(E*)> callback)
      : callback_(std::move(callback)) {}

    typedef gboolean (FuncType)(E*, void*);

    FuncType *Func() {
      return reinterpret_cast<FuncType*>(&DoCallback);
    }

    void *Ptr() {
      return static_cast<void*>(this);
    }

   private:
    static gboolean DoCallback(E *ep, EnumerateCallback *ecb) {
      return ecb->callback_(ep);
    }

    std::function<bool(E*)> callback_;
  };

  bool WriteFLACMetadata(MirageTrack *track, GError **error) {
    g_assert(mirage_track_get_number_of_fragments(track) == 2);

    GObjectPtr<MirageFragment> pregap_frag(
        mirage_track_get_fragment_by_index(track, 0, error));
    if (!pregap_frag) return false;
    absl::string_view pregap_file =
      mirage_fragment_main_data_get_filename(pregap_frag.Get());

    GObjectPtr<MirageFragment> data_frag(
        mirage_track_get_fragment_by_index(track, 1, error));
    if (!data_frag) return false;
    absl::string_view data_file =
      mirage_fragment_main_data_get_filename(data_frag.Get());

    // Don't touch non-FLAC files.
    if (!absl::EndsWith(data_file, ".flac")) return true;

    MIRAGE_DEBUG(cdm_, MIRAGE_DEBUG_WRITER, "Writing metadata to %s",
                 data_file.data());

    FLAC__StreamMetadata *metadata = FLAC__metadata_object_new(FLAC__METADATA_TYPE_CUESHEET);
    if (!metadata) {
      g_set_error(
          error, CDMANIP_ERROR, cdmanip::ERR_FLAC,
          "Failed to create FLAC__StreamMetadata");
      return false;
    }

    // cuesheet takes ownership of metadata.
    FLAC::Metadata::CueSheet cuesheet(metadata, /*copy=*/false);

    FLAC::Metadata::CueSheet::Track cuetrack;
    if (!cuetrack.is_valid()) {
      g_set_error(
          error, CDMANIP_ERROR, cdmanip::ERR_FLAC,
          "Failed to create FLAC::Metadata::CueSheet::Track");
      return false;
    }

    EnumerateCallback<MirageIndex> icb([&](MirageIndex *index) {
      FLAC__StreamMetadata_CueSheet_Index cueindex;
      cueindex.number = mirage_index_get_number(index);
      constexpr const int kSamplesPerSector = 588;
      cueindex.offset = mirage_index_get_address(index) * kSamplesPerSector;
      cuetrack.set_index(cueindex.number, cueindex);
      return true;
    });
    if (!mirage_track_enumerate_indices(track, icb.Func(), icb.Ptr())) {
      return false;
    }

    cuetrack.set_number(mirage_track_layout_get_track_number(track));
    cuetrack.set_offset(0);
    // TODO(eatnumber1): Write MCN

    if (const char *isrc = mirage_track_get_isrc(track)) {
      cuetrack.set_isrc(isrc);
    }

    int flags = mirage_track_get_flags(track);
    if (flags & MIRAGE_TRACK_FLAG_PREEMPHASIS) cuetrack.set_pre_emphasis(true);

    // According to https://xiph.org/flac/api/structFLAC____StreamMetadata__CueSheet__Track.html#a848575fc7a7292867ce76a9b3705f6e7
    constexpr int TRACK_TYPE_AUDIO = 1;
    cuetrack.set_type(TRACK_TYPE_AUDIO);

    if (!cuesheet.insert_track(/*track_num=*/0, cuetrack)) {
      g_set_error(
          error, CDMANIP_ERROR, cdmanip::ERR_FLAC,
          "Failed to set track in cuesheet");
      return false;
    }

    const char *explanation = nullptr;
    if (!cuesheet.is_legal(/*check_cd_da_subset=*/false, &explanation)) {
      g_set_error(
          error, CDMANIP_ERROR, cdmanip::ERR_FLAC,
          "Generated cuesheet is illegal: %s", explanation);
      return false;
    }

    FLAC::Metadata::SimpleIterator iter;
    if (!iter.is_valid()) {
      g_set_error(
          error, CDMANIP_ERROR, cdmanip::ERR_FLAC,
          "Failed to create FLAC::Metadata::SimpleIterator");
      return false;
    }

    if (!iter.init(data_file.data(), /*read_only=*/false,
                   /*preserve_file_stats=*/false)) {
      g_set_error(
          error, CDMANIP_ERROR, cdmanip::ERR_FLAC,
          "Failed to initialize FLAC metadata writer: %s",
          iter.status().as_cstring());
      return false;
    }

    if (!iter.insert_block_after(&cuesheet, /*use_padding=*/true)) {
      g_set_error(
          error, CDMANIP_ERROR, cdmanip::ERR_FLAC,
          "Failed to insert FLAC metadata block: %s",
          FLAC__Metadata_SimpleIteratorStatusString[iter.status()]);
      return false;
    }

    return true;
  }

  bool WriteFLACMetadata(MirageDisc *disc, GError **error) {
    EnumerateCallback<MirageSession> scb([&](MirageSession *session) {
      MIRAGE_DEBUG(cdm_, MIRAGE_DEBUG_WRITER, "Writing FLAC metadata for session %d",
                   mirage_session_layout_get_session_number(session));
      // There's some sort of bug in mirage_session_enumerate_tracks which
      // iterates an additional invalid track at the beginning and end.
      for (int i = 0; i < mirage_session_get_number_of_tracks(session); i++) {
        GObjectPtr<MirageTrack> track(
            mirage_session_get_track_by_index(session, i, error));
        if (!track) return false;
        if (!WriteFLACMetadata(track.Get(), error)) return false;
      }

      return true;
    });
    if (!mirage_disc_enumerate_sessions(disc, scb.Func(), scb.Ptr())) {
      return false;
    }
    return true;
  }

  bool GetLanguagePackData(
      MirageLanguage *language, MirageLanguagePackType pack_type,
      absl::string_view *data, GError **error) {
    gint length = -1;
    const guint8 *buf = nullptr;
    if (!mirage_language_get_pack_data(language, pack_type, &buf, &length,
                                       error)) {
      return false;
    }
    // TODO(eatnumber1): Why do I need these casts?
    *data = absl::string_view(reinterpret_cast<const char *>(buf), length);
    return true;
  }

  CompactDiscMap::Language CreateLanguage(MirageLanguage *mirage_language,
                                          GError **error) {
    CompactDiscMap::Language map_language;

    map_language.set_code(mirage_language_get_code(mirage_language));

    absl::string_view data;
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_TITLE,
                             &data, error)) {
      return {};
    }
    map_language.set_title(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_PERFORMER,
                             &data, error)) {
      return {};
    }
    map_language.set_performer(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_SONGWRITER,
                             &data, error)) {
      return {};
    }
    map_language.set_songwriter(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_COMPOSER,
                             &data, error)) {
      return {};
    }
    map_language.set_composer(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_ARRANGER,
                             &data, error)) {
      return {};
    }
    map_language.set_arranger(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_MESSAGE,
                             &data, error)) {
      return {};
    }
    map_language.set_message(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_DISC_ID,
                             &data, error)) {
      return {};
    }
    map_language.set_disc_id(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_GENRE,
                             &data, error)) {
      return {};
    }
    map_language.set_genre(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_TOC,
                             &data, error)) {
      return {};
    }
    map_language.set_toc(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_TOC2,
                             &data, error)) {
      return {};
    }
    map_language.set_toc2(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_RES_8A,
                             &data, error)) {
      return {};
    }
    map_language.set_reserved_8a(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_RES_8B,
                             &data, error)) {
      return {};
    }
    map_language.set_reserved_8b(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_RES_8C,
                             &data, error)) {
      return {};
    }
    map_language.set_reserved_8c(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_CLOSED_INFO,
                             &data, error)) {
      return {};
    }
    map_language.set_closed_info(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_UPC_ISRC,
                             &data, error)) {
      return {};
    }
    map_language.set_upc_isrc(std::string(data));
    if (!GetLanguagePackData(mirage_language, MIRAGE_LANGUAGE_PACK_SIZE,
                             &data, error)) {
      return {};
    }
    map_language.set_size(std::string(data));

    return map_language;
  }

  bool WriteLanguages(MirageTrack *mirage_track,
                      CompactDiscMap::Track *map_track, GError **error) {
    int num_languages = mirage_track_get_number_of_languages(mirage_track);
    for (int i = 0; i < num_languages; i++) {
      auto mirage_language =
          WrapGObject<MirageLanguage>(mirage_track_get_language_by_index(
                mirage_track, i, error));
      GError *err = nullptr;
      CompactDiscMap::Language map_language =
          CreateLanguage(mirage_language.Get(), &err);
      if (err != nullptr) {
        g_propagate_error(error, err);
        return false;
      }
      *map_track->add_language() = std::move(map_language);
    }
    return true;
  }

  bool WriteLanguages(MirageSession *mirage_session,
                      CompactDiscMap::Session *map_session, GError **error) {
    int num_languages = mirage_session_get_number_of_languages(mirage_session);
    for (int i = 0; i < num_languages; i++) {
      auto mirage_language = WrapGObject<MirageLanguage>(
          mirage_session_get_language_by_index(mirage_session, i, error));
      GError *err = nullptr;
      CompactDiscMap::Language map_language =
          CreateLanguage(mirage_language.Get(), &err);
      if (err != nullptr) {
        g_propagate_error(error, err);
        return false;
      }
      *map_session->add_language() = std::move(map_language);
    }
    return true;
  }


  bool WriteFlags(MirageTrack *mirage_track, CompactDiscMap::Track *map_track,
                  GError **error) {
    int flags = mirage_track_get_flags(mirage_track);

    using Track = ::cdmanip::CompactDiscMap::Track;
    if (flags & MIRAGE_TRACK_FLAG_FOURCHANNEL) {
      map_track->add_flag(Track::FLAG_4CH);
    }
    if (flags & MIRAGE_TRACK_FLAG_COPYPERMITTED) {
      map_track->add_flag(Track::FLAG_DCP);
    }
    if (flags & MIRAGE_TRACK_FLAG_PREEMPHASIS) {
      map_track->add_flag(Track::FLAG_PRE);
    }

    return true;
  }

  bool WriteFile(MirageTrack *mirage_track,
                 CompactDiscMap::Track *map_track, GError **error) {
    int num_fragments = mirage_track_get_number_of_fragments(mirage_track);
    if (num_fragments != 2) {
        g_set_error(
            error, CDMANIP_ERROR, cdmanip::ERR_UNIMPL,
            "Found %d fragments in track %d, which is unsupported.",
            num_fragments, mirage_track_layout_get_track_number(mirage_track));
        return {};
    }
    GObjectPtr<MirageFragment> pregap_frag(
        mirage_track_get_fragment_by_index(mirage_track, 0, error));
    if (!pregap_frag) return false;
    GObjectPtr<MirageFragment> data_frag(
        mirage_track_get_fragment_by_index(mirage_track, 1, error));
    if (!data_frag) return false;

    map_track->set_file(std::string(Basename(
            mirage_fragment_main_data_get_filename(data_frag.Get()))));
    map_track->set_pregap_file(std::string(Basename(
            mirage_fragment_main_data_get_filename(pregap_frag.Get()))));

    return true;
  }

  bool WriteTrackType(MirageTrack *mirage_track,
                      CompactDiscMap::Track *map_track, GError **error) {
    MirageSectorType sector_type = mirage_track_get_sector_type(mirage_track);
    switch (sector_type) {
      case MIRAGE_SECTOR_MODE0:
        map_track->set_type(CompactDiscMap::Track::TYPE_UNKNOWN);
        break;
      case MIRAGE_SECTOR_AUDIO:
        map_track->set_type(CompactDiscMap::Track::TYPE_AUDIO);
        break;
      case MIRAGE_SECTOR_MODE1:
        map_track->set_type(CompactDiscMap::Track::TYPE_MODE1_2048);
        break;
      case MIRAGE_SECTOR_MODE2_FORM1:
        map_track->set_type(CompactDiscMap::Track::TYPE_MODE2_2048);
        break;
      case MIRAGE_SECTOR_MODE2_FORM2:
        // TODO(eatnumber1): Check this
        map_track->set_type(CompactDiscMap::Track::TYPE_MODE2_2324);
        break;
      case MIRAGE_SECTOR_MODE2_MIXED:
        // TODO(eatnumber1): Check this
        map_track->set_type(CompactDiscMap::Track::TYPE_MODE2_2336);
        break;
      default:
        g_set_error(
            error, CDMANIP_ERROR, cdmanip::ERR_UNIMPL,
            "Track type %d not supported.", sector_type);
        return false;
    }
    return true;
  }

  bool WriteIndices(MirageTrack *mirage_track, CompactDiscMap::Track *map_track,
                    GError **error) {
    int num_indices = mirage_track_get_number_of_indices(mirage_track);
    for (int i = 0; i < num_indices; i++) {
      auto mirage_index =
          WrapGObject<MirageIndex>(
              mirage_track_get_index_by_number(mirage_track, i, error));
      if (!mirage_index) return false;

      int start_address = mirage_index_get_address(mirage_index.Get());

      CompactDiscMap::Track::Index &map_index = *map_track->add_index();
      map_index.set_number(mirage_index_get_number(mirage_index.Get()));
      map_index.set_offset(start_address);

      if (i + 1 == num_indices) {
        // The last index consumes the rest of the file.
        int track_len = mirage_track_layout_get_length(mirage_track);
        map_index.set_length(track_len - start_address);
      } else {
        auto next_index =
            WrapGObject<MirageIndex>(
                mirage_track_get_index_by_number(mirage_track, i + 1, error));
        int next_start_address = mirage_index_get_address(mirage_index.Get());
        map_index.set_length(next_start_address - start_address);
      }
    }

    return true;
  }

  CompactDiscMap::Track CreateTrack(MirageTrack *mirage_track, GError **error) {
    CompactDiscMap::Track map_track;

    map_track.set_number(mirage_track_layout_get_track_number(mirage_track));

    if (const char *isrc = mirage_track_get_isrc(mirage_track)) {
      map_track.set_isrc(isrc);
    }

    if (!WriteFile(mirage_track, &map_track, error)) return {};
    if (!WriteLanguages(mirage_track, &map_track, error)) return {};
    if (!WriteTrackType(mirage_track, &map_track, error)) return {};
    if (!WriteFlags(mirage_track, &map_track, error)) return {};
    if (!WriteIndices(mirage_track, &map_track, error)) return {};

    return map_track;
  }

  bool WriteTracks(MirageSession *mirage_session,
                   CompactDiscMap::Session *map_session, GError **error) {
    int num_tracks = mirage_session_get_number_of_tracks(mirage_session);
    for (int i = 0; i < num_tracks; i++) {
      auto mirage_track = WrapGObject<MirageTrack>(
          mirage_session_get_track_by_index(mirage_session, i, error));
      GError *err = nullptr;
      CompactDiscMap::Track map_track = CreateTrack(mirage_track.Get(), &err);
      if (err != nullptr) {
        g_propagate_error(error, err);
        return false;
      }
      *map_session->add_track() = std::move(map_track);
    }
    return true;
  }

  bool WriteSessionType(MirageSession *mirage_session,
                        CompactDiscMap::Session *map_session, GError **error) {
    MirageSessionType session_type =
      mirage_session_get_session_type(mirage_session);
    using Session = ::cdmanip::CompactDiscMap::Session;
    switch (session_type) {
      case MIRAGE_SESSION_CDDA:
        map_session->set_type(Session::TYPE_CDDA);
        break;
      case MIRAGE_SESSION_CDROM:
        map_session->set_type(Session::TYPE_CDROM);
        break;
      case MIRAGE_SESSION_CDI:
        map_session->set_type(Session::TYPE_CDI);
        break;
      case MIRAGE_SESSION_CDROM_XA:
        map_session->set_type(Session::TYPE_CDROM_XA);
        break;
      default:
        g_set_error(
            error, CDMANIP_ERROR, cdmanip::ERR_UNIMPL,
            "Session type %d not supported.", session_type);
        return false;
    }
    return true;
  }

  CompactDiscMap::Session CreateSession(MirageSession *mirage_session,
                                        GError **error) {
    CompactDiscMap::Session map_session;

    if (const char *mcn = mirage_session_get_mcn(mirage_session)) {
      map_session.set_mcn(mcn);
    }

    if (!WriteSessionType(mirage_session, &map_session, error)) return {};
    if (!WriteLanguages(mirage_session, &map_session, error)) return {};
    if (!WriteTracks(mirage_session, &map_session, error)) return {};

    return map_session;
  }

  bool WriteMediumType(MirageDisc *mirage_disc, CompactDiscMap *map_disc, GError **error) {
    MirageMediumType medium = mirage_disc_get_medium_type(mirage_disc);
    switch (medium) {
      case MIRAGE_MEDIUM_CD:
        map_disc->set_medium(CompactDiscMap::MEDIUM_CD);
        break;
      default:
        g_set_error(
            error, CDMANIP_ERROR, cdmanip::ERR_UNIMPL,
            "Disc type %d not supported.", medium);
        return false;
    }
    return true;
  }

  bool WriteSessions(MirageDisc *mirage_disc, CompactDiscMap *map_disc, GError **error) {
    for (int i = 0; i < mirage_disc_get_number_of_sessions(mirage_disc); i++) {
      auto mirage_session =
          WrapGObject<MirageSession>(
              mirage_disc_get_session_by_index(mirage_disc, i, error));
      GError *err = nullptr;
      CompactDiscMap::Session map_session = CreateSession(mirage_session.Get(), &err);
      if (err != nullptr) {
        g_propagate_error(error, err);
        return false;
      }
      *map_disc->add_session() = std::move(map_session);
    }
    return true;
  }

  MirageContext *context() {
    return mirage_contextual_get_context(MIRAGE_CONTEXTUAL(cdm_));
  }

  MirageWriterCdm *cdm_;
  std::string image_filename_;
};

G_DEFINE_DYNAMIC_TYPE_EXTENDED(
    MirageWriterCdm, mirage_writer_cdm, MIRAGE_TYPE_WRITER, 0,
    G_ADD_PRIVATE_DYNAMIC(MirageWriterCdm))

void mirage_writer_cdm_type_register(GTypeModule *type_module) {
  mirage_writer_cdm_register_type(type_module);
}

static void mirage_writer_cdm_init(MirageWriterCdm *self) {
  self->priv = static_cast<MirageWriterCdmPrivate *>(mirage_writer_cdm_get_instance_private(self));
  new (self->priv) MirageWriterCdmPrivate(self);
}

static void mirage_writer_cdm_class_init(MirageWriterCdmClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  MirageWriterClass *writer_class = MIRAGE_WRITER_CLASS(klass);

  gobject_class->dispose = +[](GObject *gobject) {
    MIRAGE_WRITER_CDM(gobject)->priv->Dispose();
    return G_OBJECT_CLASS(mirage_writer_cdm_parent_class)->dispose(gobject);
  };
  gobject_class->finalize = +[](GObject *gobject) {
    MIRAGE_WRITER_CDM(gobject)->priv->~MirageWriterCdmPrivate();
    return G_OBJECT_CLASS(mirage_writer_cdm_parent_class)->finalize(gobject);
  };

  writer_class->open_image = +[](MirageWriter *self, MirageDisc *disc,
                                 GError **error) -> gboolean {
    return MIRAGE_WRITER_CDM(self)->priv->OpenImage(disc, error);
  };
  writer_class->create_fragment = +[](
      MirageWriter *self, MirageTrack *track, MirageFragmentRole role,
      GError **error) {
    return MIRAGE_WRITER_CDM(self)->priv->CreateFragment(track, role, error);
  };
  writer_class->finalize_image = +[](MirageWriter *self, MirageDisc *disc,
                                     GError **error) -> gboolean {
    return MIRAGE_WRITER_CDM(self)->priv->FinalizeImage(disc, error);
  };
}

static void mirage_writer_cdm_class_finalize(MirageWriterCdmClass *klass) {}
