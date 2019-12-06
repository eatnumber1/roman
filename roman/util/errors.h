#ifndef CDMANIP_UTIL_ERRORS_H_
#define CDMANIP_UTIL_ERRORS_H_

#include <sysexits.h>

#include <glib.h>

#define CHECK_OK(gerror) \
  do { \
    if (gerror != nullptr) { \
      g_error("%s", gerror->message); \
    } \
  } while(false)

#define CDMANIP_ERROR ::roman::ErrorQuark()

namespace roman {

GQuark ErrorQuark();

enum Error : int {
  ERR_USAGE = EX_USAGE,
  ERR_UNIMPL = 110,
  ERR_UNKNOWN = EX_SOFTWARE,
  ERR_OS = EX_OSERR,
  ERR_FLAC = 111,
  ERR_OPENSSL = 112,
};

}  // namespace roman

#endif  // CDMANIP_UTIL_ERRORS_H_
