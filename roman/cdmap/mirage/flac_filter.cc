// TODO(eatnumber1): Add a param for padding.
#include "roman/cdmap/mirage/flac_filter.h"

#include <memory>
#include <string>
#include <utility>

#include <glib.h>
#include <glib/gprintf.h>

#include <FLAC++/encoder.h>

#include "absl/types/span.h"
#include "absl/container/fixed_array.h"
#include "absl/strings/string_view.h"
#include "absl/strings/str_format.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "absl/types/span.h"
#include "absl/types/optional.h"
#include "roman/util/strings.h"
#include "roman/util/errors.h"
#include "roman/util/glib.h"
#include "roman/cdmap/cdmap.pb.h"

#include "mirage/stream.h"
#include "mirage/context.h"
#include "mirage/filter-stream.h"

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

using namespace ::roman;

namespace {

class Encoder : public FLAC::Encoder::Stream {
 public:
  Encoder(MirageStream *output) : output_(output) {}

  ~Encoder() {
    if (last_error_) g_error_free(last_error_);
  }

  template <typename... Args>
  void PropagateError(GError **error, const absl::FormatSpec<Args...> &fmt,
                      Args... args) {
    FLAC__StreamEncoderState state = get_state();
    g_assert(state != FLAC__STREAM_ENCODER_OK);
    if (state == FLAC__STREAM_ENCODER_CLIENT_ERROR) {
      g_propagate_error(error, last_error_);
      last_error_ = nullptr;
    } else {
      g_set_error(
          error, ROMAN_ERROR, roman::ERR_FLAC,
          "%s: %s", absl::StrFormat(fmt, args...).c_str(),
          FLAC__StreamEncoderStateString[state]);
    }
  }

 protected:
  FLAC__StreamEncoderReadStatus read_callback(
      FLAC__byte buffer[], size_t *bytes) override {
    return FLAC__STREAM_ENCODER_READ_STATUS_UNSUPPORTED;
  }

  FLAC__StreamEncoderSeekStatus seek_callback(
      FLAC__uint64 absolute_byte_offset) override {
    if (!mirage_stream_seek(output_, absolute_byte_offset, G_SEEK_SET,
                            &last_error_)) {
      return FLAC__STREAM_ENCODER_SEEK_STATUS_ERROR;
    }
    return FLAC__STREAM_ENCODER_SEEK_STATUS_OK;
  }

  FLAC__StreamEncoderTellStatus tell_callback(
      FLAC__uint64 *absolute_byte_offset) override {
    *absolute_byte_offset = mirage_stream_tell(output_);
    return FLAC__STREAM_ENCODER_TELL_STATUS_OK;
  }

  FLAC__StreamEncoderWriteStatus write_callback(
      const FLAC__byte buffer[], std::size_t bytes, unsigned samples,
      unsigned current_frame) override {
    if (!mirage_stream_write(output_, buffer, bytes, &last_error_)) {
      return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    }
    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
  }

 private:
  GError *last_error_ = nullptr;
  MirageStream *output_;
};

}  // namespace

struct MirageFilterStreamFlacfilePrivate {
 public:
  MirageFilterStreamFlacfilePrivate(MirageFilterStreamFlacfile *flacfile)
      : flacfile_(flacfile) {
    mirage_filter_stream_generate_info(
        MIRAGE_FILTER_STREAM(flacfile_), "FILTER-FLAC",
        "FLAC File Filter", /*writable=*/true, /*num_types=*/1,
        "FLAC audio files (*.flac)", "audio/x-flac");
  }

  void Dispose() {
    if (encoder_) {
      if (!encoder_->finish()) {
        GError *err = nullptr;
        encoder_->PropagateError(&err, "Failed to finalize FLAC encoder");
        MIRAGE_DEBUG(
            flacfile_, MIRAGE_DEBUG_ERROR, "%s", err->message);
        g_error_free(err);
      }
      encoder_ = absl::nullopt;
    }
  }

  bool Open(MirageStream *stream, bool writable, GError **error) {
    if (!writable) {
      g_set_error(
          error, ROMAN_ERROR, roman::ERR_UNIMPL,
          "MirageFilterStreamFlacfile is write-only");
      return false;
    }

    encoder_.emplace(stream);

    bool ok = true;
    ok &= encoder_->set_verify(true);
    // TODO(eatnumber1): Make this configurable.
    constexpr const int kMaxCompression = 8;
    //ok &= encoder_->set_compression_level(kMaxCompression);
    ok &= encoder_->set_compression_level(0);
    ok &= encoder_->set_channels(2);
    ok &= encoder_->set_bits_per_sample(16);
    ok &= encoder_->set_sample_rate(44100);
    // TODO(eatnumber1): Write samples when done.
    ok &= encoder_->set_total_samples_estimate(0);
    g_assert(ok);

    FLAC__StreamEncoderInitStatus status = encoder_->init();
    if (status != FLAC__STREAM_ENCODER_INIT_STATUS_OK) {
      g_set_error(
          error, ROMAN_ERROR, roman::ERR_FLAC,
          "Error initializing FLAC library: %s",
          FLAC__StreamEncoderInitStatusString[status]);
      return false;
    }

    return true;
  }

