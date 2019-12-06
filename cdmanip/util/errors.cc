#include "cdmanip/util/errors.h"

namespace cdmanip {

GQuark ErrorQuark() {
  return g_quark_from_static_string("cdmanip-error-quark");
}

}  // namespace cdmanip
