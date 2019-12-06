#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>

#include "absl/flags/flag.h"
#include "absl/strings/str_format.h"
#include "absl/types/span.h"
#include "roman/subcommands/subcommands.h"
#include "dat2pb/parser.h"
#include "dat2pb/romdat.pb.h"
#include "google/protobuf/io/zero_copy_stream_impl.h"
#include "google/protobuf/text_format.h"
#include "rhutil/curl/curl.h"
#include "rhutil/module_init.h"
#include "rhutil/status.h"
#include "zip.h"
#include "roman/common_flags.h"

ABSL_FLAG(bool, binary, false,
          "Output using the binary format instead of text");

namespace roman {
namespace {

using ::dat2pb::RomDat;
using ::rhutil::CurlURL;
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
using ::rhutil::UnknownError;
using ::google::protobuf::io::OstreamOutputStream;
using ::google::protobuf::TextFormat;

class ZipDiscard {
 public:
  void operator()(zip_t *archive) {
    zip_discard(archive);
  }
};

class ZipFClose {
 public:
  void operator()(zip_file_t *file) {
    if (zip_fclose(file) != 0) {
      std::abort();
    }
  }
};

class ZipError {
 public:
  ZipError() {
    zip_error_init(&error_);
  }

  ~ZipError() {
    zip_error_fini(&error_);
  }

  zip_error_t *ptr() { return &error_; }

 private:
  zip_error_t error_;
};

class ZipFileStreambuf : public std::streambuf {
 public:
   ZipFileStreambuf(zip_t *archive, zip_file_t *file)
     : archive_(archive), file_(file) {}

   Status status() const {
     return status_;
   }

 protected:
   int underflow() override {
      zip_int64_t nbytes = zip_fread(file_, &buf_, bufsiz_);
      if (nbytes == -1) {
        status_ = UnknownErrorBuilder()
            << "Zipped read failed: " << zip_strerror(archive_);
        setg(nullptr, nullptr, nullptr);
        return traits_type::eof();
      }
      setg(buf_, buf_, buf_ + nbytes);
      if (nbytes == 0) {
        return traits_type::eof();
      }
      return traits_type::to_int_type(buf_[0]);
   }

 private:
   zip_t *archive_;
   zip_file_t *file_;
   static constexpr int bufsiz_ = 4096;
   char buf_[bufsiz_];
   Status status_;
};

class RedumpFetcher {
 public:
  RedumpFetcher() : curl_(CurlEasyInit()) {
    if (absl::GetFlag(FLAGS_verbose)) {
      CHECK_OK(CurlEasySetopt(curl_.get(), CURLOPT_VERBOSE, true));
    }

    CHECK_OK(url_.SetURL("http://redump.org"));
    CHECK_OK(CurlEasySetopt(curl_.get(), CURLOPT_CURLU, url_.GetCURLU()));
  }

  StatusOr<RomDat> GetRomDat(std::string_view console) {
    ASSIGN_OR_RETURN(
        std::stringstream strm,
        Get(absl::StrFormat("/datfile/%s/serial,version", console)));

    ZipError error;
    zip_source_t *source = zip_source_buffer_create(
        strm.str().data(), strm.str().size(), /*freep=*/false, error.ptr());
    if (source == nullptr) {
      return UnknownErrorBuilder()
          << "Failed to create a zip source: "
          << zip_error_strerror(error.ptr());
    }

    // zip_open_from_source takes ownership of "source" unless it errors.
    std::unique_ptr<zip_t, ZipDiscard> archive(
        zip_open_from_source(source, ZIP_RDONLY, error.ptr()));
    if (archive == nullptr) {
      zip_source_free(source);
      return UnknownErrorBuilder()
          << "Failed to create a zip source: "
          << zip_error_strerror(error.ptr());
    }

    if (zip_int64_t entries = zip_get_num_entries(archive.get(), /*flags=*/0);
        entries != 1) {
      return UnknownErrorBuilder()
          << "Expected only one entry in zip archive. Found " << entries;
    }

    std::unique_ptr<zip_file_t, ZipFClose> file(
        zip_fopen_index(archive.get(), /*index=*/0, /*flags=*/0));
    if (file == nullptr) {
      return UnknownErrorBuilder()
          << "Failed to open zipped file 0: " << zip_strerror(archive.get());
    }

    ZipFileStreambuf zipstrm(archive.get(), file.get());
    std::istream unzipped(&zipstrm);
    auto dat_or = dat2pb::ParseRomDat(&unzipped);
    if (auto sb = zipstrm.status(); !sb.ok()) return sb;
    return dat_or;
  }

 private:
  StatusOr<std::stringstream> Get(absl::string_view path) {
    url_.SetPath(path);

    std::stringstream strm;
    RETURN_IF_ERROR(CurlEasySetWriteCallback(
          curl_.get(),
          [&strm](std::string_view data, size_t*) -> Status {
            strm << data;
            if (strm.bad()) {
              return UnknownError("Failed to write into stream.");
            }
            return OkStatus();
          }));

    RETURN_IF_ERROR(CurlEasyPerform(curl_.get()));
    return std::move(strm);
  }

  std::unique_ptr<CURL, CurlHandleDeleter> curl_;
  CurlURL url_;
};

constexpr char kUsageMessage[] = R"(Usage: roman fetch [options] console

Fetch a datpb from redump.org. Writes the binary datpb to stdout.

The console argument must be a valid console as listed at
http://redump.org/downloads/. It is the system part of the datfile URL, so if
the URL to download "Commodore Amiga CD" is http://redump.org/datfile/acd/, the
correct argument is "acd".

Also see rclone --help for additional options.

Example usage:
$ roman fetch ps2 > ps2.datpb)";

Status SubCommandFetch(absl::Span<std::string_view> args) {
  if (args.size() != 2) {
    return InvalidArgumentError(kUsageMessage);
  }
  std::string_view console(args[1]);

  RETURN_IF_ERROR(rhutil::CurlGlobalInit());

  ASSIGN_OR_RETURN(RomDat dat, RedumpFetcher().GetRomDat(console));

  if (absl::GetFlag(FLAGS_binary)) {
    if (!dat.SerializeToOstream(&std::cout)) {
      return UnknownError("Failed to write romdat to stdout");
    }
  } else {
    OstreamOutputStream strm(&std::cout);
    if (!TextFormat::Print(dat, &strm)) {
      return UnknownError("Failed to write romdat textproto to stdout");
    }
  }

  return OkStatus();
}

static void Initialize() {
  SubCommands::Instance()->Add("fetch", &SubCommandFetch);
}
rhutil::ModuleInit module_init(&Initialize);

}  // namespace
}  // namespace roman