  ssize_t Read(absl::Span<int16_t> buffer, GError **error) {
    g_assert(encoder_);
    g_set_error(
        error, ROMAN_ERROR, roman::ERR_UNIMPL,
        "Reading from MirageFilterStreamFlacfile unsupported.");
    return 0;
  }

  ssize_t Write(absl::Span<const int16_t> buffer, GError **error) {
    if (!md5_.Update(buffer.data(), error)) return 0;

    g_assert(encoder_);
    absl::FixedArray<int32_t> widened(buffer.size());
    for (std::size_t i = 0; i < buffer.size(); i++) widened[i] = buffer[i];

    constexpr const int kNumChannels = 2;
    std::size_t num_samples_in_one_channel = widened.size() / kNumChannels;
    if (!encoder_->process_interleaved(
          widened.data(), num_samples_in_one_channel)) {
      encoder_->PropagateError(
          error, "Failed to process %d samples", buffer.size());
      return 0;
    }

    MIRAGE_DEBUG(flacfile_, MIRAGE_DEBUG_STREAM, "Wrote %zd samples",
                 buffer.size());

    current_position_ += buffer.size() * sizeof(int16_t);
    return widened.size();
  }

  bool Seek(goffset offset, GSeekType type, GError **error) {
    g_assert(encoder_);
    if (current_position_ == offset) return true;
    g_set_error(
        error, ROMAN_ERROR, roman::ERR_UNIMPL,
        "Seeking to %llx within MirageFilterStreamFlacfile unsupported.",
        offset);
    return false;
  }

  goffset Tell() {
    g_assert(encoder_);
    return current_position_;
  }

 private:
  MirageFilterStreamFlacfile *flacfile_;
  absl::optional<Encoder> encoder_;
  std::size_t current_position_ = 0;
};

G_DEFINE_TYPE_WITH_PRIVATE(
    MirageFilterStreamFlacfile, mirage_filter_stream_flacfile,
    MIRAGE_TYPE_FILTER_STREAM)

static void mirage_filter_stream_flacfile_init(MirageFilterStreamFlacfile *self) {
  self->priv = static_cast<MirageFilterStreamFlacfilePrivate *>(mirage_filter_stream_flacfile_get_instance_private(self));
  new (self->priv) MirageFilterStreamFlacfilePrivate(self);
}

static void mirage_filter_stream_flacfile_class_init(MirageFilterStreamFlacfileClass *klass) {
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  MirageFilterStreamClass *filter_stream_class = MIRAGE_FILTER_STREAM_CLASS(klass);

  gobject_class->dispose = +[](GObject *gobject) {
    MIRAGE_FILTER_STREAM_FLACFILE(gobject)->priv->Dispose();
    return G_OBJECT_CLASS(mirage_filter_stream_flacfile_parent_class)->dispose(gobject);
  };
  gobject_class->finalize = +[](GObject *gobject) {
    MIRAGE_FILTER_STREAM_FLACFILE(gobject)->priv->~MirageFilterStreamFlacfilePrivate();
    return G_OBJECT_CLASS(mirage_filter_stream_flacfile_parent_class)->finalize(gobject);
  };

  filter_stream_class->open = +[](MirageFilterStream *self, MirageStream *stream, gboolean writable, GError **error) -> gboolean {
    return MIRAGE_FILTER_STREAM_FLACFILE(self)->priv->Open(stream, writable, error);
  };

  filter_stream_class->read = +[](MirageFilterStream *self, void *buffer, gsize count, GError **error) -> gssize {
    auto sp = absl::MakeSpan(
        static_cast<int16_t*>(buffer), count / sizeof(int16_t));
    ssize_t red = MIRAGE_FILTER_STREAM_FLACFILE(self)->priv->Read(sp, error);
    if (red < 0) return red;
    red *= sizeof(int16_t);
    return red;
  };
  filter_stream_class->write = +[](MirageFilterStream *self, const void *buffer, gsize count, GError **error) -> gssize {
    auto sp = absl::MakeConstSpan(
        static_cast<const int16_t*>(buffer), count / sizeof(int16_t));
    ssize_t written = MIRAGE_FILTER_STREAM_FLACFILE(self)->priv->Write(sp, error);
    if (written <= 0) return written;
    written *= sizeof(int16_t);
    g_assert(written == count);
    return written;
  };
  filter_stream_class->seek = +[](MirageFilterStream *self, goffset offset, GSeekType type, GError **error) -> gboolean {
    return MIRAGE_FILTER_STREAM_FLACFILE(self)->priv->Seek(offset, type, error);
  };
  filter_stream_class->tell = +[](MirageFilterStream *self) -> goffset {
    return MIRAGE_FILTER_STREAM_FLACFILE(self)->priv->Tell();
  };
}
